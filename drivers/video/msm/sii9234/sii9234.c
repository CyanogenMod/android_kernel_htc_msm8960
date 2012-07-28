/* drivers/i2c/chips/sii9234.c - sii9234 optical sensors driver
 *
 * Copyright (C) 2010 HTC, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*********************************************************************
*  Inculde
**********************************************************************/
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <asm/setup.h>
#include <linux/wakelock.h>
#include <mach/mhl.h>
#include <mach/debug_display.h>
#include <mach/board.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <mach/cable_detect.h>

#include "sii9234.h"
#include "TPI.h"
#include "mhl_defs.h"


/*********************************************************************
  Define & Macro
***********************************************************************/
#define SII9234_I2C_RETRY_COUNT 2
/*********************************************************************
  Type Definitions
***********************************************************************/
typedef struct {
	struct i2c_client *i2c_client;
	struct workqueue_struct *wq;
	struct wake_lock wake_lock;
	int (*pwrCtrl)(int); /* power to the chip */
	void (*mhl_usb_switch)(int);
	void (*mhl_1v2_power)(bool enable);
	int  (*mhl_power_vote)(bool enable);
	int (*enable_5v)(int on);
	struct delayed_work init_delay_work;
	struct delayed_work init_complete_work;
	struct delayed_work irq_timeout_work;
	struct delayed_work mhl_on_delay_work;
	struct delayed_work turn_off_5v;
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
	struct delayed_work detect_charger_work;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int reset_pin;
	int intr_pin;
	int ci2ca_pin;
	int irq;
	bool isMHL;
	enum usb_connect_type statMHL;
	struct work_struct mhl_notifier_work;
	mhl_board_params board_params;
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
	struct input_dev *input_dev;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_width_min;
	int abs_width_max;
	uint16_t finger_count;
	T_MHL_REMOTE_FINGER_DATA *finger_data;
	T_MHL_REMOTE_KEY_DATA *key_data;
#endif
} T_MHL_SII9234_INFO;

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
/*
 * For now there is no touch, width, and pressure data sent from TV.
 * Thus we use fixed value here.
 */
#define DEMOTV_DEFAULT_WIDTH 50/*20*/
#define DEMOTV_DEFAULT_TOUCH 5/*15*/
#define DEMOTV_DEFAULT_PRESSURE 5/*15*/
#endif

/*********************************************************************
   Variable & Extern variable
**********************************************************************/
static T_MHL_SII9234_INFO *sii9234_info_ptr;
/*********************************************************************
  Prototype & Extern function
**********************************************************************/
static void sii9234_irq_do_work(struct work_struct *work);
static DECLARE_WORK(sii9234_irq_work, sii9234_irq_do_work);

static DEFINE_MUTEX(mhl_early_suspend_sem);
unsigned long suspend_jiffies;
unsigned long irq_jiffies;
bool g_bEnterEarlySuspend = false;
bool g_bProbe = false;
bool mhl_wakeuped = false;
static bool g_bInitCompleted = false;
static bool sii9244_interruptable = false;
static bool need_simulate_cable_out = false;
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
static bool g_bPollDetect = false;
#endif
static struct dentry *dbg_entry_dir, *dbg_entry_a3, *dbg_entry_dbg_on;
u8 dbg_drv_str_a3 = 0xEB, dbg_drv_str_on = 0;

#define MHL_RCP_KEYEVENT
#define MHL_ISR_TIMEOUT 5

#ifdef MHL_RCP_KEYEVENT
struct input_dev *input_dev;
#endif
static struct platform_device *mhl_dev; /* Device structure */

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
static T_MHL_REMOTE_FINGER_DATA g_mhl_finger_data[2];
static T_MHL_REMOTE_KEY_DATA g_mhl_key_data;
static bool g_back_key_pressed = false;
static bool g_touch_pressed[MHL_SII9234_TOUCH_FINGER_NUM_MAX] = {false};
static void Mhl_Proc_Remote_Event(T_MHL_SII9234_INFO *pInfo);
static void Mhl_Proc_Reset_Key_Status(void);
#endif
/*********************************************************************
	Functions
**********************************************************************/
#ifdef CONFIG_CABLE_DETECT_ACCESSORY
static DEFINE_MUTEX(mhl_notify_sem);

#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
static void detect_charger_handler(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = container_of(
			w, T_MHL_SII9234_INFO, detect_charger_work.work);

	mutex_lock(&mhl_early_suspend_sem);

	PR_DISP_DEBUG("%s: query status every 2 second\n", __func__);
	SiiMhlTxReadDevcap(0x02);

	mutex_unlock(&mhl_early_suspend_sem);

	queue_delayed_work(pInfo->wq, &pInfo->detect_charger_work, HZ*2);
}
#endif
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
void check_mhl_5v_status(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	/*
		for the case of (1)plug dongle +AC +HDMI  (2)remove HDMI (3) plug HDMI,
		we should turn off internal 5v in this case step3
	*/
	if(pInfo->isMHL && (pInfo->statMHL == CONNECT_TYPE_AC || pInfo->statMHL == CONNECT_TYPE_USB )){
		if(pInfo->enable_5v)
			pInfo->enable_5v(0);
	}
}
#endif
void update_mhl_status(bool isMHL, enum usb_connect_type statMHL)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	PR_DISP_DEBUG("%s: -+-+-+-+- MHL is %sconnected, status = %d -+-+-+-+-\n",
		__func__, isMHL?"":"NOT ", statMHL);
	pInfo->isMHL = isMHL;
	pInfo->statMHL = statMHL;

	queue_work(pInfo->wq, &pInfo->mhl_notifier_work);

#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
	if (isMHL && (statMHL == CONNECT_TYPE_INTERNAL)) {
		if (!g_bPollDetect) {
			g_bPollDetect = true;
			queue_delayed_work(pInfo->wq, &pInfo->detect_charger_work, HZ/2);
			cancel_delayed_work(&pInfo->turn_off_5v);
		}
	} else if (statMHL == CONNECT_TYPE_AC || statMHL == CONNECT_TYPE_USB) {
		cancel_delayed_work(&pInfo->detect_charger_work);
		g_bPollDetect = false;
		/*disable boost 5v after 1 second*/
		queue_delayed_work(pInfo->wq, &pInfo->turn_off_5v, HZ);
	}
	else {
		g_bPollDetect = false;
		cancel_delayed_work(&pInfo->detect_charger_work);
	}
#endif

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
	Mhl_Proc_Reset_Key_Status();
#endif

}

static void send_mhl_connect_notify(struct work_struct *w)
{
	static struct t_mhl_status_notifier *mhl_notifier;
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	if (!pInfo)
		return;

	PR_DISP_DEBUG("%s: %d\n", __func__, pInfo->isMHL);
	mutex_lock(&mhl_notify_sem);
	list_for_each_entry(mhl_notifier,
		&g_lh_mhl_detect_notifier_list,
		mhl_notifier_link) {
			if (mhl_notifier->func != NULL)
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
				mhl_notifier->func(pInfo->isMHL, pInfo->statMHL);
#else
				mhl_notifier->func(pInfo->isMHL, false);
#endif
		}
	mutex_unlock(&mhl_notify_sem);
}

int mhl_detect_register_notifier(struct t_mhl_status_notifier *notifier)
{
	if (!notifier || !notifier->name || !notifier->func)
		return -EINVAL;

	mutex_lock(&mhl_notify_sem);
	list_add(&notifier->mhl_notifier_link,
		&g_lh_mhl_detect_notifier_list);
	mutex_unlock(&mhl_notify_sem);
	return 0;
}
#endif
int sii9234_I2C_RxData(uint8_t deviceID, char *rxData, uint32_t length)
{
	uint8_t loop_i;
	uint8_t slave_addr = deviceID >> 1;
	struct i2c_msg msgs[] = {
		{
		 .addr = slave_addr,
		 .flags = 0,
		 .len = 1,
		 .buf = rxData,
		 },
		{
		 .addr = slave_addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	sii9234_info_ptr->i2c_client->addr = slave_addr;
	for (loop_i = 0; loop_i < SII9234_I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(sii9234_info_ptr->i2c_client->adapter, msgs, 2) > 0)
			break;

		mdelay(10);
	}

	if (loop_i >= SII9234_I2C_RETRY_COUNT) {
		PR_DISP_DEBUG("%s retry over %d\n", __func__, SII9234_I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

int sii9234_I2C_TxData(uint8_t deviceID, char *txData, uint32_t length)
{
	uint8_t loop_i;
	uint8_t slave_addr = deviceID >> 1;
	struct i2c_msg msg[] = {
		{
		 .addr = slave_addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	sii9234_info_ptr->i2c_client->addr = slave_addr;
	for (loop_i = 0; loop_i < SII9234_I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(sii9234_info_ptr->i2c_client->adapter, msg, 1) > 0)
			break;

		mdelay(10);
	}

	if (loop_i >= SII9234_I2C_RETRY_COUNT) {
		PR_DISP_DEBUG("%s retry over %d\n", __func__, SII9234_I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

int sii9234_get_intr_status(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	return gpio_get_value(pInfo->intr_pin);
}

void sii9234_reset(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	gpio_set_value(pInfo->reset_pin, 0);
	mdelay(2);
	gpio_set_value(pInfo->reset_pin, 1);
}

int sii9234_get_ci2ca(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	return pInfo->ci2ca_pin;
}

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
static void Mhl_Proc_Remote_Event(T_MHL_SII9234_INFO *pInfo)
{
	uint16_t evt_type = E_MHL_EVT_UNKNOWN;
	int remote_evt_en = E_MHL_FALSE;
	uint8_t i;
	int x, y, z;
	static int prev_x[MHL_SII9234_TOUCH_FINGER_NUM_MAX] = { -1 };
	static int prev_y[MHL_SII9234_TOUCH_FINGER_NUM_MAX] = { -1 };

	remote_evt_en = Tpi_query_remote_type(&evt_type);
	if (remote_evt_en == E_MHL_FALSE)
		return;

	Tpi_reset_remote_intr();
	switch (evt_type) {
	case E_MHL_EVT_REMOTE_KEY:
		pInfo->key_data = &g_mhl_key_data;
		Tpi_query_remote_keyInfo(pInfo->key_data);
		if (pInfo->key_data->keyCode == KEY_BACK)
			g_back_key_pressed = true;
		input_report_key(pInfo->input_dev, pInfo->key_data->keyCode, 1);
		input_sync(pInfo->input_dev);
		break;
	case E_MHL_EVT_REMOTE_KEY_RELEASE:
		pInfo->key_data = &g_mhl_key_data;
		Tpi_query_remote_keyInfo(pInfo->key_data);
		if (pInfo->key_data->keyCode == KEY_BACK)
			g_back_key_pressed = false;
		input_report_key(pInfo->input_dev, pInfo->key_data->keyCode, 0);
		input_sync(pInfo->input_dev);
		break;
	case E_MHL_EVT_REMOTE_TOUCH:
		pInfo->finger_count = 0;
		pInfo->finger_data = g_mhl_finger_data;
		Tpi_query_remote_touchInfo(&(pInfo)->finger_count, pInfo->finger_data);
		for (i = 0; (i < pInfo->finger_count) && (i < MHL_SII9234_TOUCH_FINGER_NUM_MAX); i++) {
			z = pInfo->finger_data[i].z;
			if (z == E_MHL_EVT_REMOTE_TOUCH_PRESS) {
				g_touch_pressed[i] = true;
				x = pInfo->finger_data[i].x;
				y = pInfo->finger_data[i].y;
				/* don't send the same position */
				if (x != prev_x[i] || y != prev_y[i]) {
					prev_x[i] = x;
					prev_y[i] = y;
					input_report_abs(pInfo->input_dev, ABS_MT_PRESSURE, DEMOTV_DEFAULT_PRESSURE);
					input_report_abs(pInfo->input_dev, ABS_MT_TOUCH_MAJOR, DEMOTV_DEFAULT_TOUCH);
					input_report_abs(pInfo->input_dev, ABS_MT_WIDTH_MAJOR, DEMOTV_DEFAULT_WIDTH);
					input_report_abs(pInfo->input_dev, ABS_MT_POSITION_X, x);
					input_report_abs(pInfo->input_dev, ABS_MT_POSITION_Y, y);
					input_mt_sync(pInfo->input_dev);
				}
			} else if (z == E_MHL_EVT_REMOTE_TOUCH_RELEASE) {
				g_touch_pressed[i] = false;
				prev_x[i] = -1;
				prev_y[i] = -1;
				input_mt_sync(pInfo->input_dev);
			}
		}
		/* Type-A: after all the fingers are processed */
		input_sync(pInfo->input_dev);
		break;
	default:
		M_MHL_SEND_DEBUG("[Err]: MHL ISR invalid event = %d\r\n", evt_type);
		break;
	}

	/* add this sleep to prevent interrupt flood */
	hr_msleep(1);
}

void Mhl_Proc_Reset_Key_Status(void)
{
	int i;
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	if (g_back_key_pressed) {
		PR_DISP_INFO("MHL disconnected. Released back key from TV!\n");
		g_back_key_pressed = false;
		input_report_key(pInfo->input_dev, KEY_BACK, 0);
		input_sync(pInfo->input_dev);
	}

	for (i = 0; i < MHL_SII9234_TOUCH_FINGER_NUM_MAX; i++) {
		if (g_touch_pressed[i]) {
			PR_DISP_INFO("MHL disconnected. Released finger %d from TV!\n", i);
			g_touch_pressed[i] = false;
			input_report_abs(pInfo->input_dev, ABS_MT_AMPLITUDE, 0);
			input_report_abs(pInfo->input_dev, ABS_MT_POSITION, 1 << 31);
		}
	}

}
#endif

static void sii9234_irq_do_work(struct work_struct *work)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	mutex_lock(&mhl_early_suspend_sem);
	if (!g_bEnterEarlySuspend) {
		uint8_t		event;
		uint8_t		eventParameter;
		if(time_after(jiffies, irq_jiffies + HZ/20))
		{
			irq_jiffies = jiffies;
			/*PR_DISP_DEBUG("MHL ISR\n");*/
			need_simulate_cable_out = false;
			cancel_delayed_work(&pInfo->irq_timeout_work);
			SiiMhlTxGetEvents(&event, &eventParameter);
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
			Mhl_Proc_Remote_Event(pInfo);
#endif
			ProcessRcp(event, eventParameter);
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);

	enable_irq(pInfo->irq);
}

void sii9234_disableIRQ(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	cancel_work_sync(&sii9234_irq_work);
	if (sii9244_interruptable) {
		PR_DISP_DEBUG("%s\n", __func__);
		disable_irq_nosync(pInfo->irq);
		sii9244_interruptable = false;
	}
}

int sii9234_power_vote(bool enable)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (pInfo->mhl_power_vote) {
		if (enable)
			pInfo->mhl_power_vote(1);
		else
			pInfo->mhl_power_vote(0);
	}
	return 0;
}

static irqreturn_t sii9234_irq_handler(int irq, void *data)
{
	T_MHL_SII9234_INFO *pInfo = data;

	if (pInfo->wq) {
		disable_irq_nosync(pInfo->irq);
		queue_work(pInfo->wq, &sii9234_irq_work);
	} else
		PR_DISP_DEBUG("%s: workqueue is not ready yet.", __func__);

	return IRQ_HANDLED;
}

void sii9234_send_keyevent(uint32_t key, uint32_t type)
{
#ifdef MHL_RCP_KEYEVENT
	PR_DISP_DEBUG("CBUS key_event: %d\n", key);
	if (type == 0) {
		input_report_key(input_dev, key, 1);
		input_report_key(input_dev, key, 0);
		input_sync(input_dev);
	}
#endif
}

#ifdef MHL_RCP_KEYEVENT
/* Sysfs method to input simulated coordinates */
static ssize_t write_keyevent(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t count)
{
	int key;

	/* parsing input data */
	sscanf(buffer, "%d", &key);


	PR_DISP_DEBUG("key_event: %d\n", key);

	/* Report key event */
	switch (key) {
	case 0:
		input_report_key(input_dev, KEY_HOME, 1);
		input_report_key(input_dev, KEY_HOME, 0);
		break;
	case 1:
		input_report_key(input_dev, KEY_UP, 1);
		input_report_key(input_dev, KEY_UP, 0);
		break;
	case 2:
		input_report_key(input_dev, KEY_DOWN, 1);
		input_report_key(input_dev, KEY_DOWN, 0);
		break;
	case 3:
		input_report_key(input_dev, KEY_LEFT, 1);
		input_report_key(input_dev, KEY_LEFT, 0);
		break;
	case 4:
		input_report_key(input_dev, KEY_RIGHT, 1);
		input_report_key(input_dev, KEY_RIGHT, 0);
		break;
	case 5:
		input_report_key(input_dev, KEY_ENTER, 1);
		input_report_key(input_dev, KEY_ENTER, 0);
		break;
	case 6:
		input_report_key(input_dev, KEY_SELECT, 1);
		input_report_key(input_dev, KEY_SELECT, 0);
		break;
	default:
		input_report_key(input_dev, KEY_OK, 1);
		input_report_key(input_dev, KEY_OK, 0);
		break;
	}
	input_sync(input_dev);
	return count;
}

/* Attach the sysfs write method */
static DEVICE_ATTR(rcp_event, 0644, NULL, write_keyevent);
#endif

void sii9234_mhl_device_wakeup(void)
{
	int err;
	int ret = 0 ;

	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	PR_DISP_INFO("sii9234_mhl_device_wakeup()\n");

	if (!g_bInitCompleted) {
		PR_DISP_INFO("MHL inserted before HDMI related function was ready! Wait more 5 sec...\n");
		queue_delayed_work(pInfo->wq, &pInfo->init_delay_work, HZ*5);
		return;
	}

	/* MHL_USB_SW for Verdi  0: switch to MHL;  others projects, 1 : switch to MHL */
	if (pInfo->mhl_usb_switch)
		pInfo->mhl_usb_switch(1);

	gpio_set_value(pInfo->reset_pin, 1); /* Reset High */

	if (pInfo->mhl_1v2_power)
		pInfo->mhl_1v2_power(1);

	err = TPI_Init();
	if (err != 1)
		PR_DISP_INFO("TPI can't init\n");

	sii9244_interruptable = true;
	PR_DISP_INFO("Enable Sii9244 IRQ\n");

	/*request irq pin again, for solving MHL_INT is captured to INUT LOW*/
	if(mhl_wakeuped) {
		disable_irq_nosync(pInfo->irq);
		free_irq(pInfo->irq, pInfo);

		ret = request_irq(pInfo->irq, sii9234_irq_handler, IRQF_TRIGGER_LOW, "mhl_sii9234_evt", pInfo);
		if (ret < 0) {
			PR_DISP_DEBUG("%s: request_irq(%d) failed for gpio %d (%d)\n",
				__func__, pInfo->irq, pInfo->intr_pin, ret);
			ret = -EIO;
		}
	}

	enable_irq(pInfo->irq);
	mhl_wakeuped = true;

	/*switch to D0, we now depends on Sii9244 to detect the connection by MHL interrupt*/
	/*if there is no IRQ in the following steps , the status of connect will be in-correct and cannot be recovered*/
	/*add a mechanism to simulate cable out to prevent this case.*/
	need_simulate_cable_out = true;
	queue_delayed_work(pInfo->wq, &pInfo->irq_timeout_work, HZ * MHL_ISR_TIMEOUT);

}

static void init_delay_handler(struct work_struct *w)
{
	PR_DISP_INFO("init_delay_handler()\n");

	update_mhl_status(false, CONNECT_TYPE_UNKNOWN);
}

static void init_complete_handler(struct work_struct *w)
{
	PR_DISP_INFO("init_complete_handler()\n");
	/*make sure the MHL is in sleep *& usb_bypass mode*/
	TPI_Init();
	g_bInitCompleted = true;
}
static void irq_timeout_handler(struct work_struct *w)
{
	if(need_simulate_cable_out) {
		T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
		int ret = 0 ;
		/*need to request_irq again on 8960 VLE, or this prevention is not working*/
		PR_DISP_INFO("%s , There is no MHL ISR simulate cable out.\n", __func__);
		disable_irq_nosync(pInfo->irq);
		TPI_Init();
		free_irq(pInfo->irq, pInfo);
		ret = request_irq(pInfo->irq, sii9234_irq_handler, IRQF_TRIGGER_LOW, "mhl_sii9234_evt", pInfo);
		if (ret < 0) {
			PR_DISP_DEBUG("%s: request_irq(%d) failed for gpio %d (%d)\n",
				__func__, pInfo->irq, pInfo->intr_pin, ret);
			ret = -EIO;
		}
		enable_irq(pInfo->irq);
		update_mhl_status(false, CONNECT_TYPE_UNKNOWN);
	}
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static int sii9234_suspend(struct i2c_client *client, pm_message_t mesg)
{
	if (Status_Query() != POWER_STATE_D3)
		SiiMhlTxDrvTmdsControl(false);
	return 0;
}

static void sii9234_EnableTMDS(void)
{
	if (Status_Query() == POWER_STATE_D0_MHL)
		SiiMhlTxDrvTmdsControl(true);
}

#if 0
static int sii9234_resume(struct i2c_client *client)
{
	if (Status_Query() != POWER_STATE_D3)
		SiiMhlTxDrvTmdsControl(true);
	return 0;
}
#endif

void sii9234_change_usb_owner(bool bMHL)
{

}

static void sii9234_early_suspend(struct early_suspend *h)
{
	T_MHL_SII9234_INFO *pInfo;
	pInfo = container_of(h, T_MHL_SII9234_INFO, early_suspend);

	PR_DISP_DEBUG("%s(isMHL=%d)\n", __func__, pInfo->isMHL);

	mutex_lock(&mhl_early_suspend_sem);
	/* Enter the early suspend state...*/
	g_bEnterEarlySuspend = true;
	suspend_jiffies = jiffies;

	/* Cancel the previous TMDS on delay work...*/
	cancel_delayed_work(&pInfo->mhl_on_delay_work);
	if (pInfo->isMHL) {
		/* Turn-off the TMDS output...*/
		sii9234_suspend(pInfo->i2c_client, PMSG_SUSPEND);
		/*For the case of dongle without HDMI cable*/

		if(!tpi_get_hpd_state()){
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
			cancel_delayed_work(&pInfo->detect_charger_work);
#endif
			sii9234_disableIRQ();

			if (pInfo->mhl_1v2_power)
				pInfo->mhl_1v2_power(0);
			/*follow suspend GPIO state,  disable hdmi HPD*/
			if (pInfo->mhl_usb_switch)
				pInfo->mhl_usb_switch(0);
			/*D3 mode with internal switch in by-pass mode*/
			TPI_Init();
		} else {
			if (pInfo->enable_5v)
				pInfo->enable_5v(0);
			if (pInfo->mhl_1v2_power)
				pInfo->mhl_1v2_power(0);
		}
	} else {
		/*in case cable_detect call D2ToD3(), make sure MHL chip is go Sleep mode correctly*/
		TPI_Init();
	}
	mutex_unlock(&mhl_early_suspend_sem);
}

static void sii9234_late_resume(struct early_suspend *h)
{
	T_MHL_SII9234_INFO *pInfo;
	pInfo = container_of(h, T_MHL_SII9234_INFO, early_suspend);

	PR_DISP_DEBUG("sii9234_late_resume()\n");

	mutex_lock(&mhl_early_suspend_sem);
	queue_delayed_work(pInfo->wq, &pInfo->mhl_on_delay_work, HZ);

	g_bEnterEarlySuspend = false;
	mutex_unlock(&mhl_early_suspend_sem);
}
static void mhl_turn_off_5v(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if(pInfo->enable_5v)
		pInfo->enable_5v(0);
}
static void mhl_on_delay_handler(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	mutex_lock(&mhl_early_suspend_sem);
	if (IsMHLConnection()) {
		/*have HDMI cable on dongle*/
		fill_black_screen();
		sii9234_EnableTMDS();

		if (pInfo->mhl_1v2_power)
			pInfo->mhl_1v2_power(1);

		PR_DISP_DEBUG("MHL has connected. No SimulateCableOut!!!\n");
		mutex_unlock(&mhl_early_suspend_sem);
		return;
	}
	else {
		if(pInfo->isMHL){
			/*MHL dongle plugged but no HDMI calbe*/
			PR_DISP_DEBUG("notify cable out, re-init cable & mhl\n");
			update_mhl_status(false, CONNECT_TYPE_UNKNOWN);
			TPI_Init();
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);
}
#endif


/*add debugfs for driving strength 0xA3*/
static int sii_debugfs_init(void)
{
	dbg_entry_dir = debugfs_create_dir("mhl_debugfs", NULL);
	if (!dbg_entry_dir) {
		PR_DISP_DEBUG("Fail to create debugfs dir: mhl_debugfs\n");
		return -1;
	}
	dbg_entry_a3 = debugfs_create_u8("strength", 0644, dbg_entry_dir, &dbg_drv_str_a3);
	if (!dbg_entry_a3)
		PR_DISP_DEBUG("Fail to create debugfs: strength\n");
	dbg_entry_dbg_on = debugfs_create_u8("dbg_on", 0644, dbg_entry_dir, &dbg_drv_str_on);
	if (!dbg_entry_dbg_on)
		PR_DISP_DEBUG("Fail to create debugfs: dbg_on\n");

	return 0;
}

static int sii9234_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = E_MHL_OK;
	bool rv = TRUE;
	T_MHL_SII9234_INFO *pInfo;
	T_MHL_PLATFORM_DATA *pdata;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		PR_DISP_DEBUG("%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	pInfo = kzalloc(sizeof(T_MHL_SII9234_INFO), GFP_KERNEL);
	if (!pInfo) {
		PR_DISP_DEBUG("%s: alloc memory error!!\n", __func__);
		return -ENOMEM;
	}
	pInfo->i2c_client = client;
	pdata = client->dev.platform_data;
	if (!pdata) {
		PR_DISP_DEBUG("%s: Assign platform_data error!!\n", __func__);
		ret = -EBUSY;
		goto err_platform_data_null;
	}
	pInfo->irq = client->irq;
	i2c_set_clientdata(client, pInfo);
	pInfo->reset_pin = pdata->gpio_reset;
	pInfo->intr_pin = pdata->gpio_intr;
	pInfo->ci2ca_pin = pdata->ci2ca;

	pInfo->pwrCtrl = pdata->power;
	pInfo->mhl_usb_switch = pdata->mhl_usb_switch;
	pInfo->mhl_1v2_power = pdata->mhl_1v2_power;
	pInfo->mhl_power_vote = pdata->mhl_power_vote;
	pInfo->enable_5v = pdata->enable_5v;
	sii9234_info_ptr = pInfo;
	/*making sure TPI_INIT() is working fine with pInfo*/
	g_bProbe = true;
	/* Power ON */
	if (pInfo->pwrCtrl)
		pInfo->pwrCtrl(1);
	if(board_build_flag() != SHIP_BUILD)
		sii_debugfs_init();

	/* Pin Config */
	gpio_request(pInfo->reset_pin, "mhl_sii9234_gpio_reset");
	gpio_direction_output(pInfo->reset_pin, 0);
	gpio_request(pInfo->intr_pin, "mhl_sii9234_gpio_intr");
	gpio_direction_input(pInfo->intr_pin);
	rv = TPI_Init();
	if (rv != TRUE) {
		PR_DISP_DEBUG("%s: can't init\n", __func__);
		ret = -ENOMEM;
		goto err_init;
	}

	INIT_DELAYED_WORK(&pInfo->init_delay_work, init_delay_handler);
	INIT_DELAYED_WORK(&pInfo->init_complete_work, init_complete_handler);
	INIT_DELAYED_WORK(&pInfo->irq_timeout_work, irq_timeout_handler);
	INIT_DELAYED_WORK(&pInfo->mhl_on_delay_work, mhl_on_delay_handler);
	INIT_DELAYED_WORK(&pInfo->turn_off_5v, mhl_turn_off_5v);

#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
	INIT_DELAYED_WORK(&pInfo->detect_charger_work, detect_charger_handler);
#endif
	irq_jiffies = jiffies;
	pInfo->irq = gpio_to_irq(pInfo->irq);
	ret = request_irq(pInfo->irq, sii9234_irq_handler, IRQF_TRIGGER_LOW, "mhl_sii9234_evt", pInfo);
	if (ret < 0) {
		PR_DISP_DEBUG("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, pInfo->irq, pInfo->intr_pin, ret);
		ret = -EIO;
		goto err_request_intr_pin;
	}
	disable_irq_nosync(pInfo->irq);
	sii9244_interruptable = false;

	pInfo->wq = create_workqueue("mhl_sii9234_wq");
	if (!pInfo->wq) {
		PR_DISP_DEBUG("%s: can't create workqueue\n", __func__);
		ret = -ENOMEM;
		goto err_create_workqueue;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	pInfo->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING + 1;
	pInfo->early_suspend.suspend = sii9234_early_suspend;
	pInfo->early_suspend.resume = sii9234_late_resume;
	register_early_suspend(&pInfo->early_suspend);
#endif
#ifdef CONFIG_CABLE_DETECT_ACCESSORY
	INIT_WORK(&pInfo->mhl_notifier_work, send_mhl_connect_notify);
#endif
	/* Register a platform device */
	mhl_dev = platform_device_register_simple("mhl", -1, NULL, 0);
	if (IS_ERR(mhl_dev)) {
		PR_DISP_DEBUG("mhl init: error\n");
		return PTR_ERR(mhl_dev);
	}
	/* Create a sysfs node to read simulated coordinates */

#ifdef MHL_RCP_KEYEVENT
	ret = device_create_file(&mhl_dev->dev, &dev_attr_rcp_event);

	input_dev = input_allocate_device();
	if (!input_dev) {
		PR_DISP_DEBUG("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_init;
	}
	/* indicate that we generate key events */
	set_bit(EV_KEY, input_dev->evbit);
	/* indicate that we generate *any* key event */
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_HOME, input_dev->keybit);
	set_bit(KEY_ENTER, input_dev->keybit);
	set_bit(KEY_LEFT, input_dev->keybit);
	set_bit(KEY_UP, input_dev->keybit);
	set_bit(KEY_DOWN, input_dev->keybit);
	set_bit(KEY_RIGHT, input_dev->keybit);
	set_bit(KEY_PLAY, input_dev->keybit);
	set_bit(KEY_STOP, input_dev->keybit);
	set_bit(KEY_PLAYPAUSE, input_dev->keybit);
	set_bit(KEY_REWIND, input_dev->keybit);
	set_bit(KEY_FASTFORWARD, input_dev->keybit);

	input_dev->name = "rcp_events";

	ret = input_register_device(input_dev);
	if (ret < 0)
		PR_DISP_DEBUG("MHL: can't register input devce\n");
#endif

	/* Initiate a 5 sec delay which will change the "g_bInitCompleted" be true after it...*/
	queue_delayed_work(pInfo->wq, &pInfo->init_complete_work, HZ*5);

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
	pInfo->input_dev = input_allocate_device();
	if (pInfo->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_INFO "Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	pInfo->input_dev->name = "tv-touchscreen";
	set_bit(EV_SYN, pInfo->input_dev->evbit);
	set_bit(EV_ABS, pInfo->input_dev->evbit);
	set_bit(EV_KEY, pInfo->input_dev->evbit);
	set_bit(KEY_BACK, pInfo->input_dev->keybit);
	set_bit(KEY_HOME, pInfo->input_dev->keybit);
	set_bit(KEY_MENU, pInfo->input_dev->keybit);
	set_bit(KEY_SEARCH, pInfo->input_dev->keybit);
	set_bit(KEY_APP_SWITCH, pInfo->input_dev->keybit);

	printk(KERN_INFO "input_set_abs_params: mix_x %d, max_x %d, min_y %d, max_y %d\n",
			pdata->abs_x_min, pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);

	input_set_abs_params(pInfo->input_dev, ABS_MT_POSITION_X,
		pdata->abs_x_min, pdata->abs_x_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_POSITION_Y,
		pdata->abs_y_min, pdata->abs_y_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_TOUCH_MAJOR,
		pdata->abs_width_min, pdata->abs_width_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_WIDTH_MAJOR,
		pdata->abs_width_min, pdata->abs_width_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_PRESSURE,
		pdata->abs_pressure_min, pdata->abs_pressure_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_AMPLITUDE,
		0, (pdata->abs_pressure_max << 16) | pdata->abs_width_max, 0, 0);
	input_set_abs_params(pInfo->input_dev, ABS_MT_POSITION,
		0, ((1<<31) | (pdata->abs_x_max << 16) | pdata->abs_y_max), 0, 0);
	ret = input_register_device(pInfo->input_dev);
	if (ret) {
		printk(KERN_INFO "Failed to register input device\n");
		goto err_input_register_device_failed;
	}
#endif

	PR_DISP_DEBUG("%s: Probe success!\n", __func__);
	return ret;
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
err_input_register_device_failed:
	input_free_device(pInfo->input_dev);
err_input_dev_alloc_failed:
#endif
err_create_workqueue:
	gpio_free(pInfo->reset_pin);
	gpio_free(pInfo->intr_pin);
err_init:
err_request_intr_pin:
err_platform_data_null:
	kfree(pInfo);
err_check_functionality_failed:
	return ret;
}

static int sii9234_remove(struct i2c_client *client)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;

	gpio_free(pInfo->reset_pin);
	gpio_free(pInfo->intr_pin);

	if(board_build_flag() != SHIP_BUILD)
		debugfs_remove(dbg_entry_dir);

#ifndef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&pInfo->early_suspend);
#endif
	destroy_workqueue(pInfo->wq);
	kfree(pInfo);
	return E_MHL_OK;
}

static const struct i2c_device_id sii9234_i2c_id[] = {
	{MHL_SII9234_I2C_NAME, 0},
	{}
};

static struct i2c_driver sii9234_driver = {
	.id_table = sii9234_i2c_id,
	.probe = sii9234_probe,
	.remove = sii9234_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = sii9234_suspend,
	.resume = sii9234_resume,
#endif
	.driver = {
		.name = MHL_SII9234_I2C_NAME,
		.owner = THIS_MODULE,
	},
};
static void mhl_usb_status_notifier_func(int cable_type)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	/*in suspend mode, vbus change should be wakeup the system*/
	mutex_lock(&mhl_early_suspend_sem);

	if(cable_get_accessory_type() == DOCK_STATE_MHL && g_bEnterEarlySuspend){
		if(pInfo->statMHL == CONNECT_TYPE_INTERNAL) {
			if(time_after(jiffies, suspend_jiffies + HZ))
				update_mhl_status(true, CONNECT_TYPE_AC);
		} else {
			update_mhl_status(true, CONNECT_TYPE_INTERNAL);
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);
}

static struct t_usb_status_notifier usb_status_notifier = {
	.name = "mhl_vbus_detect",
	.func = mhl_usb_status_notifier_func,
};

static int __init sii9234_init(void)
{
	usb_register_notifier(&usb_status_notifier);
	return i2c_add_driver(&sii9234_driver);
}


static void __exit sii9234_exit(void)
{
	i2c_del_driver(&sii9234_driver);
}

module_init(sii9234_init);
module_exit(sii9234_exit);

MODULE_DESCRIPTION("SiI9234 Driver");
MODULE_LICENSE("GPL");
