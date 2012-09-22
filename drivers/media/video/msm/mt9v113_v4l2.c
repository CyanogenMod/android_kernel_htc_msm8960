/*
 * HTC Corporation Proprietary Rights Acknowledgment
 *
 * Copyright (C) 2008 HTC Corporation
 *
 * All Rights Reserved.
 *
 * The information contained in this work is the exclusive property
 * of HTC Corporation("HTC").  Only the user who is legally authorized
 * by HTC ("Authorized User") has right to employ this work within the
 * scope of this statement.  Nevertheless, the Authorized User shall not
 * use this work for any purpose other than the purpose agreed by HTC.
 * Any and all addition or modification to this work shall be  unconditionally
 * granted back to HTC and such addition or modification shall be solely
 * owned by HTC.  No right is granted under this statement, including but not
 * limited to, distribution, reproduction, and transmission, except as
 * otherwise provided in this statement.  Any other usage of this work shall
 *  be subject to the further written consent of HTC.
 */


#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#ifdef CONFIG_MSM_CAMERA_8X60
#include <mach/camera-8x60.h>
#endif
#include <media/msm_camera.h>


#include <mach/gpio.h>
#include "mt9v113.h"
#include <asm/mach-types.h>
#include <media/v4l2-subdev.h>
#include <mach/camera.h>
#include "msm.h"
#include "msm_ispif.h"

/* mt9v113 Registers and their values */
/* Sensor Core Registers */

#define  MT9V113_MODEL_ID     	0x2280 /* Model ID */
#define  MT9V113_MODEL_ID_ADDR  0x0000 /* the address for reading Model ID*/
static int op_mode;
static int32_t config_csi = 0;
/* Read Mode */
#define MT9V113_REG_READ_MODE_ADDR_1	0x2717
#define MT9V113_REG_READ_MODE_ADDR_2	0x272D
#define MT9V113_READ_NORMAL_MODE	0x0024	/* without mirror/flip */
#define MT9V113_READ_MIRROR_FLIP	0x0027	/* with mirror/flip */

struct mt9v113_work {
	struct work_struct work;
};

static struct mt9v113_work *mt9v113_sensorw;
static struct i2c_client *mt9v113_client;

struct mt9v113_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;
	uint16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum mt9v113_resolution_t prev_res;
	enum mt9v113_resolution_t pict_res;
	enum mt9v113_resolution_t curr_res;
	enum mt9v113_test_mode_t set_test;

	struct v4l2_subdev *sensor_dev;
	struct mt9v113_format *fmt;
};

static struct msm_sensor_output_info_t mt9v113_dimensions[] = {
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x34A,
		.frame_length_lines = 0x22A,
		.vt_pixel_clk = 28000000,
		.op_pixel_clk = 28000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x34A,
		.frame_length_lines = 0x22A,
		.vt_pixel_clk = 28000000,
		.op_pixel_clk = 28000000,
		.binning_factor = 1,
	},
};

static struct mt9v113_ctrl_t *mt9v113_ctrl;
static struct platform_device *mt9v113_pdev;

static DECLARE_WAIT_QUEUE_HEAD(mt9v113_wait_queue);

/* DEFINE_MUTEX(mt9v113_sem); */


#define MAX_I2C_RETRIES 20
#define CHECK_STATE_TIME 100

struct mt9v113_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};

static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; /* number sent */

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}

static int mt9v113_i2c_txdata(unsigned short saddr,
				  unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer_retry(mt9v113_client->adapter, msg, 1) < 0) {
		pr_err("mt9v113_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int mt9v113_i2c_write(unsigned short saddr,
				 unsigned short waddr, unsigned short wdata,
				 enum mt9v113_width width)
{
	int rc = -EIO;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN:{
			/*pr_info("i2c_write, WORD_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = (waddr & 0xFF00) >> 8;
			buf[1] = (waddr & 0x00FF);
			buf[2] = (wdata & 0xFF00) >> 8;
			buf[3] = (wdata & 0x00FF);

			rc = mt9v113_i2c_txdata(saddr, buf, 4);
		}
		break;

	case BYTE_LEN:{
			/*pr_info("i2c_write, BYTE_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = waddr;
			buf[1] = wdata;
			rc = mt9v113_i2c_txdata(saddr, buf, 2);
		}
		break;

	default:
		break;
	}

	if (rc < 0)
		pr_info("i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		     waddr, wdata);

	return rc;
}

static int mt9v113_i2c_write_table(struct mt9v113_i2c_reg_conf
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mt9v113_i2c_write(mt9v113_client->addr,
				       reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				       reg_conf_tbl->width);
		if (rc < 0) {
		pr_err("%s: num_of_items_in_table=%d\n", __func__,
			num_of_items_in_table);
			break;
		}
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}

static int mt9v113_i2c_rxdata(unsigned short saddr,
			      unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 2,  /* .len = 1, */
		 .buf = rxdata,
		 },
		{
		 .addr = saddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer_retry(mt9v113_client->adapter, msgs, 2) < 0) {
		pr_err("mt9v113_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

/*read 2 bytes data from sensor via I2C */
static int32_t mt9v113_i2c_read_w(unsigned short saddr, unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = mt9v113_i2c_rxdata(saddr, buf, 2);
	if (rc < 0)
		return rc;

	*rdata = buf[0] << 8 | buf[1];

	if (rc < 0)
		CDBG("mt9v113_i2c_read_w failed!\n");

	return rc;
}

#if 0
static int mt9v113_i2c_read(unsigned short saddr,
				unsigned short raddr, unsigned char *rdata)
{
	int rc = 0;
	unsigned char buf[1];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	rc = mt9v113_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;
	*rdata = buf[0];
	if (rc < 0)
		pr_info("mt9v113_i2c_read failed!\n");

	return rc;
}
#endif
static int mt9v113_i2c_write_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, unsigned short state)
{
	int rc;
	unsigned short check_value;
	unsigned short check_bit;

	if (state)
		check_bit = 0x0001 << bit;
	else
		check_bit = 0xFFFF & (~(0x0001 << bit));
	pr_info("mt9v113_i2c_write_bit check_bit:0x%4x", check_bit);
	rc = mt9v113_i2c_read_w(saddr, raddr, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: mt9v113: 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);
	if (state)
		check_value = (check_value | check_bit);
	else
		check_value = (check_value & check_bit);

	pr_info("%s: mt9v113: Set to 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);

	rc = mt9v113_i2c_write(saddr, raddr, check_value,
		WORD_LEN);
	return rc;
}

static int mt9v113_i2c_check_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, int check_state)
{
	int k;
	unsigned short check_value;
	unsigned short check_bit;
	check_bit = 0x0001 << bit;
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		mt9v113_i2c_read_w(mt9v113_client->addr,
			      raddr, &check_value);
		if (check_state) {
			if ((check_value & check_bit))
			break;
		} else {
			if (!(check_value & check_bit))
			break;
		}
		msleep(1);
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s failed addr:0x%2x data check_bit:0x%2x",
			__func__, raddr, check_bit);
		return -1;
	}
	return 1;
}
/*
static int mt9v113_set_gpio(int num, int status)
{
	int rc = 0;
	rc = gpio_request(num, "mt9v113");
	if (!rc) {
		gpio_direction_output(num, status);
		msleep(5);
	} else
		pr_err("GPIO(%d) request faile", status);

	gpio_free(num);

	return rc;
}
*/
static inline int resume(void)
{
	int k = 0, rc = 0;
	unsigned short check_value;

	/* enter SW Active mode */
	/* write 0x0016[5] to 1  */
	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0016, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: mt9v113: 0x0016 reg value = 0x%x\n", __func__,
		check_value);

	check_value = (check_value|0x0020);

	pr_info("%s: mt9v113: Set to 0x0016 reg value = 0x%x\n", __func__,
		check_value);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016, check_value,
		WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter Active mode fail\n", __func__);
		return rc;
	}

	/* write 0x0018[0] to 0 */
	pr_info("resume,check_value=0x%x", check_value);
	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0018, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: mt9v113: 0x0018 reg value = 0x%x\n", __func__,
		check_value);

	check_value = (check_value & 0xFFFE);

	pr_info("%s: mt9v113: Set to 0x0018 reg value = 0x%x\n", __func__,
		check_value);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, check_value,
		WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter Active mode fail\n", __func__);
		return rc;
	}

	/* check 0x0018[14] is 0 */
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		mt9v113_i2c_read_w(mt9v113_client->addr,
			  0x0018, &check_value);

		pr_info("%s: mt9v113: 0x0018 reg value = 0x%x\n", __func__,
			check_value);

		if (!(check_value & 0x4000)) {/* check state of 0x0018 */
			pr_info("%s: (check 0x0018[14] is 0) k=%d\n",
				__func__, k);
			break;
		}
		msleep(1);	/*MAX: delay 100ms */
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s: check status time out (check 0x0018[14] is 0)\n",
			__func__);
		return -EIO;
	}

/* HTC_START sync from previous project */
	/* check 0x301A[2] is 1 */
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		mt9v113_i2c_read_w(mt9v113_client->addr,
			  0x301A, &check_value);
		if (check_value & 0x0004) {/* check state of 0x301A */
			pr_info("[CAM]%s: (check 0x301A[2] is 1) k=%d\n",
				__func__, k);
			break;
		}
		msleep(1);	/*MAX: delay 100ms */
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("[CAM]%s: check status time out (check 0x301A[2] is 1)\n",
			__func__);
		return -EIO;
	}
/* HTC_END */

	/* check 0x31E0 is 0x003 */
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x31E0,
			&check_value);
		if (check_value == 0x0003) { /* check state of 0x31E0 */
			pr_info("%s: (check 0x31E0 is 0x003 ) k=%d\n",
				__func__, k);
			break;
		}
		msleep(1);	/*MAX: delay 100ms */
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s: check status time out (check 0x31E0 is 0x003 )\n",
			__func__);
		return -EIO;
	}

	/* write 0x31E0 to 0x0001 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x31E0, 0x0001,
	WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter Active mode fail\n", __func__);
		return rc;
	}

    msleep(2);

	return rc;
}

static inline int suspend(void)
{
	int k = 0, rc = 0;
	unsigned short check_value;

	/* enter SW Standby mode */
	/* write 0x0018[3] to 1 */
	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0018, &check_value);
	if (rc < 0)
	  return rc;

	check_value = (check_value|0x0008);

	pr_info("suspend,check_value=0x%x", check_value);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, check_value,
		WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter standy mode fail\n", __func__);
		return rc;
	}
	/* write 0x0018[0] to 1 */
	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0018, &check_value);
	if (rc < 0)
	  return rc;

	check_value = (check_value|0x0001);

	pr_info("%s: mt9v113: Set to 0x0018 reg value = 0x%x\n", __func__,
		check_value);

	pr_info("suspend,2,check_value=0x%x", check_value);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, check_value,
		WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter standy mode fail\n", __func__);
		return rc;
	}

	/* check 0x0018[14] is 1 */
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		mt9v113_i2c_read_w(mt9v113_client->addr,
			  0x0018, &check_value);
		if ((check_value & 0x4000)) { /* check state of 0x0018 */
			pr_info("%s: ( check 0x0018[14] is 1 ) k=%d\n",
				__func__, k);
			break;
		}
		msleep(1);	/*MAX: delay 100ms */
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s: check status time out\n", __func__);
		return -EIO;
	}
    msleep(2);
	return rc;
}

static int mt9v113_reg_init(void)
{
	int rc = 0, k = 0;
	unsigned short check_value;
	struct msm_camera_sensor_info *sinfo = mt9v113_pdev->dev.platform_data;

    /* Power Up Start */
	pr_info("%s: Power Up Start\n", __func__);

	rc = mt9v113_i2c_write(mt9v113_client->addr,
					0x0018, 0x4028, WORD_LEN);
	if (rc < 0)
		goto reg_init_fail;

	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x0018, 14, 0);
	if (rc < 0)
		goto reg_init_fail;

/* HTC_START sync from previous project */
	/* check 0x301A[2] is 1 */
	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x301A, 2, 1);
	if (rc < 0)
		goto reg_init_fail;
/* HTC_END */

	rc = mt9v113_i2c_write_table(&mt9v113_regs.power_up_tbl[0],
				     mt9v113_regs.power_up_tbl_size);
	if (rc < 0) {
		pr_err("%s: Power Up fail\n", __func__);
		goto reg_init_fail;
	}

	/* RESET and MISC Control */
	pr_info("%s: RESET and MISC Control\n", __func__);

	rc = mt9v113_i2c_write(mt9v113_client->addr,
					0x0018, 0x4028, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x0018, 14, 0);
	if (rc < 0)
		goto reg_init_fail;

#if 0	/* HTC_START sync from previous project */
	rc = mt9v113_i2c_write(mt9v113_client->addr,
					0x001A, 0x0003, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	mdelay(30);

	rc = mt9v113_i2c_write(mt9v113_client->addr,
					0x001A, 0x0000, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	mdelay(30);

	rc = mt9v113_i2c_write(mt9v113_client->addr,
					0x0018, 0x4028, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x0018, 14, 0);
	if (rc < 0)
		goto reg_init_fail;
#endif	/* HTC_END */
/* HTC_START sync from previous project */
	/* check 0x301A[2] is 1 */
	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x301A, 2, 1);
	if (rc < 0)
		goto reg_init_fail;

	/* check 0x31E0[1] is 0, Aptina command BITFIELD= 0x31E0 , 2, 0 // core only tags defects. SOC will correct them. */
	rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x31E0, 1, 0);
	if (rc < 0)
		goto reg_init_fail;
/* HTC_END */

	if (sinfo->csi_if) {
	    /*RESET_AND_MISC_CONTROL Parallel output port en MIPI*/
	    rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x001A, 9, 0);
	    if (rc < 0)
	      goto reg_init_fail;

	    /*MIPI control*/
	    /* ---------------------------------------------------------------------- */
	    /* Apply Aptina vendor's suggestion to fix incorrect color issue for MIPI */
	    /* Set to enter STB(standby) after waiting for EOF(end of frame)          */

	    rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x3400, 4, 1);
	    if (rc < 0)
	      goto reg_init_fail;

/* HTC_START sync from previous project */
		/* add retry for writing 0x3400[4] to 1 - ask by optical Steven */
		for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x3400,
				&check_value);
			pr_info("%s: mt9v113: 0x3400 reg value = 0x%4x\n", __func__, check_value);
			if (check_value & 0x0010) { /* check state of 0x3400[4] */
				pr_info("[CAM]%s: (check 0x3400[4] is 1 ) k=%d\n",
				__func__, k);
			break;
		} else {
			check_value = (check_value | 0x0010);
			pr_info("%s: mt9v113: Set to 0x3400 reg value = 0x%4x\n", __func__, check_value);
				rc = mt9v113_i2c_write(mt9v113_client->addr, 0x3400,
				check_value, WORD_LEN);
			if (rc < 0)
				goto reg_init_fail;
		}
			msleep(1);	/*MAX: delay 100ms */
		}
		if (k == CHECK_STATE_TIME) {
			pr_err("[CAM]%s: check status time out (check 0x3400[4] is 1 )\n",
				__func__);
			goto reg_init_fail;
		}

		mdelay(10);
/* HTC_END */
	    /* ---------------------------------------------------------------------- */
	    rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x3400, 9, 1);
	    if (rc < 0)
	      goto reg_init_fail;

/* HTC_START sync from previous project */
		/* add retry for writing 0x3400[9] to 1 - ask by optical Steven */
		for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x3400,
				&check_value);
			pr_info("%s: mt9v113: 0x3400 reg value = 0x%4x\n", __func__, check_value);
			if (check_value & 0x0200) { /* check state of 0x3400[9] */
				pr_info("[CAM]%s: (check 0x3400[9] is 1 ) k=%d\n",
					__func__, k);
				break;
			} else {
				check_value = (check_value | 0x0200);
				pr_info("%s: mt9v113: Set to 0x3400 reg value = 0x%4x\n", __func__, check_value);
				rc = mt9v113_i2c_write(mt9v113_client->addr, 0x3400,
					check_value, WORD_LEN);
				if (rc < 0)
					goto reg_init_fail;
			}
			msleep(1);	/*MAX: delay 100ms */
		}
		if (k == CHECK_STATE_TIME) {
			pr_err("[CAM]%s: check status time out (check 0x3400[9] is 1 )\n",
				__func__);
			goto reg_init_fail;
		}
/* HTC_END */

	    /*OFIFO_control_sstatus*/
	    rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x321C, 7, 0);
	    if (rc < 0)
	      goto reg_init_fail;
	} else {
	    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0210, WORD_LEN);
	    if (rc < 0)
	      goto reg_init_fail;
	}

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001E, 0x0777, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;


	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016, 0x42DF, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;


	/* PLL Setup Start */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0xB04B, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0xB049, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0010, 0x021C, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0012, 0x0000, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0x244B, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	msleep(30);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0x304B, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_check_bit(mt9v113_client->addr, 0x0014, 15, 1);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0xB04A, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	/* write a serial i2c cmd from register_init_tbl of mt9v113_reg */
	rc = mt9v113_i2c_write_table(&mt9v113_regs.register_init_1[0],
			mt9v113_regs.register_init_size_1);
	if (rc < 0)
	  goto reg_init_fail;

	/* write 0x3210[3] bit to 1 */
	rc = mt9v113_i2c_write_bit(mt9v113_client->addr, 0x3210, 3, 1);
	if (rc < 0)
	  goto reg_init_fail;

	/* write a serial i2c cmd from register_init_tb2 of mt9v113_reg */
	rc = mt9v113_i2c_write_table(&mt9v113_regs.register_init_2[0],
			mt9v113_regs.register_init_size_2);
	if (rc < 0)
	  goto reg_init_fail;

	/*the last three commands in the Mode-set up Preview (VGA) /
	Capture Mode (VGA) */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
	for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103,
			WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
			&check_value);
		if (check_value == 0x0000) /* check state of 0xA103 */
			break;
		msleep(1);
	}
	if (k == CHECK_STATE_TIME)
		goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103,
			WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
			&check_value);
		if (check_value == 0x0000) /* check state of 0xA103 */
			break;
		msleep(1);
	}
	if (k == CHECK_STATE_TIME)
		goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA102, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x000F, WORD_LEN);
	if (rc < 0)
	  goto reg_init_fail;

	return rc;
reg_init_fail:
	pr_err("mt9v113 register initial fail\n");
	return rc;
}

#if 0
int mt9v113_set_flip_mirror(struct msm_camera_sensor_info *info)
{
	int rc = 0;
	if (info != NULL) {
		if (info->mirror_mode) {
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
		} else {
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
		}
		if (rc < 0)
			goto set_flip_mirror_fail;
	} else {
		pr_err("camera sensor info is NULL");
		rc = -1;
		goto set_flip_mirror_fail;
	}

	return rc;
set_flip_mirror_fail:
	pr_err("mt9v113 setting flip mirror fail\n");
	return rc;
}
#endif

/* 0726 add new feature */
static int pre_mirror_mode;
static int mt9v113_set_front_camera_mode(enum frontcam_t frontcam_value)
{
	int rc = 0;
	int k = 0;
	unsigned short check_value;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	pr_info("%s: frontcam_value=%d\n", __func__, frontcam_value);

	switch (frontcam_value) {
	case CAMERA_MIRROR:
		/*mirror and flip*/
	if (mt9v113_ctrl->sensordata->mirror_mode) {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0024, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0024, WORD_LEN);
	} else {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0027, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0027, WORD_LEN);
	}

	if (rc < 0)
		return -EIO;

		break;
	case CAMERA_REVERSE:
		/*reverse mode*/
	if (mt9v113_ctrl->sensordata->mirror_mode) {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
	} else {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
	}

	if (rc < 0)
		return -EIO;

		break;

	case CAMERA_PORTRAIT_REVERSE:
	/*portrait reverse mode*//* 0x25: do mirror */
	if (mt9v113_ctrl->sensordata->mirror_mode) {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
	} else {
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2717, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x272D, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
	}

	if (rc < 0)
		return -EIO;

		break;
	default:
		break;
	}

	/* refresh sensor */
	if (pre_mirror_mode != frontcam_value) {
	pr_info("%s: re-flash\n", __func__);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);

	for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
			0xA103, WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
			&check_value);
		if (check_value == 0x0000) /* check state of 0xA103 */
			break;
		msleep(1);
	}
	if (k == CHECK_STATE_TIME) /* time out */
		return -EIO;

	}
	pre_mirror_mode = frontcam_value;

	msleep(20);

#if 0
	for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
			0xA103, WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
			&check_value);
		if (rc > 0 && check_value == 0x0000) /* check state of 0xA103 */
			break;

		msleep(1);
		pr_info("%s: count =%d\n", __func__, k);
	}
	if (k == CHECK_STATE_TIME) /* time out */
		return -EIO;
#endif

	return 0;
}


static int mt9v113_set_sensor_mode(int mode)
{
	int rc = 0 , k;
	uint16_t check_value = 0;
	/*struct msm_camera_csi_params mt9v113_csi_params;*/
	struct msm_camera_sensor_info *sinfo = mt9v113_pdev->dev.platform_data;

	/*MIPI setting*/
	struct msm_camera_csid_params mt9v113_csid_params;
	struct msm_camera_csiphy_params mt9v113_csiphy_params;
	struct msm_camera_csid_vc_cfg mt9v113_vccfg[] = {
			{0, 0x1E, CSI_DECODE_8BIT},
			{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
			{2, CSI_RAW8, CSI_DECODE_8BIT},
		};

	pr_info("sinfo->csi_if=%d,mode=%d", sinfo->csi_if, mode);

	if (config_csi == 0) {
		if (sinfo->csi_if) {
			/* config mipi csi controller */
			pr_info("set csi config\n");
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX0, ISPIF_OFF_IMMEDIATELY));

			rc = suspend();  /*enter STB mode to garantee MIPI status keep on LP11*/

			if (rc < 0)
				pr_err("%s: suspend fail\n", __func__);

			mt9v113_csid_params.lane_cnt = 1;
			mt9v113_csid_params.lane_assign = 0xe4;
			mt9v113_csid_params.lut_params.num_cid =
				ARRAY_SIZE(mt9v113_vccfg);
			mt9v113_csid_params.lut_params.vc_cfg =
				&mt9v113_vccfg[0];
			mt9v113_csiphy_params.lane_cnt = 1;
			mt9v113_csiphy_params.settle_cnt = 20;
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
					NOTIFY_CSID_CFG, &mt9v113_csid_params);
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
					NOTIFY_CID_CHANGE, NULL);
			dsb();

			msleep(1);
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
					NOTIFY_CSIPHY_CFG, &mt9v113_csiphy_params);
			dsb();
			config_csi = 1;

			msleep(1);
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
				NOTIFY_PCLK_CHANGE, &mt9v113_dimensions[0].op_pixel_clk);
			v4l2_subdev_notify(mt9v113_ctrl->sensor_dev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX0, ISPIF_ON_FRAME_BOUNDARY));

			rc = resume();

			if (rc < 0)
				pr_err("%s: resume fail\n", __func__);
		}
	}

#if 0 /* for MIPI debug */
			while (1) {
				msm_io_read_interrupt();
				msleep(200);
			}
#endif

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		op_mode = SENSOR_PREVIEW_MODE;
		pr_info("mt9v113:sensor set mode: preview\n");

		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103,
			WORD_LEN);
		if (rc < 0)
			return rc;

		/*rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001,
		WORD_LEN);*/
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002,
		WORD_LEN);
		if (rc < 0)
			return rc;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA104,	WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			pr_info("check_value=%d", check_value);
			if (check_value == 0x0003) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) {
			pr_err("%s: Preview fail\n", __func__);
			return -EIO;
		}

		/*prevent preview image segmentation*/
		msleep(150);

		break;

	case SENSOR_SNAPSHOT_MODE:
		op_mode = SENSOR_SNAPSHOT_MODE;
		/*sinfo->kpi_sensor_start = ktime_to_ns(ktime_get());*/
		pr_info("mt9v113:sensor set mode: snapshot\n");

		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103,
			WORD_LEN);
		if (rc < 0)
			return rc;

		/* rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002,
		WORD_LEN); */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001,
		WORD_LEN);
		if (rc < 0)
			return rc;

		for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA104, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0003)/* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) {
			pr_err("%s: Snapshot fail\n", __func__);
			return -EIO;
		}
		break;

	default:
		return -EINVAL;
	}

	pr_info("mt9v113_set_sensor_mode");
	return rc;
}


static int mt9v113_set_antibanding(enum antibanding_mode antibanding_value)
{
	int rc = 0;
	unsigned short check_value = 0;
	int iRetryCnt = 20;
	pr_info("[CAM]%s: antibanding_value =%d\n", __func__, antibanding_value);

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;
	switch (antibanding_value) {
	case CAMERA_ANTI_BANDING_50HZ:
//HTC_START, chris 20120409, sync retry mech from 8x60 to avoid 50Hz banding
	while ((check_value != 0xE0) && (iRetryCnt-- > 0)) {
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA404, WORD_LEN);
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C0, WORD_LEN);
			if (rc < 0)
				return -EIO;

		msleep(5);

		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA404, WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990, &check_value);
	}

	if (check_value != 0xE0)
		pr_info("[CAM] %s: check_value: 0x%X, retry failed!\n", __func__, check_value);
//HTC_END
		break;
	case CAMERA_ANTI_BANDING_60HZ:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA404, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0080, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_ANTI_BANDING_AUTO: /*default 60Mhz */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA404, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0080, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	default:
		pr_info("[CAM]%s: Not support antibanding value = %d\n",
		   __func__, antibanding_value);
		return -EINVAL;
	}
	return 0;

}

static int mt9v113_set_sharpness(enum sharpness_mode sharpness_value)
{
	int rc = 0;

	pr_info("%s: sharpness_value = %d\n", __func__, sharpness_value);
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	switch (sharpness_value) {
	case CAMERA_SHARPNESS_X0:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB22, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x326C, 0x0400, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SHARPNESS_X1:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB22, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x326C, 0x0600, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SHARPNESS_X2:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB22, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0003, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x326C, 0x0900, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SHARPNESS_X3:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB22, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x326C, 0x0B00, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SHARPNESS_X4:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB22, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0007, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x326C, 0x0FF0, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	default:
		pr_info("%s: Not support sharpness value = %d\n",
		   __func__, sharpness_value);
		return -EINVAL;
	}
	return 0;
}


static int mt9v113_set_saturation(enum saturation_mode saturation_value)
{
	int rc = 0;

	pr_info("%s: saturation_value = %d\n", __func__, saturation_value);
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;
	switch (saturation_value) {
	case CAMERA_SATURATION_X0:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB20, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0010, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB24, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0009, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SATURATION_X05:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB20, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0035, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB24, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SATURATION_X1:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB20, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0048, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB24, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0033, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SATURATION_X15:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB20, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0063, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB24, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0045, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	case CAMERA_SATURATION_X2:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB20, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0076, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB24, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0053, WORD_LEN);
			if (rc < 0)
				return -EIO;

		break;
	default:
		pr_info("%s: Not support saturation value = %d\n",
		   __func__, saturation_value);
		return -EINVAL;
	}
	return 0;
}

static int mt9v113_set_contrast(enum contrast_mode contrast_value)
{
	int rc = 0;

	pr_info("%s: contrast_value = %d\n", __func__, contrast_value);
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	switch (contrast_value) {
	case CAMERA_CONTRAST_N2:
		rc = mt9v113_i2c_write_table(&mt9v113_regs.contract_tb0[0],
			mt9v113_regs.contract_tb0_size);
		if (rc < 0) {
			pr_err("%s: contract_tb0 fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_CONTRAST_N1:
		rc = mt9v113_i2c_write_table(&mt9v113_regs.contract_tb1[0],
			mt9v113_regs.contract_tb1_size);
		if (rc < 0) {
			pr_err("%s: contract_tb1 fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_CONTRAST_D:
		rc = mt9v113_i2c_write_table(&mt9v113_regs.contract_tb2[0],
			mt9v113_regs.contract_tb2_size);
		if (rc < 0) {
			pr_err("%s: contract_tb2 fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_CONTRAST_P1:
		rc = mt9v113_i2c_write_table(&mt9v113_regs.contract_tb3[0],
			mt9v113_regs.contract_tb3_size);
		if (rc < 0) {
			pr_err("%s: contract_tb3 fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_CONTRAST_P2:
		rc = mt9v113_i2c_write_table(&mt9v113_regs.contract_tb4[0],
			mt9v113_regs.contract_tb4_size);
		if (rc < 0) {
			pr_err("%s: contract_tb4 fail\n", __func__);
			return -EIO;
		}

		break;
	default:
		pr_info("%s: Not support contrast value = %d\n",
		   __func__, contrast_value);
		return -EINVAL;
	}
	return 0;
}

/* 20110103 add new effect feature */
static int mt9v113_set_effect(int effect)
{
	int rc = 0, k = 0;
	unsigned short check_value;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	pr_info("%s: effect = %d\n", __func__, effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2759, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6440, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x275B, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6440, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2763, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0xB023, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;

	case CAMERA_EFFECT_MONO:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2759, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6441, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x275B, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6441, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2763, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0xB023, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;

	case CAMERA_EFFECT_NEGATIVE:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2759, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6443, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x275B, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6443, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2763, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0xB023, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;

	case CAMERA_EFFECT_SEPIA:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2759, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6442, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x275B, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6442, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2763, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0xB023, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;

	case CAMERA_EFFECT_AQUA:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2759, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6442, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x275B, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x6442, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x2763, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x30D0, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	default:
		pr_info("%s: Not support effect = %d\n",
		   __func__, effect);
		return -EINVAL;
	}

	return 0;
}

static int mt9v113_set_brightness(enum brightness_t brightness_value)
{
	int rc = 0;
	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	pr_info("%s: brightness_value = %d\n", __func__, brightness_value);

	switch (brightness_value) {
	case CAMERA_BRIGHTNESS_N4: /* -2 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x001F, WORD_LEN);/*0x001E, WORD_LEN);Mu L 0209 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00CA, WORD_LEN);
			if (rc < 0)
				return -EIO;

			break;

	case CAMERA_BRIGHTNESS_N3: /* -1.5 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0025, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C9, WORD_LEN);
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_N2:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0030, WORD_LEN);/*0x0028, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C9, WORD_LEN);/*0x00C9, WORD_LEN);Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_N1:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0038, WORD_LEN);/*0x002E, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C8, WORD_LEN);/*0x00C9, WORD_LEN);Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_D: /* default 0 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x004A, WORD_LEN);/*0x004A, WORD_LEN);  //mk0131*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C8, WORD_LEN);/*0x00C9, WORD_LEN);Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_P1:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0051, WORD_LEN);/*0x003B, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C8, WORD_LEN);
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_P2:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0059, WORD_LEN);/*0x0046, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C7, WORD_LEN);/*0x00C8, WORD_LEN); Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_P3: /* 1.5 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x005F, WORD_LEN);/*0x004D, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C7, WORD_LEN);/*0x00C8, WORD_LEN); Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	case CAMERA_BRIGHTNESS_P4: /* 2 */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA24F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0068, WORD_LEN);/*0x0054, WORD_LEN);Mu L 0209*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xAB1F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00C6, WORD_LEN);/*0x00C8, WORD_LEN); Mu L 0209*/
		if (rc < 0)
			return -EIO;

		break;
	default:
		pr_info("%s: Not support brightness value = %d\n",
			__func__, brightness_value);
		 return -EINVAL;
	}
	return 0;
}

static int mt9v113_set_wb(enum wb_mode wb_value)
{
	int rc = 0, k = 0;
	unsigned short check_value;

	if (op_mode == SENSOR_SNAPSHOT_MODE)
		return 0;

	pr_info("%s: wb_value = %d\n", __func__, wb_value);
	switch (wb_value) {
	case CAMERA_AWB_AUTO:/* AUTO */
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA11F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		rc = mt9v113_i2c_write_table(&mt9v113_regs.wb_auto[0],
			mt9v113_regs.wb_auto_size);
		if (rc < 0) {
			pr_err("%s: wb_auto fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_AWB_INDOOR_HOME:/*Fluorescent*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA11F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		rc = mt9v113_i2c_write_table(&mt9v113_regs.wb_fluorescent[0],
			mt9v113_regs.wb_fluorescent_size);
		if (rc < 0) {
			pr_err("%s: wb_fluorescent fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_AWB_INDOOR_OFFICE:/*Incandescent*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA11F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		rc = mt9v113_i2c_write_table(&mt9v113_regs.wb_incandescent[0],
			mt9v113_regs.wb_incandescent_size);
		if (rc < 0) {
			pr_err("%s: wb_incandescent fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_AWB_SUNNY:/*daylight*/
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA11F, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		rc = mt9v113_i2c_write_table(&mt9v113_regs.wb_daylight[0],
			mt9v113_regs.wb_daylight_size);
		if (rc < 0) {
			pr_err("%s: wb_daylight fail\n", __func__);
			return -EIO;
		}

		break;
	case CAMERA_AWB_CLOUDY: /*Not support */
	default:
		pr_info("%s: Not support wb_value = %d\n",
		   __func__, wb_value);
		return -EINVAL;
	}
	return 0;
}

#if 0
static int mt9v113_get_iso(uint16_t *real_iso_value)
{
	int rc = 0;
	unsigned short check_value;

	/* Work-around for default iso value as ISO_400 */
	*real_iso_value = 400;

	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x3028, &check_value);
	if (rc < 0)
		return -EIO;

	*real_iso_value = (check_value * 100) / 8;

	pr_info("%s real_iso_value: %d\n", __func__, *real_iso_value);
	return rc;
}
#endif

static int mt9v113_set_iso(enum iso_mode iso_value)
{
	int rc = 0, k = 0;
	unsigned short check_value;
	pr_info("%s: iso_value =%d\n", __func__, iso_value);

	switch (iso_value) {
	case CAMERA_ISO_AUTO:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20E, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0080, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	case CAMERA_ISO_100:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20E, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0026, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	case CAMERA_ISO_200:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20E, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0046, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	case CAMERA_ISO_400:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20E, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0078, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	case CAMERA_ISO_800:
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20E, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x00A0, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		if (rc < 0)
			return -EIO;

		for (k = 0; k < CHECK_STATE_TIME; k++) {  /* retry 100 times */
			rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
				0xA103, WORD_LEN);
			rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
				&check_value);
			if (check_value == 0x0000) /* check state of 0xA103 */
				break;
			msleep(1);
		}
		if (k == CHECK_STATE_TIME) /* time out */
			return -EIO;

		break;
	default:
		pr_info("%s: Not support ISO value = %d\n",
			__func__, iso_value);
		 return -EINVAL;
	}
	return 0;
}


static int mt9v113_vreg_enable(struct platform_device *pdev)
{
	struct msm_camera_sensor_info *sdata = pdev->dev.platform_data;
	int rc;

	pr_info("%s camera vreg on\n", __func__);

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}
	rc = sdata->camera_power_on();
	return rc;
}
static int mt9v113_vreg_disable(struct platform_device *pdev)
{
	struct msm_camera_sensor_info *sdata = pdev->dev.platform_data;
	int rc;
	printk(KERN_INFO "%s camera vreg off\n", __func__);
	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}
	rc = sdata->camera_power_off();
	return rc;
}


static int mt9v113_probe_init_done(const struct msm_camera_sensor_info *data)
{
	int rc;
	printk(KERN_DEBUG "%s: %d\n", __func__, __LINE__);

	gpio_request(data->sensor_reset, "mt9v113");
	gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);

	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);

	/* HTC power disable */
	rc = mt9v113_vreg_disable(mt9v113_pdev);
	if (rc < 0)
		printk(KERN_ERR "%s failed to disable power\n", __func__);

	return 0;
}

static int mt9v113_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	/* uint8_t model_id_h = 0,model_id_l = 0; */
	uint16_t model_id;
	int rc = 0;

	pr_info("mt9v113_probe_init_sensor\n");
	rc = mt9v113_vreg_enable(mt9v113_pdev);
	if (rc < 0) {
		pr_err("__mt9v113_probe fail sensor power on error\n");
		goto probe_init_fail;
	}

	rc = gpio_request(data->sensor_reset, "mt9v113");
	if (!rc) {
		gpio_direction_output(data->sensor_reset, 0);
		msleep(1);

		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			goto probe_init_fail_vreg_off;
		}

		pr_info("mt9v113: MCLK enable clk\n");
		msm_camio_clk_rate_set(24000000);

		msleep(1);
		rc = gpio_direction_output(data->sensor_reset, 1);
		if (rc < 0) {
			goto probe_init_fail_mclk_off;
		}
	}	else {
		pr_info("mt9v113: request GPIO(sensor_reset) :%d faile\n", data->sensor_reset);
		goto probe_init_fail_vreg_off;
	}
	gpio_free(data->sensor_reset);

	msleep(1);

	/* Read the Model ID of the sensor */
	pr_info("mt9v113_probe_init_sensor,mt9v113_client->addr=0x%x", mt9v113_client->addr);
	rc = mt9v113_i2c_read_w(mt9v113_client->addr,
			      MT9V113_MODEL_ID_ADDR, &model_id);
	if (rc < 0) {
		pr_err("%s: I2C read fail\n", __func__);
		goto probe_init_fail_reset_off;
	}

	pr_info("%s: mt9v113: model_id = 0x%x\n", __func__, model_id);
	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9V113_MODEL_ID) {
	    pr_err("%s: Sensor is not MT9V113\n", __func__);
		rc = -EINVAL;
		goto probe_init_fail_reset_off;
	}

	mt9v113_ctrl->fps_divider = 1 * 0x00000400;
	mt9v113_ctrl->pict_fps_divider = 1 * 0x00000400;
	mt9v113_ctrl->set_test = TEST_OFF;
	mt9v113_ctrl->prev_res = QTR_SIZE;
	mt9v113_ctrl->pict_res = FULL_SIZE;

	if (data)
		mt9v113_ctrl->sensordata = data;

	goto init_probe_done;

probe_init_fail_reset_off:
	gpio_request(data->sensor_reset, "mt9v113");
	gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);
probe_init_fail_mclk_off:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
probe_init_fail_vreg_off:
	mt9v113_vreg_disable(mt9v113_pdev);
probe_init_fail:
	pr_err("mt9v113_probe_init_sensor fails\n");
	return rc;
init_probe_done:
	pr_info("mt9v113_probe_init_sensor finishes\n");
	return rc;

}



static int suspend_fail_retry_count_2;
#define SUSPEND_FAIL_RETRY_MAX_2 3
int mt9v113_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	uint16_t check_value = 0;

	pr_info("mt9v113_sensor_open_init");

	if (data == NULL) {
		pr_err("%s sensor data is NULL\n", __func__);
		return -EINVAL;
	}

	suspend_fail_retry_count_2 = SUSPEND_FAIL_RETRY_MAX_2;

	if (!data->power_down_disable) {

probe_suspend_fail_retry_2:
		pr_info("mt9v113_sensor_open_init  suspend_fail_retry_count_2=%d\n", suspend_fail_retry_count_2);

		mdelay(5);
	}

	mdelay(2);

	/*read ID*/
	rc = mt9v113_probe_init_sensor(data);
	if (rc < 0) {
		pr_info("mt9v113_probe_init_sensor failed!\n");
		goto prob_init_sensor_fail;
	}

	if (!data->power_down_disable) {
		/*set initial register*/
		rc = mt9v113_reg_init();
		if (rc < 0) {
			pr_err("%s: mt9v113_reg_init fail\n", __func__);

			if (suspend_fail_retry_count_2 > 0) {
				suspend_fail_retry_count_2--;
				pr_info("%s: mt9v113 reg_init fail start retry mechanism !!!\n", __func__);
				goto probe_suspend_fail_retry_2;
			}

			goto init_fail;
		}
		/*rc = suspend();  set standby mode  steven */
		if (rc < 0) {
			pr_err("%s: mt9v113 init_suspend fail\n", __func__);

			if (suspend_fail_retry_count_2 > 0) {
				suspend_fail_retry_count_2--;
				pr_info("%s: mt9v113 init_suspend fail start retry mechanism !!!\n", __func__);
				goto probe_suspend_fail_retry_2;
			}

			goto init_fail;
		}

		/* Do streaming Off */
		/* write 0x0016[5] to 0  */
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0016, &check_value);
		if (rc < 0)
		  return rc;

		pr_info("%s: mt9v113: 0x0016 reg value = 0x%x\n",
			__func__, check_value);

		check_value = (check_value&0xFFDF);

		pr_info("%s: mt9v113: Set to 0x0016 reg value = 0x%x\n",
			__func__, check_value);

		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016,
			check_value, WORD_LEN);
		if (rc < 0) {
			pr_err("%s: Enter Standby mode fail\n", __func__);
			return rc;
		}
	}

	 /*power down or standby need to :*/

	if (!data->csi_if) {
		/* standby mode to Active mode */
		rc = resume();
		if (rc < 0) {
			pr_err("%s: Enter Active mode fail\n", __func__);
			goto init_fail;
		}
	}

	config_csi = 0;
	goto init_done;

init_fail:
	pr_info("init_fail\n");
	mt9v113_probe_init_done(data);
	kfree(mt9v113_ctrl);
	return rc;
prob_init_sensor_fail:
	pr_info("init_fail\n");
	kfree(mt9v113_ctrl);
	return rc;

init_done:
	pr_info("init_done\n");
	return rc;

}

static int mt9v113_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9v113_wait_queue);
	return 0;
}

static int mt9v113_detect_sensor_status(void)
{
	int rc = 0, k = 0;
	unsigned short check_value;

	for (k = 0; k < CHECK_STATE_TIME; k++) {	/* retry 100 times */
		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C,
			0xA103, WORD_LEN);
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0990,
			&check_value);
		if (check_value == 0x0000) /* check state of 0xA103 */
			break;

		msleep(1);
	}

	if (k == CHECK_STATE_TIME) /* time out */
		pr_info("[cam]mt9v113_detect_sensor_status,time out");

	return 0;
}

static int mt9v113_set_FPS(struct fps_cfg *fps)
{
	pr_info("mt9v113_set_FPS,fps->fps_div=%d", fps->fps_div);

	if (fps->fps_div == 10) {
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x271F, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x067E, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();

		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x000C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();
	} else if (fps->fps_div == 15) {
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x271F, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0454, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();

		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0004, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();
	} else if (fps->fps_div == 1015) {
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x271F, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0454, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();

		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x000C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();
	} else if (fps->fps_div == 0) {
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0x271F, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x022A, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();

		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA20C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x000C, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA215, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0008, WORD_LEN);

		mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
		mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
		mdelay(1);

		mt9v113_detect_sensor_status();
	}

	return 0;
}

static int32_t mt9v113_get_output_info(
	struct sensor_output_info_t *sensor_output_info)
{
	int rc = 0;
	sensor_output_info->num_info = 2;
	if (copy_to_user((void *)sensor_output_info->output_info,
		mt9v113_dimensions,
		sizeof(struct msm_sensor_output_info_t) * 2))
		rc = -EFAULT;

	return rc;
}

int mt9v113_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long rc = 0;
	if (copy_from_user(&cfg_data,
			   (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	pr_info("mt9v113_ioctl, cfgtype = %d, mode = %d\n",
	     cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_GET_OUTPUT_INFO:
		rc = mt9v113_get_output_info(&cfg_data.cfg.output_info);
		break;
	case CFG_SET_MODE:
		rc = mt9v113_set_sensor_mode(cfg_data.mode);
		break;
	case CFG_SET_EFFECT:
		rc = mt9v113_set_effect(cfg_data.cfg.effect);
		break;
	case CFG_SET_ANTIBANDING:
		rc = mt9v113_set_antibanding(cfg_data.cfg.antibanding_value);
		break;
	case CFG_SET_BRIGHTNESS:
		rc = mt9v113_set_brightness(cfg_data.cfg.brightness_value);
		break;
	case CFG_SET_WB:
		rc = mt9v113_set_wb(cfg_data.cfg.wb_value);
		break;
	case CFG_SET_SHARPNESS:
		rc = mt9v113_set_sharpness(cfg_data.cfg.sharpness_value);
		break;
	case CFG_SET_SATURATION:
		rc = mt9v113_set_saturation(cfg_data.cfg.saturation_value);
		break;
	case CFG_SET_CONTRAST:
		rc = mt9v113_set_contrast(cfg_data.cfg.contrast_value);
		break;
	case CFG_SET_FRONT_CAMERA_MODE: /*0726 add new feature*/
		rc = mt9v113_set_front_camera_mode(cfg_data.cfg.frontcam_value);
		break;
	case CFG_GET_ISO:
#if 0
		rc = mt9v113_get_iso(&cfg_data.cfg.real_iso_value);
		if (copy_to_user((void *)argp,
			&cfg_data, sizeof(struct sensor_cfg_data))) {
			pr_err("%s copy to user error", __func__);
		}
		break;
#endif
	case CFG_SET_ISO:
		rc = mt9v113_set_iso(cfg_data.cfg.iso_value);
		break;
	case CFG_SET_FPS:
		rc = mt9v113_set_FPS(&(cfg_data.cfg.fps));
		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

int mt9v113_sensor_release(void)
{
	int rc = 0;
	uint16_t check_value = 0;
	struct msm_camera_sensor_info *sdata = mt9v113_pdev->dev.platform_data;

	/* enter SW standby mode */
	pr_info("%s: enter SW standby mode\n", __func__);
	/*suspend(); //steven */

	/* Do streaming Off */
	/* write 0x0016[5] to 0  */
	rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0016, &check_value);
	if (rc < 0) {
	  pr_err("%s: read streaming off status fail\n", __func__);
	  goto sensor_release;
	}

	pr_info("%s: mt9v113: 0x0016 reg value = 0x%x\n",
		__func__, check_value);

	check_value = (check_value&0xFFDF);

	pr_info("%s: mt9v113: Set to 0x0016 reg value = 0x%x\n",
		__func__, check_value);

	rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016,
		check_value, WORD_LEN);
	if (rc < 0) {
		pr_err("%s: Enter Standby mode fail\n", __func__);
		goto sensor_release;
	}

	mdelay(2);

	if (!sdata->power_down_disable) {
		rc = gpio_request(sdata->sensor_reset, "mt9v113");
		if (!rc) {
			rc = gpio_direction_output(sdata->sensor_reset, 0);
			mdelay(2);
		} else
			pr_err("GPIO(%d) request faile", sdata->sensor_reset);
		gpio_free(sdata->sensor_reset);
	}

	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);

	mdelay(2);

	/*0709: optical ask : CLK switch to Main Cam after 2nd Cam release*/
	if (sdata->camera_clk_switch != NULL && sdata->cam_select_pin) {
		pr_info("%s: doing clk switch to Main CAM)\n", __func__);
		rc = gpio_request(sdata->cam_select_pin, "mt9v113");
		if (rc < 0)
			pr_err("GPIO (%d) request fail\n", sdata->cam_select_pin);
		else
			gpio_direction_output(sdata->cam_select_pin, 0);
		gpio_free(sdata->cam_select_pin);
	}

	mdelay(2);


	if (!sdata->power_down_disable)
		mt9v113_vreg_disable(mt9v113_pdev);

sensor_release:
	return rc;
}

static const char *mt9v113Vendor = "Micron";
static const char *mt9v113NAME = "mt9v113";
static const char *mt9v113Size = "VGA CMOS";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", mt9v113Vendor, mt9v113NAME, mt9v113Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_mt9v113;

static int mt9v113_sysfs_init(void)
{
	int ret ;
	pr_info("mt9v113:kobject creat and add\n");
	android_mt9v113 = kobject_create_and_add("android_camera2", NULL);
	if (android_mt9v113 == NULL) {
		pr_info("mt9v113_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("mt9v113:sysfs_create_file\n");
	ret = sysfs_create_file(android_mt9v113, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("mt9v113_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_mt9v113);
	}

	return 0 ;
}


static int mt9v113_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9v113_sensorw = kzalloc(sizeof(struct mt9v113_work), GFP_KERNEL);

	if (!mt9v113_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mt9v113_sensorw);
	mt9v113_init_client(client);
	mt9v113_client = client;

	pr_info("mt9v113_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(mt9v113_sensorw);
	mt9v113_sensorw = NULL;
	pr_info("mt9v113_probe failed!\n");
	return rc;
}

static const struct i2c_device_id mt9v113_i2c_id[] = {
	{"mt9v113", 0},
	{},
};

static struct i2c_driver mt9v113_i2c_driver = {
	.id_table = mt9v113_i2c_id,
	.probe = mt9v113_i2c_probe,
	.remove = __exit_p(mt9v113_i2c_remove),
	.driver = {
		   .name = "mt9v113",
		   },
};



static int suspend_fail_retry_count;
#define SUSPEND_FAIL_RETRY_MAX 3

static int mt9v113_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	uint16_t check_value = 0;

	int rc = i2c_add_driver(&mt9v113_i2c_driver);
	if (rc < 0 || mt9v113_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_fail;
	}

probe_suspend_fail_retry:

    mdelay(2);
	rc = mt9v113_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail;

	/*set initial register*/
	rc = mt9v113_reg_init();
	if (rc < 0) {
		pr_err("%s: mt9v113_reg_init fail\n", __func__);
		if (suspend_fail_retry_count > 0) {
			suspend_fail_retry_count--;
			pr_info("%s: mt9v113 mt9v113_reg_init fail start retry mechanism !!!\n", __func__);
			goto probe_suspend_fail_retry;
		}

		goto probe_fail_reset_power;
	}
	/*set flip/mirror register*/
#if 0
	rc = mt9v113_set_flip_mirror(info);
	if (rc < 0) {
		pr_err("%s: mt9v113_set_flip_mirror fail\n", __func__);
		goto probe_fail_close_pwr;
	}
#endif

	/*rc = suspend();  set standby mode   steven*/
	if (rc < 0) {
		pr_err("%s: mt9v113 init_suspend fail\n", __func__);
		if (suspend_fail_retry_count > 0) {
			suspend_fail_retry_count--;
			pr_info("%s: mt9v113 init_suspend fail start retry mechanism !!!\n", __func__);
			goto probe_suspend_fail_retry;
		}

		goto probe_fail_reset_power;
	}


	if (!info->power_down_disable) {
		/* Do streaming Off */
		/* write 0x0016[5] to 0  */
		rc = mt9v113_i2c_read_w(mt9v113_client->addr, 0x0016, &check_value);
		if (rc < 0)
			return rc;

		pr_info("%s: mt9v113: 0x0016 reg value = 0x%x\n",
			__func__, check_value);

		check_value = (check_value&0xFFDF);

		pr_info("%s: mt9v113: Set to 0x0016 reg value = 0x%x\n",
			__func__, check_value);

		rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016,
			check_value, WORD_LEN);
		if (rc < 0) {
			pr_err("%s: Enter Standby mode fail\n", __func__);
			return rc;
		}
	}


	s->s_init = mt9v113_sensor_open_init;
	s->s_release = mt9v113_sensor_release;
	s->s_config = mt9v113_sensor_config;
	s->s_camera_type = FRONT_CAMERA_2D;
	s->s_mount_angle = info->sensor_platform_info->mount_angle;

	/*init done*/
	msleep(10);

	mt9v113_sysfs_init();

	mt9v113_probe_init_done(info);

	return rc;

probe_fail_reset_power:
	pr_err("mt9v113_sensor_probe: initial register fail!\n");
	mt9v113_probe_init_done(info);
	return rc;

probe_fail:
	pr_err("mt9v113_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;

}

static struct mt9v113_format mt9v113_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static int mt9v113_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	printk(KERN_DEBUG "Index is %d\n", index);
	if ((unsigned int)index >= ARRAY_SIZE(mt9v113_subdev_info))
		return -EINVAL;

	*code = mt9v113_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops mt9v113_subdev_core_ops;
static struct v4l2_subdev_video_ops mt9v113_subdev_video_ops = {
	.enum_mbus_fmt = mt9v113_enum_fmt,
};

static struct v4l2_subdev_ops mt9v113_subdev_ops = {
	.core = &mt9v113_subdev_core_ops,
	.video  = &mt9v113_subdev_video_ops,
};

static int mt9v113_sensor_probe_cb(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = 0;

	mt9v113_ctrl = kzalloc(sizeof(struct mt9v113_ctrl_t), GFP_KERNEL);
	if (!mt9v113_ctrl) {
		pr_err("mt9v113_sensor_probe failed!\n");
		return -ENOMEM;
	}

	rc = mt9v113_sensor_probe(info, s);
	if (rc < 0)
		return rc;

	/* probe is successful, init a v4l2 subdevice */
	printk(KERN_DEBUG "going into v4l2_i2c_subdev_init\n");
	if (sdev) {
		v4l2_i2c_subdev_init(sdev, mt9v113_client,
						&mt9v113_subdev_ops);
		mt9v113_ctrl->sensor_dev = sdev;
	}

	return rc;
}

static int __mt9v113_probe(struct platform_device *pdev)
{
		pr_info("__mt9v113_probe\n");
		mt9v113_pdev = pdev;
		return msm_sensor_register(pdev, mt9v113_sensor_probe_cb);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9v113_probe,
	.driver = {
		   .name = "msm_camera_mt9v113",
		   .owner = THIS_MODULE,
		   },
};

static int __init mt9v113_init(void)
{
	pr_info("mt9v113_init\n");
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9v113_init);

void mt9v113_exit(void)
{
	i2c_del_driver(&mt9v113_i2c_driver);
}
MODULE_DESCRIPTION("Micron 0.3 MP YUV sensor driver");
MODULE_LICENSE("GPL v2");

