/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
*
* File Name		: r3gd20_gyr_sysfs.c
* Authors		: MH - C&I BU - Application Team
*			: Carmine Iascone (carmine.iascone@st.com)
*			: Matteo Dameno (matteo.dameno@st.com)
*			: Both authors are willing to be considered the contact
*			: and update points for the driver.
* Version		: V 1.1.5 sysfs
* Date			: 2011/Sep/24
* Description		: R3GD20 digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
* 1.0		| 2010/May/02	| Carmine Iascone	| First Release
* 1.1.3		| 2011/Jun/24	| Matteo Dameno		| Corrects ODR Bug
* 1.1.4		| 2011/Sep/02	| Matteo Dameno		| SMB Bus Mng,
* 		|		|			| forces BDU setting
* 1.1.5		| 2011/Sep/24	| Matteo Dameno		| Introduces FIFO Feat.
* 1.1.5.1 | 2011/Nov/6  | Morris Chen     | change name from l3g to r3g
*                                         | change default FS to 2000DPS
*                                         | change default poll_rate to 50ms
*                                         | chage the attribute of sysfs file as 666
*******************************************************************************/

#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input-polldev.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/r3gd20.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/module.h>

#define D(x...) printk(KERN_DEBUG "[GYRO][R3GD20] " x)
#define I(x...) printk(KERN_INFO "[GYRO][R3GD20] " x)
#define E(x...) printk(KERN_ERR "[GYRO][R3GD20 ERROR] " x)
#define DIF(x...) \
	if (debug_flag) \
		printk(KERN_DEBUG "[GYRO][R3GD20 DEBUG] " x)

#define FS_MAX			32768

#define WHO_AM_I        0x0F

#define CTRL_REG1       0x20    
#define CTRL_REG2       0x21    
#define CTRL_REG3       0x22    
#define CTRL_REG4       0x23    
#define CTRL_REG5       0x24    
#define	REFERENCE	0x25    
#define	FIFO_CTRL_REG	0x2E    
#define FIFO_SRC_REG	0x2F    
#define	OUT_X_L		0x28    

#define AXISDATA_REG	OUT_X_L

#define ALL_ZEROES	0x00
#define PM_OFF		0x00
#define PM_NORMAL	0x08
#define ENABLE_ALL_AXES	0x07
#define ENABLE_NO_AXES	0x00
#define BW00		0x00
#define BW01		0x10
#define BW10		0x20
#define BW11		0x30
#define ODR095		0x00  
#define ODR190		0x40  
#define ODR380		0x80  
#define ODR760		0xC0  

#define	I2_DRDY		0x08
#define	I2_WTM		0x04
#define	I2_OVRUN	0x02
#define	I2_EMPTY	0x01
#define	I2_NONE		0x00
#define	I2_MASK		0x0F

#define	FS_MASK				0x30
#define	BDU_ENABLE			0x80

#define	FIFO_ENABLE	0x40
#define HPF_ENALBE	0x11

#define	FIFO_MODE_MASK		0xE0
#define	FIFO_MODE_BYPASS	0x00
#define	FIFO_MODE_FIFO		0x20
#define	FIFO_MODE_STREAM	0x40
#define	FIFO_MODE_STR2FIFO	0x60
#define	FIFO_MODE_BYPASS2STR	0x80
#define	FIFO_WATERMARK_MASK	0x1F

#define FIFO_STORED_DATA_MASK	0x1F


#define FUZZ			0
#define FLAT			0
#define I2C_AUTO_INCREMENT		0x80

#define	RES_CTRL_REG1		0
#define	RES_CTRL_REG2		1
#define	RES_CTRL_REG3		2
#define	RES_CTRL_REG4		3
#define	RES_CTRL_REG5		4
#define	RES_FIFO_CTRL_REG	5
#define	RESUME_ENTRIES		6

#define TOLERENCE		1071

#define DEBUG 0

#define HTC_WQ 1
#define HTC_SUSPEND 1
#define HTC_ATTR 1
#define cywee

#ifdef cywee
#define FIFO_LEVEL_MESK		0x1F
#endif


#define HW_WAKE_UP_TIME 160

#define WHOAMI_R3GD20		0x00D4	


struct r3gd20_triple {
	short	x,	
		y,	
		z;	
};

struct output_rate {
	int poll_rate_ms;
	u8 mask;
};

static const struct output_rate odr_table[] = {

	{	2,	ODR760|BW10},
	{	3,	ODR380|BW01},
	{	6,	ODR190|BW00},
	{	11,	ODR095|BW00},
};


static const struct r3gd20_gyr_platform_data default_r3gd20_gyr_pdata = {
	.fs_range = R3GD20_GYR_FS_2000DPS,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.poll_interval = 50,
	.min_interval = R3GD20_MIN_POLL_PERIOD_MS, 

	
			

	.watermark = 0,
	.fifomode = 0,
};

#ifdef HTC_WQ
static void polling_do_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(polling_work, polling_do_work);
#endif 

struct r3gd20_data {
	struct i2c_client *client;
	struct r3gd20_gyr_platform_data *pdata;

	struct mutex lock;

	struct input_polled_dev *input_poll_dev;
	int hw_initialized;

	atomic_t enabled;

	u8 reg_addr;
	u8 resume_state[RESUME_ENTRIES];

	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;

	bool polling_enabled;
#ifdef HTC_WQ
	struct workqueue_struct *gyro_wq;
	struct input_dev *gyro_input_dev;
#endif 

#ifdef HTC_ATTR
	struct class *htc_gyro_class;
	struct device *gyro_dev;
#endif 
	int cali_data_x;
	int cali_data_y;
	int cali_data_z;

	int is_suspended;
};

#ifdef HTC_WQ
struct r3gd20_data *g_gyro;
#endif 

static int debug_flag;

static int r3gd20_i2c_read(struct r3gd20_data *gyr,
				u8 *buf, int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;

#if 0
	if (use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(gyr->client, cmd);
			buf[0] = ret & 0xff;
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			
			ret = i2c_smbus_read_i2c_block_data(gyr->client,
								cmd, len, buf);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_read_i2c_block_data: ret:%d len:%d, "
				"command=0x%02x, ",
				ret, len, cmd);

			unsigned int ii;
			for (ii = 0; ii < len; ii++)
				D("buf[%d]=0x%02x,", ii, buf[ii]);

			D("\n");
#endif
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&gyr->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0; 
		}
		return len; 
	}
#endif 
	
	ret = i2c_master_send(gyr->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(gyr->client, buf, len);
}

static int r3gd20_i2c_write(struct r3gd20_data *gyr, u8 *buf, int len)
{
	int ret;
	u8 reg, value;

	reg = buf[0];
	value = buf[1];
#if 0
	if (use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(gyr->client, reg, value);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(gyr->client,
							reg, len, buf + 1);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);
			unsigned int ii;
			for (ii = 0; ii < (len + 1); ii++)
				D("value[%d]=0x%02x,", ii, buf[ii]);

			D("\n");
#endif
			return ret;
		}
	}
#endif 
	ret = i2c_master_send(gyr->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}


static int r3gd20_register_write(struct r3gd20_data *gyro, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err;

		buf[0] = reg_address;
		buf[1] = new_value;
		err = r3gd20_i2c_write(gyro, buf, 1);
		if (err < 0)
			return err;

	return err;
}

static int r3gd20_register_read(struct r3gd20_data *gyro, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = r3gd20_i2c_read(gyro, buf, 1);
	return err;
}

static int r3gd20_register_update(struct r3gd20_data *gyro, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;
	err = r3gd20_register_read(gyro, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[0];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		err = r3gd20_register_write(gyro, buf, reg_address,
				updated_val);
	}
	return err;
}

#if 1
static int r3gd20_update_watermark(struct r3gd20_data *gyro,
								u8 watermark)
{
	int res = 0;
	u8 buf[2];
	u8 new_value;

	mutex_lock(&gyro->lock);
	new_value = (watermark % 0x20);
	res = r3gd20_register_update(gyro, buf, FIFO_CTRL_REG,
			 FIFO_WATERMARK_MASK, new_value);
	if (res < 0) {
		E("%s : failed to update watermark\n", __func__);
		return res;
	}
	E("%s : new_value:0x%02x,watermark:0x%02x\n",
			__func__, new_value, watermark);

	gyro->resume_state[RES_FIFO_CTRL_REG] =
		((FIFO_WATERMARK_MASK & new_value) |
		(~FIFO_WATERMARK_MASK &
				gyro->resume_state[RES_FIFO_CTRL_REG]));
	gyro->pdata->watermark = new_value;
	mutex_unlock(&gyro->lock);
	return res;
}

static int r3gd20_update_fifomode(struct r3gd20_data *gyro, u8 fifomode)
{
	int res;
	u8 buf[2];
	u8 new_value;

	new_value = fifomode;
	res = r3gd20_register_update(gyro, buf, FIFO_CTRL_REG,
					FIFO_MODE_MASK, new_value);
	if (res < 0) {
		E("%s : failed to update fifoMode\n", __func__);
		return res;
	}
	gyro->resume_state[RES_FIFO_CTRL_REG] =
		((FIFO_MODE_MASK & new_value) |
		(~FIFO_MODE_MASK &
				gyro->resume_state[RES_FIFO_CTRL_REG]));
	gyro->pdata->fifomode = new_value;

	return res;
}

static int r3gd20_fifo_reset(struct r3gd20_data *gyro)
{
	u8 oldmode;
	int res;

	oldmode = gyro->pdata->fifomode;
	res = r3gd20_update_fifomode(gyro, FIFO_MODE_BYPASS);
	if (res < 0)
		return res;
	res = r3gd20_update_fifomode(gyro, oldmode);
	if (res >= 0)
		I("%s : fifo reset to: 0x%02x\n", __func__, oldmode);
	return res;
}

static int r3gd20_fifo_hwenable(struct r3gd20_data *gyro,
								u8 enable)
{
	int res;
	u8 buf[2];
	u8 set = 0x00;
	if (enable)
		set = FIFO_ENABLE;
	res = r3gd20_register_update(gyro, buf, CTRL_REG5,
			FIFO_ENABLE, set);
	if (res < 0) {
		E("%s : fifo_hw switch to:0x%02x failed\n", __func__, set);
		return res;
	}
	gyro->resume_state[RES_CTRL_REG5] =
		((FIFO_ENABLE & set) |
		(~FIFO_ENABLE & gyro->resume_state[RES_CTRL_REG5]));
	I("%s : fifo_hw_enable set to:0x%02x\n", __func__, set);
	return res;
}

static int r3gd20_manage_int2settings(struct r3gd20_data *gyro,
								u8 fifomode)
{
	int res;
	u8 buf[2];
	bool enable_fifo_hw;
	bool recognized_mode = false;
	u8 int2bits = I2_NONE;


	switch (fifomode) {
	case FIFO_MODE_FIFO:

		recognized_mode = true;
		int2bits = (I2_WTM | I2_OVRUN);
		res = r3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, int2bits);
		if (res < 0) {
			E("%s : failed to update CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & int2bits) |
			(~(I2_MASK) & gyro->resume_state[RES_CTRL_REG3]));
		enable_fifo_hw = true;
		break;

	case FIFO_MODE_BYPASS:
		recognized_mode = true;

		if (gyro->polling_enabled)
			int2bits = I2_NONE;
		else
			int2bits = I2_DRDY;
		res = r3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, int2bits);
		if (res < 0) {
			E("%s : failed to update to CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & int2bits) |
			(~I2_MASK & gyro->resume_state[RES_CTRL_REG3]));
		enable_fifo_hw = false;
		break;
	default:
		recognized_mode = false;
		res = r3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, I2_NONE);
		if (res < 0) {
			E("%s : failed to update CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		enable_fifo_hw = false;
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & 0x00) |
			(~I2_MASK & gyro->resume_state[RES_CTRL_REG3]));
		break;

	}
	if (recognized_mode) {
		res = r3gd20_update_fifomode(gyro, fifomode);
		if (res < 0) {
			E("%s : failed to set fifoMode\n", __func__);
			goto err_mutex_unlock;
		}
	}
	res = r3gd20_fifo_hwenable(gyro, enable_fifo_hw);

err_mutex_unlock:

	return res;
}

#endif
static int r3gd20_update_fs_range(struct r3gd20_data *gyro,
							u8 new_fs)
{
	int res ;
	u8 buf[2];

	buf[0] = CTRL_REG4;

	res = r3gd20_register_update(gyro, buf, CTRL_REG4,
							FS_MASK, new_fs);

	if (res < 0) {
		E("%s : failed to update fs:0x%02x\n",
			__func__, new_fs);
		return res;
	}
	gyro->resume_state[RES_CTRL_REG4] =
		((FS_MASK & new_fs) |
		(~FS_MASK & gyro->resume_state[RES_CTRL_REG4]));

	return res;
}


static int r3gd20_update_odr(struct r3gd20_data *gyro,
			unsigned int poll_interval_ms)
{
	int err = -1;
	int i;
	u8 config[2];

	for (i = ARRAY_SIZE(odr_table) - 1; i >= 0; i--) {
		if ((odr_table[i].poll_rate_ms <= poll_interval_ms) || (i == 0))
			break;
	}
#ifdef cywee
	if (poll_interval_ms > 20) {
		config[1] = 0x00;
	} else {
		config[1] = 0x40;
	}
#else
	config[1] = odr_table[i].mask;
#endif
	config[1] |= (ENABLE_ALL_AXES + PM_NORMAL);

	if (atomic_read(&gyro->enabled)) {
		config[0] = CTRL_REG1;
		err = r3gd20_i2c_write(gyro, config, 1);
		if (err < 0)
			return err;
		gyro->resume_state[RES_CTRL_REG1] = config[1];
	}


	return err;
}

#ifdef cywee
static int r3gd20_get_status(struct r3gd20_data *gyro ,unsigned char *status)
{
	int err = 0;
	unsigned char gyro_out[2];

	gyro_out[0] = (FIFO_SRC_REG); 

	err = r3gd20_i2c_read(gyro, gyro_out, 1);
	if (err < 0)
		E("%s: r3gd20_i2c_read() fails, err = %d\n", __func__, err);

	status[0] = gyro_out[0] & FIFO_LEVEL_MESK;

	return err;
}
#endif


static int r3gd20_get_data(struct r3gd20_data *gyro,
			     struct r3gd20_triple *data)
{
	int err;
	unsigned char gyro_out[6];
	
	s16 hw_d[3] = { 0 };

	gyro_out[0] = (I2C_AUTO_INCREMENT | AXISDATA_REG);

	err = r3gd20_i2c_read(gyro, gyro_out, 6);

	if (err < 0)
		return err;

	hw_d[0] = (s16) (((gyro_out[1]) << 8) | gyro_out[0]);
	hw_d[1] = (s16) (((gyro_out[3]) << 8) | gyro_out[2]);
	hw_d[2] = (s16) (((gyro_out[5]) << 8) | gyro_out[4]);

	data->x = ((gyro->pdata->negate_x) ? (-hw_d[gyro->pdata->axis_map_x])
		   : (hw_d[gyro->pdata->axis_map_x]));
	data->y = ((gyro->pdata->negate_y) ? (-hw_d[gyro->pdata->axis_map_y])
		   : (hw_d[gyro->pdata->axis_map_y]));
	data->z = ((gyro->pdata->negate_z) ? (-hw_d[gyro->pdata->axis_map_z])
		   : (hw_d[gyro->pdata->axis_map_z]));

	DIF("gyro_out: x = %d, y = %d, z = %d\n",
		data->x, data->y, data->z);

	return err;
}

static void r3gd20_report_values(struct r3gd20_data *gyr,
						struct r3gd20_triple *data)
{
	struct input_dev *input = gyr->input_poll_dev->input;

#ifdef HTC_WQ
	input = g_gyro->gyro_input_dev;
#endif 

	input_report_abs(input, ABS_X, data->x);
	input_report_abs(input, ABS_Y, data->y);
	input_report_abs(input, ABS_Z, data->z);
	input_sync(input);
}

#ifdef HTC_WQ
static void polling_do_work(struct work_struct *w)
{
	struct r3gd20_data *gyro = g_gyro;
	struct r3gd20_triple data_out;
	int err;
	unsigned char status = 0;

	mutex_lock(&gyro->lock);

#ifdef cywee
	err = r3gd20_get_status(gyro, &status);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_status failed\n");
	else {
		while(status > 0){
			err = r3gd20_get_data(gyro, &data_out);
			if (err < 0)
				dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
			else
				r3gd20_report_values(gyro, &data_out);
			status --;
		}
	}
#else
	err = r3gd20_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		r3gd20_report_values(gyro, &data_out);


#endif

	mutex_unlock(&gyro->lock);

	DIF("interval = %d, gyro->is_suspended = %d\n", gyro->input_poll_dev->
					 poll_interval, gyro->is_suspended);

	if (gyro->is_suspended != 1) {
		queue_delayed_work(gyro->gyro_wq, &polling_work,
			msecs_to_jiffies(gyro->input_poll_dev->
						 poll_interval));
	}
}
#endif 

#ifdef cywee
static int r3gd20_hw_init(struct r3gd20_data *gyro)
{
	int err;
	u8 buf[6];

	buf[0] = 0x20 ;
	buf[1] = 0x40 | 0x08 | 0x07 | 0x30;
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0) {
		E("%s: r3gd20_i2c_write fails 111\n", __func__);
		return err;
	}

	buf[0] = 0x23 ;
	buf[1] = 0x20;
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0) {
		E("%s: r3gd20_i2c_write fails 222\n", __func__);
		return err;
	}

	buf[0] = 0x24 ; 
	buf[1] = 0x40;
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0) {
		E("%s: r3gd20_i2c_write fails 333\n", __func__);
		return err;
	}

	buf[0] = FIFO_CTRL_REG; 
	buf[1] = 0x40 | 0x1F;
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0) {
		E("%s: r3gd20_i2c_write fails 444\n", __func__);
		return err;
	}

	buf[0] = 0x22 ; 
	buf[1] = 0x00;
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0) {
		E("%s: r3gd20_i2c_write fails 444\n", __func__);
		return err;
	}

	gyro->hw_initialized = 1;

	return err;
}

#else 

static int r3gd20_hw_init(struct r3gd20_data *gyro)
{
	int err;
	u8 buf[6];

	I("%s hw init\n", R3GD20_GYR_DEV_NAME);

	buf[0] = (I2C_AUTO_INCREMENT | CTRL_REG1);
	buf[1] = gyro->resume_state[RES_CTRL_REG1];
	buf[2] = gyro->resume_state[RES_CTRL_REG2];
	buf[3] = gyro->resume_state[RES_CTRL_REG3];
	buf[4] = gyro->resume_state[RES_CTRL_REG4];
	buf[5] = gyro->resume_state[RES_CTRL_REG5];

	err = r3gd20_i2c_write(gyro, buf, 5);
	if (err < 0)
		return err;

	buf[0] = FIFO_CTRL_REG;
	buf[1] = gyro->resume_state[RES_FIFO_CTRL_REG];
	err = r3gd20_i2c_write(gyro, buf, 1);
	if (err < 0)
			return err;

	gyro->hw_initialized = 1;

	return err;
}
#endif

static void r3gd20_device_power_off(struct r3gd20_data *dev_data)
{
	int err;
	u8 buf[2];

	I("%s:\n", __func__);

	buf[0] = CTRL_REG1;
	buf[1] = PM_OFF;
	err = r3gd20_i2c_write(dev_data, buf, 1);
	if (err < 0)
		dev_err(&dev_data->client->dev, "soft power off failed\n");

	if (dev_data->pdata->power_off) {
		
		disable_irq_nosync(dev_data->irq2);
		dev_data->pdata->power_off();
		dev_data->hw_initialized = 0;
	}

	if (dev_data->hw_initialized) {
		
		
		if (dev_data->pdata->gpio_int2 > 0) {
			disable_irq_nosync(dev_data->irq2);
			I("%s: power off: irq2 disabled\n",
						R3GD20_GYR_DEV_NAME);
		}
		dev_data->hw_initialized = 0;
	}
}

static int r3gd20_device_power_on(struct r3gd20_data *dev_data)
{
	int err;

	if (dev_data->pdata->power_on) {
		err = dev_data->pdata->power_on();
		if (err < 0)
			return err;
		if (dev_data->pdata->gpio_int2 > 0)
			enable_irq(dev_data->irq2);
	}


	if (!dev_data->hw_initialized) {
		err = r3gd20_hw_init(dev_data);
		if (err < 0) {
			r3gd20_device_power_off(dev_data);
			return err;
		}
	}

	if (dev_data->hw_initialized) {
		D("dev_data->pdata->gpio_int2 = %d\n", dev_data->pdata->gpio_int2);
		if (dev_data->pdata->gpio_int2 > 0) {
			enable_irq(dev_data->irq2);
			I("%s: power on: irq2 enabled\n",
						R3GD20_GYR_DEV_NAME);
		}
	}

	return 0;
}

static int r3gd20_enable(struct r3gd20_data *dev_data)
{
	int err;

	D("%s: enabled = %d\n", __func__, atomic_read(&dev_data->enabled));

	if (!atomic_cmpxchg(&dev_data->enabled, 0, 1)) {
		if (dev_data->pdata->power_LPM)
			dev_data->pdata->power_LPM(0);

		err = r3gd20_device_power_on(dev_data);
		if (err < 0) {
			atomic_set(&dev_data->enabled, 0);
			return err;
		}
#ifdef HTC_WQ
		D("Manually queue work!! HW_WAKE_UP_TIME = %d\n",
			HW_WAKE_UP_TIME);
		queue_delayed_work(dev_data->gyro_wq, &polling_work,
			msecs_to_jiffies(HW_WAKE_UP_TIME));
#else
		D("%s: queue work: interval = %d\n",
				__func__, dev_data->input_poll_dev->poll_interval);
		schedule_delayed_work(&dev_data->input_poll_dev->work,
				      msecs_to_jiffies(dev_data->input_poll_dev->poll_interval));
#endif 
	}

	return 0;
}

static int r3gd20_disable(struct r3gd20_data *dev_data)
{
	DIF("%s: dev_data->enabled = %d\n", __func__,
		atomic_read(&dev_data->enabled));

	if (atomic_cmpxchg(&dev_data->enabled, 1, 0))
		r3gd20_device_power_off(dev_data);

	I("%s: polling disabled\n", __func__);
	DIF("%s: dev_data->enabled = %d\n", __func__,
		atomic_read(&dev_data->enabled));

	if (dev_data->pdata->power_LPM)
		dev_data->pdata->power_LPM(1);

#ifdef HTC_WQ
	cancel_delayed_work_sync(&polling_work);
#else
	cancel_delayed_work_sync(&dev_data->input_poll_dev->work);
#endif 

	return 0;
}

static ssize_t attr_polling_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	
	struct r3gd20_data *gyro = g_gyro;

	mutex_lock(&gyro->lock);
	val = gyro->input_poll_dev->poll_interval;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_rate_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long interval_ms;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;
	interval_ms = max((unsigned int)interval_ms, gyro->pdata->min_interval);
	mutex_lock(&gyro->lock);
	gyro->input_poll_dev->poll_interval = interval_ms;
	gyro->pdata->poll_interval = interval_ms;
	D("r3gd20: %s: gyro->input_poll_dev->poll_interval = %d\n", __func__, gyro->input_poll_dev->poll_interval);
	r3gd20_update_odr(gyro, interval_ms);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_range_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	
	struct r3gd20_data *gyro = g_gyro;
	int range = 0;
	u8 val;
	mutex_lock(&gyro->lock);
	val = gyro->pdata->fs_range;

	switch (val) {
	case R3GD20_GYR_FS_250DPS:
		range = 250;
		break;
	case R3GD20_GYR_FS_500DPS:
		range = 500;
		break;
	case R3GD20_GYR_FS_2000DPS:
		range = 2000;
		break;
	}
	mutex_unlock(&gyro->lock);
	
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_range_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long val;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	gyro->pdata->fs_range = val;
	r3gd20_update_fs_range(gyro, val);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	
	struct r3gd20_data *gyro = g_gyro;
	int val = atomic_read(&gyro->enabled);

	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long val;

	DIF("%s: buf = %s", __func__, buf);

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		r3gd20_enable(gyro);
	else
		r3gd20_disable(gyro);

	return size;
}

static ssize_t attr_polling_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int val = 0;
	
	struct r3gd20_data *gyro = g_gyro;

	mutex_lock(&gyro->lock);
	if (gyro->polling_enabled)
		val = 1;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);
	if (val) {
		gyro->polling_enabled = true;
		r3gd20_manage_int2settings(gyro, FIFO_MODE_BYPASS);
		if (gyro->polling_enabled) {
			D("polling enabled\n");
#ifdef HTC_WQ
			queue_delayed_work(gyro->gyro_wq, &polling_work,
					   msecs_to_jiffies(gyro->input_poll_dev->
					   poll_interval));
#else
			schedule_delayed_work(&gyro->input_poll_dev->work,
					msecs_to_jiffies(gyro->
							pdata->poll_interval));
#endif 
		}
	} else {
		if (gyro->polling_enabled) {
			D("polling disabled\n");
#ifdef HTC_WQ
			cancel_delayed_work_sync(&polling_work);
#else
			cancel_delayed_work_sync(&gyro->input_poll_dev->work);
#endif 
		}
		gyro->polling_enabled = false;
		r3gd20_manage_int2settings(gyro, gyro->pdata->fifomode);
	}
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_watermark_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long watermark;
	int res;

	if (strict_strtoul(buf, 16, &watermark))
		return -EINVAL;

	res = r3gd20_update_watermark(gyro, watermark);
	if (res < 0)
		return res;

	return size;
}

static ssize_t attr_watermark_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	
	struct r3gd20_data *gyro = g_gyro;
	int val = gyro->pdata->watermark;
	return sprintf(buf, "0x%02x\n", val);
}

static ssize_t attr_fifomode_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long fifomode;
	int res;

	if (strict_strtoul(buf, 16, &fifomode))
		return -EINVAL;

	D("%s, got value:0x%02x\n", __func__, (u8)fifomode);

	mutex_lock(&gyro->lock);
	res = r3gd20_manage_int2settings(gyro, (u8) fifomode);
	mutex_unlock(&gyro->lock);

	if (res < 0)
		return res;
	return size;
}

static ssize_t attr_fifomode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	
	struct r3gd20_data *gyro = g_gyro;
	u8 val = gyro->pdata->fifomode;
	return sprintf(buf, "0x%02x\n", val);
}

#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	
	struct r3gd20_data *gyro = g_gyro;
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	x[0] = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	x[1] = val;
	rc = r3gd20_i2c_write(gyro, x, 1);
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	
	struct r3gd20_data *gyro = g_gyro;
	int rc;
	u8 data;

	mutex_lock(&gyro->lock);
	data = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	rc = r3gd20_i2c_read(gyro, &data, 1);
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	
	struct r3gd20_data *gyro = g_gyro;
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);

	gyro->reg_addr = val;

	mutex_unlock(&gyro->lock);

	return size;
}
#endif 

static ssize_t attr_debug_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *s = buf;

	s += sprintf(s, "debug_flag = %d\n", debug_flag);

	return s - buf;
}

static ssize_t attr_debug_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	debug_flag = -1;
	sscanf(buf, "%d", &debug_flag);

	D("%s: debug_flag = %d\n", __func__, debug_flag);

	return count;
}

static ssize_t attr_cali_data_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *s = buf;
	struct r3gd20_data *gyro = g_gyro;

	s += sprintf(s, "Stored calibration data (x, y, z) = (%d, %d, %d)\n",
		gyro->cali_data_x, gyro->cali_data_y,
		gyro->cali_data_z);

	D("%s: Calibration data (x, y, z) = (%d, %d, %d)\n",
		__func__, gyro->cali_data_x, gyro->cali_data_y,
			  gyro->cali_data_z);
	return s - buf;
}

static int is_valid_cali(int cali_data)
{
	if ((cali_data < TOLERENCE) && (cali_data > -TOLERENCE))
		return 1;
	else
		return 0;
}

static ssize_t attr_cali_data_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct r3gd20_data *gyro = g_gyro;

	D("%s: \n", __func__);

	if(sscanf(buf, "%d %d %d", &(gyro->cali_data_x), &(gyro->cali_data_y),
		  &(gyro->cali_data_z)) != 3) {
		E("%s: input format error!\n", __func__);
		return count;
	}

	if (!is_valid_cali(gyro->cali_data_x) ||
	    !is_valid_cali(gyro->cali_data_y) ||
	    !is_valid_cali(gyro->cali_data_z)) {
		E("%s: Invalid calibration data (x, y, z) = (%d, %d, %d)",
			__func__, gyro->cali_data_x, gyro->cali_data_y,
			gyro->cali_data_z);
		return count;
	}

	D("%s: Stored calibration data (x, y, z) = (%d, %d, %d)\n",
		__func__, gyro->cali_data_x, gyro->cali_data_y,
			  gyro->cali_data_z);

	return count;
}

static struct device_attribute attributes[] = {
	__ATTR(pollrate_ms, 0664, attr_polling_rate_show,
						attr_polling_rate_store),
	__ATTR(range, 0664, attr_range_show, attr_range_store),
	__ATTR(enable_device, 0664, attr_enable_show, attr_enable_store),
	__ATTR(enable_polling, 0664, attr_polling_mode_show, attr_polling_mode_store),
	__ATTR(fifo_samples, 0664, attr_watermark_show, attr_watermark_store),
	__ATTR(fifo_mode, 0664, attr_fifomode_show, attr_fifomode_store),
	__ATTR(cali_data, 0664, attr_cali_data_show, attr_cali_data_store),
#ifdef DEBUG
	__ATTR(reg_value, 0664, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0664, NULL, attr_addr_set),
	__ATTR(debug, 0664, attr_debug_show, attr_debug_store),
#endif
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;

#ifdef HTC_ATTR
	struct r3gd20_data *gyro = g_gyro;
	int ret = 0;

	if (gyro == NULL) {
		E("%s: g_gyro == NULL!!\n", __func__);
		return -2;
	}

	gyro->htc_gyro_class = class_create(THIS_MODULE, "htc_gyro");
	if (IS_ERR(gyro->htc_gyro_class)) {
		ret = PTR_ERR(gyro->htc_gyro_class);
		gyro->htc_gyro_class = NULL;
		E("%s: could not allocate gyro->htc_gyro_class\n", __func__);
		goto err_create_class;
	}

	gyro->gyro_dev = device_create(gyro->htc_gyro_class,
				NULL, 0, "%s", "gyro");
	if (unlikely(IS_ERR(gyro->gyro_dev))) {
		ret = PTR_ERR(gyro->gyro_dev);
		gyro->gyro_dev = NULL;
		E("%s: could not allocate gyro->gyro_dev\n", __func__);
		goto err_create_gyro_device;
	}

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(gyro->gyro_dev, attributes + i)) {
			E("%s: could not allocate attribute files\n",
				__func__);
			goto err_create_device_file;
		}
	return 0;

err_create_device_file:
	device_unregister(gyro->gyro_dev);
err_create_gyro_device:
	class_destroy(gyro->htc_gyro_class);
err_create_class:
	return ret;

#else 

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;

#endif 

}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}

static void report_triple(struct r3gd20_data *gyro)
{
	int err;
	struct r3gd20_triple data_out;

	err = r3gd20_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		r3gd20_report_values(gyro, &data_out);
}

static void r3gd20_input_poll_func(struct input_polled_dev *dev)
{
	struct r3gd20_data *gyro = dev->private;

	struct r3gd20_triple data_out;

	int err;

	DIF("%s: gyro->enabled = %d\n",
		__func__, atomic_read(&gyro->enabled));
	if (atomic_read(&gyro->enabled) == 0)
		return;

	


	mutex_lock(&gyro->lock);
	err = r3gd20_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		r3gd20_report_values(gyro, &data_out);

	mutex_unlock(&gyro->lock);

}

static void r3gd20_irq2_fifo(struct r3gd20_data *gyro)
{
	int err;
	u8 buf[2];
	u8 int_source;
	u8 samples;
	u8 workingmode;
	u8 stored_samples;

	mutex_lock(&gyro->lock);

	workingmode = gyro->pdata->fifomode;


	


	switch (workingmode) {
	case FIFO_MODE_BYPASS:
	{
		report_triple(gyro);
		break;
	}
	case FIFO_MODE_FIFO:
		samples = (gyro->pdata->watermark)+1;
		
		err = r3gd20_register_read(gyro, buf, FIFO_SRC_REG);
		if (err > 0)
			dev_err(&gyro->client->dev, "error reading fifo source reg\n");

		int_source = buf[0];
		

		stored_samples = int_source & FIFO_STORED_DATA_MASK;


		for (; samples > 0; samples--) {
#if DEBUG
			input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 1);
			input_sync(gyro->input_poll_dev->input);
#endif
			
			report_triple(gyro);

#if DEBUG
			input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 0);
			input_sync(gyro->input_poll_dev->input);
#endif
		}
		r3gd20_fifo_reset(gyro);
		break;
	}
#if DEBUG
	input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 3);
	input_sync(gyro->input_poll_dev->input);
#endif

	mutex_unlock(&gyro->lock);
}

static irqreturn_t r3gd20_isr2(int irq, void *dev)
{
	struct r3gd20_data *gyro = dev;

	disable_irq_nosync(irq);
#if DEBUG
	input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 2);
	input_sync(gyro->input_poll_dev->input);
#endif
	queue_work(gyro->irq2_work_queue, &gyro->irq2_work);
	I("%s: isr2 queued\n", R3GD20_GYR_DEV_NAME);

	return IRQ_HANDLED;
}

static void r3gd20_irq2_work_func(struct work_struct *work)
{

	struct r3gd20_data *gyro =
	container_of(work, struct r3gd20_data, irq2_work);
	r3gd20_irq2_fifo(gyro);
	
	I("%s: IRQ2 served\n", R3GD20_GYR_DEV_NAME);
	enable_irq(gyro->irq2);
}


int r3gd20_input_open(struct input_dev *input)
{
	struct r3gd20_data *gyro = input_get_drvdata(input);

	DIF("%s:\n", __func__);

	
	return 0;

	return r3gd20_enable(gyro);
}

void r3gd20_input_close(struct input_dev *dev)
{
	struct r3gd20_data *gyro = input_get_drvdata(dev);

	r3gd20_disable(gyro);
}

static int r3gd20_validate_pdata(struct r3gd20_data *gyro)
{
	
	gyro->pdata->min_interval =
		max((unsigned int) R3GD20_MIN_POLL_PERIOD_MS,
						gyro->pdata->min_interval);

	gyro->pdata->poll_interval = max(gyro->pdata->poll_interval,
			gyro->pdata->min_interval);

	if (gyro->pdata->axis_map_x > 2 ||
	    gyro->pdata->axis_map_y > 2 ||
	    gyro->pdata->axis_map_z > 2) {
		dev_err(&gyro->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			gyro->pdata->axis_map_x,
			gyro->pdata->axis_map_y,
			gyro->pdata->axis_map_z);
		return -EINVAL;
	}

	
	if (gyro->pdata->negate_x > 1 ||
	    gyro->pdata->negate_y > 1 ||
	    gyro->pdata->negate_z > 1) {
		dev_err(&gyro->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			gyro->pdata->negate_x,
			gyro->pdata->negate_y,
			gyro->pdata->negate_z);
		return -EINVAL;
	}

	
	if (gyro->pdata->poll_interval < gyro->pdata->min_interval) {
		dev_err(&gyro->client->dev,
			"minimum poll interval violated\n");
		return -EINVAL;
	}
	return 0;
}

static int r3gd20_input_init(struct r3gd20_data *gyro)
{
	int err = -1;
	struct input_dev *input;

	

	gyro->input_poll_dev = input_allocate_polled_device();
	if (!gyro->input_poll_dev) {
		err = -ENOMEM;
		dev_err(&gyro->client->dev,
			"input device allocation failed\n");
		goto err0;
	}

	gyro->input_poll_dev->private = gyro;
	gyro->input_poll_dev->poll = r3gd20_input_poll_func;
	gyro->input_poll_dev->poll_interval = gyro->pdata->poll_interval;

#ifdef HTC_WQ
	input = input_allocate_device();
	if (!input) {
		E("%s: could not allocate ls input device\n", __func__);
		return -ENOMEM;
	}
	gyro->gyro_input_dev = input;
#else
	input = gyro->input_poll_dev->input;
#endif 

	input->open = r3gd20_input_open;
	input->close = r3gd20_input_close;

	input->id.bustype = BUS_I2C;
	input->dev.parent = &gyro->client->dev;

#ifdef HTC_WQ
	input_set_drvdata(input, gyro);
#else
	input_set_drvdata(gyro->input_poll_dev->input, gyro);
#endif 

	set_bit(EV_ABS, input->evbit);

#if DEBUG
	set_bit(EV_KEY, input->keybit);
	set_bit(KEY_LEFT, input->keybit);
	input_set_abs_params(input, ABS_MISC, 0, 1, 0, 0);
#endif

	input_set_abs_params(input, ABS_X, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Y, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Z, -FS_MAX, FS_MAX, FUZZ, FLAT);

	input->name = R3GD20_GYR_DEV_NAME;

#ifdef HTC_WQ
	err = input_register_device(input);
#else
	err = input_register_polled_device(gyro->input_poll_dev);
#endif
	if (err) {
		dev_err(&gyro->client->dev,
			"unable to register input polled device %s\n",
			gyro->input_poll_dev->input->name);
		goto err1;
	}

	return 0;

err1:
	input_free_polled_device(gyro->input_poll_dev);
err0:
	return err;
}

static void r3gd20_input_cleanup(struct r3gd20_data *gyro)
{
	input_unregister_polled_device(gyro->input_poll_dev);
	input_free_polled_device(gyro->input_poll_dev);
}

static int to_signed_int(char *value)
{
	int ret_int = 0;

	if (value == NULL)
		ret_int = 0;
	else {
		ret_int = value[0] | (value[1] << 8) |
			  (value[2] << 16) |
			  (value[3] << 24);
	}

	return ret_int;
}

static int r3gd20_probe(struct i2c_client *client,
					const struct i2c_device_id *devid)
{
	struct r3gd20_data *gyro;

	

	int err = -1;

	I("%s: probe start v03.\n", R3GD20_GYR_DEV_NAME);

	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		
		err = -ENODEV;
		goto err0;
	}

	

	gyro = kzalloc(sizeof(*gyro), GFP_KERNEL);
	if (gyro == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto err0;
	}

	g_gyro = gyro;

	mutex_init(&gyro->lock);
	mutex_lock(&gyro->lock);
	gyro->client = client;

	gyro->pdata = kmalloc(sizeof(*gyro->pdata), GFP_KERNEL);
	if (gyro->pdata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err1;
	}

	if (client->dev.platform_data == NULL) {
		memcpy(gyro->pdata, &default_r3gd20_gyr_pdata,
							sizeof(*gyro->pdata));
	} else {
		memcpy(gyro->pdata, client->dev.platform_data,
						sizeof(*gyro->pdata));
	}

	err = r3gd20_validate_pdata(gyro);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err1_1;
	}

	i2c_set_clientdata(client, gyro);

	if (gyro->pdata->init) {
		err = gyro->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err1_1;
		}
	}


	memset(gyro->resume_state, 0, ARRAY_SIZE(gyro->resume_state));

	gyro->resume_state[RES_CTRL_REG1] = ALL_ZEROES | ENABLE_ALL_AXES;
	gyro->resume_state[RES_CTRL_REG2] = ALL_ZEROES;
	gyro->resume_state[RES_CTRL_REG3] = ALL_ZEROES;
	gyro->resume_state[RES_CTRL_REG4] = ALL_ZEROES | BDU_ENABLE;
	gyro->resume_state[RES_CTRL_REG5] = ALL_ZEROES;
	gyro->resume_state[RES_FIFO_CTRL_REG] = ALL_ZEROES;

	gyro->polling_enabled = true;
	gyro->is_suspended = 0;

	err = r3gd20_device_power_on(gyro);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&gyro->enabled, 1);

	err = r3gd20_update_fs_range(gyro, gyro->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range failed\n");
		goto err2;
	}

	err = r3gd20_update_odr(gyro, gyro->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto err2;
	}

	err = r3gd20_input_init(gyro);
	if (err < 0)
		goto err3;

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
			"%s device register failed\n", R3GD20_GYR_DEV_NAME);
		goto err4;
	}

	r3gd20_device_power_off(gyro);

	
	atomic_set(&gyro->enabled, 0);


	if (gyro->pdata->gpio_int2 > 0) {
		gyro->irq2 = gpio_to_irq(gyro->pdata->gpio_int2);
		I("%s: %s has set irq2 to irq:"
						" %d mapped on gpio:%d\n",
			R3GD20_GYR_DEV_NAME, __func__, gyro->irq2,
							gyro->pdata->gpio_int2);

		INIT_WORK(&gyro->irq2_work, r3gd20_irq2_work_func);
		gyro->irq2_work_queue =
			create_singlethread_workqueue("r3gd20_gyr_wq2");
		if (!gyro->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev, "cannot create "
						"work queue2: %d\n", err);
			goto err5;
		}

		err = request_irq(gyro->irq2, r3gd20_isr2,
				IRQF_TRIGGER_HIGH, "r3gd20_gyr_irq2", gyro);

		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err6;
		}
		disable_irq_nosync(gyro->irq2);
	}

	mutex_unlock(&gyro->lock);

	gyro->cali_data_x = to_signed_int(&gyro_gsensor_kvalue[4]);
	gyro->cali_data_y = to_signed_int(&gyro_gsensor_kvalue[8]);
	gyro->cali_data_z = to_signed_int(&gyro_gsensor_kvalue[12]);
	D("%s: Calibration data (x, y, z) = (%d, %d, %d)\n",
		__func__, gyro->cali_data_x, gyro->cali_data_y,
			  gyro->cali_data_z);

#ifdef HTC_WQ
	gyro->gyro_wq = create_singlethread_workqueue("gyro_wq");
	if (!gyro->gyro_wq) {
		E("%s: can't create workqueue\n", __func__);
		err = -ENOMEM;
		goto err_create_singlethread_workqueue;
	}
#endif 
	debug_flag = 0;

	I("%s: %s probed: device created successfully\n",
			__func__, R3GD20_GYR_DEV_NAME);

	return 0;


#ifdef HTC_WQ
err_create_singlethread_workqueue:
#endif 
err6:
	destroy_workqueue(gyro->irq2_work_queue);
err5:
	r3gd20_device_power_off(gyro);
	remove_sysfs_interfaces(&client->dev);
err4:
	r3gd20_input_cleanup(gyro);
err3:
	r3gd20_device_power_off(gyro);
err2:
	if (gyro->pdata->exit)
		gyro->pdata->exit();
err1_1:
	mutex_unlock(&gyro->lock);
	kfree(gyro->pdata);
err1:
	kfree(gyro);
err0:
	E("%s: Driver Initialization failed\n", R3GD20_GYR_DEV_NAME);
	return err;
}

static int r3gd20_remove(struct i2c_client *client)
{
	struct r3gd20_data *gyro = i2c_get_clientdata(client);
#if DEBUG
	I("R3GD20 driver removing\n");
#endif

	if (gyro->pdata->gpio_int2 > 0) {
		free_irq(gyro->irq2, gyro);
		gpio_free(gyro->pdata->gpio_int2);
		destroy_workqueue(gyro->irq2_work_queue);
	}

	r3gd20_input_cleanup(gyro);
	r3gd20_device_power_off(gyro);
	remove_sysfs_interfaces(&client->dev);

	kfree(gyro->pdata);
	kfree(gyro);
	return 0;
}

#ifdef HTC_SUSPEND

static int r3gd20_suspend(struct i2c_client *client, pm_message_t mesg)
{
#ifdef CONFIG_SUSPEND
	struct r3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];
	int err = -1;

	data->is_suspended = 1;
	I("%s++: data->is_suspended = %d\n", __func__, data->is_suspended);

#if DEBUG
	I("r3gd20_suspend\n");
#endif 

	
		if (data->polling_enabled) {
			D("polling disabled\n");
#ifdef HTC_WQ
			cancel_delayed_work_sync(&polling_work);
#else 
			cancel_delayed_work_sync(&data->input_poll_dev->work);
#endif 
			
		}

		mutex_lock(&data->lock);
#ifdef SLEEP
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_NO_AXES | PM_NORMAL));
#else
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x08, PM_OFF);
#endif 
		mutex_unlock(&data->lock);
	

#endif 
	if (data && (data->pdata->power_LPM))
		data->pdata->power_LPM(1);

	I("%s:--\n", __func__);
	return err;
}

static int r3gd20_resume(struct i2c_client *client)
{
#ifdef CONFIG_SUSPEND
	struct r3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];
	int err = -1;

	I("%s:++\n", __func__);
#if DEBUG
	I("r3gd20_resume\n");
#endif 

	if (atomic_read(&data->enabled)) {
		mutex_lock(&data->lock);

		if (data->polling_enabled) {
			D("polling enabled\n");
#ifdef HTC_WQ
			queue_delayed_work(data->gyro_wq, &polling_work,
					   msecs_to_jiffies(data->input_poll_dev->
					   poll_interval));
#else
			schedule_delayed_work(&data->input_poll_dev->work,
					msecs_to_jiffies(data->
							pdata->poll_interval));
#endif
		}
#ifdef SLEEP
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_ALL_AXES | PM_NORMAL));
#else
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x08, PM_NORMAL);
#endif
		mutex_unlock(&data->lock);

	}

#endif 
	data->is_suspended = 0;
	I("%s--: data->is_suspended = %d\n", __func__, data->is_suspended);
	return 0;
}

#else 

static int r3gd20_suspend(struct device *dev)
{
#define SLEEP
#ifdef CONFIG_SUSPEND
	struct i2c_client *client = to_i2c_client(dev);
	struct r3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];
	int err = -1;

	D("%s:\n", __func__);

#if DEBUG
	I("r3gd20_suspend\n");
#endif 
	I("%s\n", __func__);
	if (atomic_read(&data->enabled)) {
		mutex_lock(&data->lock);
		if (data->polling_enabled) {
			D("polling disabled\n");
#ifdef HTC_WQ
			cancel_delayed_work_sync(&polling_work);
#else
			cancel_delayed_work_sync(&data->input_poll_dev->work);
#endif 
			
		}
#ifdef SLEEP
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_NO_AXES | PM_NORMAL));
#else
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x08, PM_OFF);
#endif 
		mutex_unlock(&data->lock);
	}

#endif 
	return err;
}

static int r3gd20_resume(struct device *dev)
{
#ifdef CONFIG_SUSPEND
	struct i2c_client *client = to_i2c_client(dev);
	struct r3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];
	int err = -1;

	D("%s:\n", __func__);
#if DEBUG
	I("r3gd20_resume\n");
#endif 
	I("%s\n", __func__);
	if (atomic_read(&data->enabled)) {
		mutex_lock(&data->lock);
		if (data->polling_enabled) {
			D("polling enabled\n");
#ifdef HTC_WQ
			queue_delayed_work(data->gyro_wq, &polling_work,
					   msecs_to_jiffies(data->input_poll_dev->
					   poll_interval));
#else
			schedule_delayed_work(&data->input_poll_dev->work,
					msecs_to_jiffies(data->
							pdata->poll_interval));
#endif
		}
#ifdef SLEEP
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_ALL_AXES | PM_NORMAL));
#else
		err = r3gd20_register_update(data, buf, CTRL_REG1,
				0x08, PM_NORMAL);
#endif
		mutex_unlock(&data->lock);

	}

#endif 
	return 0;
}

#endif 

static const struct i2c_device_id r3gd20_id[] = {
	{ R3GD20_GYR_DEV_NAME , 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, r3gd20_id);

#ifndef HTC_SUSPEND
static struct dev_pm_ops r3gd20_pm = {
	.suspend = r3gd20_suspend,
	.resume = r3gd20_resume,
};
#endif 

static struct i2c_driver r3gd20_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = R3GD20_GYR_DEV_NAME,
#ifndef HTC_SUSPEND
			.pm = &r3gd20_pm,
#endif 
	},
	.probe = r3gd20_probe,
	.remove = __devexit_p(r3gd20_remove),
	.id_table = r3gd20_id,
#ifdef HTC_SUSPEND
	.suspend = r3gd20_suspend,
	.resume = r3gd20_resume,
#endif

};

static int __init r3gd20_init(void)
{
#if DEBUG
	I("%s: gyroscope sysfs driver init\n", R3GD20_GYR_DEV_NAME);
#endif
	return i2c_add_driver(&r3gd20_driver);
}

static void __exit r3gd20_exit(void)
{
#if DEBUG
	I("%s exit\n", R3GD20_GYR_DEV_NAME);
#endif
	i2c_del_driver(&r3gd20_driver);
	return;
}

module_init(r3gd20_init);
module_exit(r3gd20_exit);

MODULE_DESCRIPTION("r3gd20 digital gyroscope sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");

