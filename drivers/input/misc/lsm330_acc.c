/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
 *
 * File Name	: lsm330_acc.c
 * Authors	: MSH - Motion Mems BU - Application Team
 *			: Matteo Dameno (matteo.dameno@st.com)
 *			: Denis Ciocca (denis.ciocca@st.com)
 *			: Author is willing to be considered the contact
 *			: and update point for the driver.
 * Version	: V.1.2.7
 * Date		: 2013/May/16
 * Description	: LSM330 accelerometer driver
 *
 *******************************************************************************
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
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *
 ******************************************************************************
Version History.
	V 1.0.0		First Release
	V 1.0.2		I2C address bugfix
	V 1.2.0		Registers names compliant to correct datasheet
	V.1.2.1		Removed enable_interrupt_output sysfs file, manages int1
			and int2, implements int1 isr.
	V.1.2.2		Added HR_Timer and custom sysfs path
	V.1.2.3		Ch state program codes and state prog parameters defines
	V.1.2.5		Changes create_sysfs_interfaces
	V.1.2.6		Changes resume and suspend functions
 	V.1.2.7	 	Added rotation matrices
 ******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <linux/lsm330.h>
#include <linux/wakelock.h>

#define DIF(x...) {\
                if (DEBUG_FLAG)\
                        printk(KERN_DEBUG "[GSNR][LSM330][DEBUG]" x); }

#define DEFAULT_FIRST_SAMPLE_DELAY_NS		(10000000)
#define NUMBER_DISCARD_SAMPLES			2

#define LOAD_SM1_PROGRAM	0
#define LOAD_SM1_PARAMETERS	0
#define LOAD_SM2_PROGRAM	1
#define LOAD_SM2_PARAMETERS	1

#define SIGNIFICANT_MOTION	1

#if SIGNIFICANT_MOTION > 0
#define ABS_SIGN_MOTION ABS_WHEEL
#define THRS1_2_02G 0x09
#define THRS1_2_04G 0x05
#define THRS1_2_06G 0x03
#define THRS1_2_08G 0x02
#define THRS1_2_16G 0x01
#endif

#define G_MAX			23920640	
#define I2C_RETRY_DELAY		5		
#define I2C_RETRIES		5		
#define I2C_AUTO_INCREMENT	0x80		
#define MS_TO_NS(x)		(x*1000000L)

#define SENSITIVITY_2G		60		
#define SENSITIVITY_4G		120		
#define SENSITIVITY_6G		180		
#define SENSITIVITY_8G		240		
#define SENSITIVITY_16G		730		

#define	LSM330_ACC_FS_MASK	(0x38)

#define LSM330_ODR_MASK		(0XF0)
#define LSM330_XYZ_BDU_MASK	(0x0F)
#define LSM330_PM_OFF		(0x00)		
#define LSM330_ODR3_125		(0x10)		
#define LSM330_ODR6_25		(0x20)		
#define LSM330_ODR12_5		(0x30)		
#define LSM330_ODR25		(0x40)		
#define LSM330_ODR50		(0x50)		
#define LSM330_ODR100		(0x60)		
#define LSM330_ODR400		(0x70)		
#define LSM330_ODR800		(0x80)		
#define LSM330_ODR1600		(0x90)		

#define LSM330_INTEN_MASK		(0x01)
#define LSM330_INTEN_OFF		(0x00)
#define LSM330_INTEN_ON			(0x01)

#define LSM330_HIST1_MASK		(0xE0)
#define LSM330_SM1INT_PIN_MASK		(0x08)
#define LSM330_SM1INT_PININT2		(0x08)
#define LSM330_SM1INT_PININT1		(0x00)
#define LSM330_SM1_EN_MASK		(0x01)
#define LSM330_SM1_EN_ON		(0x01)
#define LSM330_SM1_EN_OFF		(0x00)

#define LSM330_HIST2_MASK		(0xE0)
#define LSM330_SM2INT_PIN_MASK		(0x08)
#define LSM330_SM2INT_PININT2		(0x08)
#define LSM330_SM2INT_PININT1		(0x00)
#define LSM330_SM2_EN_MASK		(0x01)
#define LSM330_SM2_EN_ON		(0x01)
#define LSM330_SM2_EN_OFF		(0x00)

#define LSM330_INT_ACT_MASK		(0x01 << 6)
#define LSM330_INT_ACT_H		(0x01 << 6)
#define LSM330_INT_ACT_L		(0x00)

#define LSM330_INT2_EN_MASK		(0x01 << 4)
#define LSM330_INT2_EN_ON		(0x01 << 4)
#define LSM330_INT2_EN_OFF		(0x00)

#define LSM330_INT1_EN_MASK		(0x01 << 3)
#ifdef CONFIG_CIR_ALWAYS_READY
#define LSM330_INT1_EN_ON  (0x28)
#else
#define LSM330_INT1_EN_ON		(0x01 << 3)
#endif
#define LSM330_INT1_EN_OFF		(0x00)


#define LSM330_BDU_EN			(0x08)
#define LSM330_ALL_AXES			(0x07)

#define LSM330_STAT_INTSM1_BIT		(0x01 << 3)
#define LSM330_STAT_INTSM2_BIT		(0x01 << 2)

#define OUT_AXISDATA_REG		LSM330_OUTX_L
#define WHOAMI_LSM330_ACC		(0x40)	

#define LSM330_WHO_AM_I			(0x0F)	

#define LSM330_OUTX_L			(0x28)	
#define LSM330_OUTX_H			(0x29)	
#define LSM330_OUTY_L			(0x2A)	
#define LSM330_OUTY_H			(0x2B)	
#define LSM330_OUTZ_L			(0x2C)	
#define LSM330_OUTZ_H			(0x2D)	
#define LSM330_LC_L			(0x16)	
#define LSM330_LC_H			(0x17)	

#define LSM330_INTERR_STAT		(0x18)	

#define LSM330_STATUS_REG		(0x27)	

#define LSM330_CTRL_REG1		(0x21)	
#define LSM330_CTRL_REG2		(0x22)	
#define LSM330_CTRL_REG3		(0x23)	
#define LSM330_CTRL_REG4		(0x20)	
#define LSM330_CTRL_REG5		(0x24)	
#define LSM330_CTRL_REG6		(0x25)	

#define LSM330_OFF_X			(0x10)	
#define LSM330_OFF_Y			(0x11)	
#define LSM330_OFF_Z			(0x12)	

#define LSM330_CS_X			(0x13)	
#define LSM330_CS_Y			(0x14)	
#define LSM330_CS_Z			(0x15)	

#define LSM330_VFC_1			(0x1B)	
#define LSM330_VFC_2			(0x1C)	
#define LSM330_VFC_3			(0x1D)	
#define LSM330_VFC_4			(0x1E)	


	
#define LSM330_STATEPR1		(0X40)	

#define LSM330_TIM4_1		(0X50)	
#define LSM330_TIM3_1		(0X51)	
#define LSM330_TIM2_1		(0X52)	
#define LSM330_TIM1_1		(0X54)	

#define LSM330_THRS2_1		(0X56)	
#define LSM330_THRS1_1		(0X57)	
#define LSM330_SA_1		(0X59)	
#define LSM330_MA_1		(0X5A)	
#define LSM330_SETT_1		(0X5B)	
#define LSM330_PPRP_1		(0X5C)	
#define LSM330_TC_1		(0X5D)	
#define LSM330_OUTS_1		(0X5F)	

	
#define LSM330_STATEPR2	(0X60)	

#define LSM330_TIM4_2		(0X70)	
#define LSM330_TIM3_2		(0X71)	
#define LSM330_TIM2_2		(0X72)	
#define LSM330_TIM1_2		(0X74)	

#define LSM330_THRS2_2		(0X76)	
#define LSM330_THRS1_2		(0X77)	
#define LSM330_DES_2		(0X78)	
#define LSM330_SA_2		(0X79)	
#define LSM330_MA_2		(0X7A)	
#define LSM330_SETT_2		(0X7B)	
#define LSM330_PPRP_2		(0X7C)	
#define LSM330_TC_2		(0X7D)	
#define LSM330_OUTS_2		(0X7F)	


#define RES_LSM330_LC_L				0
#define RES_LSM330_LC_H				1

#define RES_LSM330_CTRL_REG4			2
#define RES_LSM330_CTRL_REG1			3
#define RES_LSM330_CTRL_REG2			4
#define RES_LSM330_CTRL_REG3			5
#define RES_LSM330_CTRL_REG5			6
#define RES_LSM330_CTRL_REG6			7

#define RES_LSM330_OFF_X			8
#define RES_LSM330_OFF_Y			9
#define RES_LSM330_OFF_Z			10

#define RES_LSM330_CS_X				11
#define RES_LSM330_CS_Y				12
#define RES_LSM330_CS_Z				13

#define RES_LSM330_VFC_1			14
#define RES_LSM330_VFC_2			15
#define RES_LSM330_VFC_3			16
#define RES_LSM330_VFC_4			17

#define RES_LSM330_THRS3			18

#define RES_LSM330_TIM4_1			20
#define RES_LSM330_TIM3_1			21
#define RES_LSM330_TIM2_1_L			22
#define RES_LSM330_TIM2_1_H			23
#define RES_LSM330_TIM1_1_L			24
#define RES_LSM330_TIM1_1_H			25

#define RES_LSM330_THRS2_1			26
#define RES_LSM330_THRS1_1			27
#define RES_LSM330_SA_1				28
#define RES_LSM330_MA_1				29
#define RES_LSM330_SETT_1			30

#define RES_LSM330_TIM4_2			31
#define RES_LSM330_TIM3_2			32
#define RES_LSM330_TIM2_2_L			33
#define RES_LSM330_TIM2_2_H			34
#define RES_LSM330_TIM1_2_L			35
#define RES_LSM330_TIM1_2_H			36

#define RES_LSM330_THRS2_2			37
#define RES_LSM330_THRS1_2			38
#define RES_LSM330_DES_2			39
#define RES_LSM330_SA_2				40
#define RES_LSM330_MA_2				41
#define RES_LSM330_SETT_2			42

#define LSM330_RESUME_ENTRIES			43



#define LSM330_STATE_PR_SIZE			16

#define LSM330_SM1_DIS_SM2_DIS			(0x00)
#define LSM330_SM1_EN_SM2_DIS			(0x01)
#define LSM330_SM1_DIS_SM2_EN			(0x02)
#define LSM330_SM1_EN_SM2_EN			(0x03)

#define LSM330_INT1_DIS_INT2_DIS		(0x00)
#define LSM330_INT1_EN_INT2_DIS			(0x01)
#define LSM330_INT1_DIS_INT2_EN			(0x02)
#define LSM330_INT1_EN_INT2_EN			(0x03)
static struct rot_matrix {
       short matrix[3][3];
	} rot_matrix[] = {
		[0] = {
				.matrix = {
						{1, 0, 0},
						{0, 1, 0},
						{0, 0, 1}, }
		},
		[1] = {
				.matrix = {
						{-1, 0, 0},
						{0, -1, 0},
						{0, 0, 1}, }
		},
		[2] = {
				.matrix = {
						{0, 1, 0},
						{-1, 0, 0},
						{0, 0, 1}, }
		},
		[3] = {
				.matrix = {
						{0, -1, 0},
						{1, 0, 0},
						{0, 0, 1}, }
		},
		[4] = {
				.matrix = {
						{0, -1, 0},
						{-1, 0, 0},
						{0, 0, -1}, }
		},
		[5] = {
				.matrix = {
						{0, 1, 0},
						{1, 0, 0},
						{0, 0, -1}, }
		},
		[6] = {
				.matrix = {
						{1, 0, 0},
						{0, -1, 0},
						{0, 0, -1}, }
		},
		[7] = {
				.matrix = {
						{-1, 0, 0},
						{0, 1, 0},
						{0, 0, -1}, }
		},
};

#define D(x...) printk(KERN_DEBUG "[GSNR][LSM330] " x)
#define I(x...) printk(KERN_INFO "[GSNR][LSM330] " x)
#define W(x...) printk(KERN_WARNING "[GSNR][LSM330] " x)
#define E(x...) printk(KERN_ERR "[GSNR][LSM330] " x)
struct workqueue_struct *lsm330_workqueue = 0;

struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} lsm330_acc_odr_table[] = {
		{    1, LSM330_ODR1600 },
		{    3, LSM330_ODR400  },
		{   10, LSM330_ODR100  },
		{   20, LSM330_ODR50   },
		{   40, LSM330_ODR25   },
		{   80, LSM330_ODR12_5 },
		{  160, LSM330_ODR6_25 },
		{  320, LSM330_ODR3_125},
};

static struct lsm330_acc_platform_data default_lsm330_acc_pdata = {
	.fs_range = LSM330_ACC_G_2G,
	.chip_layout =0,
	.rot_matrix_index = 0,
	.poll_interval = 10,
	.min_interval = LSM330_ACC_MIN_POLL_PERIOD_MS,
	.gpio_int1 = LSM330_ACC_DEFAULT_INT1_GPIO,
	.gpio_int2 = LSM330_ACC_DEFAULT_INT2_GPIO,
};
static int DEBUG_FLAG = 0;
module_param(DEBUG_FLAG,int,0600);
static int calibration_version;
static int int1_gpio = LSM330_ACC_DEFAULT_INT1_GPIO;
static int int2_gpio = LSM330_ACC_DEFAULT_INT2_GPIO;
module_param(int1_gpio, int, S_IRUGO);
module_param(int2_gpio, int, S_IRUGO);
MODULE_PARM_DESC(int1_gpio, "integer: gpio number being assined to interrupt PIN1");
MODULE_PARM_DESC(int2_gpio, "integer: gpio number being assined to interrupt PIN2");

static int EVENT_IS = 1;
module_param(EVENT_IS,int,0600);

static int REG_1B = 0x7D;
module_param(REG_1B,int,0600);
static int REG_1C = 0x40;
module_param(REG_1C,int,0600);
static int REG_1D = 0x20;
module_param(REG_1D,int,0600);
static int REG_1E = 0x10;
module_param(REG_1E,int,0600);
static int REG_20 = 0x77;
module_param(REG_20,int,0600);
static int REG_23 = 0x4c;
module_param(REG_23,int,0600);
static int REG_24 = 0xc0;
module_param(REG_24,int,0600);
static int REG_25 = 0x10;
module_param(REG_25,int,0600);
static int REG_21 = 0x01;
module_param(REG_21,int,0600);
static int REG_50 = 0x10;
module_param(REG_50,int,0600);
static int REG_51 = 0x14;
module_param(REG_51,int,0600);
static int REG_52 = 0x24;
module_param(REG_52,int,0600);
static int REG_54 = 0x78;
module_param(REG_54,int,0600);
static int REG_56 = 0x03;
module_param(REG_56,int,0600);
static int REG_57 = 0x02;
module_param(REG_57,int,0600);

static int REG_5A = 0x03;
module_param(REG_5A,int,0600);
static int REG_5B = 0x21;
module_param(REG_5B,int,0600);
static int REG_40 = 0x15;
module_param(REG_40,int,0600);
static int REG_41 = 0x47;
module_param(REG_41,int,0600);
static int REG_42 = 0x03;
module_param(REG_42,int,0600);
static int REG_43 = 0x62;
module_param(REG_43,int,0600);
static int REG_44 = 0x15;
module_param(REG_44,int,0600);
static int REG_45 = 0x47;
module_param(REG_45,int,0600);
static int REG_46 = 0x03;
module_param(REG_46,int,0600);
static int REG_47 = 0x62;
module_param(REG_47,int,0600);
static int REG_48 = 0x11;
module_param(REG_48,int,0600);


struct lsm330_acc_data {
	struct i2c_client *client;
	struct lsm330_acc_platform_data *pdata;

	struct mutex lock;
	struct work_struct input_work_acc;
	struct hrtimer hr_timer_acc;
	ktime_t ktime_acc;

	struct input_dev *input_dev;

#ifdef HTC_DTAP
	struct input_dev *input_dtap;
#endif
#ifdef CUSTOM_SYSFS_PATH
	struct class *acc_class;
	struct device *acc_dev;
#endif
	short rot_matrix[3][3];
	int hw_initialized;
	
	int hw_working;
	atomic_t enabled;
	atomic_t sign_mot_enabled;
	int enable_polling;
	int on_before_suspend;
	int use_smbus;

	u16 sensitivity;
	u8 stateprogs_enable_setting;

	u8 resume_state[LSM330_RESUME_ENTRIES];
	u8 resume_stmach_program1[LSM330_STATE_PR_SIZE];
	u8 resume_stmach_program2[LSM330_STATE_PR_SIZE];

	int irq1;
	struct work_struct irq1_work;
	struct workqueue_struct *irq1_work_queue;
	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;
        int offset_buf[3];
	int chip_layout;

	unsigned int num_samples;

#ifdef DEBUG
	u8 reg_addr;
#endif

#ifdef CONFIG_CIR_ALWAYS_READY
	struct input_dev *input_cir;
	struct wake_lock cir_always_ready_wake_lock;
#endif

#if SIGNIFICANT_MOTION > 0
	struct wake_lock sig_mot_wake_lock;
#endif
	int normal_odr;
};
struct lsm330_acc_data *acc_data;

#ifdef CONFIG_CIR_ALWAYS_READY
#include <linux/wakelock.h>
static int cir_flag = 0;
static int power_key_pressed = 0;

#define ANY_MOTION_INTERRUPT                          REL_DIAL
#define ANY_MOTION_HAPPENED                      7

static int lsm330_acc_i2c_write(struct lsm330_acc_data *acc, u8 * buf, int len);
static int lsm330_acc_i2c_read(struct lsm330_acc_data *acc, u8 * buf, int len);
static int enable_any_motion_int(int en, struct lsm330_acc_data *acc){

    u8 buf[2];
    int err = 0;


    if(en == 1) {


	buf[0] = 0x23;
	buf[1] = 0x68;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x23;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_CTRL_REG3 %x", buf[0]);

	buf[0] = 0x20;
	buf[1] = 0x57;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;

	buf[0] = 0x20;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_CTRL_REG4 = %x", buf[0]);
	buf[0] = 0x24;
	buf[1] = 0x00;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x24;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_CTRL_REG5 = %x", buf[0]);

	buf[0] = 0x57;
	buf[1] = 0x4B;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x57;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_THRS1_1 = %x", buf[0]);

	buf[0] = 0x40;
	buf[1] = 0x05;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x40;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_STATEPR1 = %x", buf[0]);

	buf[0] = 0x41;
	buf[1] = 0x11;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x41;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("0x41 = %x", buf[0]);

	buf[0] = 0x59;
	buf[1] = 0xFC;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x59;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_SA_1 = %x", buf[0]);

	buf[0] = 0x5A;
	buf[1] = 0xFC;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x5A;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_MA_1 = %x", buf[0]);


	buf[0] = 0x5B;
	buf[1] = 0x01;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;
	buf[0] = 0x5B;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_SETT_1 = %x", buf[0]);


	buf[0] = 0x21;
	buf[1] = 0x01;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;

	buf[0] = 0x21;
	err = lsm330_acc_i2c_read(acc, buf, 1);

	I("LSM330_CTRL_REG1 = %x", buf[0]);
    } else if(en == 0){

	buf[0] = LSM330_CTRL_REG1;
	buf[1] = 0x00;
	err = lsm330_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		goto err_interrupt_state;

    }

    return 0;
err_interrupt_state:
    return -1;
}

static ssize_t lsm330_enable_cir_interrupt(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long enable;
	int error;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);

	error = strict_strtoul(buf, 10, &enable);
	if (error)
	    return error;
	I("%s, power_key_pressed = %d\n", __func__, power_key_pressed);
	if(enable == 1 && !power_key_pressed){ 
	    cir_flag = 1;
	    enable_irq(acc->irq1);

	    error = enable_any_motion_int(1, acc);
	    if(error == -1)
		I("Always Ready enable failed \n");
	    I("Always Ready enable = 1 \n");


	}  else if(enable == 0){

	    error = enable_any_motion_int(0, acc);
	    if(!error) {
		power_key_pressed = 0;
		cir_flag = 0;
		I("Always Ready enable = 0 \n");

	    }

	}

	return count;
}
static ssize_t lsm330_clear_powerkey_pressed(struct device *dev,
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
static ssize_t lsm330_get_powerkry_pressed(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_key_pressed);
}
static DEVICE_ATTR(clear_powerkey_flag, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP,
		lsm330_get_powerkry_pressed, lsm330_clear_powerkey_pressed);
#endif
/* sets default init values to be written in registers at probe stage */
static void lsm330_acc_set_init_register_values(struct lsm330_acc_data *acc)
{
	acc->resume_state[RES_LSM330_LC_L] = 0x00;
	acc->resume_state[RES_LSM330_LC_H] = 0x00;

	acc->resume_state[RES_LSM330_CTRL_REG1] = (0x00 | LSM330_SM1INT_PININT1);
	acc->resume_state[RES_LSM330_CTRL_REG2] = (0x00 | LSM330_SM2INT_PININT1);
	acc->resume_state[RES_LSM330_CTRL_REG3] = LSM330_INT_ACT_H;
	if(acc->pdata->gpio_int1 >= 0)
		acc->resume_state[RES_LSM330_CTRL_REG3] =
				acc->resume_state[RES_LSM330_CTRL_REG3] | \
					LSM330_INT1_EN_ON;
	if(acc->pdata->gpio_int2 >= 0)
		acc->resume_state[RES_LSM330_CTRL_REG3] =
				acc->resume_state[RES_LSM330_CTRL_REG3] | \
					LSM330_INT2_EN_ON;

	acc->resume_state[RES_LSM330_CTRL_REG4] = (LSM330_BDU_EN |
							LSM330_ALL_AXES);
#ifdef HTC_DTAP
	acc->resume_state[RES_LSM330_CTRL_REG4] = 0x77;
#endif
	acc->resume_state[RES_LSM330_CTRL_REG5] = 0x00;
	acc->resume_state[RES_LSM330_CTRL_REG6] = 0x10;

	acc->resume_state[RES_LSM330_THRS3] = 0x00;
	acc->resume_state[RES_LSM330_OFF_X] = 0x00;
	acc->resume_state[RES_LSM330_OFF_Y] = 0x00;
	acc->resume_state[RES_LSM330_OFF_Z] = 0x00;

	acc->resume_state[RES_LSM330_CS_X] = 0x00;
	acc->resume_state[RES_LSM330_CS_Y] = 0x00;
	acc->resume_state[RES_LSM330_CS_Z] = 0x00;

	acc->resume_state[RES_LSM330_VFC_1] = 0x00;
	acc->resume_state[RES_LSM330_VFC_2] = 0x00;
	acc->resume_state[RES_LSM330_VFC_3] = 0x00;
	acc->resume_state[RES_LSM330_VFC_4] = 0x00;
}

static void lsm330_acc_set_init_statepr1_inst(struct lsm330_acc_data *acc)
{
#if LOAD_SM1_PARAMETERS > 0
	acc->resume_stmach_program1[0] = 0x01;
	acc->resume_stmach_program1[1] = 0x06;
	acc->resume_stmach_program1[2] = 0X28;
	acc->resume_stmach_program1[3] = 0X03;
	acc->resume_stmach_program1[4] = 0x46;
	acc->resume_stmach_program1[5] = 0x28;
	acc->resume_stmach_program1[6] = 0x11;
	acc->resume_stmach_program1[7] = 0x00;
	acc->resume_stmach_program1[8] = 0x00;
	acc->resume_stmach_program1[9] = 0x00;
	acc->resume_stmach_program1[10] = 0x00;
	acc->resume_stmach_program1[11] = 0x00;
	acc->resume_stmach_program1[12] = 0x00;
	acc->resume_stmach_program1[13] = 0x00;
	acc->resume_stmach_program1[14] = 0x00;
	acc->resume_stmach_program1[15] = 0x00;
#else 
	acc->resume_stmach_program1[0] = 0x00;
	acc->resume_stmach_program1[1] = 0x00;
	acc->resume_stmach_program1[2] = 0X00;
	acc->resume_stmach_program1[3] = 0X00;
	acc->resume_stmach_program1[4] = 0x00;
	acc->resume_stmach_program1[5] = 0x00;
	acc->resume_stmach_program1[6] = 0x00;
	acc->resume_stmach_program1[7] = 0x00;
	acc->resume_stmach_program1[8] = 0x00;
	acc->resume_stmach_program1[9] = 0x00;
	acc->resume_stmach_program1[10] = 0x00;
	acc->resume_stmach_program1[11] = 0x00;
	acc->resume_stmach_program1[12] = 0x00;
	acc->resume_stmach_program1[13] = 0x00;
	acc->resume_stmach_program1[14] = 0x00;
	acc->resume_stmach_program1[15] = 0x00;
#endif 
}

#define SM_NOP_GNTH1 0x05
#define SM_NOP_TI3 0x03
#define SM_NOP_TI4 0x04
#define SM_TI4_GNTH1 0x45

static void lsm330_acc_set_init_statepr2_inst(struct lsm330_acc_data *acc)
{
#if LOAD_SM2_PROGRAM > 0
#if SIGNIFICANT_MOTION > 0
	acc->resume_stmach_program2[0] = SM_NOP_GNTH1;
	acc->resume_stmach_program2[1] = SM_NOP_TI3;
	acc->resume_stmach_program2[2] = SM_NOP_TI4;
	acc->resume_stmach_program2[3] = SM_TI4_GNTH1;
	acc->resume_stmach_program2[4] = 0x00;
	acc->resume_stmach_program2[5] = 0x00;
	acc->resume_stmach_program2[6] = 0x00;
	acc->resume_stmach_program2[7] = 0x00;
	acc->resume_stmach_program2[8] = 0x00;
	acc->resume_stmach_program2[9] = 0x00;
	acc->resume_stmach_program2[10] = 0x00;
	acc->resume_stmach_program2[11] = 0x00;
	acc->resume_stmach_program2[12] = 0x00;
	acc->resume_stmach_program2[13] = 0x00;
	acc->resume_stmach_program2[14] = 0x00;
	acc->resume_stmach_program2[15] = 0x00;
#else
	acc->resume_stmach_program2[0] = 0x00;
	acc->resume_stmach_program2[1] = 0x00;
	acc->resume_stmach_program2[2] = 0x00;
	acc->resume_stmach_program2[3] = 0x00;
	acc->resume_stmach_program2[4] = 0x00;
	acc->resume_stmach_program2[5] = 0x00;
	acc->resume_stmach_program2[6] = 0x00;
	acc->resume_stmach_program2[7] = 0x00;
	acc->resume_stmach_program2[8] = 0x00;
	acc->resume_stmach_program2[9] = 0x00;
	acc->resume_stmach_program2[10] = 0x00;
	acc->resume_stmach_program2[11] = 0x00;
	acc->resume_stmach_program2[12] = 0x00;
	acc->resume_stmach_program2[13] = 0x00;
	acc->resume_stmach_program2[14] = 0x00;
	acc->resume_stmach_program2[15] = 0x00;
#endif 
#else 
	acc->resume_stmach_program2[0] = 0x00;
	acc->resume_stmach_program2[1] = 0x00;
	acc->resume_stmach_program2[2] = 0x00;
	acc->resume_stmach_program2[3] = 0x00;
	acc->resume_stmach_program2[4] = 0x00;
	acc->resume_stmach_program2[5] = 0x00;
	acc->resume_stmach_program2[6] = 0x00;
	acc->resume_stmach_program2[7] = 0x00;
	acc->resume_stmach_program2[8] = 0x00;
	acc->resume_stmach_program2[9] = 0x00;
	acc->resume_stmach_program2[10] = 0x00;
	acc->resume_stmach_program2[11] = 0x00;
	acc->resume_stmach_program2[12] = 0x00;
	acc->resume_stmach_program2[13] = 0x00;
	acc->resume_stmach_program2[14] = 0x00;
	acc->resume_stmach_program2[15] = 0x00;
#endif 
}

static void lsm330_acc_set_init_statepr1_param(struct lsm330_acc_data *acc)
{
#ifdef	LOAD_SP1_PARAMETERS
	acc->resume_state[RES_LSM330_TIM4_1] = 0xa0;
	acc->resume_state[RES_LSM330_TIM3_1] = 0x04;
	acc->resume_state[RES_LSM330_TIM2_1_L] = 0x03;
	acc->resume_state[RES_LSM330_TIM2_1_H] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_1_L] = 0x04;
	acc->resume_state[RES_LSM330_TIM1_1_H] = 0x00;
	acc->resume_state[RES_LSM330_THRS2_1] = 0x30;
	acc->resume_state[RES_LSM330_THRS1_1] = 0x00;
	
	acc->resume_state[RES_LSM330_SA_1] = 0x00;
	acc->resume_state[RES_LSM330_MA_1] = 0xfc;
	acc->resume_state[RES_LSM330_SETT_1] = 0xa1;
#else 
	acc->resume_state[RES_LSM330_TIM4_1] = 0x00;
	acc->resume_state[RES_LSM330_TIM3_1] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_1_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_1_H] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_1_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_1_H] = 0x00;
	acc->resume_state[RES_LSM330_THRS2_1] = 0x00;
	acc->resume_state[RES_LSM330_THRS1_1] = 0x00;
	
	acc->resume_state[RES_LSM330_SA_1] = 0x00;
	acc->resume_state[RES_LSM330_MA_1] = 0x00;
	acc->resume_state[RES_LSM330_SETT_1] = 0x00;
#endif
}

static void lsm330_acc_set_init_statepr2_param(struct lsm330_acc_data *acc)
{
#if LOAD_SM2_PARAMETERS > 0
#if SIGNIFICANT_MOTION > 0
	acc->resume_state[RES_LSM330_TIM4_2] = 0x64;
	acc->resume_state[RES_LSM330_TIM3_2] = 0xC8;
	acc->resume_state[RES_LSM330_TIM2_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_2_H] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_H] = 0x00;
	acc->resume_state[RES_LSM330_THRS2_2] = 0x00;
	acc->resume_state[RES_LSM330_THRS1_2] = THRS1_2_02G;
	acc->resume_state[RES_LSM330_DES_2] = 0x00;
	acc->resume_state[RES_LSM330_SA_2] = 0xA8;
	acc->resume_state[RES_LSM330_MA_2] = 0xA8;
	acc->resume_state[RES_LSM330_SETT_2] = 0x13;
#else
	acc->resume_state[RES_LSM330_TIM4_2] = 0x00;
	acc->resume_state[RES_LSM330_TIM3_2] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_2_H] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_H] = 0x00;
	acc->resume_state[RES_LSM330_THRS2_2] = 0x00;
	acc->resume_state[RES_LSM330_THRS1_2] = 0x00;
	acc->resume_state[RES_LSM330_DES_2] = 0x00;
	acc->resume_state[RES_LSM330_SA_2] = 0x00;
	acc->resume_state[RES_LSM330_MA_2] = 0x00;
	acc->resume_state[RES_LSM330_SETT_2] = 0x00;
#endif  
#else	
	acc->resume_state[RES_LSM330_TIM4_2] = 0x00;
	acc->resume_state[RES_LSM330_TIM3_2] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM2_2_H] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_L] = 0x00;
	acc->resume_state[RES_LSM330_TIM1_2_H] = 0x00;
	acc->resume_state[RES_LSM330_THRS2_2] = 0x00;
	acc->resume_state[RES_LSM330_THRS1_2] = 0x00;
	acc->resume_state[RES_LSM330_DES_2] = 0x00;
	acc->resume_state[RES_LSM330_SA_2] = 0x00;
	acc->resume_state[RES_LSM330_MA_2] = 0x00;
	acc->resume_state[RES_LSM330_SETT_2] = 0x00;
#endif
}

static int lsm330_acc_i2c_read(struct lsm330_acc_data *acc,
				u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg	msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buf,
		},
		{
			.addr = acc->client->addr,
			.flags = (acc->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int lsm330_acc_i2c_write(struct lsm330_acc_data *acc, u8 * buf,
								int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
		 .addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
		 .len = len + 1,
		 .buf = buf,
		 },
	};

	if ((buf[0] == 0x20) && ((buf[1] & 0xF) != 0xF)) {
		printk(KERN_WARNING "[GSNR][LSM330_WARN] %s: buf(0, 1) = (0x%X, 0x%X)\n",
			__func__, buf[0], buf[1]);
		dump_stack();
	}

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int lsm330_acc_i2c_update(struct lsm330_acc_data *acc,
				u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 rdbuf[1] = { reg_address };
	u8 wrbuf[2] = { reg_address , 0x00 };

	u8 init_val;
	u8 updated_val;
	err = lsm330_acc_i2c_read(acc, rdbuf, 1);
	if (!(err < 0)) {
		init_val = rdbuf[0];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		wrbuf[1] = updated_val;
		err = lsm330_acc_i2c_write(acc, wrbuf, 1);
	}
	return err;
}
#ifdef HTC_DTAP
static int lsm330_db_reg_init(struct lsm330_acc_data *acc)
{
	u8 CTRL_REG2_A[2];

	u8 CTRL_REG4_A[2];
	u8 CTRL_REG5_A[2];
	u8 CTRL_REG6_A[2];
	u8 CTRL_REG7_A[2];

	u8 VFC_1[2];
	u8 VFC_2[2];
	u8 VFC_3[2];
	u8 VFC_4[2];
	u8 CTRL_x25[2];
	u8 TIM3_1[2];
	u8 TIM2_1[2];
	u8 TIM1_1[2];
	u8 THRS2_1[2];
	u8 THRS1_1[2];
	u8 MA_1[2];
	u8 SETT_1[2];
	u8 STATEPR1[2];
	u8 STATE_41[2];
	u8 STATE_42[2];
	u8 STATE_43[2];
	u8 STATE_44[2];
	u8 STATE_45[2];
	u8 STATE_46[2];
	u8 STATE_47[2];
	u8 STATE_48[2];
	u8 TIM_50[2];
	CTRL_REG2_A[0] = 0x21;
	CTRL_REG2_A[1] = REG_21;

	CTRL_REG4_A[0] = 0x23;
	CTRL_REG4_A[1] = REG_23;
	CTRL_REG5_A[0] = 0x20;
	CTRL_REG5_A[1] = REG_20;
	CTRL_REG6_A[0] = 0x24;
	CTRL_REG6_A[1] = REG_24;

	VFC_1[0] = 0x1b;
	VFC_1[1] = REG_1B;
	VFC_2[0] = 0x1c;
	VFC_2[1] = REG_1C;
	VFC_3[0] = 0x1d;
	VFC_3[1] = REG_1D;
	VFC_4[0] = 0x1e;
	VFC_4[1] = REG_1E;
	CTRL_x25[0] = 0x25;
	CTRL_x25[1] = REG_25;
	TIM3_1[0] = 0x51;
	TIM3_1[1] = REG_51;
	TIM2_1[0] = 0x52;
	TIM2_1[1] = REG_52;
	TIM1_1[0] = 0x54;
	TIM1_1[1] = REG_54;
	THRS2_1[0] = 0x56;
	THRS2_1[1] = REG_56;
	THRS1_1[0] = 0x57;
	THRS1_1[1] = REG_57;
	MA_1[0] = 0x5a;
	MA_1[1] = REG_5A;
	SETT_1[0] = 0x5b;
	SETT_1[1] = REG_5B;
	STATEPR1[0] = 0x40;
	STATEPR1[1] = REG_40;
	STATE_41[0] = 0x41;
	STATE_41[1] = REG_41;
	STATE_42[0] = 0x42;
	STATE_42[1] = REG_42;
	STATE_43[0] = 0x43;
	STATE_43[1] = REG_43;
	STATE_44[0] = 0x44;
	STATE_44[1] = REG_44;
	STATE_45[0] = 0x45;
	STATE_45[1] = REG_45;
	STATE_46[0] = 0x46;
	STATE_46[1] = REG_46;
	STATE_47[0] = 0x47;
	STATE_47[1] = REG_47;
	STATE_48[0] = 0x48;
	STATE_48[1] = REG_48;
	TIM_50[0] = 0x50;
	TIM_50[1] = REG_50;

	lsm330_acc_i2c_write(acc, CTRL_REG2_A, 1);
	lsm330_acc_i2c_write(acc, CTRL_REG4_A, 1);
	lsm330_acc_i2c_write(acc, CTRL_REG5_A, 1);
	lsm330_acc_i2c_write(acc, CTRL_REG6_A, 1);
	lsm330_acc_i2c_write(acc, CTRL_REG7_A, 1);

	lsm330_acc_i2c_write(acc, VFC_1, 1);
	lsm330_acc_i2c_write(acc, VFC_2, 1);
	lsm330_acc_i2c_write(acc, VFC_3, 1);
	lsm330_acc_i2c_write(acc, VFC_4, 1);


	lsm330_acc_i2c_write(acc, CTRL_x25, 1);
	lsm330_acc_i2c_write(acc, TIM3_1, 1);
	lsm330_acc_i2c_write(acc, TIM2_1, 1);
	lsm330_acc_i2c_write(acc, TIM1_1, 1);

	lsm330_acc_i2c_write(acc, THRS2_1, 1);
	lsm330_acc_i2c_write(acc, THRS1_1, 1);
	lsm330_acc_i2c_write(acc, MA_1, 1);
	lsm330_acc_i2c_write(acc, SETT_1, 1);

	lsm330_acc_i2c_write(acc, STATEPR1, 1);
	lsm330_acc_i2c_write(acc, STATE_41, 1);
	lsm330_acc_i2c_write(acc, STATE_42, 1);
	lsm330_acc_i2c_write(acc, STATE_43, 1);
	lsm330_acc_i2c_write(acc, STATE_44, 1);
	lsm330_acc_i2c_write(acc, STATE_45, 1);
	lsm330_acc_i2c_write(acc, STATE_46, 1);
	lsm330_acc_i2c_write(acc, STATE_47, 1);
	lsm330_acc_i2c_write(acc, STATE_48, 1);
	lsm330_acc_i2c_write(acc, TIM_50, 1);
	return 0;

}
#endif
static int lsm330_acc_hw_init(struct lsm330_acc_data *acc)
{
	int i;
	int err = -1;
	u8 buf[17];

	I("%s: hw init start\n", LSM330_ACC_DEV_NAME);

	buf[0] = LSM330_WHO_AM_I;
	err = lsm330_acc_i2c_read(acc, buf, 1);
	if (err < 0) {
	dev_warn(&acc->client->dev, "Error reading WHO_AM_I: is device "
		"available/working?\n");
		goto err_firstread;
	} else
		acc->hw_working = 1;

	if (buf[0] != WHOAMI_LSM330_ACC) {
	dev_err(&acc->client->dev,
		"device unknown. Expected: 0x%02x,"
		" Replies: 0x%02x\n", WHOAMI_LSM330_ACC, buf[0]);
		err = -1; 
		goto err_unknown_device;
	}


	buf[0] = (I2C_AUTO_INCREMENT | LSM330_LC_L);
	buf[1] = acc->resume_state[RES_LSM330_LC_L];
	buf[2] = acc->resume_state[RES_LSM330_LC_H];
	err = lsm330_acc_i2c_write(acc, buf, 2);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | LSM330_TIM4_1);
	buf[1] = acc->resume_state[RES_LSM330_TIM4_1];
	buf[2] = acc->resume_state[RES_LSM330_TIM3_1];
	buf[3] = acc->resume_state[RES_LSM330_TIM2_1_L];
	buf[4] = acc->resume_state[RES_LSM330_TIM2_1_H];
	buf[5] = acc->resume_state[RES_LSM330_TIM1_1_L];
	buf[6] = acc->resume_state[RES_LSM330_TIM1_1_H];
	buf[7] = acc->resume_state[RES_LSM330_THRS2_1];
	buf[8] = acc->resume_state[RES_LSM330_THRS1_1];
	err = lsm330_acc_i2c_write(acc, buf, 8);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | LSM330_SA_1);
	buf[1] = acc->resume_state[RES_LSM330_SA_1];
	buf[2] = acc->resume_state[RES_LSM330_MA_1];
	buf[3] = acc->resume_state[RES_LSM330_SETT_1];
	err = lsm330_acc_i2c_write(acc, buf, 3);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | LSM330_TIM4_2);
	buf[1] = acc->resume_state[RES_LSM330_TIM4_2];
	buf[2] = acc->resume_state[RES_LSM330_TIM3_2];
	buf[3] = acc->resume_state[RES_LSM330_TIM2_2_L];
	buf[4] = acc->resume_state[RES_LSM330_TIM2_2_H];
	buf[5] = acc->resume_state[RES_LSM330_TIM1_2_L];
	buf[6] = acc->resume_state[RES_LSM330_TIM1_2_H];
	buf[7] = acc->resume_state[RES_LSM330_THRS2_2];
	buf[8] = acc->resume_state[RES_LSM330_THRS1_2];
	buf[9] = acc->resume_state[RES_LSM330_DES_2];
	buf[10] = acc->resume_state[RES_LSM330_SA_2];
	buf[11] = acc->resume_state[RES_LSM330_MA_2];
	buf[12] = acc->resume_state[RES_LSM330_SETT_2];
	err = lsm330_acc_i2c_write(acc, buf, 12);
	if (err < 0)
		goto err_resume_state;

	
	buf[0] = (I2C_AUTO_INCREMENT | LSM330_STATEPR1);
	for (i = 1; i <= LSM330_STATE_PR_SIZE; i++) {
		buf[i] = acc->resume_stmach_program1[i-1];
		DIF("i=%d,sm pr1 buf[%d]=0x%02x\n", i, i, buf[i]);
	};
	err = lsm330_acc_i2c_write(acc, buf, LSM330_STATE_PR_SIZE);
	if (err < 0)
		goto err_resume_state;

	
	buf[0] = (I2C_AUTO_INCREMENT | LSM330_STATEPR2);
	for(i = 1; i <= LSM330_STATE_PR_SIZE; i++){
		buf[i] = acc->resume_stmach_program2[i-1];
		DIF("i=%d,sm pr2 buf[%d]=0x%02x\n", i, i, buf[i]);
	};
	err = lsm330_acc_i2c_write(acc, buf, LSM330_STATE_PR_SIZE);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | LSM330_CTRL_REG5);
	buf[1] = acc->resume_state[RES_LSM330_CTRL_REG5];
	buf[2] = acc->resume_state[RES_LSM330_CTRL_REG6];
	err = lsm330_acc_i2c_write(acc, buf, 2);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | LSM330_CTRL_REG1);
	buf[1] = acc->resume_state[RES_LSM330_CTRL_REG1];
	buf[2] = acc->resume_state[RES_LSM330_CTRL_REG2];
	buf[3] = acc->resume_state[RES_LSM330_CTRL_REG3];
	err = lsm330_acc_i2c_write(acc, buf, 3);
	if (err < 0)
		goto err_resume_state;


	buf[0] = (LSM330_CTRL_REG4);
	buf[1] = acc->resume_state[RES_LSM330_CTRL_REG4];
	err = lsm330_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;

	acc->hw_initialized = 1;
#ifdef HTC_DTAP
	lsm330_db_reg_init(acc);
#endif
	I("%s: hw init done\n", LSM330_ACC_DEV_NAME);

	return 0;

err_firstread:
	acc->hw_working = 0;
err_unknown_device:
err_resume_state:
	acc->hw_initialized = 0;
	dev_err(&acc->client->dev, "hw init error 0x%02x,0x%02x: %d\n", buf[0],
			buf[1], err);
	return err;
}

static void lsm330_acc_device_power_off(struct lsm330_acc_data *acc)
{
	int err;

	err = lsm330_acc_i2c_update(acc, LSM330_CTRL_REG4,
					LSM330_ODR_MASK, LSM330_PM_OFF);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power off failed: %d\n", err);

	if (acc->pdata->power_off) {
		if(acc->pdata->gpio_int1)
			disable_irq_nosync(acc->irq1);
		if(acc->pdata->gpio_int2)
			disable_irq_nosync(acc->irq2);
		acc->pdata->power_off();
		acc->hw_initialized = 0;
	}
	if (acc->hw_initialized) {
		D("%s: acc->pdata->gpio_int1 = %d, acc->pdata->gpio_int2 = %d\n", __func__, acc->pdata->gpio_int1, acc->pdata->gpio_int2);
		if(acc->pdata->gpio_int1 >= 0)
			disable_irq_nosync(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			disable_irq_nosync(acc->irq2);
		acc->hw_initialized = 0;
	}
}

static int lsm330_acc_device_power_on(struct lsm330_acc_data *acc)
{
	int err = -1;

	if (acc->pdata->power_on) {
		err = acc->pdata->power_on();
		if (err < 0) {
			dev_err(&acc->client->dev,
					"power_on failed: %d\n", err);
			return err;
		}
		if(acc->pdata->gpio_int1 >= 0)
			enable_irq(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			enable_irq(acc->irq2);
	}

	if (!acc->hw_initialized) {
		err = lsm330_acc_hw_init(acc);
		if (acc->hw_working == 1 && err < 0) {
			lsm330_acc_device_power_off(acc);
			return err;
		}
	}

	if (acc->hw_initialized) {
		if(acc->pdata->gpio_int1 >= 0)
			enable_irq(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			enable_irq(acc->irq2);
	}
	return 0;
}

#if SIGNIFICANT_MOTION > 0
static void lsm330_acc_signMotion_interrupt_action(struct lsm330_acc_data *status)
{
	input_report_abs(status->input_dev, ABS_SIGN_MOTION, 1);
	input_sync(status->input_dev);
	input_report_abs(status->input_dev, ABS_SIGN_MOTION, 0);
	input_sync(status->input_dev);
	D("%s: Significant Motion event\n", LSM330_ACC_DEV_NAME);
	atomic_set(&status->sign_mot_enabled, 0);
}
#endif

static irqreturn_t lsm330_acc_isr1(int irq, void *dev)
{
	struct lsm330_acc_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq1_work_queue, &acc->irq1_work);
	D("%s: isr1 queued\n", LSM330_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static irqreturn_t lsm330_acc_isr2(int irq, void *dev)
{
	struct lsm330_acc_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq2_work_queue, &acc->irq2_work);
	DIF("%s: isr2 queued\n", LSM330_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static void lsm330_acc_irq1_work_func(struct work_struct *work)
{

	int err = -1;
	u8 rbuf[2];
	u8 status;
	struct lsm330_acc_data *acc;

#if SIGNIFICANT_MOTION > 0
	u8 interrupt_stat = 0;
#endif
	acc = container_of(work, struct lsm330_acc_data, irq1_work);
	DIF("%s: IRQ1 triggered\n", LSM330_ACC_DEV_NAME);
	
	rbuf[0] = LSM330_INTERR_STAT;
	err = lsm330_acc_i2c_read(acc, rbuf, 1);
	DIF("%s: INTERR_STAT_REG: 0x%02x\n",
					LSM330_ACC_DEV_NAME, rbuf[0]);
#if SIGNIFICANT_MOTION > 0
	interrupt_stat = rbuf[0];
#endif

	status = rbuf[0];
	if(status & LSM330_STAT_INTSM1_BIT) {
		rbuf[0] = LSM330_OUTS_1;
		err = lsm330_acc_i2c_read(acc, rbuf, 1);
		DIF("%s: OUTS_1: 0x%02x\n",
					LSM330_ACC_DEV_NAME, rbuf[0]);
	}
	if(status & LSM330_STAT_INTSM2_BIT) {
		rbuf[0] = LSM330_OUTS_2;
		err = lsm330_acc_i2c_read(acc, rbuf, 1);
		DIF("%s: OUTS_2: 0x%02x\n",
					LSM330_ACC_DEV_NAME, rbuf[0]);
	}
	DIF("%s: IRQ1 served\n", LSM330_ACC_DEV_NAME);
#ifdef HTC_DTAP
	if(EVENT_IS==1){
		input_report_rel(acc->input_dtap, REL_WHEEL, -1);
		input_sync(acc->input_dtap);
	}

	else{
		input_report_key(acc->input_dtap, KEY_ENTER, 1 );
		input_sync(acc->input_dtap);
		input_report_key( acc->input_dtap, KEY_ENTER, 0 );
		input_sync(acc->input_dtap);
	}
#endif

#ifdef CONFIG_CIR_ALWAYS_READY
	if(cir_flag == 1){
	    wake_lock_timeout(&(acc->cir_always_ready_wake_lock), 1*HZ);
	    I("%s: wake_lock 1 second!\n", __func__);
	    input_report_rel(acc->input_cir,
		    ANY_MOTION_INTERRUPT,
		    ANY_MOTION_HAPPENED);
	    input_sync(acc->input_cir);
	}
#endif

#if SIGNIFICANT_MOTION > 0
	D("%s: interrupt_stat = 0x%x", __func__, interrupt_stat);
	if (interrupt_stat & LSM330_STAT_INTSM2_BIT) {
		wake_lock_timeout(&(acc->sig_mot_wake_lock), 1*HZ);
		I("%s: wake_lock 1 second for Significant Motion!\n", __func__);
		lsm330_acc_signMotion_interrupt_action(acc);
	}
#endif

	enable_irq(acc->irq1);
	DIF("%s: IRQ1 re-enabled\n", LSM330_ACC_DEV_NAME);
}

static void lsm330_acc_irq2_work_func(struct work_struct *work)
{
	struct lsm330_acc_data *acc;

	acc = container_of(work, struct lsm330_acc_data, irq2_work);
	DIF("%s: IRQ2 triggered\n", LSM330_ACC_DEV_NAME);
	
	DIF("%s: IRQ2 served\n", LSM330_ACC_DEV_NAME);

	enable_irq(acc->irq2);
	DIF("%s: IRQ2 re-enabled\n", LSM330_ACC_DEV_NAME);
}

static int lsm330_acc_register_masked_update(struct lsm330_acc_data *acc,
		u8 reg_address, u8 mask, u8 new_bit_values, int resume_index)
{
	u8 config[2] = {0};
	u8 init_val, updated_val;
	int err;
	int step = 0;

	config[0] = reg_address;
	err = lsm330_acc_i2c_read(acc, config, 1);
	if (err < 0)
		goto error;

	init_val = config[0];
	acc->resume_state[resume_index] = init_val;
	step = 1;
	updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
	config[0] = reg_address;
	config[1] = updated_val;
	err = lsm330_acc_i2c_write(acc, config, 1);
	if (err < 0)
		goto error;
	acc->resume_state[resume_index] = updated_val;

	return err;
	error:
		dev_err(&acc->client->dev,
			"register 0x%02x update failed at step %d, error: %d\n",
				config[0], step, err);
	return err;
}

static int lsm330_acc_update_fs_range(struct lsm330_acc_data *acc,
								u8 new_fs_range)
{
	int err=-1;
	u16 sensitivity;
	u8 sigmot_threshold;
	u8 init_val, updated_val;

	switch (new_fs_range) {
	case LSM330_ACC_G_2G:
		sensitivity = SENSITIVITY_2G;
		sigmot_threshold = THRS1_2_02G;
		break;
	case LSM330_ACC_G_4G:
		sensitivity = SENSITIVITY_4G;
		sigmot_threshold = THRS1_2_04G;
		break;
	case LSM330_ACC_G_6G:
		sensitivity = SENSITIVITY_6G;
		sigmot_threshold = THRS1_2_06G;
		break;
	case LSM330_ACC_G_8G:
		sensitivity = SENSITIVITY_8G;
		sigmot_threshold = THRS1_2_08G;
		break;
	case LSM330_ACC_G_16G:
		sensitivity = SENSITIVITY_16G;
		sigmot_threshold = THRS1_2_16G;
		break;
	default:
		dev_err(&acc->client->dev, "invalid g range requested: %u\n",
				new_fs_range);
		return -EINVAL;
	}

	if (atomic_read(&acc->enabled)) {
		err = lsm330_acc_register_masked_update(acc, LSM330_CTRL_REG5,
			LSM330_ACC_FS_MASK, new_fs_range, RES_LSM330_CTRL_REG5);
		if(err < 0) {
			dev_err(&acc->client->dev, "update g range failed\n");
			return err;
		} else
			acc->sensitivity = sensitivity;

#if SIGNIFICANT_MOTION > 0
		err = lsm330_acc_register_masked_update(acc, LSM330_THRS1_2,
			0xFF, sigmot_threshold, RES_LSM330_THRS1_2);
		if (err < 0)
			dev_err(&acc->client->dev, "update sign motion theshold"
								" failed\n");
		return err;
#endif
	} else {
		init_val = acc->resume_state[RES_LSM330_CTRL_REG5];
		updated_val = ((LSM330_ACC_FS_MASK & new_fs_range) | ((~LSM330_ACC_FS_MASK) & init_val));
		acc->resume_state[RES_LSM330_CTRL_REG5] = updated_val;

#if SIGNIFICANT_MOTION > 0
		acc->resume_state[RES_LSM330_THRS1_2] = sigmot_threshold;
#endif
		return 0;
	}

	return err;
}


static int lsm330_acc_update_odr(struct lsm330_acc_data *acc,
							int poll_interval_ms)
{
	int err = -1;
	int i;
	u8 new_odr = LSM330_ODR12_5;
	u8 updated_val;
	u8 init_val;
	u8 mask = LSM330_ODR_MASK;

	D("%s: poll_interval_ms = %d\n", __func__, poll_interval_ms);

#if SIGNIFICANT_MOTION > 0
	if (poll_interval_ms == 0)
		poll_interval_ms = 10;
#endif

#ifdef HTC_DTAP
	u8 CTRL_REG4[2];
	CTRL_REG4[0] = 0x20;
	CTRL_REG4[1] = 0x77;
#endif
	for (i = ARRAY_SIZE(lsm330_acc_odr_table) - 1; i >= 0; i--) {
		if (lsm330_acc_odr_table[i].cutoff_ms <= poll_interval_ms)
			break;
	}

	if ((i >= 0) && (i < ARRAY_SIZE(lsm330_acc_odr_table)))
		new_odr = lsm330_acc_odr_table[i].mask;

	if (atomic_read(&acc->enabled)) {
		err = lsm330_acc_register_masked_update(acc,
			LSM330_CTRL_REG4, LSM330_ODR_MASK, new_odr,
							RES_LSM330_CTRL_REG4);
		if (err < 0) {
			dev_err(&acc->client->dev, "update odr failed\n");
			return err;
		}
		acc->ktime_acc = ktime_set(0, MS_TO_NS(poll_interval_ms));
	} else {
		init_val = acc->resume_state[RES_LSM330_CTRL_REG4];
		updated_val = ((mask & new_odr) | ((~mask) & init_val));
		acc->resume_state[RES_LSM330_CTRL_REG4] = updated_val;
		return 0;
	}
#ifdef HTC_DTAP
	lsm330_acc_i2c_write(acc, CTRL_REG4, 1);
#endif
	if(err < 0)
		dev_err(&acc->client->dev, "update odr failed\n");
	return err;
}


#ifdef DEBUG
static int lsm330_acc_register_write(struct lsm330_acc_data *acc, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err = -1;

		buf[0] = reg_address;
		buf[1] = new_value;
		err = lsm330_acc_i2c_write(acc, buf, 1);
		if (err < 0)
			return err;
	return err;
}

static int lsm330_acc_register_read(struct lsm330_acc_data *acc, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = lsm330_acc_i2c_read(acc, buf, 1);
	return err;
}

static int lsm330_acc_register_update(struct lsm330_acc_data *acc, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;
	err = lsm330_acc_register_read(acc, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[0];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		err = lsm330_acc_register_write(acc, buf, reg_address,
				updated_val);
	}
	return err;
}
#endif


static int lsm330_acc_get_data(struct lsm330_acc_data *acc, int *xyz)
{
	int i, err = -1;
	
	u8 acc_data[6] = {0};
	
	s32 hw_d[3] = { 0 };
	u8 buf[2] = {0};

	acc_data[0] = (I2C_AUTO_INCREMENT | OUT_AXISDATA_REG);
	err = lsm330_acc_i2c_read(acc, acc_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = ((s16) ((acc_data[1] << 8) | acc_data[0]));
	hw_d[1] = ((s16) ((acc_data[3] << 8) | acc_data[2]));
	hw_d[2] = ((s16) ((acc_data[5] << 8) | acc_data[4]));

	hw_d[0] = hw_d[0] * acc->sensitivity;
	hw_d[1] = hw_d[1] * acc->sensitivity;
	hw_d[2] = hw_d[2] * acc->sensitivity;

	for (i = 0; i < 3; i++) {
		xyz[i] = acc->rot_matrix[0][i] * hw_d[0] +
				acc->rot_matrix[1][i] * hw_d[1] +
					acc->rot_matrix[2][i] * hw_d[2];
	}


	if ((xyz[0] == 0) && (xyz[1] == 0) && (xyz[2] == 0)) {
		I("%s: Data all Zero, try to recover!\n", __func__);

		buf[0] = LSM330_CTRL_REG4;
		err = lsm330_acc_i2c_read(acc, buf, 1);
		I("%s: Before recover, LSM330_CTRL_REG4 = 0x%X\n", __func__, buf[0]);

		err = lsm330_acc_i2c_update(acc, LSM330_CTRL_REG4,
				LSM330_XYZ_BDU_MASK, 0xF);
		if (err < 0)
			dev_err(&acc->client->dev, "%s: Recover: err = %d\n", __func__, err);

		buf[0] = LSM330_CTRL_REG4;
		err = lsm330_acc_i2c_read(acc, buf, 1);
		I("%s: After recover LSM330_CTRL_REG4 = 0x%X\n", __func__, buf[0]);
	}

	DIF("%s read raw x=%d, y=%d, z=%d\n",
			LSM330_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);

	return err;
}

static void lsm330_acc_report_values(struct lsm330_acc_data *acc,
					int *xyz)
{
	if (calibration_version) {
		xyz[0] = xyz[0] + acc->offset_buf[0] * 1000000 / 256;
		xyz[1] = xyz[1] + acc->offset_buf[1] * 1000000 / 256;
		xyz[2] = xyz[2] + acc->offset_buf[2] * 1000000 / 256;
	} else {
		xyz[0] = xyz[0] + acc->offset_buf[0] * 1000;
		xyz[1] = xyz[1] + acc->offset_buf[1] * 1000;
		xyz[2] = xyz[2] + acc->offset_buf[2] * 1000;
	}

	input_report_abs(acc->input_dev, ABS_X, xyz[0]);
	input_report_abs(acc->input_dev, ABS_Y, xyz[1]);
	input_report_abs(acc->input_dev, ABS_Z, xyz[2]);
	input_sync(acc->input_dev);
}

static void lsm330_acc_polling_manage(struct lsm330_acc_data *acc)
{
	int err = 0;

	if (acc->enable_polling) {
		if (atomic_read(&acc->enabled)) {
			
			if (acc->num_samples <= NUMBER_DISCARD_SAMPLES) {
				hrtimer_start(&acc->hr_timer_acc,
					ktime_set(0, DEFAULT_FIRST_SAMPLE_DELAY_NS),
					HRTIMER_MODE_REL);
			} else {
				
				if (acc->normal_odr == 0) {
					D("%s: Set to the ODR that AP requested: poll_interval = %u\n",
						__func__, acc->pdata->poll_interval);

					mutex_lock(&acc->lock);
					err = lsm330_acc_update_odr(acc, acc->pdata->poll_interval);
					if (err < 0)
						E("%s: update_odr failed\n", __func__);
					mutex_unlock(&acc->lock);

					acc->normal_odr = 1;
				}

				hrtimer_start(&acc->hr_timer_acc,
					acc->ktime_acc, HRTIMER_MODE_REL);

			}
		}
	} else
		hrtimer_cancel(&acc->hr_timer_acc);
}

static int lsm330_acc_enable(struct lsm330_acc_data *acc)
{
	int err;
	int i;

	I("%s: sign_mot_enabled = %d, enabled = %d\n", __func__,
		atomic_read(&acc->sign_mot_enabled),
		atomic_read(&acc->enabled));
	if (!atomic_cmpxchg(&acc->enabled, 0, 1)) {
		err = lsm330_acc_device_power_on(acc);
		if (err < 0) {
			atomic_set(&acc->enabled, 0);
			return err;
		}

		acc->num_samples = 0;
		acc->normal_odr = 0;

		D("%s: Set the ODR to 100Hz for NUMBER_DISCARD_SAMPLES = %d\n",
			__func__, NUMBER_DISCARD_SAMPLES);
		mutex_lock(&acc->lock);
		err = lsm330_acc_update_odr(acc, 10);
		if (err < 0)
			E("%s: update_odr failed\n", __func__);
		mutex_unlock(&acc->lock);

		lsm330_acc_polling_manage(acc);
	} else if (acc->on_before_suspend) {
		acc->num_samples = 0;
		acc->normal_odr = 0;

		D("%s: on_before_suspend: Set the ODR to 100Hz for NUMBER_DISCARD_SAMPLES = %d\n",
			__func__, NUMBER_DISCARD_SAMPLES);
		mutex_lock(&acc->lock);
		err = lsm330_acc_update_odr(acc, 10);
		if (err < 0)
			E("%s: on_before_suspend: update_odr failed\n", __func__);
		mutex_unlock(&acc->lock);

		lsm330_acc_polling_manage(acc);
	}

        if ((acc->pdata->gs_kvalue & (0x67 << 24)) != (0x67 << 24)) {
                acc->offset_buf[0] = 0;
                acc->offset_buf[1] = 0;
                acc->offset_buf[2] = 0;
        } else {
                acc->offset_buf[0] = (acc->pdata->gs_kvalue >> 16) & 0xFF;
                acc->offset_buf[1] = (acc->pdata->gs_kvalue >>  8) & 0xFF;
                acc->offset_buf[2] =  acc->pdata->gs_kvalue        & 0xFF;

                for (i = 0; i < 3; i++) {
                        if (acc->offset_buf[i] > 127) {
                                acc->offset_buf[i] =
                                        acc->offset_buf[i] - 256;
                        }
                }
        }

	I("%s--\n", __func__);

	return 0;
}

static int lsm330_acc_disable(struct lsm330_acc_data *acc)
{
	I("%s: acc = %p\n", __func__, acc);
	I("%s: sign_mot_enabled = %d, enabled = %d\n", __func__,
		atomic_read(&acc->sign_mot_enabled),
		atomic_read(&acc->enabled));
	if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
		cancel_work_sync(&acc->input_work_acc);
		lsm330_acc_polling_manage(acc);
#ifdef CONFIG_CIR_ALWAYS_READY
		if(cir_flag != 1)
		    lsm330_acc_device_power_off(acc);

#else
		lsm330_acc_device_power_off(acc);
#endif
	}

	return 0;
}

static ssize_t attr_get_enable_polling(struct device *dev,
					struct device_attribute *attr,
								char *buf)
{
	int val;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);

	mutex_lock(&acc->lock);
	val = acc->enable_polling;
	mutex_unlock(&acc->lock);

	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable_polling(struct device *dev,
				struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	unsigned long enable;

	if (strict_strtoul(buf, 10, &enable))
		return -EINVAL;

	I("%s: enable = %lu\n", __func__, enable);
	mutex_lock(&acc->lock);
	if (enable)
		acc->enable_polling = 1;
	else
		acc->enable_polling = 0;
	mutex_unlock(&acc->lock);

	lsm330_acc_polling_manage(acc);

	return size;
}

static ssize_t attr_get_polling_rate(struct device *dev,
					struct device_attribute *attr,
								char *buf)
{
	int val;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);

	mutex_lock(&acc->lock);
	val = acc->pdata->poll_interval;
	mutex_unlock(&acc->lock);

	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_polling_rate(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t size)
{
	
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	unsigned long interval_ms;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;

	mutex_lock(&acc->lock);

	
	
	
		acc->pdata->poll_interval = interval_ms;
		acc->normal_odr = 0;
		D("%s: acc->normal_odr = %d, interval_ms = %ld\n", __func__, acc->normal_odr, interval_ms);
	
	mutex_unlock(&acc->lock);

	return size;
}

static ssize_t attr_get_range(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 val;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	int range = 2;

	mutex_lock(&acc->lock);
	val = acc->pdata->fs_range ;
	mutex_unlock(&acc->lock);

	switch(val) {
	case LSM330_ACC_G_2G:
		range = 2;
		break;
	case LSM330_ACC_G_4G:
		range = 4;
		break;
	case LSM330_ACC_G_6G:
		range = 6;
		break;
	case LSM330_ACC_G_8G:
		range = 8;
		break;
	case LSM330_ACC_G_16G:
		range = 16;
		break;
	}

	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_set_range(struct device *dev,
				struct device_attribute *attr,
						const char *buf, size_t size)
{
	int err;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	switch(val) {
		case 2:
			range = LSM330_ACC_G_2G;
			break;
		case 4:
			range = LSM330_ACC_G_4G;
			break;
		case 6:
			range = LSM330_ACC_G_6G;
			break;
		case 8:
			range = LSM330_ACC_G_8G;
			break;
		case 16:
			range = LSM330_ACC_G_16G;
			break;
		default:
			return -1;
	}
	mutex_lock(&acc->lock);
	err = lsm330_acc_update_fs_range(acc, range);
	if(err >= 0)
		acc->pdata->fs_range = range;

	mutex_unlock(&acc->lock);
	return size;
}

static ssize_t attr_get_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	int val = atomic_read(&acc->enabled);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable(struct device *dev,
				struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		lsm330_acc_enable(acc);
	else
		lsm330_acc_disable(acc);

	return size;
}
static ssize_t lsm330_chip_layout_show(struct device *dev,      
                struct device_attribute *attr, char *buf)
{
        if (acc_data == NULL) {
                E("%s: acc_data == NULL\n", __func__);
                return 0;
        }
        return sprintf(buf, "chip_layout = %d\n", acc_data->pdata->chip_layout);
#if 0
        return sprintf(buf, "chip_layout = %d\n"
                            "axis_map_x = %d, axis_map_y = %d,"
                            " axis_map_z = %d\n"
                            "negate_x = %d, negate_y = %d, negate_z = %d\n",
                            acc_data->pdata->chip_layout,
                            acc_data->pdata->axis_map_x, acc_data->pdata->axis_map_y,
                            acc_data->pdata->axis_map_z,
                            acc_data->pdata->negate_x, acc_data->pdata->negate_y,
                            acc_data->pdata->negate_z);
#endif
}


static ssize_t lsm330_get_raw_data_show(struct device *dev,     
                struct device_attribute *attr, char *buf)
{
        struct lsm330_acc_data *lsm330 = acc_data;
        int xyz[3] = { 0 };

        if (lsm330 == NULL) {
                E("%s: lsm330 == NULL\n", __func__);
                return 0;
        }
        lsm330_acc_get_data(lsm330, xyz);

        return sprintf(buf, "x = %d, y = %d, z = %d\n",
  
				xyz[0], xyz[1], xyz[2]);
}

static ssize_t lsm330_set_k_value_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct lsm330_acc_data *lsm330 = i2c_get_clientdata(client);

        if (lsm330 == NULL) {
                E("%s: lsm330 == NULL\n", __func__);
                return 0;
        }

        return sprintf(buf, "gs_kvalue = 0x%x\n", lsm330->pdata->gs_kvalue);
}


static ssize_t lsm330_set_k_value_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct lsm330_acc_data *lsm330 = i2c_get_clientdata(client);
        int i = 0;

        if (lsm330 == NULL) {
                E("%s: lsm330 == NULL\n", __func__);
                return count;
        }

        D("%s: Set buf = %s\n", __func__, buf);

        lsm330->pdata->gs_kvalue = simple_strtoul(buf, NULL, 10);

        D("%s: lsm330->pdata->gs_kvalue = 0x%x\n", __func__,
                lsm330->pdata->gs_kvalue);

        if ((lsm330->pdata->gs_kvalue & (0x67 << 24)) != (0x67 << 24)) {
                lsm330->offset_buf[0] = 0;
                lsm330->offset_buf[1] = 0;
                lsm330->offset_buf[2] = 0;
        } else {
                lsm330->offset_buf[0] = (lsm330->pdata->gs_kvalue >> 16) & 0xFF;
                lsm330->offset_buf[1] = (lsm330->pdata->gs_kvalue >>  8) & 0xFF;
                lsm330->offset_buf[2] =  lsm330->pdata->gs_kvalue        & 0xFF;

                for (i = 0; i < 3; i++) {
                        if (lsm330->offset_buf[i] > 127) {
                                lsm330->offset_buf[i] =
                                        lsm330->offset_buf[i] - 256;
                        }
                }
        }

        return count;
}


static int lsm330_acc_state_progrs_enable_control(
				struct lsm330_acc_data *acc, u8 settings)
{
	u8 val1, val2;
	int err = -1;
	settings = settings & 0x03;

	switch (settings) {
	case LSM330_SM1_DIS_SM2_DIS:
		val1 = LSM330_SM1_EN_OFF;
		val2 = LSM330_SM2_EN_OFF;
		break;
	case LSM330_SM1_DIS_SM2_EN:
		val1 = LSM330_SM1_EN_OFF;
		val2 = LSM330_SM2_EN_ON;
#if SIGNIFICANT_MOTION > 0
		I("%s: Enable Significant Motion_1\n", __func__);
		atomic_set(&acc->sign_mot_enabled, 1);
#endif
		break;
	case LSM330_SM1_EN_SM2_DIS:
		val1 = LSM330_SM1_EN_ON;
		val2 = LSM330_SM2_EN_OFF;
		break;
	case LSM330_SM1_EN_SM2_EN:
		val1 = LSM330_SM1_EN_ON;
		val2 = LSM330_SM2_EN_ON;
#if SIGNIFICANT_MOTION > 0
		I("%s: Enable Significant Motion_2\n", __func__);
		atomic_set(&acc->sign_mot_enabled, 1);
#endif
		break;
	default :
		E("invalid state program setting : 0x%02x\n",settings);
		return err;
	}
	err = lsm330_acc_register_masked_update(acc,
		LSM330_CTRL_REG1, LSM330_SM1_EN_MASK, val1,
							RES_LSM330_CTRL_REG1);
	if (err < 0 )
		return err;

	err = lsm330_acc_register_masked_update(acc,
		LSM330_CTRL_REG2, LSM330_SM2_EN_MASK, val2,
							RES_LSM330_CTRL_REG2);
	if (err < 0 )
		return err;

	acc->stateprogs_enable_setting = settings;

	DIF("%s: stateprogs_enable_setting = 0x%02x\n",
		__func__, acc->stateprogs_enable_setting);


	return err;
}

static ssize_t attr_set_DP(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	int err = -1;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	long val=0;
	u8 CTRL_REG4_A[2];
	u8 CTRL_REG5_A[2];
	u8 CTRL_REG6_A[2];

	
	CTRL_REG4_A[0] = 0x23;
	CTRL_REG4_A[1] = 0x58;
	CTRL_REG5_A[0] = 0x20;
	CTRL_REG5_A[1] = 0x77;
	CTRL_REG6_A[0] = 0x24;
	CTRL_REG6_A[1] = 0x08;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	if (val == 0x01){
		lsm330_acc_i2c_write(acc, CTRL_REG4_A, 1);
		lsm330_acc_i2c_write(acc, CTRL_REG5_A, 1);
		lsm330_acc_i2c_write(acc, CTRL_REG6_A, 1);
	}

	if (err < 0)
		return err;
	return size;
}

static ssize_t attr_set_enable_state_prog(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	int err = -1;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	long val=0;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;


	if ( val < 0x00 || val > LSM330_SM1_EN_SM2_EN){
		W("invalid state program setting, val: %ld\n",val);
		return -EINVAL;
	}

	err = lsm330_acc_state_progrs_enable_control(acc, val);
	if (err < 0)
		return err;
	return size;
}

static ssize_t attr_get_enable_state_prog(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	u8 val, val1 = 0, val2 = 0, config[2];
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);

	config[0] = LSM330_CTRL_REG1;
	lsm330_acc_i2c_read(acc, config, 1);
	val1 = (config[0] & LSM330_SM1_EN_MASK);

	config[0] = LSM330_CTRL_REG2;
	lsm330_acc_i2c_read(acc, config, 1);
	val2 = ((config[0] & LSM330_SM2_EN_MASK) << 1);

	val = (val1 | val2);

	return sprintf(buf, "0x%02x\n", val);
}




#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	mutex_lock(&acc->lock);
	x[0] = acc->reg_addr;
	mutex_unlock(&acc->lock);
	x[1] = val;
	rc = lsm330_acc_i2c_write(acc, x, 1);
	
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&acc->lock);
	data = acc->reg_addr;
	mutex_unlock(&acc->lock);
	rc = lsm330_acc_i2c_read(acc, &data, 1);
	
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&acc->lock);
	acc->reg_addr = val;
	mutex_unlock(&acc->lock);
	return size;
}
#endif

static ssize_t attr_reg_get_all(struct device *dev, struct device_attribute *attr,
                                char *buf)
{
        struct lsm330_acc_data *acc = dev_get_drvdata(dev);
        size_t count = 0;
        u8 reg[0x7f];
        int i;
        for (i = 0 ; i < 0x7f; i++) {
		reg[i] = i;
                lsm330_acc_i2c_read(acc, reg+i, 1);

                count += sprintf(&buf[count], "0x%x: 0x%x\n", i, reg[i]);
        }
        return count;
}

static ssize_t attr_calibration_version(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	sscanf(buf, "%d" , &calibration_version);
	I("calibration_version = %d", calibration_version);
	return size;
}

static struct device_attribute attributes[] = {

	__ATTR(pollrate_ms, 0666, attr_get_polling_rate,
							attr_set_polling_rate),
	__ATTR(range, 0666, attr_get_range, attr_set_range),
	__ATTR(enable_device, 0666, attr_get_enable, attr_set_enable),
	__ATTR(enable_polling, 0666, attr_get_enable_polling, attr_set_enable_polling),
	__ATTR(enable_state_prog, 0666, attr_get_enable_state_prog,
						attr_set_enable_state_prog),
        __ATTR(chip_layout, 0666, lsm330_chip_layout_show, NULL),
        __ATTR(get_raw_data, 0666, lsm330_get_raw_data_show, NULL),
        __ATTR(set_k_value, 0666, lsm330_set_k_value_show, lsm330_set_k_value_store),
	__ATTR(double_tap, 0666, NULL, attr_set_DP),
	__ATTR(reg_value_all, 0600, attr_reg_get_all, NULL),
      __ATTR(calibration_version, 0666, NULL, attr_calibration_version),
#ifdef DEBUG
	__ATTR(reg_value, 0600, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0200, NULL, attr_addr_set),
#endif
#ifdef CONFIG_CIR_ALWAYS_READY

	__ATTR(enable_cir_interrupt, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP,
		NULL, lsm330_enable_cir_interrupt),
#endif
};

static int create_sysfs_interfaces(struct lsm330_acc_data *acc)
{
	int i;
#ifdef CONFIG_CIR_ALWAYS_READY

	struct class *lsm330_cir_powerkey_class = NULL;
	struct device *lsm330_cir_powerkey_dev = NULL;
#endif

#ifdef CUSTOM_SYSFS_PATH
	acc->acc_class = class_create(THIS_MODULE, CUSTOM_SYSFS_CLASS_NAME_ACC);
	if (acc->acc_class == NULL)
		goto custom_class_error;

	acc->acc_dev = device_create(acc->acc_class, NULL, 0, "%s", "acc");
	if (acc->acc_dev == NULL)
		goto custom_class_error;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(acc->acc_dev, attributes + i))
			goto error;
#else
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(&acc->client->dev, attributes + i))
			goto error;
#endif

#ifdef CONFIG_CIR_ALWAYS_READY
	lsm330_cir_powerkey_class = class_create(THIS_MODULE, "bma250_powerkey");
	if (IS_ERR(lsm330_cir_powerkey_class)) {
		E("%s: could not allocate lsm330_cir_powerkey_class\n", __func__);
		goto err_create_powerkey_class;
	}

	lsm330_cir_powerkey_dev = device_create(lsm330_cir_powerkey_class,
				NULL, 0, "%s", "bma250");
	if (device_create_file(lsm330_cir_powerkey_dev, &dev_attr_clear_powerkey_flag)) {
	    E("%s, create bma250_device_create_file fail!\n", __func__);
	    goto error;
	}

#endif
	return 0;

error:
#ifdef CONFIG_CIR_ALWAYS_READY
	class_destroy(lsm330_cir_powerkey_class);
err_create_powerkey_class:

#endif

	for ( ; i >= 0; i--)
#ifdef CUSTOM_SYSFS_PATH
		device_remove_file(acc->acc_dev, attributes + i);
#else
		device_remove_file(&acc->client->dev, attributes + i);
#endif

#ifdef CUSTOM_SYSFS_PATH
custom_class_error:
#endif
	dev_err(&acc->client->dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	struct lsm330_acc_data *acc = dev_get_drvdata(dev);

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);

	device_destroy(acc->acc_class, 0);
	class_destroy(acc->acc_class);

	return 0;
}

int lsm330_acc_input_open(struct input_dev *input)
{
	

	return 0;
}

void lsm330_acc_input_close(struct input_dev *dev)
{
	

	
}

static int lsm330_acc_validate_pdata(struct lsm330_acc_data *acc)
{
	acc->pdata->poll_interval = max(acc->pdata->poll_interval,
			acc->pdata->min_interval);

	if ((acc->pdata->rot_matrix_index >= ARRAY_SIZE(rot_matrix)) ||
				(acc->pdata->rot_matrix_index < 0)) {
		dev_err(&acc->client->dev, "rotation matrix index invalid.\n");
		return -EINVAL;
	}

	
	if (acc->pdata->poll_interval < acc->pdata->min_interval) {
		dev_err(&acc->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}
	memcpy(acc->rot_matrix,
			rot_matrix[acc->pdata->rot_matrix_index].matrix,
							9 * sizeof(short));
	return 0;
}

static int lsm330_acc_input_init(struct lsm330_acc_data *acc)
{
	int err;

	acc->input_dev = input_allocate_device();
	if (!acc->input_dev) {
		err = -ENOMEM;
		dev_err(&acc->client->dev, "input device allocation failed\n");
		goto err0;
	}

	acc->input_dev->open = lsm330_acc_input_open;
	acc->input_dev->close = lsm330_acc_input_close;
	acc->input_dev->name = LSM330_ACC_DEV_NAME;

	acc->input_dev->id.bustype = BUS_I2C;
	acc->input_dev->dev.parent = &acc->client->dev;

	input_set_drvdata(acc->input_dev, acc);

	set_bit(EV_ABS, acc->input_dev->evbit);
	
	set_bit(ABS_MISC, acc->input_dev->absbit);
	
	set_bit(ABS_WHEEL, acc->input_dev->absbit);

	input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, 0, 0);
	input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, 0, 0);
	input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, 0, 0);
	
	set_bit(ABS_MISC, acc->input_dev->absbit);
	input_set_abs_params(acc->input_dev, ABS_MISC, INT_MIN, INT_MAX, 0, 0);
	
#if SIGNIFICANT_MOTION > 0
	set_bit(ABS_SIGN_MOTION, acc->input_dev->absbit);
	input_set_abs_params(acc->input_dev, ABS_SIGN_MOTION, 0, 1, 0, 0);
#endif


	err = input_register_device(acc->input_dev);
	if (err) {
		dev_err(&acc->client->dev,
				"unable to register input device %s\n",
				acc->input_dev->name);
		goto err1;
	}

#ifdef CONFIG_CIR_ALWAYS_READY
	acc->input_cir = input_allocate_device();
	if (!acc->input_cir) {
	    goto err1;
	}
	acc->input_cir->name = "CIRSensor";
	acc->input_cir->id.bustype = BUS_I2C;

	input_set_capability(acc->input_cir, EV_REL, ANY_MOTION_INTERRUPT);
	err = input_register_device(acc->input_cir);
	if (err) {
	    goto err_register_input_cir_device;
	}
	wake_lock_init(&(acc->cir_always_ready_wake_lock), WAKE_LOCK_SUSPEND, "cir_always_ready");
#endif

#if SIGNIFICANT_MOTION > 0
	wake_lock_init(&(acc->sig_mot_wake_lock), WAKE_LOCK_SUSPEND, "significant_motion");
#endif
	return 0;

#ifdef CONFIG_CIR_ALWAYS_READY
err_register_input_cir_device:
	input_unregister_device(acc->input_dev);
	input_free_device(acc->input_cir);
#endif
err1:
	input_free_device(acc->input_dev);
err0:
	return err;
}

static void lsm330_acc_input_cleanup(struct lsm330_acc_data *acc)
{
	input_unregister_device(acc->input_dev);
	input_free_device(acc->input_dev);
}

static void poll_function_work_acc(struct work_struct *input_work_acc)
{
	struct lsm330_acc_data *acc;
	int xyz[3] = { 0 };
	int err;
	acc = container_of((struct work_struct *)input_work_acc,
					struct lsm330_acc_data, input_work_acc);

	err = lsm330_acc_get_data(acc, xyz);
	if (err < 0)
		dev_err(&acc->client->dev, "get_accelerometer_data failed\n");
	else
		lsm330_acc_report_values(acc, xyz);

	
	if (acc->num_samples <= NUMBER_DISCARD_SAMPLES)
		acc->num_samples++;

	lsm330_acc_polling_manage(acc);
}

enum hrtimer_restart poll_function_read_acc(struct hrtimer *timer)
{
	struct lsm330_acc_data *acc;

	acc = container_of((struct hrtimer *)timer,
					struct lsm330_acc_data, hr_timer_acc);

	queue_work(lsm330_workqueue, &acc->input_work_acc);
	return HRTIMER_NORESTART;
}

static int lsm330_acc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	struct lsm330_acc_data *acc;
	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK ;

	int err = -1;

#ifdef HTC_DTAP
       struct input_dev *dev_dtap;

#endif
	dev_info(&client->dev, "probe start.\n");

	acc = kzalloc(sizeof(struct lsm330_acc_data), GFP_KERNEL);
	if (acc == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
					"%d\n", err);
		goto exit_check_functionality_failed;
	}

	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)){
			acc->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			acc->use_smbus = 0;
			goto exit_check_functionality_failed;
		}
	} else {
		acc->use_smbus = 0;
	}

	if(lsm330_workqueue == 0)
			lsm330_workqueue = create_workqueue("lsm330_workqueue");

	hrtimer_init(&acc->hr_timer_acc, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	acc->hr_timer_acc.function = &poll_function_read_acc;

	mutex_init(&acc->lock);
	mutex_lock(&acc->lock);

	acc->client = client;
	i2c_set_clientdata(client, acc);

	acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
	if (acc->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n",
				err);
		goto err_mutexunlock;
	}

	if(client->dev.platform_data == NULL) {
		default_lsm330_acc_pdata.gpio_int1 = int1_gpio;
		default_lsm330_acc_pdata.gpio_int2 = int2_gpio;
		memcpy(acc->pdata, &default_lsm330_acc_pdata,
							sizeof(*acc->pdata));
		dev_info(&client->dev, "using default platform_data\n");
	} else {
		memcpy(acc->pdata, client->dev.platform_data,
							sizeof(*acc->pdata));
	}

	calibration_version = gyro_gsensor_kvalue[36];

	if (acc->pdata) {
                acc->chip_layout = acc->pdata->chip_layout;
		D("gs_kvalue:%x", gs_kvalue);
                acc->pdata->gs_kvalue = gs_kvalue;
        } else {
                acc->chip_layout = 0;
                acc->pdata->gs_kvalue = 0;
        }
        I("LSM330 G-sensor I2C driver: gs_kvalue = 0x%X\n",
                acc->pdata->gs_kvalue);

        acc_data = acc;

        D("%s: layout = %d\n", __func__, acc_data->chip_layout);

	err = lsm330_acc_validate_pdata(acc);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}

	if (acc->pdata->init) {
		err = acc->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err_pdata_init;
		}
	}

	if(acc->pdata->gpio_int1 >= 0){
		acc->irq1 = gpio_to_irq(acc->pdata->gpio_int1);
		I("%s: %s has set irq1 to irq: %d "
							"mapped on gpio:%d\n",
			LSM330_ACC_DEV_NAME, __func__, acc->irq1,
							acc->pdata->gpio_int1);
	}

	if(acc->pdata->gpio_int2 >= 0){
		acc->irq2 = gpio_to_irq(acc->pdata->gpio_int2);
		I("%s: %s has set irq2 to irq: %d "
							"mapped on gpio:%d\n",
			LSM330_ACC_DEV_NAME, __func__, acc->irq2,
							acc->pdata->gpio_int2);
	}

	
	memset(acc->resume_state, 0, ARRAY_SIZE(acc->resume_state));
	lsm330_acc_set_init_register_values(acc);
	
	lsm330_acc_set_init_statepr1_param(acc);
	lsm330_acc_set_init_statepr1_inst(acc);
	
	lsm330_acc_set_init_statepr2_param(acc);
	lsm330_acc_set_init_statepr2_inst(acc);

	err = lsm330_acc_device_power_on(acc);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err_pdata_init;
	}
	acc->enable_polling = 1;
	atomic_set(&acc->enabled, 1);

	err = lsm330_acc_update_fs_range(acc, acc->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range failed\n");
		goto  err_power_off;
	}

	err = lsm330_acc_update_odr(acc, acc->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto  err_power_off;
	}

	err = lsm330_acc_input_init(acc);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}
#ifdef HTC_DTAP
	dev_dtap = input_allocate_device();
	if (!dev_dtap) {
		kfree(acc);
		input_free_device(dev_dtap);
		return -ENOMEM;
	}

#endif

#ifdef HTC_DTAP
	dev_dtap->name = "DTAPPSensor";
	dev_dtap->id.bustype = BUS_I2C;
	set_bit(EV_KEY, dev_dtap->evbit);
	set_bit(EV_REL, dev_dtap->evbit);
	set_bit(EV_REL, dev_dtap->evbit);
	input_set_capability(dev_dtap, EV_REL, REL_X);
	input_set_capability(dev_dtap, EV_REL, REL_Y);
	input_set_capability(dev_dtap, EV_KEY, BTN_LEFT);
	input_set_capability(dev_dtap, EV_KEY, BTN_RIGHT);
	input_set_capability(dev_dtap, EV_KEY, BTN_MIDDLE);
	input_set_capability(dev_dtap, EV_KEY, KEY_ENTER);

	input_set_capability(dev_dtap, EV_REL, REL_WHEEL);
	input_set_capability(dev_dtap, EV_REL, REL_HWHEEL);
	input_set_drvdata(dev_dtap, acc);

#endif


#ifdef HTC_DTAP
	err = input_register_device(dev_dtap);
	if (err) {
		dev_err(&acc->client->dev,
                                "unable to register input device %s\n",
                                acc->input_dtap->name);
                goto err_register_dtap_fail;
        }

	acc->input_dtap = dev_dtap;
#endif

	err = create_sysfs_interfaces(acc);
	if (err < 0) {
		dev_err(&client->dev,
		   "device LSM330_ACC_DEV_NAME sysfs register failed\n");
		goto err_input_cleanup;
	}

#ifdef CUSTOM_SYSFS_PATH
	dev_set_drvdata(acc->acc_dev, acc);
#endif
	lsm330_acc_device_power_off(acc);

	
	atomic_set(&acc->enabled, 0);

#if SIGNIFICANT_MOTION > 0
	atomic_set(&acc->sign_mot_enabled, 0);
#endif
	if(acc->pdata->gpio_int1 >= 0){
		INIT_WORK(&acc->irq1_work, lsm330_acc_irq1_work_func);
		acc->irq1_work_queue =
			create_singlethread_workqueue("lsm330_acc_wq1");
		if (!acc->irq1_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue1: %d\n", err);
			goto err_remove_sysfs_int;
		}
		err = request_irq(acc->irq1, lsm330_acc_isr1,
			IRQF_TRIGGER_RISING, "lsm330_acc_irq1", acc);
		enable_irq_wake(acc->irq1);
		if (err < 0) {
			dev_err(&client->dev, "request irq1 failed: %d\n", err);
			goto err_destoyworkqueue1;
		}
	}

	if(acc->pdata->gpio_int2 >= 0){
		INIT_WORK(&acc->irq2_work, lsm330_acc_irq2_work_func);
		acc->irq2_work_queue =
			create_singlethread_workqueue("lsm330_acc_wq2");
		if (!acc->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue2: %d\n", err);
			goto err_free_irq1;
		}
		err = request_irq(acc->irq2, lsm330_acc_isr2,
				IRQF_TRIGGER_RISING, "lsm330_acc_irq2", acc);
		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err_destoyworkqueue2;
		}
		disable_irq_nosync(acc->irq2);
	}

	INIT_WORK(&acc->input_work_acc, poll_function_work_acc);

	mutex_unlock(&acc->lock);

	acc->normal_odr = 0;

	dev_info(&client->dev, "%s: probed\n", LSM330_ACC_DEV_NAME);

	return 0;

err_destoyworkqueue2:
	if(acc->pdata->gpio_int2 >= 0)
		destroy_workqueue(acc->irq2_work_queue);
err_free_irq1:
	free_irq(acc->irq1, acc);
err_destoyworkqueue1:
	if(acc->pdata->gpio_int1 >= 0)
		destroy_workqueue(acc->irq1_work_queue);
err_remove_sysfs_int:
	remove_sysfs_interfaces(&client->dev);
#ifdef HTC_DTAP
err_register_dtap_fail:
        input_free_device(acc->input_dtap);
#endif
err_input_cleanup:
	lsm330_acc_input_cleanup(acc);
err_power_off:
	lsm330_acc_device_power_off(acc);
err_pdata_init:
	if (acc->pdata->exit)
		acc->pdata->exit();
exit_kfree_pdata:
	kfree(acc->pdata);
err_mutexunlock:
	mutex_unlock(&acc->lock);
	if(lsm330_workqueue) {
			flush_workqueue(lsm330_workqueue);
			destroy_workqueue(lsm330_workqueue);
	}
	kfree(acc);
exit_check_functionality_failed:
	E("%s: Driver Init failed\n", LSM330_ACC_DEV_NAME);
	return err;
}

static int __devexit lsm330_acc_remove(struct i2c_client *client)
{
	struct lsm330_acc_data *acc = i2c_get_clientdata(client);

	if(acc->pdata->gpio_int1 >= 0){
		free_irq(acc->irq1, acc);
		gpio_free(acc->pdata->gpio_int1);
		destroy_workqueue(acc->irq1_work_queue);
	}

	if(acc->pdata->gpio_int2 >= 0){
		free_irq(acc->irq2, acc);
		gpio_free(acc->pdata->gpio_int2);
		destroy_workqueue(acc->irq2_work_queue);
	}

	lsm330_acc_device_power_off(acc);
	lsm330_acc_input_cleanup(acc);
	remove_sysfs_interfaces(&client->dev);

	if (acc->pdata->exit)
		acc->pdata->exit();

	if(lsm330_workqueue) {
			flush_workqueue(lsm330_workqueue);
			destroy_workqueue(lsm330_workqueue);
	}

	kfree(acc->pdata);
	kfree(acc);

	return 0;
}

#ifdef CONFIG_PM
static int lsm330_acc_resume(struct i2c_client *client)
{
	int err = 0;
	struct lsm330_acc_data *acc = i2c_get_clientdata(client);

	if (acc->on_before_suspend) {
		acc->enable_polling = 1;
		I("%s: enable_polling = %d\n", __func__, acc->enable_polling);
		err = lsm330_acc_enable(acc);
	}

	return err;
}

static int lsm330_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
#if SIGNIFICANT_MOTION > 0
	int err = 0;
#endif
	struct lsm330_acc_data *acc = i2c_get_clientdata(client);

	acc->on_before_suspend = atomic_read(&acc->enabled);

#if SIGNIFICANT_MOTION > 0
	if (!atomic_read(&acc->sign_mot_enabled)) {
		err = lsm330_acc_disable(acc);
	} else {
		if (acc->on_before_suspend) {
			acc->enable_polling = 0;
			I("%s: on_before_suspend: enable_polling = %d\n", __func__, acc->enable_polling);
			lsm330_acc_polling_manage(acc);
		}
	}
#else
	err = lsm330_acc_disable(acc);
#endif
	return err;
}
#else 
#define lsm330_acc_suspend	NULL
#define lsm330_acc_resume	NULL
#endif 

static const struct i2c_device_id lsm330_acc_id[]
		= { { LSM330_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, lsm330_acc_id);

static struct i2c_driver lsm330_acc_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = LSM330_ACC_DEV_NAME,
		  },
	.probe = lsm330_acc_probe,
	.remove = __devexit_p(lsm330_acc_remove),
	.suspend = lsm330_acc_suspend,
	.resume = lsm330_acc_resume,
	.id_table = lsm330_acc_id,
};

static int __init lsm330_acc_init(void)
{
	I("%s accelerometer driver: init\n", LSM330_ACC_DEV_NAME);
	return i2c_add_driver(&lsm330_acc_driver);
}

static void __exit lsm330_acc_exit(void)
{
	I("%s accelerometer driver exit\n", LSM330_ACC_DEV_NAME);
	i2c_del_driver(&lsm330_acc_driver);
	return;
}

module_init(lsm330_acc_init);
module_exit(lsm330_acc_exit);

MODULE_DESCRIPTION("lsm330 accelerometer driver");
MODULE_AUTHOR("Matteo Dameno, Denis Ciocca, STMicroelectronics");
MODULE_LICENSE("GPL");

