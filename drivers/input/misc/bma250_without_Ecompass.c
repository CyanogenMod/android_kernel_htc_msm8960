/* drivers/i2c/chips/bma250.c - bma250 G-sensor driver
 *
 * Copyright (C) 2008-2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/bma250.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include<linux/earlysuspend.h>
#include <linux/akm8975.h>
#include <linux/export.h>
#include <linux/module.h>


#define D(x...) printk(KERN_DEBUG "[GSNR][BMA250 NO_COMP] " x)
#define I(x...) printk(KERN_INFO "[GSNR][BMA250 NO_COMP] " x)
#define E(x...) printk(KERN_ERR "[GSNR][BMA250 NO_COMP ERROR] " x)
#define DIF(x...) \
	if (debug_flag) \
		printk(KERN_DEBUG "[GSNR][BMA250 NO_COMP DEBUG] " x)

#define DEFAULT_RANGE	BMA_RANGE_2G
#define DEFAULT_BW	BMA_BW_31_25HZ

#define RETRY_TIMES	10

static struct i2c_client *this_client;

struct bma250_data {
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend early_suspend;
#ifdef CONFIG_CIR_ALWAYS_READY
	struct input_dev *input_cir;
	struct work_struct irq_work;
	int irq;
#endif

};

#ifdef CONFIG_CIR_ALWAYS_READY
static int cir_flag = 0;
static int power_key_pressed = 0;
#endif

int ignore_first_event;

static struct bma250_platform_data *pdata;
static atomic_t PhoneOn_flag = ATOMIC_INIT(0);
#define DEVICE_ACCESSORY_ATTR(_name, _mode, _show, _store) \
struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)

#define	ASENSE_TARGET			0x02D0
#define TIME_LOG		50
static unsigned int numCount;
bool sensors_on;
static atomic_t m_flag;
static atomic_t a_flag;
static atomic_t t_flag;
static atomic_t mv_flag;
static short suspend_flag;

struct akm8975_data {
	struct input_dev *input_dev;
	struct work_struct work;
	struct delayed_work input_work;
	int	poll_interval;
	struct early_suspend early_suspend;
};
short user_offset[3];

struct mutex gsensor_lock;
struct akm8975_data *akm8975_misc_data;

static int debug_flag;
static char update_user_calibrate_data;

#ifdef CONFIG_CIR_ALWAYS_READY

static int BMA_set_mode(unsigned char mode);
static int BMA_I2C_RxData(char *rxData, int length);
static int BMA_I2C_TxData(char *txData, int length);
#define BMA250_GET_BITSLICE(regvar, bitname)\
            ((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA250_SET_BITSLICE(regvar, bitname, val)\
                    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define SLOP_INTERRUPT                          REL_DIAL


#define SLOPE_INTERRUPT_X_HAPPENED                      7
#define SLOPE_INTERRUPT_Y_HAPPENED                      8
#define SLOPE_INTERRUPT_Z_HAPPENED                      9
#define SLOPE_INTERRUPT_X_NEGATIVE_HAPPENED             10
#define SLOPE_INTERRUPT_Y_NEGATIVE_HAPPENED             11
#define SLOPE_INTERRUPT_Z_NEGATIVE_HAPPENED             12


#define PAD_LOWG                                        0
#define PAD_HIGHG                                       1
#define PAD_SLOP                                        2
#define PAD_DOUBLE_TAP                                  3
#define PAD_SINGLE_TAP                                  4
#define PAD_ORIENT                                      5
#define PAD_FLAT                                        6

#define BMA250_CHIP_ID_REG                      0x00
#define BMA250_VERSION_REG                      0x01
#define BMA250_X_AXIS_LSB_REG                   0x02
#define BMA250_X_AXIS_MSB_REG                   0x03
#define BMA250_Y_AXIS_LSB_REG                   0x04
#define BMA250_Y_AXIS_MSB_REG                   0x05
#define BMA250_Z_AXIS_LSB_REG                   0x06
#define BMA250_Z_AXIS_MSB_REG                   0x07
#define BMA250_TEMP_RD_REG                      0x08
#define BMA250_STATUS1_REG                      0x09
#define BMA250_STATUS2_REG                      0x0A
#define BMA250_STATUS_TAP_SLOPE_REG             0x0B
#define BMA250_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA250_RANGE_SEL_REG                    0x0F
#define BMA250_BW_SEL_REG                       0x10
#define BMA250_MODE_CTRL_REG                    0x11
#define BMA250_LOW_NOISE_CTRL_REG               0x12
#define BMA250_DATA_CTRL_REG                    0x13
#define BMA250_RESET_REG                        0x14
#define BMA250_INT_ENABLE1_REG                  0x16
#define BMA250_INT_ENABLE2_REG                  0x17
#define BMA250_INT1_PAD_SEL_REG                 0x19
#define BMA250_INT_DATA_SEL_REG                 0x1A
#define BMA250_INT2_PAD_SEL_REG                 0x1B
#define BMA250_INT_SRC_REG                      0x1E
#define BMA250_INT_SET_REG                      0x20
#define BMA250_INT_CTRL_REG                     0x21
#define BMA250_LOW_DURN_REG                     0x22
#define BMA250_LOW_THRES_REG                    0x23
#define BMA250_LOW_HIGH_HYST_REG                0x24
#define BMA250_HIGH_DURN_REG                    0x25
#define BMA250_HIGH_THRES_REG                   0x26
#define BMA250_SLOPE_DURN_REG                   0x27
#define BMA250_SLOPE_THRES_REG                  0x28
#define BMA250_TAP_PARAM_REG                    0x2A
#define BMA250_TAP_THRES_REG                    0x2B
#define BMA250_ORIENT_PARAM_REG                 0x2C
#define BMA250_THETA_BLOCK_REG                  0x2D
#define BMA250_THETA_FLAT_REG                   0x2E
#define BMA250_FLAT_HOLD_TIME_REG               0x2F
#define BMA250_STATUS_LOW_POWER_REG             0x31
#define BMA250_SELF_TEST_REG                    0x32
#define BMA250_EEPROM_CTRL_REG                  0x33
#define BMA250_SERIAL_CTRL_REG                  0x34
#define BMA250_CTRL_UNLOCK_REG                  0x35
#define BMA250_OFFSET_CTRL_REG                  0x36
#define BMA250_OFFSET_PARAMS_REG                0x37
#define BMA250_OFFSET_FILT_X_REG                0x38
#define BMA250_OFFSET_FILT_Y_REG                0x39
#define BMA250_OFFSET_FILT_Z_REG                0x3A
#define BMA250_OFFSET_UNFILT_X_REG              0x3B
#define BMA250_OFFSET_UNFILT_Y_REG              0x3C
#define BMA250_OFFSET_UNFILT_Z_REG              0x3D
#define BMA250_SPARE_0_REG                      0x3E
#define BMA250_SPARE_1_REG                      0x3F



#define BMA250_INT_MODE_SEL__POS                0
#define BMA250_INT_MODE_SEL__LEN                4
#define BMA250_INT_MODE_SEL__MSK                0x0F
#define BMA250_INT_MODE_SEL__REG                BMA250_INT_CTRL_REG

#define BMA250_SLOPE_DUR__POS                    0
#define BMA250_SLOPE_DUR__LEN                    2
#define BMA250_SLOPE_DUR__MSK                    0x03
#define BMA250_SLOPE_DUR__REG                    BMA250_SLOPE_DURN_REG


#define BMA250_SLOPE_THRES__POS                  0
#define BMA250_SLOPE_THRES__LEN                  8
#define BMA250_SLOPE_THRES__MSK                  0xFF
#define BMA250_SLOPE_THRES__REG                  BMA250_SLOPE_THRES_REG


#define BMA250_EN_INT1_PAD_LOWG__POS        0
#define BMA250_EN_INT1_PAD_LOWG__LEN        1
#define BMA250_EN_INT1_PAD_LOWG__MSK        0x01
#define BMA250_EN_INT1_PAD_LOWG__REG        BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_HIGHG__POS       1
#define BMA250_EN_INT1_PAD_HIGHG__LEN       1
#define BMA250_EN_INT1_PAD_HIGHG__MSK       0x02
#define BMA250_EN_INT1_PAD_HIGHG__REG       BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_SLOPE__POS       2
#define BMA250_EN_INT1_PAD_SLOPE__LEN       1
#define BMA250_EN_INT1_PAD_SLOPE__MSK       0x04
#define BMA250_EN_INT1_PAD_SLOPE__REG       BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_DB_TAP__POS      4
#define BMA250_EN_INT1_PAD_DB_TAP__LEN      1
#define BMA250_EN_INT1_PAD_DB_TAP__MSK      0x10
#define BMA250_EN_INT1_PAD_DB_TAP__REG      BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_SNG_TAP__POS     5
#define BMA250_EN_INT1_PAD_SNG_TAP__LEN     1
#define BMA250_EN_INT1_PAD_SNG_TAP__MSK     0x20
#define BMA250_EN_INT1_PAD_SNG_TAP__REG     BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_ORIENT__POS      6
#define BMA250_EN_INT1_PAD_ORIENT__LEN      1
#define BMA250_EN_INT1_PAD_ORIENT__MSK      0x40
#define BMA250_EN_INT1_PAD_ORIENT__REG      BMA250_INT1_PAD_SEL_REG

#define BMA250_EN_INT1_PAD_FLAT__POS        7
#define BMA250_EN_INT1_PAD_FLAT__LEN        1
#define BMA250_EN_INT1_PAD_FLAT__MSK        0x80
#define BMA250_EN_INT1_PAD_FLAT__REG        BMA250_INT1_PAD_SEL_REG


#define BMA250_EN_SLOPE_X_INT__POS         0
#define BMA250_EN_SLOPE_X_INT__LEN         1
#define BMA250_EN_SLOPE_X_INT__MSK         0x01
#define BMA250_EN_SLOPE_X_INT__REG         BMA250_INT_ENABLE1_REG

#define BMA250_EN_SLOPE_Y_INT__POS         1
#define BMA250_EN_SLOPE_Y_INT__LEN         1
#define BMA250_EN_SLOPE_Y_INT__MSK         0x02
#define BMA250_EN_SLOPE_Y_INT__REG         BMA250_INT_ENABLE1_REG

#define BMA250_EN_SLOPE_Z_INT__POS         2
#define BMA250_EN_SLOPE_Z_INT__LEN         1
#define BMA250_EN_SLOPE_Z_INT__MSK         0x04
#define BMA250_EN_SLOPE_Z_INT__REG         BMA250_INT_ENABLE1_REG

#define BMA250_EN_SLOPE_XYZ_INT__POS         0
#define BMA250_EN_SLOPE_XYZ_INT__LEN         3
#define BMA250_EN_SLOPE_XYZ_INT__MSK         0x07
#define BMA250_EN_SLOPE_XYZ_INT__REG         BMA250_INT_ENABLE1_REG

#define BMA250_EN_DOUBLE_TAP_INT__POS      4
#define BMA250_EN_DOUBLE_TAP_INT__LEN      1
#define BMA250_EN_DOUBLE_TAP_INT__MSK      0x10
#define BMA250_EN_DOUBLE_TAP_INT__REG      BMA250_INT_ENABLE1_REG

#define BMA250_EN_SINGLE_TAP_INT__POS      5
#define BMA250_EN_SINGLE_TAP_INT__LEN      1
#define BMA250_EN_SINGLE_TAP_INT__MSK      0x20
#define BMA250_EN_SINGLE_TAP_INT__REG      BMA250_INT_ENABLE1_REG

#define BMA250_EN_ORIENT_INT__POS          6
#define BMA250_EN_ORIENT_INT__LEN          1
#define BMA250_EN_ORIENT_INT__MSK          0x40
#define BMA250_EN_ORIENT_INT__REG          BMA250_INT_ENABLE1_REG

#define BMA250_EN_FLAT_INT__POS            7
#define BMA250_EN_FLAT_INT__LEN            1
#define BMA250_EN_FLAT_INT__MSK            0x80
#define BMA250_EN_FLAT_INT__REG            BMA250_INT_ENABLE1_REG



#define BMA250_EN_HIGHG_X_INT__POS         0
#define BMA250_EN_HIGHG_X_INT__LEN         1
#define BMA250_EN_HIGHG_X_INT__MSK         0x01
#define BMA250_EN_HIGHG_X_INT__REG         BMA250_INT_ENABLE2_REG

#define BMA250_EN_HIGHG_Y_INT__POS         1
#define BMA250_EN_HIGHG_Y_INT__LEN         1
#define BMA250_EN_HIGHG_Y_INT__MSK         0x02
#define BMA250_EN_HIGHG_Y_INT__REG         BMA250_INT_ENABLE2_REG

#define BMA250_EN_HIGHG_Z_INT__POS         2
#define BMA250_EN_HIGHG_Z_INT__LEN         1
#define BMA250_EN_HIGHG_Z_INT__MSK         0x04
#define BMA250_EN_HIGHG_Z_INT__REG         BMA250_INT_ENABLE2_REG

#define BMA250_EN_NEW_DATA_INT__POS        4
#define BMA250_EN_NEW_DATA_INT__LEN        1
#define BMA250_EN_NEW_DATA_INT__MSK        0x10
#define BMA250_EN_NEW_DATA_INT__REG        BMA250_INT_ENABLE2_REG


#define BMA250_EN_LOWG_INT__POS            3
#define BMA250_EN_LOWG_INT__LEN            1
#define BMA250_EN_LOWG_INT__MSK            0x08
#define BMA250_EN_LOWG_INT__REG            BMA250_INT_ENABLE2_REG



static int bma250_smbus_read_byte(struct i2c_client *client,
	unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_read_byte_data(client, reg_addr);
    if (dummy < 0)
	return -1;
    *data = dummy & 0x000000ff;

    return 0;
}
static int bma250_smbus_write_byte(struct i2c_client *client,
	unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
    if (dummy < 0)
	return -1;
    return 0;
}
static int bma250_get_interruptstatus1(struct i2c_client *client, unsigned char
	*intstatus)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma250_smbus_read_byte(client, BMA250_STATUS1_REG, &data);
    *intstatus = data;

    return comres;
}
static int bma250_set_Int_Mode(struct i2c_client *client, unsigned char Mode)
{
    int comres = 0;
    unsigned char data = 0;



    comres = bma250_smbus_read_byte(client,
	    BMA250_INT_MODE_SEL__REG, &data);
    data = BMA250_SET_BITSLICE(data, BMA250_INT_MODE_SEL, Mode);
    comres = bma250_smbus_write_byte(client,
	    BMA250_INT_MODE_SEL__REG, &data);


    return comres;
}


static int bma250_set_slope_duration(struct i2c_client *client, unsigned char
	duration)
{
    int comres = 0;
    unsigned char data = 0;


    comres = bma250_smbus_read_byte(client,
	    BMA250_SLOPE_DUR__REG, &data);
    data = BMA250_SET_BITSLICE(data, BMA250_SLOPE_DUR, duration);
    comres = bma250_smbus_write_byte(client,
	    BMA250_SLOPE_DUR__REG, &data);


    return comres;
}

static int bma250_set_slope_threshold(struct i2c_client *client,
	unsigned char threshold)
{
    int comres = 0;
    unsigned char data = 0;


    comres = bma250_smbus_read_byte(client,
	    BMA250_SLOPE_THRES__REG, &data);
    data = BMA250_SET_BITSLICE(data, BMA250_SLOPE_THRES, threshold);
    comres = bma250_smbus_write_byte(client,
	    BMA250_SLOPE_THRES__REG, &data);


    return comres;
}

static int bma250_set_Int_Enable(struct i2c_client *client, unsigned char
	InterruptType , unsigned char value)
{
    int comres = 0;
    unsigned char data1 = 0, data2 = 0;



    comres = bma250_smbus_read_byte(client, BMA250_INT_ENABLE1_REG, &data1);
    comres = bma250_smbus_read_byte(client, BMA250_INT_ENABLE2_REG, &data2);


    value = value & 1;
    switch (InterruptType) {
	case 0:

	    data2 = BMA250_SET_BITSLICE(data2, BMA250_EN_LOWG_INT, value);
	    break;
	case 1:


	    data2 = BMA250_SET_BITSLICE(data2, BMA250_EN_HIGHG_X_INT,
		    value);
	    break;
	case 2:


	    data2 = BMA250_SET_BITSLICE(data2, BMA250_EN_HIGHG_Y_INT,
		    value);
	    break;
	case 3:


	    data2 = BMA250_SET_BITSLICE(data2, BMA250_EN_HIGHG_Z_INT,
		    value);
	    break;
	case 4:


	    data2 = BMA250_SET_BITSLICE(data2, BMA250_EN_NEW_DATA_INT,
		    value);
	    break;
	case 5:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_SLOPE_X_INT,
		    value);
	    break;
	case 6:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_SLOPE_Y_INT,
		    value);
	    break;
	case 7:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_SLOPE_Z_INT,
		    value);
	    break;

	case 8:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_SINGLE_TAP_INT,
		    value);
	    break;
	case 9:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_DOUBLE_TAP_INT,
		    value);
	    break;
	case 10:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_ORIENT_INT, value);
	    break;
	case 11:


	    data1 = BMA250_SET_BITSLICE(data1, BMA250_EN_FLAT_INT, value);
	    break;
	default:
	    break;
    }
    comres = bma250_smbus_write_byte(client, BMA250_INT_ENABLE1_REG,
	    &data1);
    comres = bma250_smbus_write_byte(client, BMA250_INT_ENABLE2_REG,
	    &data2);


    return comres;
}

static int bma250_set_int1_pad_sel(struct i2c_client *client, unsigned char
	int1sel)
{
    int comres = 0;
    unsigned char data = 0;
    unsigned char state = 0;
    state = 0x01;


    switch (int1sel) {
	case 0:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_LOWG__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_LOWG,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_LOWG__REG, &data);
	    break;
	case 1:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_HIGHG__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_HIGHG,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_HIGHG__REG, &data);
	    break;
	case 2:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_SLOPE__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_SLOPE,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_SLOPE__REG, &data);
	    break;
	case 3:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_DB_TAP__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_DB_TAP,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_DB_TAP__REG, &data);
	    break;
	case 4:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_SNG_TAP__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_SNG_TAP,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_SNG_TAP__REG, &data);
	    break;
	case 5:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_ORIENT__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_ORIENT,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_ORIENT__REG, &data);
	    break;
	case 6:
	    comres = bma250_smbus_read_byte(client,
		    BMA250_EN_INT1_PAD_FLAT__REG, &data);
	    data = BMA250_SET_BITSLICE(data, BMA250_EN_INT1_PAD_FLAT,
		    state);
	    comres = bma250_smbus_write_byte(client,
		    BMA250_EN_INT1_PAD_FLAT__REG, &data);
	    break;
	default:
	    break;
    }

    return comres;
}
static void bma250_irq_work_func(struct work_struct *work)
{
    struct bma250_data *bma250 = container_of((struct work_struct *)work,
	    struct bma250_data, irq_work);

    unsigned char status = 0;



    bma250_get_interruptstatus1(this_client, &status);
    I("bma250_irq_work_func, status = 0x%x\n", status);
    input_report_rel(bma250->input_cir,
	    SLOP_INTERRUPT,
	    SLOPE_INTERRUPT_X_NEGATIVE_HAPPENED);
    input_report_rel(bma250->input_cir,
	    SLOP_INTERRUPT,
	    SLOPE_INTERRUPT_Y_NEGATIVE_HAPPENED);
    input_report_rel(bma250->input_cir,
	    SLOP_INTERRUPT,
	    SLOPE_INTERRUPT_X_HAPPENED);
    input_report_rel(bma250->input_cir,
	    SLOP_INTERRUPT,
	    SLOPE_INTERRUPT_Y_HAPPENED);
    input_sync(bma250->input_cir);
    enable_irq(bma250->irq);

}

static irqreturn_t bma250_irq_handler(int irq, void *handle)
{


    struct bma250_data *data = handle;

    disable_irq_nosync(data->irq);

    if (data == NULL)
	return IRQ_HANDLED;
    if (this_client == NULL)
	return IRQ_HANDLED;


    schedule_work(&data->irq_work);

    return IRQ_HANDLED;


}
static ssize_t bma250_enable_interrupt(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
    unsigned long enable;
    int error;

    error = strict_strtoul(buf, 10, &enable);
    if (error)
	return error;
    I("bma250_enable_interrupt, power_key_pressed = %d\n", power_key_pressed);
    if(enable == 1 && !power_key_pressed){

	cir_flag = 1;


	error = bma250_set_Int_Mode(this_client, 1);

	error += bma250_set_slope_duration(this_client, 0x01);
	error += bma250_set_slope_threshold(this_client, 0x07);


	error += bma250_set_Int_Enable(this_client, 5, 1);
	error += bma250_set_Int_Enable(this_client, 6, 1);
	error += bma250_set_Int_Enable(this_client, 7, 0);
	error += bma250_set_int1_pad_sel(this_client, PAD_SLOP);

	BMA_set_mode(bma250_MODE_NORMAL);

	if (error)
	    return error;
	I("Always Ready enable = 1 \n");

    }  else if(enable == 0){

	error += bma250_set_Int_Enable(this_client, 5, 0);
	error += bma250_set_Int_Enable(this_client, 6, 0);
	error += bma250_set_Int_Enable(this_client, 7, 0);

	power_key_pressed = 0;
	cir_flag = 0;
	if (error)
	    return error;
	I("Always Ready enable = 0 \n");

    }       return count;
}
static ssize_t bma250_clear_powerkey_pressed(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
    unsigned long powerkey_pressed;
    int error;
    error = strict_strtoul(buf, 10, &powerkey_pressed);
    if (error)
	return error;

    if(powerkey_pressed == 1) {
	power_key_pressed = 1;
    }
    else if(powerkey_pressed == 0) {
	power_key_pressed = 0;
    }
    return count;
}
static ssize_t bma250_get_powerkry_pressed(struct device *dev,
	struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", power_key_pressed);
}
static DEVICE_ACCESSORY_ATTR(enable_cir_interrupt, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP,
	NULL, bma250_enable_interrupt);
static DEVICE_ACCESSORY_ATTR(clear_powerkey_flag, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP,
	bma250_get_powerkry_pressed, bma250_clear_powerkey_pressed);
#endif
static int BMA_I2C_RxData(char *rxData, int length)
{
	int retry;
	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = rxData,
		},
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	for (retry = 0; retry <= RETRY_TIMES; retry++) {
		if (i2c_transfer(this_client->adapter, msgs, 2) > 0)
			break;
		else
			mdelay(10);
	}

	if (retry > RETRY_TIMES) {
		E("%s: retry over %d\n", __func__, RETRY_TIMES);
		return -EIO;
	} else
		return 0;
}

static int BMA_I2C_TxData(char *txData, int length)
{
	int retry;
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (retry = 0; retry <= RETRY_TIMES; retry++) {
		if (i2c_transfer(this_client->adapter, msg, 1) > 0)
			break;
		else
			mdelay(10);
	}

	if (retry > RETRY_TIMES) {
		E("%s: retry over %d\n", __func__, RETRY_TIMES);
		return -EIO;
	} else
		return 0;
}

static int BMA_Init(void)
{
	char buffer[4] = "";
	int ret;
	unsigned char range = 0, bw = 0;
	char range_from_chip = 0, bw_from_chip = 0;
	int i = 0;

	memset(buffer, 0, 4);

	buffer[0] = bma250_RANGE_SEL_REG;
	ret = BMA_I2C_RxData(buffer, 2);
	if (ret < 0)
		return -1;

	range_from_chip = buffer[0];
	bw_from_chip = buffer[1];
	D("%s++: range = 0x%02X, bw = 0x%02X\n",
		__func__, range_from_chip, bw_from_chip);

	range = DEFAULT_RANGE;
	bw = DEFAULT_BW;

	for (i = 0; i < RETRY_TIMES; i++) {
		if (bw_from_chip != DEFAULT_BW) {
			buffer[1] = bw;
			buffer[0] = bma250_BW_SEL_REG;
			D("%s: i = %d, bw_from_chip = 0x%02X, writing buffer[1] = 0x%02X",
				__func__, i, bw_from_chip, buffer[1]);
			ret = BMA_I2C_TxData(buffer, 2);
			if (ret < 0) {
				E("%s: Write bma250_BW_SEL_REG fail\n",
					__func__);
				return -1;
			}

			buffer[0] = bma250_BW_SEL_REG;
			ret = BMA_I2C_RxData(buffer, 1);
			if (ret < 0) {
				E("%s: Read bma250_BW_SEL_REG fail\n",
					__func__);
				return -1;
			}
			bw_from_chip = buffer[0];
		} else
			break;
	}

	for (i = 0; i < RETRY_TIMES; i++) {
		if (range_from_chip != DEFAULT_RANGE) {
			buffer[1] = range;
			buffer[0] = bma250_RANGE_SEL_REG;
			D("%s: i = %d, range_from_chip = 0x%02X, writing buffer[1] = 0x%02X",
				__func__, i, range_from_chip, buffer[1]);
			ret = BMA_I2C_TxData(buffer, 2);
			if (ret < 0) {
				E("%s: Write bma250_BW_SEL_REG fail\n",
					__func__);
				return -1;
			}

			buffer[0] = bma250_RANGE_SEL_REG;
			ret = BMA_I2C_RxData(buffer, 1);
			if (ret < 0) {
				E("%s: Read bma250_RANGE_SEL_REG fail\n",
					__func__);
				return -1;
			}
			range_from_chip = buffer[0];
		} else
			break;
	}

	D("%s--: range = 0x%02X, bw = 0x%02X\n",
		__func__, range_from_chip, bw_from_chip);

	return 0;

}

static int BMA_TransRBuff(short *rbuf)
{
	char buffer[6];
	int ret;

	memset(buffer, 0, 6);

	buffer[0] = bma250_X_AXIS_LSB_REG;
	ret = BMA_I2C_RxData(buffer, 6);
	if (ret < 0)
		return ret;


	rbuf[0] = (short)(buffer[1] << 8 | buffer[0]);
	rbuf[0] >>= 6;
	rbuf[1] = (short)(buffer[3] << 8 | buffer[2]);
	rbuf[1] >>= 6;
	rbuf[2] = (short)(buffer[5] << 8 | buffer[4]);
	rbuf[2] >>= 6;

	DIF("%s: (x, y, z) = (%d, %d, %d)\n",
		__func__, rbuf[0], rbuf[1], rbuf[2]);

	return 1;
}

static int BMA_set_mode(unsigned char mode)
{
	char buffer[2] = "";
	int ret = 0;
	unsigned char data1 = 0;

	I("Gsensor %s mode = 0x%02x\n", mode ? "disable" : "enable", mode);

	memset(buffer, 0, 2);

#ifdef CONFIG_CIR_ALWAYS_READY
	if(cir_flag && mode == bma250_MODE_SUSPEND) {
	    return 0;
	} else {
#endif

	if (mode < 2) {
		buffer[0] = bma250_MODE_CTRL_REG;
		ret = BMA_I2C_RxData(buffer, 1);
		if (ret < 0)
			return -1;


		switch (mode) {
		case bma250_MODE_NORMAL:
			ignore_first_event = 1;
			data1 = buffer[0] & 0x7F;
			break;
		case bma250_MODE_SUSPEND:
			data1 = buffer[0] | 0x80;
			break;
		default:
			break;
		}


		buffer[0] = bma250_MODE_CTRL_REG;
		buffer[1] = data1;
		ret = BMA_I2C_TxData(buffer, 2);
	} else
		ret = E_OUT_OF_RANGE;

#ifdef CONFIG_CIR_ALWAYS_READY
	}
#endif


	return ret;
}

static int BMA_GET_INT(void)
{
	int ret;
	ret = gpio_get_value(pdata->intr);
	return ret;
}

static int bma_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int bma_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long bma_ioctl(struct file *file, unsigned int cmd,
	   unsigned long arg)
{

	void __user *argp = (void __user *)arg;

	char rwbuf[8] = "";
	int ret = -1;
	short buf[8], temp;
	int kbuf = 0;

	DIF("%s: cmd = 0x%x\n", __func__, cmd);

	switch (cmd) {
	case BMA_IOCTL_READ:
	case BMA_IOCTL_WRITE:
	case BMA_IOCTL_SET_MODE:
	case BMA_IOCTL_SET_CALI_MODE:
	case BMA_IOCTL_SET_UPDATE_USER_CALI_DATA:
		if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case BMA_IOCTL_READ_ACCELERATION:
		if (copy_from_user(&buf, argp, sizeof(buf)))
			return -EFAULT;
		break;
	case BMA_IOCTL_WRITE_CALI_VALUE:
		if (copy_from_user(&kbuf, argp, sizeof(kbuf)))
			return -EFAULT;
		break;
	default:
		break;
	}

	switch (cmd) {
	case BMA_IOCTL_INIT:
		ret = BMA_Init();
		if (ret < 0)
			return ret;
		break;
	case BMA_IOCTL_READ:
		if (rwbuf[0] < 1)
			return -EINVAL;
		break;
	case BMA_IOCTL_WRITE:
		if (rwbuf[0] < 2)
			return -EINVAL;
		break;
	case BMA_IOCTL_WRITE_CALI_VALUE:
		pdata->gs_kvalue = kbuf;
		I("%s: Write calibration value: 0x%X\n",
			__func__, pdata->gs_kvalue);
		break;
	case BMA_IOCTL_READ_ACCELERATION:
		ret = BMA_TransRBuff(&buf[0]);
		if (ret < 0)
			return ret;
		break;
	case BMA_IOCTL_READ_CALI_VALUE:
		if ((pdata->gs_kvalue & (0x67 << 24)) != (0x67 << 24)) {
			rwbuf[0] = 0;
			rwbuf[1] = 0;
			rwbuf[2] = 0;
		} else {
			rwbuf[0] = (pdata->gs_kvalue >> 16) & 0xFF;
			rwbuf[1] = (pdata->gs_kvalue >>  8) & 0xFF;
			rwbuf[2] =  pdata->gs_kvalue        & 0xFF;
		}
		DIF("%s: CALI(x, y, z) = (%d, %d, %d)\n",
			__func__, rwbuf[0], rwbuf[1], rwbuf[2]);
		break;
	case BMA_IOCTL_SET_MODE:
		BMA_set_mode(rwbuf[0]);
		break;
	case BMA_IOCTL_GET_INT:
		temp = BMA_GET_INT();
		break;
	case BMA_IOCTL_GET_CHIP_LAYOUT:
		if (pdata)
			temp = pdata->chip_layout;
		break;
	case BMA_IOCTL_GET_CALI_MODE:
		if (pdata)
			temp = pdata->calibration_mode;
		break;
	case BMA_IOCTL_SET_CALI_MODE:
		if (pdata)
			pdata->calibration_mode = rwbuf[0];
		break;
	case BMA_IOCTL_GET_UPDATE_USER_CALI_DATA:
		temp = update_user_calibrate_data;
		break;
	case BMA_IOCTL_SET_UPDATE_USER_CALI_DATA:
		update_user_calibrate_data = rwbuf[0];
		break;

	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case BMA_IOCTL_READ:
		break;
	case BMA_IOCTL_READ_ACCELERATION:
		if (copy_to_user(argp, &buf, sizeof(buf)))
			return -EFAULT;
		break;
	case BMA_IOCTL_READ_CALI_VALUE:
		if (copy_to_user(argp, &rwbuf, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case BMA_IOCTL_GET_INT:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	case BMA_IOCTL_GET_CHIP_LAYOUT:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	case BMA_IOCTL_GET_CALI_MODE:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	case BMA_IOCTL_GET_UPDATE_USER_CALI_DATA:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	default:
		break;
	}

	return 0;
}

static void gsensor_poll_work_func(struct work_struct *work)
{
	short buffer[8];
	short gval[3], offset_buf[3], g_acc[3];
	short m_Alayout[3][3];
	int i, j, k;
	struct akm8975_data *akm = container_of(work, struct akm8975_data,
						input_work.work);

	mutex_lock(&gsensor_lock);

	if (atomic_read(&a_flag)) {
		if (BMA_TransRBuff(&buffer[0]) < 0) {
			E("gsensor_poll_work_func: gsensor_read error\n");
			mutex_unlock(&gsensor_lock);
			return;
		}

		if ((pdata->gs_kvalue & (0x67 << 24)) != (0x67 << 24)) {
			offset_buf[0] = 0;
			offset_buf[1] = 0;
			offset_buf[2] = 0;
		} else {
			offset_buf[0] = (pdata->gs_kvalue >> 16) & 0xFF;
			offset_buf[1] = (pdata->gs_kvalue >>  8) & 0xFF;
			offset_buf[2] =  pdata->gs_kvalue        & 0xFF;

			for (i = 0; i < 3; i++) {
				if (offset_buf[i] > 127)
					offset_buf[i] = offset_buf[i] - 256;
			}
		}
		for (i = 0; i < 3; i++) {

			gval[i] = ((buffer[i] + offset_buf[i] - user_offset[i])
					*ASENSE_TARGET) / 256;
		}
		for (j = 0; j < 3; j++) {
			for (k = 0; k < 3; k++)
				m_Alayout[j][k] = pdata->layouts[3][j][k];
		}


		g_acc[0] = (gval[0])*m_Alayout[0][0] +
			   (gval[1])*m_Alayout[0][1] +
			   (gval[2])*m_Alayout[0][2];
		g_acc[1] = (gval[0])*m_Alayout[1][0] +
			   (gval[1])*m_Alayout[1][1] +
			   (gval[2])*m_Alayout[1][2];
		g_acc[2] = (gval[0])*m_Alayout[2][0] +
			   (gval[1])*m_Alayout[2][1] +
			   (gval[2])*m_Alayout[2][2];

		if (ignore_first_event >= 3) {
			input_report_abs(akm->input_dev, ABS_X, g_acc[0]);
			input_report_abs(akm->input_dev, ABS_Y, g_acc[1]);
			input_report_abs(akm->input_dev, ABS_Z, g_acc[2]);
			input_sync(akm->input_dev);
		} else {
			ignore_first_event++;
			D("Ignore X = %d Ignore Y = %d Ignore Z = %d\n ", g_acc[0], g_acc[1], g_acc[2]);
		}

		numCount++;
		if ((numCount % TIME_LOG) == 0)
			D("GSensor [%d,%d,%d]\n", gval[0], gval[1], gval[2]);
	}


	mutex_unlock(&gsensor_lock);
	schedule_delayed_work(&akm->input_work,
			msecs_to_jiffies(akm->poll_interval));
}

int gsensor_off(void)
{
	int 	ret = 0;

	mutex_lock(&gsensor_lock);
	D("gsensor_off++\n");

	numCount = 0;

	BMA_set_mode(bma250_MODE_SUSPEND);

	D("gsensor_off--\n");
	mutex_unlock(&gsensor_lock);

	return ret;
}

int sensor_open(void)
{
	int ret;
	char buffer, mode = -1;
	int i = 0;

	D("sensor_open++\n");

	if (!(atomic_read(&a_flag) || atomic_read(&t_flag))) {
		D("sensor_open: No need to open due to the flag: "
		  "a_flag = %d, t_flag = %d\n",
			atomic_read(&a_flag), atomic_read(&t_flag));
		return 0;
	}

	if (atomic_read(&a_flag)) {
		for (i = 0; i < RETRY_TIMES; i++) {
			BMA_set_mode(bma250_MODE_NORMAL);

			usleep(2000);

			ret = BMA_Init();
			if (ret) {
				E("BMA_Init error, ret = %d\n", ret);
				return ret;
			}

			buffer = bma250_MODE_CTRL_REG;
			ret = BMA_I2C_RxData(&buffer, 1);
			if (ret < 0) {
				E("%s: Read bma250_MODE_CTRL_REG fail!\n",
					__func__);
				return -1;
			}
			mode = ((buffer & 0x80) >> 7);
			if (mode == 1) {
				D("%s: Set mode retry = %d\n", __func__, i);
				if (i >= (RETRY_TIMES - 1)) {
					E("%s: Set mode Fail!!\n", __func__);
					return -1;
				}
			} else
				break;
		}
	}

	if (akm8975_misc_data->poll_interval <= 0) {
		D("[Gsensor] the delay interval was set to %d, use default"
			" value 200\n",
			akm8975_misc_data->poll_interval);
		akm8975_misc_data->poll_interval = 200;
	}

	#if 1
	akm8975_misc_data->input_dev->absinfo[ABS_X].value = 0xffff;
	akm8975_misc_data->input_dev->absinfo[ABS_Y].value = 0xffff;
	akm8975_misc_data->input_dev->absinfo[ABS_Z].value = 0xffff;
	#endif

	sensors_on = true;

	#if 1
	schedule_delayed_work(&akm8975_misc_data->input_work, 0);
	#else
	schedule_delayed_work(&akm8975_misc_data->input_work,
		msecs_to_jiffies(akm8975_misc_data->poll_interval));
	#endif

	return 0;
}

int sensor_close(void)
{
	int ret = 0;

	D("sensor_close++\n");

	if (!(atomic_read(&a_flag))) {
		ret = gsensor_off();
		if (ret < 0)
			E("gsensor_off error\n");
	}

	cancel_delayed_work(&akm8975_misc_data->input_work);
	sensors_on = false;

	return ret;
}

void sensor_poll_interval_set(uint32_t poll_interval)
{

	D("sensor_poll_interval_set: %d ms\n", poll_interval);

	if (poll_interval <= 0) {
		D("sensor_poll_interval_reset: 200 ms\n");
		poll_interval = 200;
	}

	akm8975_misc_data->poll_interval = poll_interval;

	if (sensors_on == false && suspend_flag == 0)
		sensor_open();
}

int sensor_poll_interval_get(void)
{
	D("sensor_poll_interval_get: %d msec\n",
		akm8975_misc_data->poll_interval);
	return akm8975_misc_data->poll_interval;
}

int akm_aot_open(struct inode *inode, struct file *file)
{
	I("gsensor_aot_open\n");
	sensor_open();

	return 0;
}

int akm_aot_release(struct inode *inode, struct file *file)
{
	I("gsensor_aot_release\n");
	sensor_close();

	return 0;
}

long akm_aot_ioctl(struct file *file,
	      unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	short flag = 0;


	switch (cmd) {
	case ECS_IOCTL_APP_SET_MFLAG:
	case ECS_IOCTL_APP_SET_AFLAG:
	case ECS_IOCTL_APP_SET_TFLAG:
	case ECS_IOCTL_APP_SET_MVFLAG:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		if (flag < 0 || flag > 1)
			return -EINVAL;
		break;
	case ECS_IOCTL_APP_SET_DELAY:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		break;
	case ECS_IOCTL_SET_YPR:
		if (copy_from_user(&user_offset, argp, sizeof(user_offset)))
			return -EFAULT;
		D("akm ECS_IOCTL_SET_YPR: %d, %d, %d\n", user_offset[0],
			user_offset[1], user_offset[2]);
		break;
	default:
		break;
	}

	switch (cmd) {
	case ECS_IOCTL_APP_SET_MFLAG:
		atomic_set(&m_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_MFLAG:
		flag = atomic_read(&m_flag);
		break;
	case ECS_IOCTL_APP_SET_AFLAG:
		atomic_set(&a_flag, flag);
		D("ESC_IOCTL_APP_SET_AFLAG: %d\n", atomic_read(&a_flag));
		if (flag == 0) {
			D("set Aflag to close gsensor\n");
			sensor_close();
		}
		break;
	case ECS_IOCTL_APP_GET_AFLAG:
		flag = atomic_read(&a_flag);
		D("ESC_IOCTL_APP_GET_AFLAG: %d\n", atomic_read(&a_flag));
		break;
	case ECS_IOCTL_APP_SET_TFLAG:
		atomic_set(&t_flag, flag);
		D("ESC_IOCTL_APP_SET_TFLAG: %d\n", atomic_read(&t_flag));
		if (flag == 0) {
			D("set Tflag to close Tsensor\n");
			sensor_close();
		}
		break;
	case ECS_IOCTL_APP_GET_TFLAG:
		flag = atomic_read(&t_flag);
		D("ESC_IOCTL_APP_GET_TFLAG: %d\n", atomic_read(&t_flag));
		break;
	case ECS_IOCTL_APP_SET_MVFLAG:
		atomic_set(&mv_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_MVFLAG:
		flag = atomic_read(&mv_flag);
		break;
	case ECS_IOCTL_APP_SET_DELAY:
		D("ESC_IOCTL_APP_SET_DELAY: %d", flag);
		sensor_poll_interval_set(flag);
		break;
	case ECS_IOCTL_APP_GET_DELAY:
		flag = sensor_poll_interval_get();
		break;
	case ECS_IOCTL_SET_YPR:
		break;
	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case ECS_IOCTL_APP_GET_MFLAG:
	case ECS_IOCTL_APP_GET_AFLAG:
	case ECS_IOCTL_APP_GET_TFLAG:
	case ECS_IOCTL_APP_GET_MVFLAG:
	case ECS_IOCTL_APP_GET_DELAY:
		if (copy_to_user(argp, &flag, sizeof(flag)))
			return -EFAULT;
		break;
	case ECS_IOCTL_SET_YPR:
		break;
	default:
		break;
	}

	return 0;
}

const struct file_operations akm_aot_fops = {
	.owner = THIS_MODULE,
	.open = akm_aot_open,
	.release = akm_aot_release,
	.unlocked_ioctl = akm_aot_ioctl,
};

struct miscdevice akm_aot_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "akm8975_aot",
	.fops = &akm_aot_fops,
};

#ifdef EARLY_SUSPEND_BMA

static void bma250_early_suspend(struct early_suspend *handler)
{
	if (!atomic_read(&PhoneOn_flag))
		BMA_set_mode(bma250_MODE_SUSPEND);
	else
		D("bma250_early_suspend: PhoneOn_flag is set\n");
}

static void bma250_late_resume(struct early_suspend *handler)
{
	BMA_set_mode(bma250_MODE_NORMAL);
}

#else

static int bma250_suspend(struct i2c_client *client, pm_message_t mesg)
{
	BMA_set_mode(bma250_MODE_SUSPEND);

	return 0;
}

static int bma250_resume(struct i2c_client *client)
{
	BMA_set_mode(bma250_MODE_NORMAL);
	return 0;
}
#endif

static ssize_t bma250_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *s = buf;
	s += sprintf(s, "%d\n", atomic_read(&PhoneOn_flag));
	return s - buf;
}

static ssize_t bma250_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	if (count == (strlen("enable") + 1) &&
	   strncmp(buf, "enable", strlen("enable")) == 0) {
		atomic_set(&PhoneOn_flag, 1);
		D("bma250_store: PhoneOn_flag=%d\n",
			atomic_read(&PhoneOn_flag));
		return count;
	}
	if (count == (strlen("disable") + 1) &&
	   strncmp(buf, "disable", strlen("disable")) == 0) {
		atomic_set(&PhoneOn_flag, 0);
		D("bma250_store: PhoneOn_flag=%d\n",
			atomic_read(&PhoneOn_flag));
		return count;
	}
	E("bma250_store: invalid argument\n");
	return -EINVAL;

}

static DEVICE_ACCESSORY_ATTR(PhoneOnOffFlag, 0664, \
	bma250_show, bma250_store);

static ssize_t debug_flag_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	char *s = buf;
	char buffer, range = -1, bandwidth = -1, mode = -1;
	int ret;

	buffer = bma250_BW_SEL_REG;
	ret = BMA_I2C_RxData(&buffer, 1);
	if (ret < 0)
		return -1;
	bandwidth = (buffer & 0x1F);

	buffer = bma250_RANGE_SEL_REG;
	ret = BMA_I2C_RxData(&buffer, 1);
	if (ret < 0)
		return -1;
	range = (buffer & 0xF);

	buffer = bma250_MODE_CTRL_REG;
	ret = BMA_I2C_RxData(&buffer, 1);
	if (ret < 0)
		return -1;
	mode = ((buffer & 0x80) >> 7);

	s += sprintf(s, "debug_flag = %d, range = 0x%x, bandwidth = 0x%x, "
		"mode = 0x%x\n", debug_flag, range, bandwidth, mode);

	return s - buf;
}
static ssize_t debug_flag_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{

	debug_flag = -1;
	sscanf(buf, "%d", &debug_flag);

	D("%s: debug_flag = %d\n", __func__, debug_flag);
	return count;

}

static DEVICE_ACCESSORY_ATTR(debug_en, 0664, \
	debug_flag_show, debug_flag_store);

static int bma250_registerAttr(void)
{
	int ret;
	struct class *htc_accelerometer_class;
	struct device *accelerometer_dev;
#ifdef CONFIG_CIR_ALWAYS_READY
	 struct class *bma250_powerkey_class = NULL;
	struct device *bma250_powerkey_dev = NULL;
#endif

	htc_accelerometer_class = class_create(THIS_MODULE,
					"htc_accelerometer");
	if (IS_ERR(htc_accelerometer_class)) {
		ret = PTR_ERR(htc_accelerometer_class);
		htc_accelerometer_class = NULL;
		goto err_create_class;
	}

	accelerometer_dev = device_create(htc_accelerometer_class,
				NULL, 0, "%s", "accelerometer");
	if (unlikely(IS_ERR(accelerometer_dev))) {
		ret = PTR_ERR(accelerometer_dev);
		accelerometer_dev = NULL;
		goto err_create_accelerometer_device;
	}


	ret = device_create_file(accelerometer_dev, &dev_attr_PhoneOnOffFlag);
	if (ret)
		goto err_create_accelerometer_device_file;


	ret = device_create_file(accelerometer_dev, &dev_attr_debug_en);
	if (ret)
		goto err_create_accelerometer_debug_en_device_file;

#ifdef CONFIG_CIR_ALWAYS_READY
	ret = device_create_file(accelerometer_dev, &dev_attr_enable_cir_interrupt);

	if (ret)
		goto err_create_accelerometer_debug_en_device_file;

	bma250_powerkey_class = class_create(THIS_MODULE, "bma250_powerkey");
	if (IS_ERR(bma250_powerkey_class)) {
	    ret = PTR_ERR(bma250_powerkey_class);
	    bma250_powerkey_class = NULL;
	    E("%s: could not allocate bma250_powerkey_class\n", __func__);
	    goto err_create_bma250_powerkey_class_failed;
	}
	bma250_powerkey_dev= device_create(bma250_powerkey_class,
		NULL, 0, "%s", "bma250");

	ret = device_create_file(bma250_powerkey_dev, &dev_attr_clear_powerkey_flag);


	if (ret)
		goto err_create_accelerometer_clear_powerkey_flag_device_file;
#endif
	return 0;

#ifdef CONFIG_CIR_ALWAYS_READY
err_create_bma250_powerkey_class_failed:
	class_destroy(bma250_powerkey_class);
err_create_accelerometer_clear_powerkey_flag_device_file:
	device_remove_file(accelerometer_dev, &dev_attr_enable_cir_interrupt);
#endif
err_create_accelerometer_debug_en_device_file:
	device_remove_file(accelerometer_dev, &dev_attr_PhoneOnOffFlag);
err_create_accelerometer_device_file:
	device_unregister(accelerometer_dev);
err_create_accelerometer_device:
	class_destroy(htc_accelerometer_class);
err_create_class:

	return ret;
}

static const struct file_operations bma_fops = {
	.owner = THIS_MODULE,
	.open = bma_open,
	.release = bma_release,
	.unlocked_ioctl = bma_ioctl,
};

static struct miscdevice bma_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bma150",
	.fops = &bma_fops,
};

static int bma250_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct bma250_data *bma;
	char buffer[2];
	int err = 0;
	struct akm8975_data *akm;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	bma = kzalloc(sizeof(struct bma250_data), GFP_KERNEL);
	if (!bma) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, bma);

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		E("bma250_init_client: platform data is NULL\n");
		goto exit_platform_data_null;
	}

	pdata->gs_kvalue = gs_kvalue;
	I("BMA250 G-sensor I2C driver: gs_kvalue = 0x%X\n",
		pdata->gs_kvalue);

	this_client = client;
	ignore_first_event = 0;

	buffer[0] = bma250_CHIP_ID_REG;
	err = BMA_I2C_RxData(buffer, 1);
	if (err < 0)
		goto exit_wrong_ID;
	D("%s: CHIP ID = 0x%02x\n", __func__, buffer[0]);
	if ((buffer[0] != 0x3) && (buffer[0] != 0xF9)) {
		E("Wrong chip ID of BMA250 or BMA250E!!\n");
		goto exit_wrong_ID;
	}

	err = BMA_Init();
	if (err < 0) {
		E("bma250_probe: bma_init failed\n");
		goto exit_init_failed;
	}

	err = misc_register(&bma_device);
	if (err) {
		E("bma250_probe: device register failed\n");
		goto exit_misc_device_register_failed;
	}

#ifdef EARLY_SUSPEND_BMA
	bma->early_suspend.suspend = bma250_early_suspend;
	bma->early_suspend.resume = bma250_late_resume;
	register_early_suspend(&bma->early_suspend);
#endif

	err = bma250_registerAttr();
	if (err) {
		E("%s: set spi_bma150_registerAttr fail!\n", __func__);
		goto err_registerAttr;
	}

	akm = kzalloc(sizeof(struct akm8975_data), GFP_KERNEL);
	if (!akm) {
		err = -ENOMEM;
		E(
		       "%s : Failed"
		       " kzalloc data\n", __func__);
		goto exit_alloc_data_failed_akm;
	}
	akm8975_misc_data = akm;
	akm->input_dev = input_allocate_device();

	if (!akm->input_dev) {
		err = -ENOMEM;
		E(
		       "%s : Failed"
		       " to allocate input device\n", __func__);
		goto exit_input_dev_alloc_failed;
	}

#ifdef CONFIG_CIR_ALWAYS_READY

	bma->input_cir = input_allocate_device();
	if (!bma->input_cir) {
	    kfree(bma);
	    input_free_device(akm->input_dev);
	    return -ENOMEM;
	}

	bma->input_cir->name = "CIRSensor";
	bma->input_cir->id.bustype = BUS_I2C;

	input_set_capability(bma->input_cir, EV_REL, SLOP_INTERRUPT);
#endif
	set_bit(EV_ABS, akm->input_dev->evbit);

	input_set_abs_params(akm->input_dev, ABS_X, -1872, 1872, 0, 0);

	input_set_abs_params(akm->input_dev, ABS_Y, -1872, 1872, 0, 0);

	input_set_abs_params(akm->input_dev, ABS_Z, -1872, 1872, 0, 0);

	input_set_abs_params(akm->input_dev, ABS_WHEEL, -32768, 3, 0, 0);

	akm->input_dev->name = "compass";

	err = input_register_device(akm->input_dev);

	if (err) {
		E(
		       "%s : Unable to register"
		       " input device: %s\n", __func__,
		       akm->input_dev->name);
		goto exit_input_register_device_failed;
	}
#ifdef CONFIG_CIR_ALWAYS_READY
	err = input_register_device(bma->input_cir);
	if (err < 0) {
	    goto err_register_input_cir_device;
	}
#endif
	err = misc_register(&akm_aot_device);
	if (err) {
		E(
		       "%s : akm_aot_device register "
		       "failed\n",  __func__);
		goto exit_misc_device_register_failed_akm;
	}
	mutex_init(&gsensor_lock);

	INIT_DELAYED_WORK(&akm->input_work, gsensor_poll_work_func);

#ifdef CONFIG_CIR_ALWAYS_READY
	INIT_WORK(&bma->irq_work, bma250_irq_work_func);
	bma->irq = client->irq;
	err = request_irq(bma->irq, bma250_irq_handler, IRQF_TRIGGER_RISING,
		"bma250", bma);
	enable_irq_wake(bma->irq);
	if (err)
	    E("could not request irq\n");

#endif
	atomic_set(&a_flag, 1);
	I("%s: BMA250_NO_COMP: I2C retry 10 times version. OK\n", __func__);

	debug_flag = 0;
	suspend_flag = 0;
	sensors_on = false;
	numCount = 0;

	return 0;

exit_misc_device_register_failed_akm:
#ifdef CONFIG_CIR_ALWAYS_READY
	input_unregister_device(bma->input_cir);
err_register_input_cir_device:
#endif
exit_input_register_device_failed:
#ifdef CONFIG_CIR_ALWAYS_READY
	input_free_device(bma->input_cir);
#endif
	input_free_device(akm->input_dev);
exit_input_dev_alloc_failed:
	kfree(akm);
exit_alloc_data_failed_akm:
err_registerAttr:
exit_misc_device_register_failed:
exit_init_failed:
exit_wrong_ID:
exit_platform_data_null:
	kfree(bma);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int bma250_remove(struct i2c_client *client)
{
	struct bma250_data *bma = i2c_get_clientdata(client);
	kfree(bma);
	return 0;
}

static const struct i2c_device_id bma250_id[] = {
	{ BMA250_I2C_NAME_REMOVE_ECOMPASS, 0 },
	{ }
};

static struct i2c_driver bma250_driver = {
	.probe = bma250_probe,
	.remove = bma250_remove,
	.id_table	= bma250_id,

#ifndef EARLY_SUSPEND_BMA
	.suspend = bma250_suspend,
	.resume = bma250_resume,
#endif
	.driver = {
		   .name = BMA250_I2C_NAME_REMOVE_ECOMPASS,
		   },
};

static int __init bma250_init(void)
{
	I("BMA250 G-sensor driver: init\n");
	return i2c_add_driver(&bma250_driver);
}

static void __exit bma250_exit(void)
{
	i2c_del_driver(&bma250_driver);
}

module_init(bma250_init);
module_exit(bma250_exit);

MODULE_DESCRIPTION("BMA250 G-sensor driver");
MODULE_LICENSE("GPL");

