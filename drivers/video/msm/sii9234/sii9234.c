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
#include <linux/async.h>

#include "sii9234.h"
#include "TPI.h"
#include "mhl_defs.h"

#define MHL_RCP_KEYEVENT
#define MHL_ISR_TIMEOUT 5

#define MHL_DEBUGFS_TIMEOUT 10
#define MHL_DEBUGFS_AMP 5

#define SII9234_I2C_RETRY_COUNT 2

#define sii_gpio_set_value(pin, val)	\
	do {	\
		if (gpio_cansleep(pin))	\
			gpio_set_value_cansleep(pin, val);	\
		else \
			gpio_set_value(pin, val);	\
	} while(0)
#define sii_gpio_get_value(pin)	\
	gpio_cansleep(pin)?	\
		gpio_get_value_cansleep(pin):	\
		gpio_get_value(pin)
typedef struct {
	struct i2c_client *i2c_client;
	struct workqueue_struct *wq;
	struct wake_lock wake_lock;
	int (*pwrCtrl)(int); 
	void (*mhl_usb_switch)(int);
	void (*mhl_1v2_power)(bool enable);
	int  (*mhl_power_vote)(bool enable);
	int (*enable_5v)(int on);
	int (*mhl_lpm_power)(bool on);
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
} T_MHL_SII9234_INFO;

static T_MHL_SII9234_INFO *sii9234_info_ptr;
static void sii9234_irq_do_work(struct work_struct *work);
static DECLARE_WORK(sii9234_irq_work, sii9234_irq_do_work);

static DEFINE_MUTEX(mhl_early_suspend_sem);
unsigned long suspend_jiffies;
unsigned long irq_jiffies;
bool g_bEnterEarlySuspend = false;
bool g_bProbe = false;
bool disable_interswitch = false;
bool mhl_wakeuped = false;
static bool g_bInitCompleted = false;
static bool sii9244_interruptable = false;
static bool need_simulate_cable_out = false;
#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
static bool g_bPollDetect = false;
#ifdef CONFIG_ARCH_MSM8X60
int htc_batt_turn_off_mhl_dongle_5v(void);
#endif
#endif
static bool g_bLowPowerModeOn = false;

static struct dentry *dbg_entry_dir, *dbg_entry_a3, *dbg_entry_a6, *dbg_entry_dbg_on;
static struct dentry *dbg_entry_con_test_timeout, *dbg_entry_con_test_on,
	*dbg_entry_con_test_random;

u8 dbg_drv_str_a3 = 0xEB, dbg_drv_str_a6 = 0x0C, dbg_drv_str_on = 0;
static u8 dbg_con_test_timeout = MHL_DEBUGFS_TIMEOUT, dbg_con_test_on = 0,
	dbg_con_test_random = 1;
void hdmi_set_switch_state(bool enable);

#ifdef MHL_RCP_KEYEVENT
struct input_dev *input_dev;
#endif
static struct platform_device *mhl_dev; 

static int dbg_con_get_timeout(void)
{
	int ret = dbg_con_test_timeout;

	if (dbg_con_test_random) {
		int t = MHL_DEBUGFS_TIMEOUT;
		int s = jiffies%MHL_DEBUGFS_AMP;
		ret = jiffies%2?t+s:t-s;
	}

	PR_DISP_INFO("%s: timeout = %d\n", __func__, ret);

	return ret;
}

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
	if (!pInfo)
		return;
	if(pInfo->isMHL && (pInfo->statMHL == CONNECT_TYPE_MHL_AC || pInfo->statMHL == CONNECT_TYPE_USB )){
#ifdef CONFIG_ARCH_MSM8X60
	htc_batt_turn_off_mhl_dongle_5v();
#else

		if(pInfo->enable_5v)
			pInfo->enable_5v(0);
#endif
	}
}
#endif
void update_mhl_status(bool isMHL, enum usb_connect_type statMHL)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;

	PR_DISP_INFO("%s: -+-+-+-+- MHL is %sconnected, status = %d -+-+-+-+-\n",
		__func__, isMHL?"":"NOT ", statMHL);
	pInfo->isMHL = isMHL;
	pInfo->statMHL = statMHL;

	if(!isMHL)
		sii9234_power_vote(false);

	queue_work(pInfo->wq, &pInfo->mhl_notifier_work);

#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
	if (isMHL && (statMHL == CONNECT_TYPE_INTERNAL)) {
		if (!g_bPollDetect) {
			g_bPollDetect = true;
			queue_delayed_work(pInfo->wq, &pInfo->detect_charger_work, HZ/2);
			cancel_delayed_work(&pInfo->turn_off_5v);
		}
	} else if (statMHL == CONNECT_TYPE_MHL_AC || statMHL == CONNECT_TYPE_USB) {
		cancel_delayed_work(&pInfo->detect_charger_work);
		g_bPollDetect = false;
		
		queue_delayed_work(pInfo->wq, &pInfo->turn_off_5v, HZ);
	}
	else {
		g_bPollDetect = false;
		cancel_delayed_work(&pInfo->detect_charger_work);
	}
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
	hdmi_set_switch_state(pInfo->isMHL);
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
	if (!sii9234_info_ptr)
		return 0;
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
	if (!sii9234_info_ptr)
		return 0;
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
	if (!pInfo)
		return -1;

	return sii_gpio_get_value(pInfo->intr_pin);
}

void sii9234_reset(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;

	sii_gpio_set_value(pInfo->reset_pin, 0);
	mdelay(2);
	sii_gpio_set_value(pInfo->reset_pin, 1);
}

int sii9234_get_ci2ca(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return -1;
	return pInfo->ci2ca_pin;
}

static void sii9234_irq_do_work(struct work_struct *work)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;
	mutex_lock(&mhl_early_suspend_sem);
	if (!g_bEnterEarlySuspend) {
		uint8_t		event;
		uint8_t		eventParameter;
		if(time_after(jiffies, irq_jiffies + HZ/20))
		{
			irq_jiffies = jiffies;
			
			need_simulate_cable_out = false;
			if(!dbg_con_test_on)
				cancel_delayed_work(&pInfo->irq_timeout_work);
			SiiMhlTxGetEvents(&event, &eventParameter);
			ProcessRcp(event, eventParameter);
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);

	enable_irq(pInfo->irq);
}

void sii9234_disableIRQ(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;
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

	if (!pInfo)
		return 0;
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
static ssize_t write_keyevent(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t count)
{
	int key;

	
	sscanf(buffer, "%d", &key);


	PR_DISP_DEBUG("key_event: %d\n", key);

	
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

static DEVICE_ATTR(rcp_event, 0644, NULL, write_keyevent);
#endif

void sii9234_mhl_device_wakeup(void)
{
	int err;
	int ret = 0 ;

	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;

	PR_DISP_INFO("sii9234_mhl_device_wakeup()\n");

	mutex_lock(&mhl_early_suspend_sem);

	sii9234_power_vote(true);

	if (!g_bInitCompleted) {
		PR_DISP_INFO("MHL inserted before HDMI related function was ready! Wait more 5 sec...\n");
		queue_delayed_work(pInfo->wq, &pInfo->init_delay_work, HZ*5);
		mutex_unlock(&mhl_early_suspend_sem);
		return;
	}

	
	if (pInfo->mhl_usb_switch)
		pInfo->mhl_usb_switch(1);

	sii_gpio_set_value(pInfo->reset_pin, 1); 

	if (g_bLowPowerModeOn) {
		g_bLowPowerModeOn = false;
		if (pInfo->mhl_lpm_power)
			pInfo->mhl_lpm_power(0);
	}

	if (pInfo->mhl_1v2_power)
		pInfo->mhl_1v2_power(1);

	err = TPI_Init();
	if (err != 1)
		PR_DISP_INFO("TPI can't init\n");

	sii9244_interruptable = true;
	PR_DISP_INFO("Enable Sii9244 IRQ\n");

	
	if(mhl_wakeuped) {
		disable_irq_nosync(pInfo->irq);
		free_irq(pInfo->irq, pInfo);

		ret = request_irq(pInfo->irq, sii9234_irq_handler, IRQF_TRIGGER_LOW, "mhl_sii9234_evt", pInfo);
		if (ret < 0) {
			PR_DISP_DEBUG("%s: request_irq(%d) failed for gpio %d (%d)\n",
				__func__, pInfo->irq, pInfo->intr_pin, ret);
			ret = -EIO;
		}
	} else
		enable_irq(pInfo->irq);

	mhl_wakeuped = true;

	
	
	
	need_simulate_cable_out = true;
	if(!dbg_con_test_on)
		queue_delayed_work(pInfo->wq, &pInfo->irq_timeout_work, HZ * MHL_ISR_TIMEOUT);
	else
		queue_delayed_work(pInfo->wq, &pInfo->irq_timeout_work, HZ * dbg_con_get_timeout());

	mutex_unlock(&mhl_early_suspend_sem);
}

static void init_delay_handler(struct work_struct *w)
{
	PR_DISP_INFO("init_delay_handler()\n");

	TPI_Init();
	update_mhl_status(false, CONNECT_TYPE_UNKNOWN);
}

static void init_complete_handler(struct work_struct *w)
{
	PR_DISP_INFO("init_complete_handler()\n");
	
	TPI_Init();
	g_bInitCompleted = true;
}
static void irq_timeout_handler(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if(!dbg_con_test_on) {
		if(need_simulate_cable_out) {
			int ret = 0 ;
			if (!pInfo)
				return;
			
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
	} else {
		sii9234_disableIRQ();
		if (pInfo->mhl_1v2_power)
			pInfo->mhl_1v2_power(0);
		if (pInfo->mhl_usb_switch)
			pInfo->mhl_usb_switch(0);
		TPI_Init();
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
	if (!pInfo)
		return;
	PR_DISP_INFO("%s(isMHL=%d)\n", __func__, pInfo->isMHL);

	
	if (pInfo->isMHL && !tpi_get_hpd_state())
		sii9234_disableIRQ();

	mutex_lock(&mhl_early_suspend_sem);
	
	g_bEnterEarlySuspend = true;
	suspend_jiffies = jiffies;

	
	cancel_delayed_work(&pInfo->mhl_on_delay_work);
	if (pInfo->isMHL) {
		
		if(!tpi_get_hpd_state()){
			
			sii9234_suspend(pInfo->i2c_client, PMSG_SUSPEND);

#ifdef CONFIG_INTERNAL_CHARGING_SUPPORT
			cancel_delayed_work(&pInfo->detect_charger_work);
#endif
			
			if (pInfo->mhl_1v2_power)
				pInfo->mhl_1v2_power(0);
			if (pInfo->mhl_usb_switch)
				pInfo->mhl_usb_switch(0);
			
			TPI_Init();
		} else {
		}
	} else {
		
		
		if (cable_get_accessory_type() != DOCK_STATE_MHL )
			disable_interswitch = true;
		if (!g_bLowPowerModeOn) {
			g_bLowPowerModeOn = true;
			if (pInfo->mhl_lpm_power)
				pInfo->mhl_lpm_power(1);
		}
		disable_interswitch = false;
	}
	mutex_unlock(&mhl_early_suspend_sem);
}

static void sii9234_late_resume(struct early_suspend *h)
{
	T_MHL_SII9234_INFO *pInfo;
	pInfo = container_of(h, T_MHL_SII9234_INFO, early_suspend);
	if (!pInfo)
		return;
	PR_DISP_INFO("sii9234_late_resume()\n");

	mutex_lock(&mhl_early_suspend_sem);
	queue_delayed_work(pInfo->wq, &pInfo->mhl_on_delay_work, HZ);

	g_bEnterEarlySuspend = false;
	mutex_unlock(&mhl_early_suspend_sem);
}
static void mhl_turn_off_5v(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;
#ifdef CONFIG_ARCH_MSM8X60
        htc_batt_turn_off_mhl_dongle_5v();
#else
	if(pInfo->enable_5v)
		pInfo->enable_5v(0);
#endif
}
static void mhl_on_delay_handler(struct work_struct *w)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return;

	mutex_lock(&mhl_early_suspend_sem);
	if (IsMHLConnection()) {
		
		PR_DISP_DEBUG("MHL has connected. No SimulateCableOut!!!\n");
		mutex_unlock(&mhl_early_suspend_sem);
		return;
	}
	else {
		if(pInfo->isMHL){
			
			PR_DISP_DEBUG("notify cable out, re-init cable & mhl\n");
			update_mhl_status(false, CONNECT_TYPE_UNKNOWN);
			TPI_Init();
		} else {
			if (g_bLowPowerModeOn) {
				g_bLowPowerModeOn = false;
				if (pInfo->mhl_lpm_power)
					pInfo->mhl_lpm_power(0);
			}
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);
}
#endif

extern void fake_plug(bool plug);

static int mhl_con_event_open(struct inode *inode, struct file *file)
{
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}
static int mhl_con_event_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mhl_con_event_write(struct file *file, const char __user *buff,
	size_t count, loff_t *ppos)
{
	int con_event = 0;
	char debug_buf[5];

	if (count >= sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;

	sscanf(debug_buf, "%d", &con_event);

	if(con_event == 1)
		fake_plug(true);
	else
		fake_plug(false);
	return count;
}

static const struct file_operations mhl_con_event_fops = {
	.open = mhl_con_event_open,
	.release = mhl_con_event_release,
	.read = NULL,
	.write = mhl_con_event_write,
};



static int sii_debugfs_init(void)
{
	dbg_entry_dir = debugfs_create_dir("mhl", NULL);
	if (!dbg_entry_dir) {
		PR_DISP_DEBUG("Fail to create debugfs dir: mhl\n");
		return -1;
	}
	dbg_entry_a3 = debugfs_create_u8("strength_a3", 0644, dbg_entry_dir, &dbg_drv_str_a3);
	if (!dbg_entry_a3)
		PR_DISP_DEBUG("Fail to create debugfs: strength_a3\n");
	dbg_entry_a6 = debugfs_create_u8("strength_a6", 0644, dbg_entry_dir, &dbg_drv_str_a6);
	if (!dbg_entry_a6)
		PR_DISP_DEBUG("Fail to create debugfs: strength_a6\n");
	dbg_entry_dbg_on = debugfs_create_u8("dbg_on", 0644, dbg_entry_dir, &dbg_drv_str_on);
	if (!dbg_entry_dbg_on)
		PR_DISP_DEBUG("Fail to create debugfs: dbg_on\n");

	
	dbg_entry_con_test_timeout = debugfs_create_u8("con_test_timeout", 0644, dbg_entry_dir, &dbg_con_test_timeout);
	if (!dbg_entry_con_test_timeout)
		PR_DISP_DEBUG("Fail to create debugfs: con_test_timeout\n");
	dbg_entry_con_test_on = debugfs_create_u8("con_test_on", 0644, dbg_entry_dir, &dbg_con_test_on);
	if (!dbg_entry_dbg_on)
		PR_DISP_DEBUG("Fail to create debugfs: con_test_on\n");
	dbg_entry_con_test_random = debugfs_create_u8("con_test_random", 0644, dbg_entry_dir, &dbg_con_test_random);
        if (!dbg_entry_dbg_on)
                PR_DISP_DEBUG("Fail to create debugfs: con_test_random\n");

	
	if (debugfs_create_file("con_event", 0644, dbg_entry_dir, 0, &mhl_con_event_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}
	return 0;
}

void sii9234_request_abort(void)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	need_simulate_cable_out = true;
	PR_DISP_INFO("reuqest to abort connection");
	queue_delayed_work(pInfo->wq, &pInfo->irq_timeout_work, HZ);
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
	pInfo->mhl_lpm_power = pdata->mhl_lpm_power;
	pInfo->mhl_power_vote = pdata->mhl_power_vote;
	pInfo->enable_5v = pdata->enable_5v;
	sii9234_info_ptr = pInfo;
	
	g_bProbe = true;
	
	if (pInfo->pwrCtrl)
		pInfo->pwrCtrl(1);
	
	if(1)
		sii_debugfs_init();

	
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
#ifndef CONFIG_ARCH_MSM8X60
	pInfo->irq = gpio_to_irq(pInfo->irq);
#endif
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
	
	mhl_dev = platform_device_register_simple("mhl", -1, NULL, 0);
	if (IS_ERR(mhl_dev)) {
		PR_DISP_DEBUG("mhl init: error\n");
		return PTR_ERR(mhl_dev);
	}
	

#ifdef MHL_RCP_KEYEVENT
	ret = device_create_file(&mhl_dev->dev, &dev_attr_rcp_event);

	input_dev = input_allocate_device();
	if (!input_dev) {
		PR_DISP_DEBUG("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_init;
	}
	
	set_bit(EV_KEY, input_dev->evbit);
	
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
	
	queue_delayed_work(pInfo->wq, &pInfo->init_complete_work, HZ*10);
	PR_DISP_DEBUG("%s: Probe success!\n", __func__);
	return ret;
err_create_workqueue:
	gpio_free(pInfo->reset_pin);
	gpio_free(pInfo->intr_pin);
err_init:
err_request_intr_pin:
err_platform_data_null:
	kfree(pInfo);
	sii9234_info_ptr = NULL;
err_check_functionality_failed:
	return ret;
}

static int sii9234_remove(struct i2c_client *client)
{
	T_MHL_SII9234_INFO *pInfo = sii9234_info_ptr;
	if (!pInfo)
		return -1;
	gpio_free(pInfo->reset_pin);
	gpio_free(pInfo->intr_pin);

	
	if(1)
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
	if (!pInfo)
		return;
	
	mutex_lock(&mhl_early_suspend_sem);

	if(cable_get_accessory_type() == DOCK_STATE_MHL && g_bEnterEarlySuspend){
		if(pInfo->statMHL == CONNECT_TYPE_INTERNAL) {
			if(time_after(jiffies, suspend_jiffies + HZ))
				update_mhl_status(true, CONNECT_TYPE_MHL_AC);
		} else {
			update_mhl_status(true, CONNECT_TYPE_INTERNAL);
			hdmi_set_switch_state(false);
		}
	}
	mutex_unlock(&mhl_early_suspend_sem);
}

static struct t_usb_status_notifier usb_status_notifier = {
	.name = "mhl_vbus_detect",
	.func = mhl_usb_status_notifier_func,
};

static void __init sii9234_init_async(void *unused, async_cookie_t cookie)
{
	htc_usb_register_notifier(&usb_status_notifier);
	i2c_add_driver(&sii9234_driver);
}

static int __init sii9234_init(void)
{
	async_schedule(sii9234_init_async, NULL);
	return 0;
}

static void __exit sii9234_exit(void)
{
	i2c_del_driver(&sii9234_driver);
}

module_init(sii9234_init);
module_exit(sii9234_exit);

MODULE_DESCRIPTION("SiI9234 Driver");
MODULE_LICENSE("GPL");
