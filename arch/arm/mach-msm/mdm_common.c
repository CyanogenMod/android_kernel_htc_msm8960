/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/debugfs.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/mfd/pmic8058.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <mach/mdm2.h>
#include <mach/restart.h>
#include <mach/subsystem_notif.h>
#include <mach/subsystem_restart.h>
#include <linux/msm_charm.h>
#include <mach/msm_watchdog.h>
#include "mdm_private.h"
#include "sysmon.h"
#include <mach/msm_rtb.h>

#include <linux/proc_fs.h>
#include <mach/board_htc.h>

#ifdef CONFIG_HTC_POWEROFF_MODEM_IN_OFFMODE_CHARGING
enum {
	BOARD_MFG_MODE_NORMAL = 0,
	BOARD_MFG_MODE_FACTORY2,
	BOARD_MFG_MODE_RECOVERY,
	BOARD_MFG_MODE_CHARGE,
	BOARD_MFG_MODE_POWERTEST,
	BOARD_MFG_MODE_OFFMODE_CHARGING,
	BOARD_MFG_MODE_MFGKERNEL,
	BOARD_MFG_MODE_MODEM_CALIBRATION,
};
#endif


#if defined(pr_warn)
#undef pr_warn
#endif
#define pr_warn(x...) do {				\
			printk(KERN_WARN "[MDM][COMM] "x);		\
	} while (0)

#if defined(pr_debug)
#undef pr_debug
#endif
#define pr_debug(x...) do {				\
			printk(KERN_DEBUG "[MDM][COMM] "x);		\
	} while (0)

#if defined(pr_info)
#undef pr_info
#endif
#define pr_info(x...) do {				\
			printk(KERN_INFO "[MDM][COMM] "x);		\
	} while (0)

#if defined(pr_err)
#undef pr_err
#endif
#define pr_err(x...) do {				\
			printk(KERN_ERR "[MDM][COMM] "x);		\
	} while (0)

#define HTC_MDM_ERROR_CONFIRM_TIME_MS	10

#define MDM_MODEM_TIMEOUT	6000
#define MDM_MODEM_DELTA	100
#define MDM_BOOT_TIMEOUT	60000L
#define MDM_RDUMP_TIMEOUT	180000L

static int mdm_debug_on;
static struct workqueue_struct *mdm_queue;
static struct workqueue_struct *mdm_sfr_queue;
static void mdm_status_fn(struct work_struct *work);
static void dump_mdm_related_gpio(void);
static DECLARE_WORK(mdm_status_work, mdm_status_fn);
static struct workqueue_struct *mdm_gpio_monitor_queue;
static bool mdm_status_change_notified;


#define EXTERNAL_MODEM "external_modem"

static struct mdm_modem_drv *mdm_drv;

DECLARE_COMPLETION(mdm_needs_reload);
DECLARE_COMPLETION(mdm_boot);
DECLARE_COMPLETION(mdm_ram_dumps);

static int first_boot = 1;

#define RD_BUF_SIZE			100
#define SFR_MAX_RETRIES		10
#define SFR_RETRY_INTERVAL	1000


#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
#define MODEM_ERRMSG_LIST_LEN 10

struct mdm_msr_info {
	int valid;
	struct timespec msr_time;
	char modem_errmsg[RD_BUF_SIZE];
};
int mdm_msr_index = 0;
static spinlock_t msr_info_lock;
static struct mdm_msr_info msr_info_list[MODEM_ERRMSG_LIST_LEN];
#endif

static void dump_gpio(char *name, unsigned int gpio);
static int set_mdm_errmsg(void __user *msg);
static int notify_mdm_nv_write_done(void);

static int mdm_loaded_status_proc(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	int ret;
	char *p = page;

	if (off > 0) {
		ret = 0;
	} else {
		p += sprintf(p, "%d\n", mdm_drv->mdm_ready);
		ret = p - page;
	}

	return ret;
}

static void mdm_loaded_info(void)
{
	struct proc_dir_entry *entry = NULL;

	entry = create_proc_read_entry("mdm9k_status", 0, NULL, mdm_loaded_status_proc, NULL);
}

#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
static ssize_t modem_silent_reset_info_store(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t count)
{
	int len = 0;
	unsigned long flags;

	spin_lock_irqsave(&msr_info_lock, flags);

	msr_info_list[mdm_msr_index].valid = 1;
	msr_info_list[mdm_msr_index].msr_time = current_kernel_time();
	snprintf(msr_info_list[mdm_msr_index].modem_errmsg, RD_BUF_SIZE, "%s", buf);
	len = strlen(msr_info_list[mdm_msr_index].modem_errmsg);
	if(msr_info_list[mdm_msr_index].modem_errmsg[len-1] == '\n')
	{
		msr_info_list[mdm_msr_index].modem_errmsg[len-1] = '\0';
	}

	if(++mdm_msr_index >= MODEM_ERRMSG_LIST_LEN) {
		mdm_msr_index = 0;
	}

	spin_unlock_irqrestore(&msr_info_lock, flags);

	return count;
}

static ssize_t modem_silent_reset_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i = 0;
	char tmp[RD_BUF_SIZE+30];
	unsigned long flags;

	spin_lock_irqsave(&msr_info_lock, flags);

	for( i=0; i<MODEM_ERRMSG_LIST_LEN; i++ ) {
		if( msr_info_list[i].valid != 0 ) {
			
			snprintf(tmp, RD_BUF_SIZE+30, "%ld-%s|\n\r", msr_info_list[i].msr_time.tv_sec, msr_info_list[i].modem_errmsg);
			strcat(buf, tmp);
			memset(tmp, 0, RD_BUF_SIZE+30);
		}
		msr_info_list[i].valid = 0;
		memset(msr_info_list[i].modem_errmsg, 0, RD_BUF_SIZE);
	}
	strcat(buf, "\n\r\0");

	spin_unlock_irqrestore(&msr_info_lock, flags);

	return strlen(buf);
}
static DEVICE_ATTR(msr_info, S_IRUSR | S_IROTH | S_IRGRP | S_IWUSR,
	modem_silent_reset_info_show, modem_silent_reset_info_store);

static struct kobject *subsystem_restart_reason_obj;

static ssize_t mdm_subsystem_restart_reason_store(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t count)
{
	return modem_silent_reset_info_store(dev, attr, buf, count);
}

static ssize_t mdm_subsystem_restart_reason_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return modem_silent_reset_info_show(dev, attr, buf);
}

static DEVICE_ATTR(subsystem_restart_reason_nonblock, S_IRUSR | S_IROTH | S_IRGRP | S_IWUSR,
        mdm_subsystem_restart_reason_show, mdm_subsystem_restart_reason_store);

static int mdm_subsystem_restart_properties_init(void)
{
	int ret = 0;
	subsystem_restart_reason_obj = kobject_create_and_add("subsystem_restart_properties", NULL);
	if (subsystem_restart_reason_obj == NULL) {
		pr_info("kobject_create_and_add: subsystem_restart_properties failed\n");
                return -EFAULT;
	}

	ret = sysfs_create_file(subsystem_restart_reason_obj,
             &dev_attr_subsystem_restart_reason_nonblock.attr);
        if (ret) {
                pr_info("sysfs_create_file: subsystem_restart_reason_nonblock failed\n");
                return -EFAULT;
        }
	return 0;
}

static int mdm_subsystem_restart_properties_release(void)
{
	
	if(subsystem_restart_reason_obj != NULL) {
		sysfs_remove_file(subsystem_restart_reason_obj,
			&dev_attr_subsystem_restart_reason_nonblock.attr);
		kobject_put(subsystem_restart_reason_obj);
	}
	return 0;
}

static int modem_silent_reset_info_sysfs_attrs(struct platform_device *pdev)
{
	int i = 0;
	unsigned long flags;

	spin_lock_irqsave(&msr_info_lock, flags);

	mdm_msr_index = 0;
	for( i=0; i<MODEM_ERRMSG_LIST_LEN; i++ ) {
		msr_info_list[i].valid = 0;
		memset(msr_info_list[i].modem_errmsg, 0, RD_BUF_SIZE);
	}

	spin_unlock_irqrestore(&msr_info_lock, flags);

	return device_create_file(&pdev->dev, &dev_attr_msr_info);
}
#endif


static void mdm_restart_reason_fn(struct work_struct *work)
{
	int ret, ntries = 0;
	char sfr_buf[RD_BUF_SIZE];
#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
	unsigned long flags;
#endif

	do {
		msleep(SFR_RETRY_INTERVAL);
#if defined(CONFIG_BUILD_EDIAG)
                pr_info("SYSMON is supposed to be used as char dev with specific purpose.\n");
                ret = -EINVAL;
#else
		ret = sysmon_get_reason(SYSMON_SS_EXT_MODEM,
					sfr_buf, sizeof(sfr_buf));
#endif
		if (ret) {
			pr_err("%s: Error retrieving mdm restart reason, ret = %d, "
					"%d/%d tries\n", __func__, ret,
					ntries + 1,	SFR_MAX_RETRIES);
		} else {
			pr_err("mdm restart reason: %s\n", sfr_buf);
#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
			spin_lock_irqsave(&msr_info_lock, flags);

			msr_info_list[mdm_msr_index].valid = 1;
			msr_info_list[mdm_msr_index].msr_time = current_kernel_time();
			snprintf(msr_info_list[mdm_msr_index].modem_errmsg, RD_BUF_SIZE, "%s", sfr_buf);
			if(++mdm_msr_index >= MODEM_ERRMSG_LIST_LEN) {
				mdm_msr_index = 0;
			}

			spin_unlock_irqrestore(&msr_info_lock, flags);
#endif
			break;
		}
	} while (++ntries < SFR_MAX_RETRIES);
}

static DECLARE_WORK(sfr_reason_work, mdm_restart_reason_fn);

static void mdm_status_check_fn(struct work_struct *work)
{
	int value = 0;

	msleep(3000); 
	pr_info("%s mdm_status_change notified? %c\n", __func__, mdm_status_change_notified ? 'Y': 'N');
	if (!mdm_status_change_notified) {
		dump_mdm_related_gpio();
		value = gpio_get_value(mdm_drv->mdm2ap_status_gpio);
		if (value == 1)
			queue_work_on(0, mdm_queue, &mdm_status_work);
	}
}

static DECLARE_WORK(mdm_status_check_work, mdm_status_check_fn);

#define MDM2AP_STATUS_CHANGE_MAX_WAIT_TIME		5000
#define MDM2AP_STATUS_CHANGE_TOTAL_CHECK_TIME		20
static int mdm_hsic_reconnectd_check_fn(void)
{
	int ret = 0;
	int i = 0, value = 0;
	for ( i = 0; i <= MDM2AP_STATUS_CHANGE_TOTAL_CHECK_TIME; i++ ) {
		value = gpio_get_value( mdm_drv->mdm2ap_status_gpio );
		if ( value == 1 && mdm_drv->mdm_hsic_reconnectd == 1 ) {
			ret = 1;
			break;
		}
		if ( i < MDM2AP_STATUS_CHANGE_TOTAL_CHECK_TIME ) {
			msleep( (MDM2AP_STATUS_CHANGE_MAX_WAIT_TIME / MDM2AP_STATUS_CHANGE_TOTAL_CHECK_TIME) );
		}
	}
	if ( ret == 0 ) {
		pr_info("%s: return 0, mdm_hsic_reconnectd=[%d]\n", __func__, mdm_drv->mdm_hsic_reconnectd);
	} else {
		pr_info("%s: return 1\n", __func__);
	}

	return ret;
}

long mdm_modem_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	int status, ret = 0;

	if (_IOC_TYPE(cmd) != CHARM_CODE) {
		pr_err("%s: invalid ioctl code\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s: Entering ioctl cmd = %d\n", __func__, _IOC_NR(cmd));
	switch (cmd) {
	case WAKE_CHARM:
		pr_info("%s: Powering on mdm\n", __func__);
		mdm_drv->mdm_ready = 0;	
		mdm_drv->mdm_hsic_reconnectd = 0;
		mdm_drv->ops->power_on_mdm_cb(mdm_drv);
		break;
	case CHECK_FOR_BOOT:
		if (gpio_get_value(mdm_drv->mdm2ap_status_gpio) == 0)
			put_user(1, (unsigned long __user *) arg);
		else
			put_user(0, (unsigned long __user *) arg);
		break;
	case NORMAL_BOOT_DONE:
		{
			int ret_mdm_hsic_reconnectd = 0;
			pr_debug("%s: check if mdm is booted up\n", __func__);
			get_user(status, (unsigned long __user *) arg);
			if (status) {
				pr_debug("%s: normal boot failed\n", __func__);
				mdm_drv->mdm_boot_status = -EIO;
			} else {
				pr_info("%s: normal boot done\n", __func__);
				mdm_drv->mdm_boot_status = 0;
			}
			
	                mdm_status_change_notified = false;
	                queue_work_on(0, mdm_gpio_monitor_queue, &mdm_status_check_work);
	                
			mdm_drv->mdm_ready = 1;

			if (mdm_drv->ops->normal_boot_done_cb != NULL)
				mdm_drv->ops->normal_boot_done_cb(mdm_drv);

			ret_mdm_hsic_reconnectd = mdm_hsic_reconnectd_check_fn();

			if ( ret_mdm_hsic_reconnectd == 1 ) {
				pr_info("%s: ret_mdm_hsic_reconnectd == 1\n", __func__);
			} else {
				pr_info("%s: ret_mdm_hsic_reconnectd == 0\n", __func__);
			}

			if (!first_boot)
				complete(&mdm_boot);
			else
				first_boot = 0;
		}
		break;
	case RAM_DUMP_DONE:
		pr_debug("%s: mdm done collecting RAM dumps\n", __func__);
		get_user(status, (unsigned long __user *) arg);
		if (status)
			mdm_drv->mdm_ram_dump_status = -EIO;
		else {
			pr_info("%s: ramdump collection completed\n", __func__);
			mdm_drv->mdm_ram_dump_status = 0;
		}
		complete(&mdm_ram_dumps);
		break;
	case WAIT_FOR_RESTART:
		pr_debug("%s: wait for mdm to need images reloaded\n",
				__func__);

		if (mdm_drv) {
	        dump_gpio("MDM2AP_STATUS", mdm_drv->mdm2ap_status_gpio);
	        dump_gpio("MDM2AP_ERRFATAL", mdm_drv->mdm2ap_errfatal_gpio);
		}

		ret = wait_for_completion_interruptible(&mdm_needs_reload);
		if (!ret && mdm_drv) {
			put_user(mdm_drv->boot_type,
					 (unsigned long __user *) arg);

			pr_err("%s: mdm_drv->boot_type:%d\n", __func__, mdm_drv->boot_type);
		}
		INIT_COMPLETION(mdm_needs_reload);
		break;
	case GET_MFG_MODE:
		pr_info("%s: board_mfg_mode()=%d\n", __func__, board_mfg_mode());
		put_user(board_mfg_mode(),
				 (unsigned long __user *) arg);
		break;
	case SET_MODEM_ERRMSG:
		pr_info("%s: Set modem fatal errmsg\n", __func__);
		ret = set_mdm_errmsg((void __user *) arg);
		break;
	case GET_RADIO_FLAG:
		pr_info("%s:get_radio_flag()=%x\n", __func__, get_radio_flag());

		
		if ((get_radio_flag() & RADIO_FLAG_USB_UPLOAD) && mdm_drv != NULL) {
			pr_info("AP2MDM_STATUS GPIO:%d\n", mdm_drv->ap2mdm_status_gpio);
			pr_info("AP2MDM_ERRFATAL GPIO:%d\n", mdm_drv->ap2mdm_errfatal_gpio);
			pr_info("AP2MDM_PMIC_RESET_N GPIO:%d\n", mdm_drv->ap2mdm_pmic_reset_n_gpio);
			pr_info("MDM2AP_STATUS GPIO:%d\n", mdm_drv->mdm2ap_status_gpio);
			pr_info("MDM2AP_ERRFATAL GPIO:%d\n", mdm_drv->mdm2ap_errfatal_gpio);
			pr_info("MDM2AP_HSIC_READY GPIO:%d\n", mdm_drv->mdm2ap_hsic_ready_gpio);
			pr_info("AP2MDM_IPC1 GPIO:%d\n", mdm_drv->ap2mdm_ipc1_gpio);
		}
		

		put_user(get_radio_flag(),
				 (unsigned long __user *) arg);
		break;
	case EFS_SYNC_DONE:
		pr_info("%s: efs sync is done\n", __func__);
		break;
	case NV_WRITE_DONE:
		pr_info("%s: NV write done!\n", __func__);
		notify_mdm_nv_write_done();
		break;
	case HTC_POWER_OFF_CHARM:
		pr_info("%s: (HTC_POWER_OFF_CHARM)Powering off mdm\n", __func__);
		if ( mdm_drv->ops->htc_power_down_mdm_cb ) {
			mdm_drv->mdm_ready = 0;
			mdm_drv->mdm_hsic_reconnectd = 0;
			mdm_drv->ops->htc_power_down_mdm_cb(mdm_drv);
		}
		break;
	case HTC_UPDATE_CRC_RESTART_LEVEL:
		pr_info("%s: (HTC_UPDATE_CRC_RESTART_LEVEL)\n", __func__);
		subsystem_update_restart_level_for_crc();
		break;
	default:
		pr_err("%s: invalid ioctl cmd = %d\n", __func__, _IOC_NR(cmd));
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void dump_gpio(char *name, unsigned int gpio)
{
        if (gpio == 0) {
               pr_err("%s: Cannot dump %s, due to invalid gpio number %d\n", __func__, name, gpio);
                return;
       }
        pr_info("%s: %s\t= %d\n", __func__, name, gpio_get_value(gpio));

	return;
}

static void dump_mdm_related_gpio(void)
{
        dump_gpio("AP2MDM_STATUS", mdm_drv->ap2mdm_status_gpio);
        
        dump_gpio("AP2MDM_ERRFATAL", mdm_drv->ap2mdm_errfatal_gpio);
        dump_gpio("AP2MDM_PMIC_RESET_N", mdm_drv->ap2mdm_pmic_reset_n_gpio);

        dump_gpio("MDM2AP_STATUS", mdm_drv->mdm2ap_status_gpio);
        
        dump_gpio("MDM2AP_ERRFATAL", mdm_drv->mdm2ap_errfatal_gpio);

	return;
}

static char modem_errmsg[MODEM_ERRMSG_LEN];
static int set_mdm_errmsg(void __user *msg)
{
	memset(modem_errmsg, 0, sizeof(modem_errmsg));
	if (unlikely(copy_from_user(modem_errmsg, msg, MODEM_ERRMSG_LEN))) {
		pr_err("%s: copy modem_errmsg failed\n", __func__);
		return -EFAULT;
	}
	modem_errmsg[MODEM_ERRMSG_LEN-1] = '\0';
	pr_info("%s: set modem errmsg: %s\n", __func__, modem_errmsg);
	return 0;
}

char *get_mdm_errmsg(void)
{
	if (strlen(modem_errmsg) <= 0) {
		pr_err("%s: can not get mdm errmsg.\n", __func__);
		return NULL;
	}
	return modem_errmsg;
}
EXPORT_SYMBOL(get_mdm_errmsg);

static int notify_mdm_nv_write_done(void)
{
	gpio_direction_output(mdm_drv->ap2mdm_ipc1_gpio, 1);
	msleep(1);
	gpio_direction_output(mdm_drv->ap2mdm_ipc1_gpio, 0);
	return 0;
}

extern void set_mdm2ap_errfatal_restart_flag(unsigned);
static void mdm_crash_dump_dbg_info(void)
{
	dump_mdm_related_gpio();

	
	printk(KERN_INFO "=== Show qcks stack ===\n");
	show_thread_group_state_filter("qcks", 0);
	printk(KERN_INFO "\n");

	printk(KERN_INFO "=== Show efsks stack ===\n");
	show_thread_group_state_filter("efsks", 0);
	printk(KERN_INFO "\n");

	printk(KERN_INFO "=== Show ks stack ===\n");
	show_thread_group_state_filter("ks", 0);
	printk(KERN_INFO "\n");

	pr_info("### Show Blocked State in ###\n");
	show_state_filter(TASK_UNINTERRUPTIBLE);
	if (get_restart_level() == RESET_SOC)
		msm_rtb_disable();

	if (get_restart_level() == RESET_SOC)
		set_mdm2ap_errfatal_restart_flag(1);
}

static void mdm_fatal_fn(struct work_struct *work)
{
	
	int i;
	int value = gpio_get_value(mdm_drv->mdm2ap_errfatal_gpio);

	if (value == 1) {
		for (i = HTC_MDM_ERROR_CONFIRM_TIME_MS; i > 0; i--) {
			msleep(1);
			if (gpio_get_value(mdm_drv->mdm2ap_errfatal_gpio) == 0) {
				pr_info("%s: mdm fatal high %d(ms) confirm failed... Abort!\n", __func__, HTC_MDM_ERROR_CONFIRM_TIME_MS);
				return;
			}
		}
	} else if (value == 0) {
		pr_info("%s: mdm fatal high is a false alarm!\n", __func__);
		return;
	}

	mdm_crash_dump_dbg_info();
	

	pr_info("%s: Reseting the mdm due to an errfatal\n", __func__);

	subsystem_restart(EXTERNAL_MODEM);
}

static DECLARE_WORK(mdm_fatal_work, mdm_fatal_fn);

static void mdm_status_fn(struct work_struct *work)
{
	int i;
	int value = gpio_get_value(mdm_drv->mdm2ap_status_gpio);

	if (!mdm_drv->mdm_ready)
		return;

	
	if (value == 0) {
		for (i = HTC_MDM_ERROR_CONFIRM_TIME_MS; i > 0; i--) {
			msleep(1);
			if (gpio_get_value(mdm_drv->mdm2ap_status_gpio) == 1) {
				pr_info("%s: mdm status low %d(ms) confirm failed... Abort!\n", __func__, HTC_MDM_ERROR_CONFIRM_TIME_MS);
				return;
			}
		}
	}
	

	if ( ( get_radio_flag() & RADIO_FLAG_USB_UPLOAD ) ) {
		if ( value == 0 ) {
			int val_gpio = 0;
			msleep(40);
			val_gpio = gpio_get_value(mdm_drv->mdm2ap_hsic_ready_gpio);
			pr_info("%s:mdm2ap_hsic_ready_gpio=[%d]\n", __func__, val_gpio);
		}
	}

	
	mdm_status_change_notified = true;
	
	mdm_drv->ops->status_cb(mdm_drv, value);

	pr_debug("%s: status:%d\n", __func__, value);

	if (value == 0) {
		pr_info("%s: unexpected reset external modem\n", __func__);

		
		mdm_crash_dump_dbg_info();
		

		subsystem_restart(EXTERNAL_MODEM);
	} else if (value == 1) {
		pr_info("%s: status = 1: mdm is now ready\n", __func__);
	}
}

#ifdef CONFIG_QSC_MODEM
void qsc_boot_after_mdm_bootloader(int state);
static void qsc_boot_up_fn(struct work_struct *work)
{
	free_irq(mdm_drv->mdm2ap_bootloader_irq, NULL);

	qsc_boot_after_mdm_bootloader(MDM_BOOTLOAER_GPIO_IRQ_RECEIVED);
}
static DECLARE_WORK(qsc_boot_up_work, qsc_boot_up_fn);
#endif

static void mdm_disable_irqs(void)
{
	disable_irq_nosync(mdm_drv->mdm_errfatal_irq);
	disable_irq_nosync(mdm_drv->mdm_status_irq);

}

static irqreturn_t mdm_errfatal(int irq, void *dev_id)
{
	pr_err("%s: detect mdm errfatal pin rising\n", __func__);

	if (mdm_drv->mdm_ready &&
		(gpio_get_value(mdm_drv->mdm2ap_status_gpio) == 1)) {
		pr_err("%s: mdm got errfatal interrupt\n", __func__);
		pr_debug("%s: scheduling work now\n", __func__);
		queue_work_on(0, mdm_queue, &mdm_fatal_work);	
	}
	return IRQ_HANDLED;
}

static int mdm_modem_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mdm_modem_fops = {
	.owner		= THIS_MODULE,
	.open		= mdm_modem_open,
	.unlocked_ioctl	= mdm_modem_ioctl,
};


static struct miscdevice mdm_modem_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "mdm",
	.fops	= &mdm_modem_fops
};

static int mdm_panic_prep(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	int i;

	pr_debug("%s: setting AP2MDM_ERRFATAL high for a non graceful reset\n",
			 __func__);
	mdm_disable_irqs();
	gpio_set_value(mdm_drv->ap2mdm_errfatal_gpio, 1);

	for (i = MDM_MODEM_TIMEOUT; i > 0; i -= MDM_MODEM_DELTA) {
		pet_watchdog();
		mdelay(MDM_MODEM_DELTA);
		if (gpio_get_value(mdm_drv->mdm2ap_status_gpio) == 0)
			break;
	}
	if (i <= 0)
		pr_err("%s: MDM2AP_STATUS never went low\n", __func__);
	return NOTIFY_DONE;
}

static struct notifier_block mdm_panic_blk = {
	.notifier_call  = mdm_panic_prep,
};

static irqreturn_t mdm_status_change(int irq, void *dev_id)
{
	pr_debug("%s: mdm sent status change interrupt\n", __func__);

	queue_work_on(0, mdm_queue, &mdm_status_work);		

	return IRQ_HANDLED;
}

#ifdef CONFIG_QSC_MODEM
static irqreturn_t mdm_in_bootloader(int irq, void *dev_id)
{
	pr_info("%s: got mdm2ap_bootloader interrupt\n", __func__);

	queue_work(mdm_queue, &qsc_boot_up_work);

	return IRQ_HANDLED;
}
#endif

static int mdm_subsys_shutdown(const struct subsys_data *crashed_subsys)
{
	mdm_drv->mdm_ready = 0;
	gpio_direction_output(mdm_drv->ap2mdm_errfatal_gpio, 1);
	if (mdm_drv->pdata->ramdump_delay_ms > 0) {
		msleep(mdm_drv->pdata->ramdump_delay_ms);
	}
	mdm_drv->ops->power_down_mdm_cb(mdm_drv);
	return 0;
}

static int mdm_subsys_powerup(const struct subsys_data *crashed_subsys)
{
	gpio_direction_output(mdm_drv->ap2mdm_errfatal_gpio, 0);
	gpio_direction_output(mdm_drv->ap2mdm_status_gpio, 1);
	
	mdm_drv->boot_type = CHARM_NORMAL_BOOT;
	pr_info("%s: mdm_needs_reload\n", __func__);
	complete(&mdm_needs_reload);
	if (!wait_for_completion_timeout(&mdm_boot,
			msecs_to_jiffies(MDM_BOOT_TIMEOUT))) {
		mdm_drv->mdm_boot_status = -ETIMEDOUT;
		pr_info("%s: mdm modem restart timed out.\n", __func__);
	} else {
		pr_info("%s: mdm modem has been restarted\n", __func__);
		
		queue_work_on(0, mdm_sfr_queue, &sfr_reason_work);	
	}
	INIT_COMPLETION(mdm_boot);
	return mdm_drv->mdm_boot_status;
}

static int mdm_subsys_ramdumps(int want_dumps,
				const struct subsys_data *crashed_subsys)
{
	mdm_drv->mdm_ram_dump_status = 0;
	if (want_dumps) {
		mdm_drv->boot_type = CHARM_RAM_DUMPS;
		complete(&mdm_needs_reload);
		if (!wait_for_completion_timeout(&mdm_ram_dumps,
				msecs_to_jiffies(MDM_RDUMP_TIMEOUT))) {
			mdm_drv->mdm_ram_dump_status = -ETIMEDOUT;
			pr_info("%s: mdm modem ramdumps timed out.\n",
					__func__);
		} else
			pr_info("%s: mdm modem ramdumps completed.\n",
					__func__);
		INIT_COMPLETION(mdm_ram_dumps);
		gpio_direction_output(mdm_drv->ap2mdm_errfatal_gpio, 1);
		mdm_drv->ops->power_down_mdm_cb(mdm_drv);
	}
	return mdm_drv->mdm_ram_dump_status;
}

static struct subsys_data mdm_subsystem = {
	.shutdown = mdm_subsys_shutdown,
	.ramdump = mdm_subsys_ramdumps,
	.powerup = mdm_subsys_powerup,
	.name = EXTERNAL_MODEM,
};

static int mdm_debug_on_set(void *data, u64 val)
{
	mdm_debug_on = val;
	if (mdm_drv->ops->debug_state_changed_cb)
		mdm_drv->ops->debug_state_changed_cb(mdm_debug_on);
	return 0;
}

static int mdm_debug_on_get(void *data, u64 *val)
{
	*val = mdm_debug_on;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mdm_debug_on_fops,
			mdm_debug_on_get,
			mdm_debug_on_set, "%llu\n");

static int mdm_debugfs_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("mdm_dbg", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debugfs_create_file("debug_on", 0644, dent, NULL,
			&mdm_debug_on_fops);
	return 0;
}

static void mdm_modem_initialize_data(struct platform_device  *pdev,
				struct mdm_ops *mdm_ops)
{
	struct resource *pres;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"MDM2AP_ERRFATAL");
	if (pres)
		mdm_drv->mdm2ap_errfatal_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_ERRFATAL");
	if (pres)
		mdm_drv->ap2mdm_errfatal_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"MDM2AP_STATUS");
	if (pres)
		mdm_drv->mdm2ap_status_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_STATUS");
	if (pres)
		mdm_drv->ap2mdm_status_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"MDM2AP_WAKEUP");
	if (pres)
		mdm_drv->mdm2ap_wakeup_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_WAKEUP");
	if (pres)
		mdm_drv->ap2mdm_wakeup_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_PMIC_RESET_N");
	if (pres)
		mdm_drv->ap2mdm_pmic_reset_n_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_KPDPWR_N");
	if (pres)
		mdm_drv->ap2mdm_kpdpwr_n_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"MDM2AP_HSIC_READY");
	if (pres)
		mdm_drv->mdm2ap_hsic_ready_gpio = pres->start;

	
	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"AP2MDM_IPC1");
	if (pres)
		mdm_drv->ap2mdm_ipc1_gpio = pres->start;

	
	pres = platform_get_resource_byname(pdev, IORESOURCE_IO,
							"MDM2AP_BOOTLOADER");
	if (pres)
		mdm_drv->mdm2ap_bootloader_gpio = pres->start;

#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
	modem_silent_reset_info_sysfs_attrs(pdev);
	mdm_subsystem_restart_properties_init();
#endif

	
	mdm_drv->boot_type                  = CHARM_NORMAL_BOOT;

	mdm_drv->ops      = mdm_ops;
	mdm_drv->pdata    = pdev->dev.platform_data;
}

extern void register_ap2mdm_pmic_reset_n_gpio(unsigned);	
int mdm_common_create(struct platform_device  *pdev,
					  struct mdm_ops *p_mdm_cb)
{
	int ret = -1, irq;

#ifdef CONFIG_HTC_POWEROFF_MODEM_IN_OFFMODE_CHARGING
	
	int mfg_mode = BOARD_MFG_MODE_NORMAL;
	
	mfg_mode = board_mfg_mode();
	if ( mfg_mode == BOARD_MFG_MODE_OFFMODE_CHARGING ) {
		unsigned ap2mdm_pmic_reset_n_gpio = 0;
		struct resource *pres;
		pr_info("%s: BOARD_MFG_MODE_OFFMODE_CHARGING\n", __func__);
		pres = platform_get_resource_byname(pdev, IORESOURCE_IO, "AP2MDM_PMIC_RESET_N");
		if (pres) {
			ap2mdm_pmic_reset_n_gpio = pres->start;
			
			if ( ap2mdm_pmic_reset_n_gpio > 0 ) {
				gpio_request(ap2mdm_pmic_reset_n_gpio, "AP2MDM_PMIC_RESET_N");
				gpio_direction_output(ap2mdm_pmic_reset_n_gpio, 0);
				pr_info("%s: Pull AP2MDM_PMIC_RESET_N(%d) to low\n", __func__, ap2mdm_pmic_reset_n_gpio);
			}
		} else {
			pr_info("%s: pres=NULL\n", __func__);
		}
		return 0;
	} else {
		pr_info("%s: mfg_mode=[%d]\n", __func__, mfg_mode);
	}
	
#else
	pr_info("%s: CONFIG_HTC_POWEROFF_MODEM_IN_OFFMODE_CHARGING not set\n", __func__);
#endif

	mdm_drv = kzalloc(sizeof(struct mdm_modem_drv), GFP_KERNEL);
	if (mdm_drv == NULL) {
		pr_err("%s: kzalloc fail.\n", __func__);
		goto alloc_err;
	}

	mdm_modem_initialize_data(pdev, p_mdm_cb);

	if (mdm_drv->ops->debug_state_changed_cb)
		mdm_drv->ops->debug_state_changed_cb(mdm_debug_on);

	register_ap2mdm_pmic_reset_n_gpio(mdm_drv->ap2mdm_pmic_reset_n_gpio);	

	gpio_request(mdm_drv->ap2mdm_status_gpio, "AP2MDM_STATUS");
	gpio_request(mdm_drv->ap2mdm_errfatal_gpio, "AP2MDM_ERRFATAL");
	
	
	
	gpio_request(mdm_drv->ap2mdm_pmic_reset_n_gpio, "AP2MDM_PMIC_RESET_N");
	gpio_request(mdm_drv->mdm2ap_status_gpio, "MDM2AP_STATUS");
	gpio_request(mdm_drv->mdm2ap_errfatal_gpio, "MDM2AP_ERRFATAL");
	gpio_request(mdm_drv->mdm2ap_hsic_ready_gpio, "MDM2AP_HSIC_READY");
	
	gpio_request(mdm_drv->ap2mdm_ipc1_gpio, "AP2MDM_IPC1");
#ifdef CONFIG_QSC_MODEM
	gpio_request(mdm_drv->mdm2ap_bootloader_gpio, "MDM2AP_BOOTLOADER");
#endif
	

	if (mdm_drv->ap2mdm_wakeup_gpio > 0)
		gpio_request(mdm_drv->ap2mdm_wakeup_gpio, "AP2MDM_WAKEUP");

#if 0
	gpio_direction_output(mdm_drv->ap2mdm_status_gpio, 1);
#endif
	gpio_direction_output(mdm_drv->ap2mdm_errfatal_gpio, 0);
	
	gpio_direction_output(mdm_drv->ap2mdm_ipc1_gpio, 0);

#ifdef CONFIG_QSC_MODEM
	
	gpio_tlmm_config(GPIO_CFG((mdm_drv->mdm2ap_bootloader_gpio),  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_input(mdm_drv->mdm2ap_bootloader_gpio);  
#endif
	

	if (mdm_drv->ap2mdm_wakeup_gpio > 0)
		gpio_direction_output(mdm_drv->ap2mdm_wakeup_gpio, 0);

	gpio_direction_input(mdm_drv->mdm2ap_status_gpio);
	gpio_direction_input(mdm_drv->mdm2ap_errfatal_gpio);

	mdm_queue = create_singlethread_workqueue("mdm_queue");
	if (!mdm_queue) {
		pr_err("%s: could not create workqueue. All mdm "
				"functionality will be disabled\n",
			__func__);
		ret = -ENOMEM;
		goto fatal_err;
	}

	mdm_sfr_queue = alloc_workqueue("mdm_sfr_queue", 0, 0);
	if (!mdm_sfr_queue) {
		pr_err("%s: could not create workqueue mdm_sfr_queue."
			" All mdm functionality will be disabled\n",
			__func__);
		ret = -ENOMEM;
		destroy_workqueue(mdm_queue);
		goto fatal_err;
	}

	
	mdm_gpio_monitor_queue = create_singlethread_workqueue("mdm_gpio_monitor_queue");
	if (!mdm_gpio_monitor_queue) {
		pr_err("%s: could not create workqueue for monitoring GPIO status \n",
			__func__);
		destroy_workqueue(mdm_gpio_monitor_queue);
	}
	

	atomic_notifier_chain_register(&panic_notifier_list, &mdm_panic_blk);
	mdm_debugfs_init();

	mdm_loaded_info();	

	
	ssr_register_subsystem(&mdm_subsystem);

	
	irq = MSM_GPIO_TO_INT(mdm_drv->mdm2ap_errfatal_gpio);
	if (irq < 0) {
		pr_err("%s: could not get MDM2AP_ERRFATAL IRQ resource. "
			"error=%d No IRQ will be generated on errfatal.",
			__func__, irq);
		goto errfatal_err;
	}
	ret = request_irq(irq, mdm_errfatal,
		IRQF_TRIGGER_RISING , "mdm errfatal", NULL);

	if (ret < 0) {
		pr_err("%s: MDM2AP_ERRFATAL IRQ#%d request failed with error=%d"
			". No IRQ will be generated on errfatal.",
			__func__, irq, ret);
		goto errfatal_err;
	}
	mdm_drv->mdm_errfatal_irq = irq;

errfatal_err:

	
	irq = MSM_GPIO_TO_INT(mdm_drv->mdm2ap_status_gpio);
	if (irq < 0) {
		pr_err("%s: could not get MDM2AP_STATUS IRQ resource. "
			"error=%d No IRQ will be generated on status change.",
			__func__, irq);
		goto status_err;
	}

	ret = request_threaded_irq(irq, NULL, mdm_status_change,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_SHARED,
		"mdm status", mdm_drv);

	if (ret < 0) {
		pr_err("%s: MDM2AP_STATUS IRQ#%d request failed with error=%d"
			". No IRQ will be generated on status change.",
			__func__, irq, ret);
		goto status_err;
	}
	mdm_drv->mdm_status_irq = irq;

status_err:
	mdm_drv->ops->power_on_mdm_cb(mdm_drv);

#ifdef CONFIG_QSC_MODEM
	
	irq = MSM_GPIO_TO_INT(mdm_drv->mdm2ap_bootloader_gpio);
	if (irq < 0) {
		pr_err("%s: could not get mdm2ap_bootloader irq resource, error=%d. Skip waiting for MDM_BOOTLOADER interrupt.",
			__func__, irq);
	}
	else
	{
		ret = request_threaded_irq(irq, NULL, mdm_in_bootloader, IRQF_TRIGGER_RISING, "mdm in bootloader", NULL);

		if (ret < 0) {
			pr_err("%s: mdm2ap_bootloader irq request failed with error=%d. Skip waiting for MDM_BOOTLOADER interrupt.",
				__func__, ret);
		}
		else
		{
			qsc_boot_after_mdm_bootloader(MDM_BOOTLOAER_GPIO_IRQ_REGISTERED);
			mdm_drv->mdm2ap_bootloader_irq = irq;
			pr_info("%s: Registered mdm2ap_bootloader irq, gpio<%d> irq<%d> ret<%d>\n"
			, __func__, mdm_drv->mdm2ap_bootloader_gpio, irq, ret);
		}
	}
#endif

	pr_info("%s: Registering mdm modem\n", __func__);
	return misc_register(&mdm_modem_misc);

fatal_err:
	gpio_free(mdm_drv->ap2mdm_status_gpio);
	gpio_free(mdm_drv->ap2mdm_errfatal_gpio);
	gpio_free(mdm_drv->ap2mdm_kpdpwr_n_gpio);
	gpio_free(mdm_drv->ap2mdm_pmic_reset_n_gpio);
	gpio_free(mdm_drv->mdm2ap_status_gpio);
	gpio_free(mdm_drv->mdm2ap_errfatal_gpio);
	gpio_free(mdm_drv->mdm2ap_hsic_ready_gpio);
	
	gpio_free(mdm_drv->ap2mdm_ipc1_gpio);
#ifdef CONFIG_QSC_MODEM
	gpio_free(mdm_drv->mdm2ap_bootloader_gpio);
#endif
	

	if (mdm_drv->ap2mdm_wakeup_gpio > 0)
		gpio_free(mdm_drv->ap2mdm_wakeup_gpio);

	kfree(mdm_drv);
	ret = -ENODEV;

alloc_err:
	return ret;
}

int mdm_common_modem_remove(struct platform_device *pdev)
{
	int ret;
#ifdef CONFIG_HTC_POWEROFF_MODEM_IN_OFFMODE_CHARGING
	
	int mfg_mode = BOARD_MFG_MODE_NORMAL;
	mfg_mode = board_mfg_mode();
	if ( mfg_mode == BOARD_MFG_MODE_OFFMODE_CHARGING ) {
		unsigned ap2mdm_pmic_reset_n_gpio = 0;
		struct resource *pres;
		pr_info("%s: BOARD_MFG_MODE_OFFMODE_CHARGING\n", __func__);
		pres = platform_get_resource_byname(pdev, IORESOURCE_IO, "AP2MDM_PMIC_RESET_N");
		if (pres) {
			ap2mdm_pmic_reset_n_gpio = pres->start;
			
			if ( ap2mdm_pmic_reset_n_gpio > 0 ) {
				gpio_free(ap2mdm_pmic_reset_n_gpio);
				pr_info("%s: gpio_free AP2MDM_PMIC_RESET_N(%d)\n", __func__, ap2mdm_pmic_reset_n_gpio);
			}
		}
		return 0;
	}
	
#endif
	gpio_free(mdm_drv->ap2mdm_status_gpio);
	gpio_free(mdm_drv->ap2mdm_errfatal_gpio);
	gpio_free(mdm_drv->ap2mdm_kpdpwr_n_gpio);
	gpio_free(mdm_drv->ap2mdm_pmic_reset_n_gpio);
	gpio_free(mdm_drv->mdm2ap_status_gpio);
	gpio_free(mdm_drv->mdm2ap_errfatal_gpio);
	gpio_free(mdm_drv->mdm2ap_hsic_ready_gpio);
	
	gpio_free(mdm_drv->ap2mdm_ipc1_gpio);
#ifdef CONFIG_QSC_MODEM
	gpio_free(mdm_drv->mdm2ap_bootloader_gpio);
#endif
	

	if (mdm_drv->ap2mdm_wakeup_gpio > 0)
		gpio_free(mdm_drv->ap2mdm_wakeup_gpio);

	kfree(mdm_drv);

	ret = misc_deregister(&mdm_modem_misc);

#ifdef CONFIG_HTC_STORE_MODEM_RESET_INFO
	mdm_subsystem_restart_properties_release();
#endif
	return ret;
}

void mdm_common_modem_shutdown(struct platform_device *pdev)
{
#ifdef CONFIG_HTC_POWEROFF_MODEM_IN_OFFMODE_CHARGING
	
	int mfg_mode = BOARD_MFG_MODE_NORMAL;
	mfg_mode = board_mfg_mode();
	if ( mfg_mode == BOARD_MFG_MODE_OFFMODE_CHARGING ) {
		pr_info("%s: BOARD_MFG_MODE_OFFMODE_CHARGING\n", __func__);
		return;
	}
	
#endif

	pr_info("%s: setting AP2MDM_STATUS low for a graceful restart\n",
		__func__);

	mdm_disable_irqs();
	
	gpio_set_value(mdm_drv->ap2mdm_status_gpio, 0);
	

	mdm_drv->ops->power_down_mdm_cb(mdm_drv);

}

int mdm_common_htc_get_mdm2ap_errfatal_level(void)
{
	int value = 0;

	if (mdm_drv != NULL && mdm_drv->mdm2ap_errfatal_gpio != 0)
		value = gpio_get_value(mdm_drv->mdm2ap_errfatal_gpio);

	pr_info("%s: %d\n", __func__, value);

	return value;
}
EXPORT_SYMBOL_GPL(mdm_common_htc_get_mdm2ap_errfatal_level);

int mdm_common_htc_get_mdm2ap_status_level(void)
{
	int value = 0;

	if (mdm_drv != NULL && mdm_drv->mdm2ap_status_gpio != 0)
		value = gpio_get_value(mdm_drv->mdm2ap_status_gpio);

	pr_info("%s: %d\n", __func__, value);

	return value;
}
EXPORT_SYMBOL_GPL(mdm_common_htc_get_mdm2ap_status_level);


