/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>

#include <linux/usb.h>
#include <linux/usb/otg.h>
#include <linux/usb/ulpi.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>
#include <linux/usb/quirks.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/usb/msm_hsusb_hw.h>
#include <linux/usb/htc_info.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include <mach/clk.h>
#include <mach/msm_xo.h>
#include <mach/msm_bus.h>
#include <mach/rpm-regulator.h>
#include <mach/cable_detect.h>
#include <mach/board.h>
#include <mach/board_htc.h>

#define MSM_USB_BASE	(motg->regs)
#define DRIVER_NAME	"msm_otg"

extern int htc_battery_set_max_input_current(int target_ma);
static int htc_otg_vbus;
int USB_disabled;
static struct msm_otg *the_msm_otg;

static int re_enable_host;
static int stop_usb_host;

enum {
    NOT_ON_AUTOBOT,
    DOCK_ON_AUTOBOT,
    HTC_MODE_RUNNING
};


enum {
	DEFAULT_STATE,
	TRY_STOP_HOST_STATE,
	STOP_HOST_STATE,
	TRY_ENABLE_HOST_STATE
};

static DEFINE_MUTEX(smwork_sem);
static DEFINE_MUTEX(notify_sem);
static void send_usb_connect_notify(struct work_struct *w)
{
	static struct t_usb_status_notifier *notifier;
	struct msm_otg *motg = container_of(w, struct msm_otg,	notifier_work);
	struct usb_otg *otg;
	if (!motg)
		return;
	otg = motg->phy.otg;

	motg->connect_type_ready = 1;
	USBH_INFO("send connect type %d\n", motg->connect_type);
	mutex_lock(&notify_sem);
	list_for_each_entry(notifier, &g_lh_usb_notifier_list, notifier_link) {
		if (notifier->func != NULL) {
			
				notifier->func(motg->connect_type);
		}
	}
	mutex_unlock(&notify_sem);
	if ((board_mfg_mode() == 5 || USB_disabled )&& motg->connect_type == CONNECT_TYPE_USB) { 
		pm_runtime_put_noidle(otg->phy->dev);
		pm_runtime_suspend(otg->phy->dev);
	}
	if (motg->chg_type == USB_CDP_CHARGER)
		htc_battery_set_max_input_current(900);
}

int htc_usb_register_notifier(struct t_usb_status_notifier *notifier)
{
	if (!notifier || !notifier->name || !notifier->func)
		return -EINVAL;

	mutex_lock(&notify_sem);
	list_add(&notifier->notifier_link,
		&g_lh_usb_notifier_list);
	mutex_unlock(&notify_sem);
	return 0;
}

int usb_is_connect_type_ready(void)
{
	if (!the_msm_otg)
		return 0;
	return the_msm_otg->connect_type_ready;
}
EXPORT_SYMBOL(usb_is_connect_type_ready);

int usb_get_connect_type(void)
{
	if (!the_msm_otg)
		return 0;
#ifdef CONFIG_MACH_VERDI_LTE
	if (the_msm_otg->connect_type == CONNECT_TYPE_USB_9V_AC)
		return CONNECT_TYPE_9V_AC;
#endif
	return the_msm_otg->connect_type;
}
EXPORT_SYMBOL(usb_get_connect_type);


static bool is_msm_otg_support_power_collapse(struct msm_otg *motg)
{
	bool ret = true;

	if (!motg->pdata->ldo_power_collapse)
		return false;

	if (get_kernel_flag() & KERNEL_FLAG_SERIAL_HSL_ENABLE &&
		(motg->pdata->ldo_power_collapse & POWER_COLLAPSE_LDO3V3)) {
		
		motg->pdata->ldo_power_collapse &= ~POWER_COLLAPSE_LDO3V3;
		USBH_INFO("%s: kernel_flag_serial_hsl_enable."
			" POWER_COLLAPSE_LDO3V3 disabled\n", __func__);
	}

	if (motg->pdata->ldo_power_collapse & POWER_COLLAPSE_LDO3V3)
		USBH_INFO("%s: POWER_COLLAPSE_LDO3V3\n", __func__);
	if (motg->pdata->ldo_power_collapse & POWER_COLLAPSE_LDO1V8)
		USBH_INFO("%s: POWER_COLLAPSE_LDO1V8\n", __func__);

	if (board_mfg_mode() == 8) {
		ret = false;
		USBH_DEBUG("%s:No. under mfgkernel\n", __func__);
	}
	return ret;
}

#define ID_TIMER_FREQ		(jiffies + msecs_to_jiffies(500))
#define ULPI_IO_TIMEOUT_USEC	(10 * 1000)
#define USB_PHY_3P3_VOL_MIN	3050000 
#define USB_PHY_3P3_VOL_MAX	3300000 
#define USB_PHY_3P3_HPM_LOAD	50000	
#define USB_PHY_3P3_LPM_LOAD	4000	

#define USB_PHY_1P8_VOL_MIN	1800000 
#define USB_PHY_1P8_VOL_MAX	1800000 
#define USB_PHY_1P8_HPM_LOAD	50000	
#define USB_PHY_1P8_LPM_LOAD	4000	

#define USB_PHY_VDD_DIG_VOL_NONE	0 
#define USB_PHY_VDD_DIG_VOL_MIN	1045000 
#define USB_PHY_VDD_DIG_VOL_MAX	1320000 

static DECLARE_COMPLETION(pmic_vbus_init);
static struct msm_otg *the_msm_otg;
static bool debug_aca_enabled;
static bool debug_bus_voting_enabled;

static struct regulator *hsusb_3p3;
static struct regulator *hsusb_1p8;
static struct regulator *hsusb_vddcx;
static struct regulator *mhl_usb_hs_switch;
static struct power_supply *psy;

static bool aca_id_turned_on;
static inline bool aca_enabled(void)
{
#ifdef CONFIG_USB_MSM_ACA
	return true;
#else
	return debug_aca_enabled;
#endif
}

static const int vdd_val[VDD_TYPE_MAX][VDD_VAL_MAX] = {
		{  
			[VDD_NONE]	= RPM_VREG_CORNER_NONE,
			[VDD_MIN]	= RPM_VREG_CORNER_NOMINAL,
			[VDD_MAX]	= RPM_VREG_CORNER_HIGH,
		},
		{ 
			[VDD_NONE]	= USB_PHY_VDD_DIG_VOL_NONE,
			[VDD_MIN]	= USB_PHY_VDD_DIG_VOL_MIN,
			[VDD_MAX]	= USB_PHY_VDD_DIG_VOL_MAX,
		},
};

static int msm_hsusb_ldo_init(struct msm_otg *motg, int init)
{
	int rc = 0;

	if (init) {
		if (motg->pdata->ldo_3v3_name)
			hsusb_3p3 = regulator_get(motg->phy.dev, motg->pdata->ldo_3v3_name);
		else
			hsusb_3p3 = devm_regulator_get(motg->phy.dev, "HSUSB_3p3");
		if (IS_ERR(hsusb_3p3)) {
			USBH_ERR("unable to get hsusb 3p3\n");
			return PTR_ERR(hsusb_3p3);
		}

		rc = regulator_set_voltage(hsusb_3p3, USB_PHY_3P3_VOL_MIN,
				USB_PHY_3P3_VOL_MAX);
		if (rc) {
			USBH_ERR("unable to set voltage level for"
					"hsusb 3p3\n");
			return rc;
		}
		if (motg->pdata->ldo_1v8_name)
			hsusb_1p8 = regulator_get(motg->phy.dev, motg->pdata->ldo_1v8_name);
		else
			hsusb_1p8 = devm_regulator_get(motg->phy.dev, "HSUSB_1p8");
		if (IS_ERR(hsusb_1p8)) {
			USBH_ERR("unable to get hsusb 1p8\n");
			rc = PTR_ERR(hsusb_1p8);
			goto put_3p3_lpm;
		}
		rc = regulator_set_voltage(hsusb_1p8, USB_PHY_1P8_VOL_MIN,
				USB_PHY_1P8_VOL_MAX);
		if (rc) {
			dev_err(motg->otg.dev, "unable to set voltage level for"
					"hsusb 1p8\n");
			goto put_1p8;
		}

		return 0;
	}

put_1p8:
	regulator_set_voltage(hsusb_1p8, 0, USB_PHY_1P8_VOL_MAX);
put_3p3_lpm:
	regulator_set_voltage(hsusb_3p3, 0, USB_PHY_3P3_VOL_MAX);
	return rc;
}

static int msm_hsusb_config_vddcx(int high)
{
	struct msm_otg *motg = the_msm_otg;
	enum usb_vdd_type vdd_type = motg->vdd_type;
	int max_vol = vdd_val[vdd_type][VDD_MAX];
	int min_vol;
	int ret;

	min_vol = vdd_val[vdd_type][!!high];
	ret = regulator_set_voltage(hsusb_vddcx, min_vol, max_vol);
	if (ret) {
		USBH_ERR("%s: unable to set the voltage for regulator "
			"HSUSB_VDDCX\n", __func__);
		return ret;
	}

	USBH_DEBUG("%s: min_vol:%d max_vol:%d\n", __func__, min_vol, max_vol);

	return ret;
}

static int msm_hsusb_ldo_enable(struct msm_otg *motg, int on)
{
	int ret = 0;

	if (IS_ERR(hsusb_1p8)) {
		USBH_ERR("%s: HSUSB_1p8 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (IS_ERR(hsusb_3p3)) {
		USBH_ERR("%s: HSUSB_3p3 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (on) {
		ret = regulator_set_optimum_mode(hsusb_1p8,
				USB_PHY_1P8_HPM_LOAD);
		if (ret < 0) {
			USBH_ERR("%s: Unable to set HPM of the regulator:"
				"HSUSB_1p8\n", __func__);
			return ret;
		} else
			USBH_INFO("%s: hsusb_1p8: %duA\n", __func__, USB_PHY_1P8_HPM_LOAD);

		ret = regulator_enable(hsusb_1p8);
		if (ret) {
			USBH_ERR("%s: unable to enable the hsusb 1p8\n",
				__func__);
			regulator_set_optimum_mode(hsusb_1p8, 0);
			return ret;
		}

		ret = regulator_set_optimum_mode(hsusb_3p3,
				USB_PHY_3P3_HPM_LOAD);
		if (ret < 0) {
			USBH_ERR("%s: Unable to set HPM of the regulator:"
				"HSUSB_3p3\n", __func__);
			regulator_set_optimum_mode(hsusb_1p8, 0);
			regulator_disable(hsusb_1p8);
			return ret;
		} else
			USBH_INFO("%s: hsusb_3p3: %duA\n", __func__, USB_PHY_3P3_HPM_LOAD);

		ret = regulator_enable(hsusb_3p3);
		if (ret) {
			USBH_ERR("%s: unable to enable the hsusb 3p3\n",
				__func__);
			regulator_set_optimum_mode(hsusb_3p3, 0);
			regulator_set_optimum_mode(hsusb_1p8, 0);
			regulator_disable(hsusb_1p8);
			return ret;
		}

	} else {
		if (motg->pdata->ldo_power_collapse & POWER_COLLAPSE_LDO1V8) {
			ret = regulator_disable(hsusb_1p8);
			if (ret) {
				USBH_ERR("%s: unable to disable the hsusb 1p8\n",
						__func__);
				return ret;
			}

			ret = regulator_set_optimum_mode(hsusb_1p8, 0);
			if (ret < 0)
				USBH_ERR("%s: Unable to set LPM of the regulator:"
						"HSUSB_1p8\n", __func__);
			else
				USBH_INFO("%s: hsusb_1p8: 0uA\n", __func__);
		}
		if (motg->pdata->ldo_power_collapse & POWER_COLLAPSE_LDO3V3) {
			ret = regulator_disable(hsusb_3p3);
			if (ret) {
				USBH_ERR("%s: unable to disable the hsusb 3p3\n",
						__func__);
				return ret;
			}
			ret = regulator_set_optimum_mode(hsusb_3p3, 0);
			if (ret < 0)
				USBH_ERR("%s: Unable to set LPM of the regulator:"
						"HSUSB_3p3\n", __func__);
			else
				USBH_INFO("%s: hsusb_3p3: 0uA\n", __func__);
		}
	}

	USBH_DEBUG("reg (%s)\n", on ? "HPM" : "LPM");
	return ret < 0 ? ret : 0;
}

static const char *state_string(enum usb_otg_state state)
{
	switch (state) {
	case OTG_STATE_A_IDLE:		return "a_idle";
	case OTG_STATE_A_WAIT_VRISE:	return "a_wait_vrise";
	case OTG_STATE_A_WAIT_BCON:	return "a_wait_bcon";
	case OTG_STATE_A_HOST:		return "a_host";
	case OTG_STATE_A_SUSPEND:	return "a_suspend";
	case OTG_STATE_A_PERIPHERAL:	return "a_peripheral";
	case OTG_STATE_A_WAIT_VFALL:	return "a_wait_vfall";
	case OTG_STATE_A_VBUS_ERR:	return "a_vbus_err";
	case OTG_STATE_B_IDLE:		return "b_idle";
	case OTG_STATE_B_SRP_INIT:	return "b_srp_init";
	case OTG_STATE_B_PERIPHERAL:	return "b_peripheral";
	case OTG_STATE_B_WAIT_ACON:	return "b_wait_acon";
	case OTG_STATE_B_HOST:		return "b_host";
	default:			return "UNDEFINED";
	}
}

static const char *chg_state_string(enum usb_chg_state state)
{
	switch (state) {
	case USB_CHG_STATE_UNDEFINED:		return "CHG_STATE_UNDEFINED";
	case USB_CHG_STATE_WAIT_FOR_DCD:	return "CHG_WAIT_FOR_DCD";
	case USB_CHG_STATE_DCD_DONE:	return "CHG_DCD_DONE";
	case USB_CHG_STATE_PRIMARY_DONE:		return "CHG_PRIMARY_DONE";
	case USB_CHG_STATE_SECONDARY_DONE:	return "CHG_SECONDARY_DONE";
	case USB_CHG_STATE_DETECTED:	return "CHG_DETECTED";
	default:			return "UNDEFINED";
	}
}

static void msm_hsusb_mhl_switch_enable(struct msm_otg *motg, bool on)
{
	struct msm_otg_platform_data *pdata = motg->pdata;

	if (!pdata->mhl_enable)
		return;

	if (!mhl_usb_hs_switch) {
		pr_err("%s: mhl_usb_hs_switch is NULL.\n", __func__);
		return;
	}

	if (on) {
		if (regulator_enable(mhl_usb_hs_switch))
			pr_err("unable to enable mhl_usb_hs_switch\n");
	} else {
		regulator_disable(mhl_usb_hs_switch);
	}
}

static int ulpi_read(struct usb_phy *phy, u32 reg)
{
	struct msm_otg *motg = container_of(phy, struct msm_otg, phy);
	int cnt = 0;

	if (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY)
		udelay(200);

	
	writel(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
	       USB_ULPI_VIEWPORT);

	
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		USBH_WARNING("ulpi_read: timeout %08x reg: 0x%x\n",
			readl(USB_ULPI_VIEWPORT), reg);
		return -ETIMEDOUT;
	}
	return ULPI_DATA_READ(readl(USB_ULPI_VIEWPORT));
}

static int ulpi_write(struct usb_phy *phy, u32 val, u32 reg)
{
	struct msm_otg *motg = container_of(phy, struct msm_otg, phy);
	int cnt = 0;

	if (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY)
		udelay(200);

	
	writel(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val),
	       USB_ULPI_VIEWPORT);

	
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		USBH_WARNING("ulpi_write: timeout reg: 0x%x ,val: 0x%x\n", reg, val);
		return -ETIMEDOUT;
	}
	return 0;
}

ssize_t otg_show_usb_phy_setting(char *buf)
{
	struct msm_otg *motg = the_msm_otg;
	unsigned length = 0;
	int i;

	for (i = 0; i <= 0x14; i++)
		length += sprintf(buf + length, "0x%x = 0x%x\n",
					i, ulpi_read(&motg->phy, i));

	for (i = 0x30; i <= 0x37; i++)
		length += sprintf(buf + length, "0x%x = 0x%x\n",
					i, ulpi_read(&motg->phy, i));

	if (motg->pdata->phy_type == SNPS_28NM_INTEGRATED_PHY) {
		for (i = 0x80; i <= 0x83; i++)
			length += sprintf(buf + length, "0x%x = 0x%x\n",
						i, ulpi_read(&motg->phy, i));
	}

	return length;
}

ssize_t otg_store_usb_phy_setting(const char *buf, size_t count)
{
	struct msm_otg *motg = the_msm_otg;
	char *token[10];
	unsigned long reg;
	unsigned long value;
	int i;

	USBH_INFO("%s\n", buf);
	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");

	i = strict_strtoul(token[0], 16, (unsigned long *)&reg);
	if (i < 0) {
		USBH_ERR("%s: reg %d\n", __func__, i);
		return 0;
	}
	i = strict_strtoul(token[1], 16, (unsigned long *)&value);
	if (i < 0) {
		USBH_ERR("%s: value %d\n", __func__, i);
		return 0;
	}
	USBH_INFO("Set 0x%02lx = 0x%02lx\n", reg, value);

	ulpi_write(&motg->phy, value, reg);

	return count;
}

static struct usb_phy_io_ops msm_otg_io_ops = {
	.read = ulpi_read,
	.write = ulpi_write,
};

static void ulpi_init(struct msm_otg *motg)
{
	struct file *fp;
	mm_segment_t fs;
	unsigned long addr, val;
	const char *delim = ",";
	const char *nextline = "\n";
	const char *colon = ":";
	char *buf, *stmp, *valtmp, *result;
	int ret;

	struct msm_otg_platform_data *pdata = motg->pdata;
	int *seq = pdata->phy_init_seq;

	fp = filp_open("/data/usb_phy.txt", O_RDONLY,0);

	if(!IS_ERR(fp)){
		buf = kmalloc(256, GFP_KERNEL);
		fs = get_fs();
		set_fs(get_ds());
		fp->f_op->read(fp,buf,255,&fp->f_pos);
		set_fs(fs);
		
		for (stmp = strsep(&buf, nextline) ; stmp != NULL ; stmp = strsep(&buf, nextline)) {
			for (valtmp = strsep(&stmp, delim) ; stmp != NULL ; valtmp = strsep(&stmp, delim)) {
				result = strsep(&valtmp, colon);
				ret = strict_strtoul(result, 16, &addr);
				result = strsep(&valtmp, colon);
				ret = strict_strtoul(result, 16, &val);
				USBH_INFO("ulpi: file write  0x%02x to 0x%02x", (int)val, (int)addr);
				ulpi_write(&motg->phy, (int)val, (int)addr);
			}
		}
		kfree(buf);
		filp_close(fp, NULL);
		return;
	}

	if (!seq)
		return;

	while (seq[0] >= 0) {
		USBH_INFO("ulpi: write 0x%02x to 0x%02x\n",
				seq[0], seq[1]);
		ulpi_write(&motg->phy, seq[0], seq[1]);
		seq += 2;
	}
}

static int msm_otg_link_clk_reset(struct msm_otg *motg, bool assert)
{
	int ret;

	if (IS_ERR(motg->clk))
		return 0;

	if (assert) {
		ret = clk_reset(motg->clk, CLK_RESET_ASSERT);
		if (ret)
			USBH_ERR("usb hs_clk assert failed\n");
	} else {
		ret = clk_reset(motg->clk, CLK_RESET_DEASSERT);
		if (ret)
			USBH_ERR("usb hs_clk deassert failed\n");
	}
	return ret;
}

static int msm_otg_phy_clk_reset(struct msm_otg *motg)
{
	int ret;

	if (IS_ERR(motg->phy_reset_clk))
		return 0;

	ret = clk_reset(motg->phy_reset_clk, CLK_RESET_ASSERT);
	if (ret) {
		USBH_ERR("usb phy clk assert failed\n");
		return ret;
	}
	usleep_range(10000, 12000);
	ret = clk_reset(motg->phy_reset_clk, CLK_RESET_DEASSERT);
	if (ret)
		USBH_ERR("usb phy clk deassert failed\n");
	return ret;
}

static int msm_otg_phy_reset(struct msm_otg *motg)
{
	u32 val;
	int ret;
	int retries;

	ret = msm_otg_link_clk_reset(motg, 1);
	if (ret)
		return ret;
	ret = msm_otg_phy_clk_reset(motg);
	if (ret)
		return ret;
	ret = msm_otg_link_clk_reset(motg, 0);
	if (ret)
		return ret;

	val = readl(USB_PORTSC) & ~PORTSC_PTS_MASK;
	writel(val | PORTSC_PTS_ULPI, USB_PORTSC);

	for (retries = 3; retries > 0; retries--) {
		ret = ulpi_write(&motg->phy, ULPI_FUNC_CTRL_SUSPENDM,
				ULPI_CLR(ULPI_FUNC_CTRL));
		if (!ret)
			break;
		ret = msm_otg_phy_clk_reset(motg);
		if (ret)
			return ret;
	}
	if (!retries)
		return -ETIMEDOUT;

	
	ret = msm_otg_phy_clk_reset(motg);
	if (ret)
		return ret;

	for (retries = 3; retries > 0; retries--) {
		ret = ulpi_read(&motg->phy, ULPI_DEBUG);
		if (ret != -ETIMEDOUT)
			break;
		ret = msm_otg_phy_clk_reset(motg);
		if (ret)
			return ret;
	}
	if (!retries)
		return -ETIMEDOUT;

	USBH_INFO("phy_reset: success\n");
	return 0;
}

#define LINK_RESET_TIMEOUT_USEC		(250 * 1000)
static int msm_otg_link_reset(struct msm_otg *motg)
{
	int cnt = 0;

	writel_relaxed(USBCMD_RESET, USB_USBCMD);
	while (cnt < LINK_RESET_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_USBCMD) & USBCMD_RESET))
			break;
		udelay(1);
		cnt++;
	}
	if (cnt >= LINK_RESET_TIMEOUT_USEC)
		return -ETIMEDOUT;

	
	writel_relaxed(0x80000000, USB_PORTSC);
	writel_relaxed(0x0, USB_AHBBURST);
	writel_relaxed(0x08, USB_AHBMODE);

	return 0;
}

static int msm_otg_reset(struct usb_phy *phy)
{
	struct msm_otg *motg = container_of(phy, struct msm_otg, phy);
	struct msm_otg_platform_data *pdata = motg->pdata;
	int ret;
	u32 val = 0;
	u32 ulpi_val = 0;
	USBH_INFO("%s\n", __func__);

	if (pdata->disable_reset_on_disconnect) {
		if (motg->reset_counter)
			return 0;
		else
			motg->reset_counter++;
	}

	if (!IS_ERR(motg->clk))
		clk_prepare_enable(motg->clk);
	ret = msm_otg_phy_reset(motg);
	if (ret) {
		USBH_ERR("phy_reset failed\n");
		return ret;
	}

	
	if (readl(USB_OTGSC) & OTGSC_IDPU)
		writel(readl(USB_OTGSC) & ~OTGSC_IDPU, USB_OTGSC);

	aca_id_turned_on = false;
	ret = msm_otg_link_reset(motg);
	if (ret) {
		dev_err(phy->dev, "link reset failed\n");
		return ret;
	}
	msleep(100);

	ulpi_init(motg);

	
	mb();

	if (!IS_ERR(motg->clk))
		clk_disable_unprepare(motg->clk);

	if (pdata->otg_control == OTG_PHY_CONTROL) {
		val = readl_relaxed(USB_OTGSC);
		if (pdata->mode == USB_OTG) {
			ulpi_val = ULPI_INT_IDGRD | ULPI_INT_SESS_VALID;
			val |= OTGSC_IDIE | OTGSC_BSVIE;
		} else if (pdata->mode == USB_PERIPHERAL) {
			ulpi_val = ULPI_INT_SESS_VALID;
			val |= OTGSC_BSVIE;
		}
		writel_relaxed(val, USB_OTGSC);
		ulpi_write(phy, ulpi_val, ULPI_USB_INT_EN_RISE);
		ulpi_write(phy, ulpi_val, ULPI_USB_INT_EN_FALL);
	} else if (pdata->otg_control == OTG_PMIC_CONTROL) {
		ulpi_write(phy, OTG_COMP_DISABLE,
			ULPI_SET(ULPI_PWR_CLK_MNG_REG));
		
		pm8xxx_usb_id_pullup(1);
	}

	return 0;
}

static const char *timer_string(int bit)
{
	switch (bit) {
	case A_WAIT_VRISE:		return "a_wait_vrise";
	case A_WAIT_VFALL:		return "a_wait_vfall";
	case B_SRP_FAIL:		return "b_srp_fail";
	case A_WAIT_BCON:		return "a_wait_bcon";
	case A_AIDL_BDIS:		return "a_aidl_bdis";
	case A_BIDL_ADIS:		return "a_bidl_adis";
	case B_ASE0_BRST:		return "b_ase0_brst";
	case A_TST_MAINT:		return "a_tst_maint";
	case B_TST_SRP:			return "b_tst_srp";
	case B_TST_CONFIG:		return "b_tst_config";
	default:			return "UNDEFINED";
	}
}

static enum hrtimer_restart msm_otg_timer_func(struct hrtimer *hrtimer)
{
	struct msm_otg *motg = container_of(hrtimer, struct msm_otg, timer);

	switch (motg->active_tmout) {
	case A_WAIT_VRISE:
		
		set_bit(A_VBUS_VLD, &motg->inputs);
		break;
	case A_TST_MAINT:
		
		set_bit(A_BUS_DROP, &motg->inputs);
		break;
	case B_TST_SRP:
		set_bit(B_BUS_REQ, &motg->inputs);
		break;
	case B_TST_CONFIG:
		clear_bit(A_CONN, &motg->inputs);
		break;
	default:
		set_bit(motg->active_tmout, &motg->tmouts);
	}

	pr_debug("expired %s timer\n", timer_string(motg->active_tmout));
	queue_work(system_nrt_wq, &motg->sm_work);
	return HRTIMER_NORESTART;
}

static void msm_otg_del_timer(struct msm_otg *motg)
{
	int bit = motg->active_tmout;

	pr_debug("deleting %s timer. remaining %lld msec\n", timer_string(bit),
			div_s64(ktime_to_us(hrtimer_get_remaining(
					&motg->timer)), 1000));
	hrtimer_cancel(&motg->timer);
	clear_bit(bit, &motg->tmouts);
}

static void msm_otg_start_timer(struct msm_otg *motg, int time, int bit)
{
	clear_bit(bit, &motg->tmouts);
	motg->active_tmout = bit;
	pr_debug("starting %s timer\n", timer_string(bit));
	hrtimer_start(&motg->timer,
			ktime_set(time / 1000, (time % 1000) * 1000000),
			HRTIMER_MODE_REL);
}

static void msm_otg_init_timer(struct msm_otg *motg)
{
	hrtimer_init(&motg->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	motg->timer.function = msm_otg_timer_func;
}

static int msm_otg_start_hnp(struct usb_otg *otg)
{
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);

	if (otg->phy->state != OTG_STATE_A_HOST) {
		pr_err("HNP can not be initiated in %s state\n",
				otg_state_string(otg->phy->state));
		return -EINVAL;
	}

	pr_debug("A-Host: HNP initiated\n");
	clear_bit(A_BUS_REQ, &motg->inputs);
	queue_work(system_nrt_wq, &motg->sm_work);
	return 0;
}

static int msm_otg_start_srp(struct usb_otg *otg)
{
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);
	u32 val;
	int ret = 0;

	if (otg->phy->state != OTG_STATE_B_IDLE) {
		pr_err("SRP can not be initiated in %s state\n",
				otg_state_string(otg->phy->state));
		ret = -EINVAL;
		goto out;
	}

	if ((jiffies - motg->b_last_se0_sess) < msecs_to_jiffies(TB_SRP_INIT)) {
		pr_debug("initial conditions of SRP are not met. Try again"
				"after some time\n");
		ret = -EAGAIN;
		goto out;
	}

	pr_debug("B-Device SRP started\n");

	ulpi_write(otg->phy, 0x03, 0x97);
	val = readl_relaxed(USB_OTGSC);
	writel_relaxed((val & ~OTGSC_INTSTS_MASK) | OTGSC_HADP, USB_OTGSC);

	
out:
	return ret;
}

static void msm_otg_host_hnp_enable(struct usb_otg *otg, bool enable)
{
	struct usb_hcd *hcd = bus_to_hcd(otg->host);
	struct usb_device *rhub = otg->host->root_hub;

	if (enable) {
		pm_runtime_disable(&rhub->dev);
		rhub->state = USB_STATE_NOTATTACHED;
		hcd->driver->bus_suspend(hcd);
		clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	} else {
		usb_remove_hcd(hcd);
		msm_otg_reset(otg->phy);
		usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
	}
}

static int msm_otg_set_suspend(struct usb_phy *phy, int suspend)
{
	struct msm_otg *motg = container_of(phy, struct msm_otg, phy);

	if (aca_enabled() || (test_bit(ID, &motg->inputs) &&
				 !test_bit(ID_A, &motg->inputs)))
		return 0;

	if (atomic_read(&motg->in_lpm) == suspend)
		return 0;

	if (suspend) {
		switch (phy->state) {
		case OTG_STATE_A_WAIT_BCON:
			if (TA_WAIT_BCON > 0)
				break;
			
		case OTG_STATE_A_HOST:
			pr_debug("host bus suspend\n");
			clear_bit(A_BUS_REQ, &motg->inputs);
			queue_work(system_nrt_wq, &motg->sm_work);
			break;
		case OTG_STATE_B_PERIPHERAL:
			pr_debug("peripheral bus suspend\n");
			if (!(motg->caps & ALLOW_LPM_ON_DEV_SUSPEND))
				break;
			set_bit(A_BUS_SUSPEND, &motg->inputs);
			queue_work(system_nrt_wq, &motg->sm_work);
			break;

		default:
			break;
		}
	} else {
		switch (phy->state) {
		case OTG_STATE_A_SUSPEND:
			
			set_bit(A_BUS_REQ, &motg->inputs);
			phy->state = OTG_STATE_A_HOST;

			
			pm_runtime_resume(phy->dev);
			break;
		case OTG_STATE_B_PERIPHERAL:
			pr_debug("peripheral bus resume\n");
			if (!(motg->caps & ALLOW_LPM_ON_DEV_SUSPEND))
				break;
			clear_bit(A_BUS_SUSPEND, &motg->inputs);
			queue_work(system_nrt_wq, &motg->sm_work);
			break;
		default:
			break;
		}
	}
	return 0;
}

#define PHY_SUSPEND_TIMEOUT_USEC	(500 * 1000)
#define PHY_RESUME_TIMEOUT_USEC	(100 * 1000)

#ifdef CONFIG_PM_SLEEP
static int msm_otg_suspend(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	struct usb_bus *bus = phy->otg->host;
	struct usb_otg *otg = motg->phy.otg;
	struct msm_otg_platform_data *pdata = motg->pdata;
	int cnt = 0;
	bool host_bus_suspend, device_bus_suspend, dcp;
	u32 phy_ctrl_val = 0, cmd_val;
	unsigned ret;
	u32 portsc;

	if (atomic_read(&motg->in_lpm))
		return 0;

	USBH_INFO("%s\n", __func__);

	disable_irq(motg->irq);
	host_bus_suspend = phy->otg->host && !test_bit(ID, &motg->inputs);
	device_bus_suspend = phy->otg->gadget && test_bit(ID, &motg->inputs) &&
		test_bit(A_BUS_SUSPEND, &motg->inputs) &&
		motg->caps & ALLOW_LPM_ON_DEV_SUSPEND;
	dcp = motg->chg_type == USB_DCP_CHARGER;

	if (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY) {
		ulpi_read(phy, 0x14);
		if (pdata->otg_control == OTG_PHY_CONTROL)
			ulpi_write(phy, 0x01, 0x30);
		ulpi_write(phy, 0x08, 0x09);
	}


	portsc = readl_relaxed(USB_PORTSC);
	if (!(portsc & PORTSC_PHCD)) {
		writel_relaxed(portsc | PORTSC_PHCD,
				USB_PORTSC);
		while (cnt < PHY_SUSPEND_TIMEOUT_USEC) {
			if (readl_relaxed(USB_PORTSC) & PORTSC_PHCD)
				break;
			udelay(1);
			cnt++;
		}
	}

	if (cnt >= PHY_SUSPEND_TIMEOUT_USEC) {
		USBH_WARNING("Unable to suspend PHY\n");
		msm_otg_reset(phy);
		enable_irq(motg->irq);
		return -ETIMEDOUT;
	}

	if (otg->phy->state == OTG_STATE_A_WAIT_BCON) {
		USBH_INFO("%s:enable the ASYNC_INTR\n", __func__);
		cmd_val = readl_relaxed(USB_USBCMD);
		if (host_bus_suspend || device_bus_suspend ||
				(motg->pdata->otg_control == OTG_PHY_CONTROL && dcp))
			cmd_val |= ASYNC_INTR_CTRL | ULPI_STP_CTRL;
		else
			cmd_val |= ULPI_STP_CTRL;
		writel_relaxed(cmd_val, USB_USBCMD);
	} else {
		
		
		writel(readl(USB_USBCMD) | ULPI_STP_CTRL, USB_USBCMD);
	}
	if (motg->caps & ALLOW_PHY_RETENTION && !host_bus_suspend &&
		!device_bus_suspend && !dcp) {
		phy_ctrl_val = readl_relaxed(USB_PHY_CTRL);
		if (motg->pdata->otg_control == OTG_PHY_CONTROL)
			
			phy_ctrl_val |=
				(PHY_IDHV_INTEN | PHY_OTGSESSVLDHV_INTEN);
		
		phy_ctrl_val &= ~(1<<10);
		writel_relaxed(phy_ctrl_val & ~PHY_RETEN, USB_PHY_CTRL);
		motg->lpm_flags |= PHY_RETENTIONED;
	}

	
	mb();
	if (!motg->pdata->core_clk_always_on_workaround) {
		clk_disable_unprepare(motg->pclk);
		clk_disable_unprepare(motg->core_clk);
	}

	
	if (!host_bus_suspend) {
		ret = msm_xo_mode_vote(motg->xo_handle, MSM_XO_MODE_OFF);
		if (ret)
			dev_err(phy->dev, "%s failed to devote for "
				"TCXO D0 buffer%d\n", __func__, ret);
		else
			motg->lpm_flags |= XO_SHUTDOWN;
	}

	if (motg->caps & ALLOW_PHY_POWER_COLLAPSE &&
			!host_bus_suspend && !dcp) {
		msm_hsusb_ldo_enable(motg, 0);
		motg->lpm_flags |= PHY_PWR_COLLAPSED;
	}

	
	if ((motg->lpm_flags & PHY_RETENTIONED) ||
	    (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY &&	!host_bus_suspend && !device_bus_suspend && !dcp)) {
		msm_hsusb_config_vddcx(0);
		msm_hsusb_mhl_switch_enable(motg, 0);
	}

	if (device_may_wakeup(phy->dev)) {
		enable_irq_wake(motg->irq);
		if (motg->pdata->pmic_id_irq)
			enable_irq_wake(motg->pdata->pmic_id_irq);
	}
	if (bus)
		clear_bit(HCD_FLAG_HW_ACCESSIBLE, &(bus_to_hcd(bus))->flags);

	atomic_set(&motg->in_lpm, 1);
	enable_irq(motg->irq);

	USBH_INFO("USB in low power mode\n");

	return 0;
}

int msm_otg_setclk(int on)
{
    if (on) {
        if (!the_msm_otg->pdata->core_clk_always_on_workaround) {
            clk_prepare_enable(the_msm_otg->core_clk);
            
        }
    }
    else {
        if (!the_msm_otg->pdata->core_clk_always_on_workaround) {
            clk_disable_unprepare(the_msm_otg->core_clk);
            
        }
    }
    return 0;
}
EXPORT_SYMBOL(msm_otg_setclk);

static int msm_otg_resume(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	struct usb_bus *bus = phy->otg->host;
	struct usb_otg *otg = motg->phy.otg;
	int cnt = 0;
	unsigned temp;
	u32 phy_ctrl_val = 0;
	unsigned ret;

	if (!atomic_read(&motg->in_lpm))
		return 0;

	USBH_INFO("%s\n", __func__);
	
	if (motg->lpm_flags & XO_SHUTDOWN) {
		ret = msm_xo_mode_vote(motg->xo_handle, MSM_XO_MODE_ON);
		if (ret)
			dev_err(phy->dev, "%s failed to vote for "
				"TCXO D0 buffer%d\n", __func__, ret);
		motg->lpm_flags &= ~XO_SHUTDOWN;
	}

	if (!motg->pdata->core_clk_always_on_workaround) {
		clk_prepare_enable(motg->core_clk);
		clk_prepare_enable(motg->pclk);
	}

	if (motg->lpm_flags & PHY_PWR_COLLAPSED) {
		msm_hsusb_ldo_enable(motg, 1);
		motg->lpm_flags &= ~PHY_PWR_COLLAPSED;
		USBH_DEBUG("exit phy power collapse...\n");
	}

	
	if (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY) {
		msm_hsusb_mhl_switch_enable(motg, 1);
		msm_hsusb_config_vddcx(1);
	}

	if (motg->lpm_flags & PHY_RETENTIONED) {
		msm_hsusb_mhl_switch_enable(motg, 1);
		msm_hsusb_config_vddcx(1);
		phy_ctrl_val = readl_relaxed(USB_PHY_CTRL);
		phy_ctrl_val |= PHY_RETEN;
		if (motg->pdata->otg_control == OTG_PHY_CONTROL)
			
			phy_ctrl_val &=
				~(PHY_IDHV_INTEN | PHY_OTGSESSVLDHV_INTEN);
		
		phy_ctrl_val |= (1<<10);
		writel_relaxed(phy_ctrl_val, USB_PHY_CTRL);
		motg->lpm_flags &= ~PHY_RETENTIONED;
	}

	temp = readl(USB_USBCMD);
	if (otg->phy->state != OTG_STATE_A_WAIT_BCON) {
		USBH_INFO("%s:disable the ASYNC_INTR\n", __func__);
		temp &= ~ASYNC_INTR_CTRL;
	}
	temp &= ~ULPI_STP_CTRL;
	writel(temp, USB_USBCMD);

	if (!(readl(USB_PORTSC) & PORTSC_PHCD))
		goto skip_phy_resume;

	writel(readl(USB_PORTSC) & ~PORTSC_PHCD, USB_PORTSC);
	while (cnt < PHY_RESUME_TIMEOUT_USEC) {
		if (!(readl(USB_PORTSC) & PORTSC_PHCD))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= PHY_RESUME_TIMEOUT_USEC) {
		USBH_ERR("Unable to resume USB."
				"Re-plugin the cable\n");
		msm_otg_reset(phy);
	}

skip_phy_resume:
	if (device_may_wakeup(phy->dev)) {
		disable_irq_wake(motg->irq);
		if (motg->pdata->pmic_id_irq)
			disable_irq_wake(motg->pdata->pmic_id_irq);
	}
	if (bus)
		set_bit(HCD_FLAG_HW_ACCESSIBLE, &(bus_to_hcd(bus))->flags);

	atomic_set(&motg->in_lpm, 0);

	if (motg->async_int) {
		motg->async_int = 0;
		enable_irq(motg->irq);
	}

	USBH_INFO("USB exited from low power mode\n");

	return 0;
}
#endif
static int msm_otg_notify_chg_type(struct msm_otg *motg)
{
	static int charger_type;
	if (charger_type == motg->chg_type)
		return 0;

	if (motg->chg_type == USB_SDP_CHARGER)
		charger_type = POWER_SUPPLY_TYPE_USB;
	else if (motg->chg_type == USB_CDP_CHARGER)
		charger_type = POWER_SUPPLY_TYPE_USB_CDP;
	else if (motg->chg_type == USB_DCP_CHARGER ||
			motg->chg_type == USB_PROPRIETARY_CHARGER)
		charger_type = POWER_SUPPLY_TYPE_USB_DCP;
	else if ((motg->chg_type == USB_ACA_DOCK_CHARGER ||
		motg->chg_type == USB_ACA_A_CHARGER ||
		motg->chg_type == USB_ACA_B_CHARGER ||
		motg->chg_type == USB_ACA_C_CHARGER))
		charger_type = POWER_SUPPLY_TYPE_USB_ACA;
	else
		charger_type = POWER_SUPPLY_TYPE_BATTERY;

	return pm8921_set_usb_power_supply_type(charger_type);
}

static int msm_otg_notify_power_supply(struct msm_otg *motg, unsigned mA)
{

	if (!psy)
		goto psy_not_supported;

	if (motg->cur_power == 0 && mA > 0) {
		
		if (power_supply_set_online(psy, true))
			goto psy_not_supported;
	} else if (motg->cur_power > 0 && mA == 0) {
		
		if (power_supply_set_online(psy, false))
			goto psy_not_supported;
		return 0;
	}
	
	if (power_supply_set_current_limit(psy, 1000*mA))
		goto psy_not_supported;

	return 0;

psy_not_supported:
	dev_dbg(motg->phy.dev, "Power Supply doesn't support USB charger\n");
	return -ENXIO;
}

static void msm_otg_notify_charger(struct msm_otg *motg, unsigned mA)
{
	struct usb_gadget *g = motg->phy.otg->gadget;

	if (g && g->is_a_peripheral)
		return;

	if ((motg->chg_type == USB_ACA_DOCK_CHARGER ||
		motg->chg_type == USB_ACA_A_CHARGER ||
		motg->chg_type == USB_ACA_B_CHARGER ||
		motg->chg_type == USB_ACA_C_CHARGER) &&
			mA > IDEV_ACA_CHG_LIMIT)
		mA = IDEV_ACA_CHG_LIMIT;

	if (msm_otg_notify_chg_type(motg))
		dev_err(motg->phy.dev,
			"Failed notifying %d charger type to PMIC\n",
							motg->chg_type);

	if (motg->cur_power == mA)
		return;

	dev_info(motg->phy.dev, "Avail curr from USB = %u\n", mA);

	if (msm_otg_notify_power_supply(motg, mA))
		pm8921_charger_vbus_draw(mA);

	motg->cur_power = mA;
}

static void msm_otg_notify_usb_attached(void)
{
	struct msm_otg *motg = the_msm_otg;

	if (motg->connect_type != CONNECT_TYPE_USB) {
		motg->connect_type = CONNECT_TYPE_USB;
		queue_work(motg->usb_wq, &motg->notifier_work);
	}

	motg->ac_detect_count = 0;
	__cancel_delayed_work(&motg->ac_detect_work);
}


static int msm_otg_set_power(struct usb_phy *phy, unsigned mA)
{
	struct msm_otg *motg = container_of(phy, struct msm_otg, phy);

	if (motg->chg_type == USB_SDP_CHARGER)
		msm_otg_notify_charger(motg, mA);

	return 0;
}

static void msm_otg_start_host(struct usb_otg *otg, int on)
{
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);
	struct msm_otg_platform_data *pdata = motg->pdata;
	struct usb_hcd *hcd;

	if (!otg->host)
		return;

	if (USB_disabled) {
		if (stop_usb_host == TRY_STOP_HOST_STATE) {
			USBH_INFO("[USB_disabled] %s(%d) to stop host \n", __func__ , on);
			stop_usb_host = STOP_HOST_STATE;
		} else {
			USBH_INFO("[USB_disabled] %s(%d) return\n", __func__ , on);
			
			if (on) {
				re_enable_host = TRY_ENABLE_HOST_STATE;
				stop_usb_host = STOP_HOST_STATE;
			}
			return;
		}
	}

	hcd = bus_to_hcd(otg->host);

	if (on) {
		USBH_DEBUG("host on\n");

		if (pdata->otg_control == OTG_PHY_CONTROL)
			ulpi_write(otg->phy, OTG_COMP_DISABLE,
				ULPI_SET(ULPI_PWR_CLK_MNG_REG));

		if (pdata->setup_gpio)
			pdata->setup_gpio(OTG_STATE_A_HOST);
		usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
	} else {
		USBH_DEBUG("host off\n");

		usb_remove_hcd(hcd);
		
		writel_relaxed(0x80000000, USB_PORTSC);

		if (pdata->setup_gpio)
			pdata->setup_gpio(OTG_STATE_UNDEFINED);

		if (pdata->otg_control == OTG_PHY_CONTROL)
			ulpi_write(otg->phy, OTG_COMP_DISABLE,
				ULPI_CLR(ULPI_PWR_CLK_MNG_REG));
	}
}

static int msm_otg_usbdev_notify(struct notifier_block *self,
			unsigned long action, void *priv)
{
	struct msm_otg *motg = container_of(self, struct msm_otg, usbdev_nb);
	struct usb_otg *otg = motg->phy.otg;
	struct usb_device *udev = priv;

	if (action == USB_BUS_ADD || action == USB_BUS_REMOVE)
		goto out;

	if (udev->bus != otg->host)
		goto out;
	if (!udev->parent || udev->parent->parent ||
			motg->chg_type == USB_ACA_DOCK_CHARGER)
		goto out;

	switch (action) {
	case USB_DEVICE_ADD:
		if (aca_enabled())
			usb_disable_autosuspend(udev);
		if (otg->phy->state == OTG_STATE_A_WAIT_BCON) {
			pr_debug("B_CONN set\n");
			set_bit(B_CONN, &motg->inputs);
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_HOST;
			if (udev->quirks & USB_QUIRK_OTG_PET)
				msm_otg_start_timer(motg, TA_TST_MAINT,
						A_TST_MAINT);
		}
		
	case USB_DEVICE_CONFIG:
		if (udev->actconfig)
			motg->mA_port = udev->actconfig->desc.bMaxPower * 2;
		else
			motg->mA_port = IUNIT;
		if (otg->phy->state == OTG_STATE_B_HOST)
			msm_otg_del_timer(motg);
		break;
	case USB_DEVICE_REMOVE:
		if ((otg->phy->state == OTG_STATE_A_HOST) ||
			(otg->phy->state == OTG_STATE_A_SUSPEND)) {
			pr_debug("B_CONN clear\n");
			clear_bit(B_CONN, &motg->inputs);
			if (udev->bus->otg_vbus_off) {
				udev->bus->otg_vbus_off = 0;
				set_bit(A_BUS_DROP, &motg->inputs);
			}
			queue_work(system_nrt_wq, &motg->sm_work);
		}
	default:
		break;
	}
	if (test_bit(ID_A, &motg->inputs))
		msm_otg_notify_charger(motg, IDEV_ACA_CHG_MAX -
				motg->mA_port);
out:
	return NOTIFY_OK;
}

static void msm_hsusb_vbus_power(struct msm_otg *motg, bool on)
{
	int ret;
	static bool vbus_is_on;

	if (vbus_is_on == on)
		return;

	if (motg->pdata->vbus_power) {
		ret = motg->pdata->vbus_power(on);
		if (!ret)
			vbus_is_on = on;
	} else {
		
		vbus_is_on = on;
	}

	if (on) {
		motg->connect_type = CONNECT_TYPE_INTERNAL;
		queue_work(motg->usb_wq, &motg->notifier_work);
	} else {
		motg->connect_type = CONNECT_TYPE_CLEAR;
		queue_work(motg->usb_wq, &motg->notifier_work);
	}
	return;
#if 0
	if (!vbus_otg) {
		pr_err("vbus_otg is NULL.");
		return;
	}

	if (on) {
		msm_otg_notify_host_mode(motg, on);
		ret = regulator_enable(vbus_otg);
		if (ret) {
			pr_err("unable to enable vbus_otg\n");
			return;
		}
		vbus_is_on = true;
	} else {
		ret = regulator_disable(vbus_otg);
		if (ret) {
			pr_err("unable to disable vbus_otg\n");
			return;
		}
		msm_otg_notify_host_mode(motg, on);
		vbus_is_on = false;
	}
#endif
}

static int msm_otg_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);
	struct usb_hcd *hcd;

	if (motg->pdata->mode == USB_PERIPHERAL) {
		USBH_INFO("Host mode is not supported\n");
		return -ENODEV;
	}

	if (!host) {
		USB_WARNING("%s: no host\n", __func__);
		if (otg->phy->state == OTG_STATE_A_HOST) {
			pm_runtime_get_sync(otg->phy->dev);
			usb_unregister_notify(&motg->usbdev_nb);
			msm_otg_start_host(otg, 0);
			msm_hsusb_vbus_power(motg, 0);
			otg->host = NULL;
			otg->phy->state = OTG_STATE_UNDEFINED;
			queue_work(system_nrt_wq, &motg->sm_work);
		} else {
			otg->host = NULL;
		}

		return 0;
	}

	hcd = bus_to_hcd(host);
	hcd->power_budget = motg->pdata->power_budget;

#ifdef CONFIG_USB_OTG
	host->otg_port = 1;
#endif
	motg->usbdev_nb.notifier_call = msm_otg_usbdev_notify;
	usb_register_notify(&motg->usbdev_nb);
	otg->host = host;
	USBH_DEBUG("host driver registered w/ tranceiver\n");

	if (motg->pdata->mode == USB_HOST || otg->gadget) {
		USB_WARNING("host only, otg->gadget exist\n");
		pm_runtime_get_sync(otg->phy->dev);
		queue_work(system_nrt_wq, &motg->sm_work);
	}

	return 0;
}

static void msm_otg_start_peripheral(struct usb_otg *otg, int on)
{
	int ret;
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);
	struct msm_otg_platform_data *pdata = motg->pdata;

	if (!otg->gadget)
		return;

	if (on) {
		USBH_DEBUG("gadget on\n");
		
		wake_lock(&motg->wlock);
		if (pdata->setup_gpio)
			pdata->setup_gpio(OTG_STATE_B_PERIPHERAL);

		
		if (motg->bus_perf_client && debug_bus_voting_enabled) {
			ret = msm_bus_scale_client_update_request(
					motg->bus_perf_client, 1);
			if (ret)
				dev_err(motg->phy.dev, "%s: Failed to vote for "
					   "bus bandwidth %d\n", __func__, ret);
		}
		usb_gadget_vbus_connect(otg->gadget);
	} else {
		USBH_DEBUG("gadget off\n");
		usb_gadget_vbus_disconnect(otg->gadget);
		
		if (motg->bus_perf_client) {
			ret = msm_bus_scale_client_update_request(
					motg->bus_perf_client, 0);
			if (ret)
				dev_err(motg->phy.dev, "%s: Failed to devote "
					   "for bus bw %d\n", __func__, ret);
		}
		if (pdata->setup_gpio)
			pdata->setup_gpio(OTG_STATE_UNDEFINED);
		
		wake_unlock(&motg->wlock);
	}

}

static void usb_disable_work(struct work_struct *w)
{
	struct msm_otg *motg = the_msm_otg;
	struct usb_phy *usb_phy = &motg->phy;
	USBH_INFO("%s\n", __func__);
	motg->chg_type = USB_DCP_CHARGER;
	motg->chg_state = USB_CHG_STATE_DETECTED;
	msm_otg_start_peripheral(usb_phy->otg, 0);
	usb_phy->state = OTG_STATE_B_IDLE;
	queue_work(system_nrt_wq, &motg->sm_work);
}

static void msm_otg_notify_usb_disabled(void)
{
	struct msm_otg *motg = the_msm_otg;
	struct usb_phy *usb_phy = &motg->phy;
	USBH_INFO("%s\n", __func__);
	usb_gadget_vbus_disconnect(usb_phy->otg->gadget);
	queue_work(system_nrt_wq, &motg->usb_disable_work);
}

static int msm_otg_set_peripheral(struct usb_otg *otg,
			struct usb_gadget *gadget)
{
	struct msm_otg *motg = container_of(otg->phy, struct msm_otg, phy);

	if (motg->pdata->mode == USB_HOST) {
		USBH_ERR("Peripheral mode is not supported\n");
		return -ENODEV;
	}

	if (!gadget) {
		USB_WARNING("%s: no gadget\n", __func__);
		if (otg->phy->state == OTG_STATE_B_PERIPHERAL) {
			pm_runtime_get_sync(otg->phy->dev);
			msm_otg_start_peripheral(otg, 0);
			otg->gadget = NULL;
			otg->phy->state = OTG_STATE_UNDEFINED;
			queue_work(system_nrt_wq, &motg->sm_work);
		} else {
			otg->gadget = NULL;
		}

		return 0;
	}
	otg->gadget = gadget;
	USBH_DEBUG("peripheral driver registered w/ tranceiver\n");

	if (motg->pdata->mode == USB_PERIPHERAL || otg->host) {
		USB_WARNING("peripheral only, otg->host exist\n");
		pm_runtime_get_sync(otg->phy->dev);
		queue_work(system_nrt_wq, &motg->sm_work);
	}

	return 0;
}

static bool msm_chg_aca_detect(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 int_sts;
	bool ret = false;

	if (!aca_enabled())
		goto out;

	if (motg->pdata->phy_type == CI_45NM_INTEGRATED_PHY)
		goto out;

	int_sts = ulpi_read(phy, 0x87);
	switch (int_sts & 0x1C) {
	case 0x08:
		if (!test_and_set_bit(ID_A, &motg->inputs)) {
			USBH_DEBUG("ID_A\n");
			motg->chg_type = USB_ACA_A_CHARGER;
			motg->chg_state = USB_CHG_STATE_DETECTED;
			clear_bit(ID_B, &motg->inputs);
			clear_bit(ID_C, &motg->inputs);
			set_bit(ID, &motg->inputs);
			ret = true;
		}
		break;
	case 0x0C:
		if (!test_and_set_bit(ID_B, &motg->inputs)) {
			USBH_DEBUG("ID_B\n");
			motg->chg_type = USB_ACA_B_CHARGER;
			motg->chg_state = USB_CHG_STATE_DETECTED;
			clear_bit(ID_A, &motg->inputs);
			clear_bit(ID_C, &motg->inputs);
			set_bit(ID, &motg->inputs);
			ret = true;
		}
		break;
	case 0x10:
		if (!test_and_set_bit(ID_C, &motg->inputs)) {
			USBH_DEBUG("ID_C\n");
			motg->chg_type = USB_ACA_C_CHARGER;
			motg->chg_state = USB_CHG_STATE_DETECTED;
			clear_bit(ID_A, &motg->inputs);
			clear_bit(ID_B, &motg->inputs);
			set_bit(ID, &motg->inputs);
			ret = true;
		}
		break;
	case 0x04:
		if (test_and_clear_bit(ID, &motg->inputs)) {
			dev_dbg(phy->dev, "ID_GND\n");
			motg->chg_type = USB_INVALID_CHARGER;
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
			clear_bit(ID_A, &motg->inputs);
			clear_bit(ID_B, &motg->inputs);
			clear_bit(ID_C, &motg->inputs);
			ret = true;
		}
		break;
	default:
		ret = test_and_clear_bit(ID_A, &motg->inputs) |
			test_and_clear_bit(ID_B, &motg->inputs) |
			test_and_clear_bit(ID_C, &motg->inputs) |
			!test_and_set_bit(ID, &motg->inputs);
		if (ret) {
			USBH_DEBUG("ID A/B/C/GND is no more\n");
			motg->chg_type = USB_INVALID_CHARGER;
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
		}
	}
out:
	return ret;
}

static void msm_chg_enable_aca_det(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;

	if (!aca_enabled())
		return;

	switch (motg->pdata->phy_type) {
	case SNPS_28NM_INTEGRATED_PHY:
		
		writel_relaxed(readl_relaxed(USB_OTGSC) & ~(OTGSC_IDPU |
				OTGSC_IDIE), USB_OTGSC);
		ulpi_write(phy, 0x01, 0x0C);
		ulpi_write(phy, 0x10, 0x0F);
		ulpi_write(phy, 0x10, 0x12);
		
		pm8xxx_usb_id_pullup(0);
		
		ulpi_write(phy, 0x20, 0x85);
		aca_id_turned_on = true;
		break;
	default:
		break;
	}
}

static void msm_chg_enable_aca_intr(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;

	if (!aca_enabled())
		return;

	switch (motg->pdata->phy_type) {
	case SNPS_28NM_INTEGRATED_PHY:
		
		ulpi_write(phy, 0x01, 0x94);
		break;
	default:
		break;
	}
}

static void msm_chg_disable_aca_intr(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;

	if (!aca_enabled())
		return;

	switch (motg->pdata->phy_type) {
	case SNPS_28NM_INTEGRATED_PHY:
		ulpi_write(phy, 0x01, 0x95);
		break;
	default:
		break;
	}
}

static bool msm_chg_check_aca_intr(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	bool ret = false;

	if (!aca_enabled())
		return ret;

	switch (motg->pdata->phy_type) {
	case SNPS_28NM_INTEGRATED_PHY:
		if (ulpi_read(phy, 0x91) & 1) {
			USBH_DEBUG("RID change\n");
			ulpi_write(phy, 0x01, 0x92);
			ret = msm_chg_aca_detect(motg);
		}
	default:
		break;
	}
	return ret;
}

static void msm_otg_id_timer_func(unsigned long data)
{
	struct msm_otg *motg = (struct msm_otg *) data;

	if (!aca_enabled())
		return;

	if (atomic_read(&motg->in_lpm)) {
		dev_dbg(motg->phy.dev, "timer: in lpm\n");
		return;
	}

	if (motg->phy.state == OTG_STATE_A_SUSPEND)
		goto out;

	if (msm_chg_check_aca_intr(motg)) {
		dev_dbg(motg->phy.dev, "timer: aca work\n");
		queue_work(system_nrt_wq, &motg->sm_work);
	}

out:
	if (!test_bit(ID, &motg->inputs) || test_bit(ID_A, &motg->inputs))
		mod_timer(&motg->id_timer, ID_TIMER_FREQ);
}

static bool msm_chg_check_secondary_det(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;
	bool ret = false;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		ret = chg_det & (1 << 4);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x87);
		ret = chg_det & 1;
		break;
	default:
		break;
	}
	return ret;
}

static void msm_chg_enable_secondary_det(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		
		chg_det |= ~(1 << 1);
		ulpi_write(phy, chg_det, 0x34);
		udelay(20);
		
		chg_det &= ~(1 << 3);
		ulpi_write(phy, chg_det, 0x34);
		
		chg_det &= ~(1 << 2);
		ulpi_write(phy, chg_det, 0x34);
		
		chg_det &= ~(1 << 1);
		ulpi_write(phy, chg_det, 0x34);
		udelay(20);
		
		chg_det &= ~(1 << 0);
		ulpi_write(phy, chg_det, 0x34);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		ulpi_write(phy, 0x8, 0x85);
		ulpi_write(phy, 0x2, 0x85);
		ulpi_write(phy, 0x1, 0x85);
		break;
	default:
		break;
	}
}

static bool msm_chg_check_primary_det(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;
	bool ret = false;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		ret = chg_det & (1 << 4);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x87);
		ret = chg_det & 1;
		
		ulpi_write(phy, 0x3, 0x86);
		msleep(20);
		break;
	default:
		break;
	}
	return ret;
}

static void msm_chg_enable_primary_det(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		
		chg_det &= ~(1 << 0);
		ulpi_write(phy, chg_det, 0x34);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		ulpi_write(phy, 0x2, 0x85);
		ulpi_write(phy, 0x1, 0x85);
		break;
	default:
		break;
	}
}

static bool msm_chg_check_dcd(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 line_state;
	bool ret = false;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		line_state = ulpi_read(phy, 0x15);
		ret = !(line_state & 1);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		line_state = ulpi_read(phy, 0x87);
		ret = line_state & 2;
		break;
	default:
		break;
	}
	return ret;
}

static void msm_chg_disable_dcd(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		chg_det &= ~(1 << 5);
		ulpi_write(phy, chg_det, 0x34);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		ulpi_write(phy, 0x10, 0x86);
		break;
	default:
		break;
	}
}

static void msm_chg_enable_dcd(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 chg_det;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		
		chg_det |= (1 << 5);
		ulpi_write(phy, chg_det, 0x34);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		
		ulpi_write(phy, 0x10, 0x85);
		break;
	default:
		break;
	}
}

static void msm_chg_block_on(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 func_ctrl, chg_det;

	
	func_ctrl = ulpi_read(phy, ULPI_FUNC_CTRL);
	func_ctrl &= ~ULPI_FUNC_CTRL_OPMODE_MASK;
	func_ctrl |= ULPI_FUNC_CTRL_OPMODE_NONDRIVING;
	ulpi_write(phy, func_ctrl, ULPI_FUNC_CTRL);

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		
		chg_det &= ~(1 << 3);
		ulpi_write(phy, chg_det, 0x34);
		
		chg_det &= ~(1 << 1);
		ulpi_write(phy, chg_det, 0x34);
		udelay(20);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		
		ulpi_write(phy, 0x1F, 0x86);
		
		ulpi_write(phy, 0x1F, 0x92);
		ulpi_write(phy, 0x1F, 0x95);
		udelay(100);
		break;
	default:
		break;
	}
}

static void msm_chg_block_off(struct msm_otg *motg)
{
	struct usb_phy *phy = &motg->phy;
	u32 func_ctrl, chg_det;

	switch (motg->pdata->phy_type) {
	case CI_45NM_INTEGRATED_PHY:
		chg_det = ulpi_read(phy, 0x34);
		
		chg_det |= ~(1 << 1);
		ulpi_write(phy, chg_det, 0x34);
		break;
	case SNPS_28NM_INTEGRATED_PHY:
		
		ulpi_write(phy, 0x3F, 0x86);
		
		ulpi_write(phy, 0x1F, 0x92);
		ulpi_write(phy, 0x1F, 0x95);
		break;
	default:
		break;
	}

	
	func_ctrl = ulpi_read(phy, ULPI_FUNC_CTRL);
	func_ctrl &= ~ULPI_FUNC_CTRL_OPMODE_MASK;
	func_ctrl |= ULPI_FUNC_CTRL_OPMODE_NORMAL;
	ulpi_write(phy, func_ctrl, ULPI_FUNC_CTRL);
}

static const char *chg_to_string(enum usb_chg_type chg_type)
{
	switch (chg_type) {
	case USB_SDP_CHARGER:		return "USB_SDP_CHARGER";
	case USB_DCP_CHARGER:		return "USB_DCP_CHARGER";
	case USB_CDP_CHARGER:		return "USB_CDP_CHARGER";
	case USB_ACA_A_CHARGER:		return "USB_ACA_A_CHARGER";
	case USB_ACA_B_CHARGER:		return "USB_ACA_B_CHARGER";
	case USB_ACA_C_CHARGER:		return "USB_ACA_C_CHARGER";
	case USB_ACA_DOCK_CHARGER:	return "USB_ACA_DOCK_CHARGER";
	case USB_PROPRIETARY_CHARGER:	return "USB_PROPRIETARY_CHARGER";
	default:			return "INVALID_CHARGER";
	}
}

#define MSM_CHG_DCD_POLL_TIME		(100 * HZ/1000) 
#define MSM_CHG_DCD_MAX_RETRIES		6 
#define MSM_CHG_PRIMARY_DET_TIME	(50 * HZ/1000) 
#define MSM_CHG_SECONDARY_DET_TIME	(50 * HZ/1000) 
static void msm_chg_detect_work(struct work_struct *w)
{
	struct msm_otg *motg = container_of(w, struct msm_otg, chg_work.work);
	struct usb_otg *otg = motg->phy.otg;
	bool is_dcd = false, tmout, vout, is_aca;
	u32 line_state, dm_vlgc;
	unsigned long delay;

	USBH_INFO("%s: state:%s\n", __func__,
		chg_state_string(motg->chg_state));
	if (otg->phy->state >= OTG_STATE_A_IDLE) {
		motg->chg_state = USB_CHG_STATE_UNDEFINED;
		USBH_INFO("%s: usb host, charger state:%s\n", __func__, chg_state_string(motg->chg_state));
		if (motg->connect_type != CONNECT_TYPE_NONE) {
			motg->connect_type = CONNECT_TYPE_NONE;
			queue_work(motg->usb_wq, &motg->notifier_work);
		}
		return;
	}
	switch (motg->chg_state) {
	case USB_CHG_STATE_UNDEFINED:
		msm_chg_block_on(motg);
		if (motg->pdata->enable_dcd)
			msm_chg_enable_dcd(motg);
		msm_chg_enable_aca_det(motg);
		motg->chg_state = USB_CHG_STATE_WAIT_FOR_DCD;
		motg->dcd_retries = 0;
		delay = MSM_CHG_DCD_POLL_TIME;
		break;
	case USB_CHG_STATE_WAIT_FOR_DCD:
		is_aca = msm_chg_aca_detect(motg);
		if (is_aca) {
			if (test_bit(ID_A, &motg->inputs)) {
				motg->chg_state = USB_CHG_STATE_WAIT_FOR_DCD;
			} else {
				delay = 0;
				break;
			}
		}
		if (motg->pdata->enable_dcd)
			is_dcd = msm_chg_check_dcd(motg);
		tmout = ++motg->dcd_retries == MSM_CHG_DCD_MAX_RETRIES;
		if (is_dcd || tmout) {
			if (motg->pdata->enable_dcd)
				msm_chg_disable_dcd(motg);
			msm_chg_enable_primary_det(motg);
			delay = MSM_CHG_PRIMARY_DET_TIME;
			motg->chg_state = USB_CHG_STATE_DCD_DONE;
		} else {
			delay = MSM_CHG_DCD_POLL_TIME;
		}
		break;
	case USB_CHG_STATE_DCD_DONE:
		vout = msm_chg_check_primary_det(motg);
		line_state = readl_relaxed(USB_PORTSC) & PORTSC_LS;
		dm_vlgc = line_state & PORTSC_LS_DM;
		if (vout && !dm_vlgc) { 
			if (test_bit(ID_A, &motg->inputs)) {
				motg->chg_type = USB_ACA_DOCK_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_UNKNOWN;
				delay = 0;
				break;
			}
			if (line_state) { 
				motg->chg_type = USB_PROPRIETARY_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_AC;
				USBH_INFO("DP > VLGC\n");
				delay = 0;
			} else {
				msm_chg_enable_secondary_det(motg);
				delay = MSM_CHG_SECONDARY_DET_TIME;
				motg->chg_state = USB_CHG_STATE_PRIMARY_DONE;
			}
		} else { 
			if (test_bit(ID_A, &motg->inputs)) {
				motg->chg_type = USB_ACA_A_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_UNKNOWN;
				delay = 0;
				break;
			}

			if (line_state) {
				motg->chg_type = USB_PROPRIETARY_CHARGER;
				motg->connect_type = CONNECT_TYPE_AC;
				USBH_INFO("DP > VLGC or/and DM > VLGC\n");
			} else {
				motg->chg_type = USB_SDP_CHARGER;
				motg->connect_type = CONNECT_TYPE_UNKNOWN;
			}

			motg->chg_state = USB_CHG_STATE_DETECTED;
			delay = 0;
		}
		break;
	case USB_CHG_STATE_PRIMARY_DONE:
		vout = msm_chg_check_secondary_det(motg);
		if (vout) {
			motg->chg_type = USB_DCP_CHARGER;
			motg->connect_type = CONNECT_TYPE_AC;
		} else {
			motg->chg_type = USB_CDP_CHARGER;
			motg->connect_type = CONNECT_TYPE_USB;
		}
		motg->chg_state = USB_CHG_STATE_SECONDARY_DONE;
		
	case USB_CHG_STATE_SECONDARY_DONE:
		motg->chg_state = USB_CHG_STATE_DETECTED;
	case USB_CHG_STATE_DETECTED:
		msm_chg_block_off(motg);
		msm_chg_enable_aca_det(motg);
		udelay(100);
		msm_chg_enable_aca_intr(motg);
		USBH_INFO("chg_type = %s\n",
			chg_to_string(motg->chg_type));
		queue_work(system_nrt_wq, &motg->sm_work);
		queue_work(motg->usb_wq, &motg->notifier_work);
		return;
	default:
		return;
	}

	queue_delayed_work(system_nrt_wq, &motg->chg_work, delay);
}

static void msm_otg_init_sm(struct msm_otg *motg)
{
	struct msm_otg_platform_data *pdata = motg->pdata;
	u32 otgsc = readl(USB_OTGSC);

	switch (pdata->mode) {
	case USB_OTG:
		if (pdata->otg_control == OTG_USER_CONTROL) {
			if (pdata->default_mode == USB_HOST) {
				clear_bit(ID, &motg->inputs);
			} else if (pdata->default_mode == USB_PERIPHERAL) {
				set_bit(ID, &motg->inputs);
				set_bit(B_SESS_VLD, &motg->inputs);
			} else {
				set_bit(ID, &motg->inputs);
				clear_bit(B_SESS_VLD, &motg->inputs);
			}
		} else if (pdata->otg_control == OTG_PHY_CONTROL) {
			if (otgsc & OTGSC_ID) {
				set_bit(ID, &motg->inputs);
			} else {
				clear_bit(ID, &motg->inputs);
				set_bit(A_BUS_REQ, &motg->inputs);
			}
			if (otgsc & OTGSC_BSV)
				set_bit(B_SESS_VLD, &motg->inputs);
			else
				clear_bit(B_SESS_VLD, &motg->inputs);
		} else if (pdata->otg_control == OTG_PMIC_CONTROL) {
			if (pdata->pmic_id_irq) {
				unsigned long flags;
				local_irq_save(flags);
				if (irq_read_line(pdata->pmic_id_irq))
					set_bit(ID, &motg->inputs);
				else
					clear_bit(ID, &motg->inputs);
				local_irq_restore(flags);
			} else
				set_bit(ID, &motg->inputs);

			wait_for_completion(&pmic_vbus_init);
		}
		break;
	case USB_HOST:
		clear_bit(ID, &motg->inputs);
		break;
	case USB_PERIPHERAL:
		set_bit(ID, &motg->inputs);
		if (pdata->otg_control == OTG_PHY_CONTROL) {
			if (otgsc & OTGSC_BSV)
				set_bit(B_SESS_VLD, &motg->inputs);
			else
				clear_bit(B_SESS_VLD, &motg->inputs);
		} else if (pdata->otg_control == OTG_PMIC_CONTROL) {
			wait_for_completion(&pmic_vbus_init);
		}
		break;
	default:
		break;
	}

	if (test_bit(B_SESS_VLD, &motg->inputs)) {
		if (motg->pdata->usb_uart_switch)
			motg->pdata->usb_uart_switch(0);
	}
}

static void msm_otg_sm_work(struct work_struct *w)
{
	struct msm_otg *motg = container_of(w, struct msm_otg, sm_work);
	struct usb_otg *otg = motg->phy.otg;
	bool work = 0, srp_reqd;
	USBH_INFO("%s: state:%s bit:0x%08x\n", __func__,
		state_string(motg->phy.state), (unsigned) motg->inputs);

	if (!motg->phy.dev) {
		USB_ERR("%s: otg->dev was null. state: %d\n",
						__func__, motg->phy.state);
		return;
	}
	mutex_lock(&smwork_sem);
	pm_runtime_resume(otg->phy->dev);
	pr_debug("%s work\n", otg_state_string(otg->phy->state));
	switch (otg->phy->state) {
	case OTG_STATE_UNDEFINED:
		dev_dbg(otg->dev, "OTG_STATE_UNDEFINED state\n");
		msm_otg_reset(otg->phy);
		msm_otg_init_sm(motg);
		psy = power_supply_get_by_name("usb");
		if (!psy)
			pr_err("couldn't get usb power supply\n");
		otg->phy->state = OTG_STATE_B_IDLE;
		if (!test_bit(B_SESS_VLD, &motg->inputs) &&
				test_bit(ID, &motg->inputs)) {
			pm_runtime_put_noidle(otg->phy->dev);
			pm_runtime_suspend(otg->phy->dev);
			break;
		}
		
	case OTG_STATE_B_IDLE:
		dev_dbg(otg->dev, "OTG_STATE_B_IDLE state\n");
		if ((!test_bit(ID, &motg->inputs) ||
				test_bit(ID_A, &motg->inputs)) && otg->host) {
			USBH_INFO("!id || id_a\n");
			clear_bit(B_BUS_REQ, &motg->inputs);
			set_bit(A_BUS_REQ, &motg->inputs);
			otg->phy->state = OTG_STATE_A_IDLE;
			if (motg->connect_type != CONNECT_TYPE_NONE) {
				motg->connect_type = CONNECT_TYPE_NONE;
				queue_work(motg->usb_wq, &motg->notifier_work);
			}
			work = 1;
		} else if (test_bit(B_SESS_VLD, &motg->inputs)) {
			USBH_INFO("b_sess_vld\n");
			switch (motg->chg_state) {
			case USB_CHG_STATE_UNDEFINED:
				msm_chg_detect_work(&motg->chg_work.work);
				break;
			case USB_CHG_STATE_DETECTED:
				switch (motg->chg_type) {
				case USB_DCP_CHARGER:
					
					ulpi_write(otg->phy, 0x2, 0x85);
					
				case USB_PROPRIETARY_CHARGER:
					msm_otg_notify_charger(motg,
							IDEV_CHG_MAX);
					if (motg->reset_phy_before_lpm)
						msm_otg_reset(otg->phy);
					pm_runtime_put_noidle(otg->phy->dev);
					pm_runtime_suspend(otg->phy->dev);
					break;
				case USB_ACA_B_CHARGER:
					msm_otg_notify_charger(motg,
							IDEV_ACA_CHG_MAX);
					break;
				case USB_CDP_CHARGER:
					msm_otg_notify_charger(motg,
							IDEV_CHG_MAX);
					msm_otg_start_peripheral(otg, 1);
					otg->phy->state =
						OTG_STATE_B_PERIPHERAL;
					break;
				case USB_ACA_C_CHARGER:
					msm_otg_notify_charger(motg,
							IDEV_ACA_CHG_MAX);
					msm_otg_start_peripheral(otg, 1);
					otg->phy->state =
						OTG_STATE_B_PERIPHERAL;
					break;
				case USB_SDP_CHARGER:
					
					ulpi_write(otg->phy, 0x3, 0x86);
					msm_otg_notify_charger(motg, IDEV_CHG_MIN);
					msm_otg_start_peripheral(otg, 1);
					otg->phy->state = OTG_STATE_B_PERIPHERAL;
					motg->ac_detect_count = 0;
					queue_delayed_work(system_nrt_wq, &motg->ac_detect_work, 2 * HZ);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		} else if (test_bit(B_BUS_REQ, &motg->inputs)) {
			USBH_INFO("b_sess_end && b_bus_req\n");
			if (msm_otg_start_srp(otg) < 0) {
				clear_bit(B_BUS_REQ, &motg->inputs);
				work = 1;
				break;
			}
			otg->phy->state = OTG_STATE_B_SRP_INIT;
			msm_otg_start_timer(motg, TB_SRP_FAIL, B_SRP_FAIL);
			break;
		} else {
			pr_debug("chg_work cancel");
			USBH_INFO("!b_sess_vld && id\n");
			cancel_delayed_work_sync(&motg->chg_work);
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
			motg->chg_type = USB_INVALID_CHARGER;
			msm_otg_notify_charger(motg, 0);
			msm_otg_reset(otg->phy);

			if (motg->connect_type != CONNECT_TYPE_NONE) {
				motg->connect_type = CONNECT_TYPE_NONE;
				queue_work(motg->usb_wq, &motg->notifier_work);
			}

			pm_runtime_put_noidle(otg->phy->dev);
			pm_runtime_suspend(otg->phy->dev);
		}
		break;
	case OTG_STATE_B_SRP_INIT:
		if (!test_bit(ID, &motg->inputs) ||
				test_bit(ID_A, &motg->inputs) ||
				test_bit(ID_C, &motg->inputs) ||
				(test_bit(B_SESS_VLD, &motg->inputs) &&
				!test_bit(ID_B, &motg->inputs))) {
			USBH_INFO("!id || id_a/c || b_sess_vld+!id_b\n");
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_B_IDLE;
			ulpi_write(otg->phy, 0x0, 0x98);
			work = 1;
		} else if (test_bit(B_SRP_FAIL, &motg->tmouts)) {
			USBH_INFO("b_srp_fail\n");
			pr_info("A-device did not respond to SRP\n");
			clear_bit(B_BUS_REQ, &motg->inputs);
			clear_bit(B_SRP_FAIL, &motg->tmouts);
			otg_send_event(OTG_EVENT_NO_RESP_FOR_SRP);
			ulpi_write(otg->phy, 0x0, 0x98);
			otg->phy->state = OTG_STATE_B_IDLE;
			motg->b_last_se0_sess = jiffies;
			work = 1;
		}
		break;
	case OTG_STATE_B_PERIPHERAL:
		dev_dbg(otg->dev, "OTG_STATE_B_PERIPHERAL state\n");
		if (!test_bit(ID, &motg->inputs) ||
				test_bit(ID_A, &motg->inputs) ||
				test_bit(ID_B, &motg->inputs) ||
				!test_bit(B_SESS_VLD, &motg->inputs)) {
			if (motg->connect_type != CONNECT_TYPE_NONE) {
				motg->connect_type = CONNECT_TYPE_NONE;
				queue_work(motg->usb_wq, &motg->notifier_work);
			}

			if (check_htc_mode_status() != NOT_ON_AUTOBOT) {
				htc_mode_enable(0);
				android_switch_default();
			}
			USBH_INFO("!id  || id_a/b || !b_sess_vld\n");
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
			motg->chg_type = USB_INVALID_CHARGER;
			msm_otg_notify_charger(motg, 0);
			srp_reqd = otg->gadget->otg_srp_reqd;
			msm_otg_start_peripheral(otg, 0);
			if (test_bit(ID_B, &motg->inputs))
				clear_bit(ID_B, &motg->inputs);
			clear_bit(B_BUS_REQ, &motg->inputs);
			otg->phy->state = OTG_STATE_B_IDLE;
			motg->ac_detect_count = 0;
			cancel_delayed_work_sync(&motg->ac_detect_work);
			motg->b_last_se0_sess = jiffies;
			if (srp_reqd)
				msm_otg_start_timer(motg,
					TB_TST_SRP, B_TST_SRP);
			else
				work = 1;
		} else if (test_bit(B_BUS_REQ, &motg->inputs) &&
				otg->gadget->b_hnp_enable &&
				test_bit(A_BUS_SUSPEND, &motg->inputs)) {
			USBH_INFO("b_bus_req && b_hnp_en && a_bus_suspend\n");
			msm_otg_start_timer(motg, TB_ASE0_BRST, B_ASE0_BRST);
			udelay(1000);
			msm_otg_start_peripheral(otg, 0);
			otg->phy->state = OTG_STATE_B_WAIT_ACON;
			otg->host->is_b_host = 1;
			msm_otg_start_host(otg, 1);
		} else if (test_bit(A_BUS_SUSPEND, &motg->inputs) &&
				   test_bit(B_SESS_VLD, &motg->inputs)) {
			pr_debug("a_bus_suspend && b_sess_vld\n");
			if (motg->caps & ALLOW_LPM_ON_DEV_SUSPEND) {
				pm_runtime_put_noidle(otg->phy->dev);
				pm_runtime_suspend(otg->phy->dev);
			}
		} else if (test_bit(ID_C, &motg->inputs)) {
			USBH_INFO("id_c\n");
			msm_otg_notify_charger(motg, IDEV_ACA_CHG_MAX);
		} else if (test_bit(B_SESS_VLD, &motg->inputs)) {
			
			if (motg->chg_type == USB_DCP_CHARGER ||
				(motg->chg_type == USB_SDP_CHARGER &&
				 USB_disabled)
				) {
				msm_otg_start_peripheral(otg, 0);
				otg->phy->state = OTG_STATE_B_IDLE;
				work = 1;
				motg->ac_detect_count = 0;
				cancel_delayed_work_sync(&motg->ac_detect_work);
			} else
				USBH_DEBUG("do nothing !!!\n");
		} else
			USBH_DEBUG("do nothing !!\n");
		break;
	case OTG_STATE_B_WAIT_ACON:
		if (!test_bit(ID, &motg->inputs) ||
				test_bit(ID_A, &motg->inputs) ||
				test_bit(ID_B, &motg->inputs) ||
				!test_bit(B_SESS_VLD, &motg->inputs)) {
			USBH_INFO("!id || id_a/b || !b_sess_vld\n");
			msm_otg_del_timer(motg);
			msm_otg_start_host(otg, 0);
			otg->host->is_b_host = 0;

			clear_bit(B_BUS_REQ, &motg->inputs);
			clear_bit(A_BUS_SUSPEND, &motg->inputs);
			motg->b_last_se0_sess = jiffies;
			otg->phy->state = OTG_STATE_B_IDLE;
			msm_otg_reset(otg->phy);
			work = 1;
		} else if (test_bit(A_CONN, &motg->inputs)) {
			USBH_INFO("a_conn\n");
			clear_bit(A_BUS_SUSPEND, &motg->inputs);
			otg->phy->state = OTG_STATE_B_HOST;
			msm_otg_start_timer(motg, TB_TST_CONFIG,
						B_TST_CONFIG);
		} else if (test_bit(B_ASE0_BRST, &motg->tmouts)) {
			USBH_INFO("b_ase0_brst_tmout\n");
			pr_info("B HNP fail:No response from A device\n");
			msm_otg_start_host(otg, 0);
			msm_otg_reset(otg->phy);
			otg->host->is_b_host = 0;
			clear_bit(B_ASE0_BRST, &motg->tmouts);
			clear_bit(A_BUS_SUSPEND, &motg->inputs);
			clear_bit(B_BUS_REQ, &motg->inputs);
			otg_send_event(OTG_EVENT_HNP_FAILED);
			otg->phy->state = OTG_STATE_B_IDLE;
			work = 1;
		} else if (test_bit(ID_C, &motg->inputs)) {
			msm_otg_notify_charger(motg, IDEV_ACA_CHG_MAX);
		}
		break;
	case OTG_STATE_B_HOST:
		if (!test_bit(B_BUS_REQ, &motg->inputs) ||
				!test_bit(A_CONN, &motg->inputs) ||
				!test_bit(B_SESS_VLD, &motg->inputs)) {
			USBH_INFO("!b_bus_req || !a_conn || !b_sess_vld\n");
			clear_bit(A_CONN, &motg->inputs);
			clear_bit(B_BUS_REQ, &motg->inputs);
			msm_otg_start_host(otg, 0);
			otg->host->is_b_host = 0;
			otg->phy->state = OTG_STATE_B_IDLE;
			msm_otg_reset(otg->phy);
			work = 1;
		} else if (test_bit(ID_C, &motg->inputs)) {
			msm_otg_notify_charger(motg, IDEV_ACA_CHG_MAX);
		}
		break;
	case OTG_STATE_A_IDLE:
		otg->default_a = 1;
		if (test_bit(ID, &motg->inputs) &&
			!test_bit(ID_A, &motg->inputs)) {
			USBH_INFO("id && !id_a\n");
			otg->default_a = 0;
			clear_bit(A_BUS_DROP, &motg->inputs);
			otg->phy->state = OTG_STATE_B_IDLE;
			del_timer_sync(&motg->id_timer);
			msm_otg_link_reset(motg);
			msm_chg_enable_aca_intr(motg);
			msm_otg_notify_charger(motg, 0);
			work = 1;
		} else if (!test_bit(A_BUS_DROP, &motg->inputs) &&
				(test_bit(A_SRP_DET, &motg->inputs) ||
				 test_bit(A_BUS_REQ, &motg->inputs))) {
			USBH_INFO("!a_bus_drop && (a_srp_det || a_bus_req)\n");

			clear_bit(A_SRP_DET, &motg->inputs);
			
			writel_relaxed((readl_relaxed(USB_OTGSC) &
					~OTGSC_INTSTS_MASK) &
					~OTGSC_DPIE, USB_OTGSC);

			otg->phy->state = OTG_STATE_A_WAIT_VRISE;
			usleep_range(10000, 12000);
			
			if (test_bit(ID_A, &motg->inputs))
				msm_otg_notify_charger(motg, 0);
			else
				msm_hsusb_vbus_power(motg, 1);
			msm_otg_start_timer(motg, TA_WAIT_VRISE, A_WAIT_VRISE);
		} else {
			USBH_INFO("No session requested\n");
			clear_bit(A_BUS_DROP, &motg->inputs);
			if (test_bit(ID_A, &motg->inputs)) {
					msm_otg_notify_charger(motg,
							IDEV_ACA_CHG_MAX);
			} else if (!test_bit(ID, &motg->inputs)) {
				msm_otg_notify_charger(motg, 0);
				writel_relaxed(0x13, USB_USBMODE);
				writel_relaxed((readl_relaxed(USB_OTGSC) &
						~OTGSC_INTSTS_MASK) |
						OTGSC_DPIE, USB_OTGSC);
				mb();
			}
		}
		break;
	case OTG_STATE_A_WAIT_VRISE:
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs) ||
				test_bit(A_WAIT_VRISE, &motg->tmouts)) {
			USBH_INFO("id || a_bus_drop || a_wait_vrise_tmout\n");
			clear_bit(A_BUS_REQ, &motg->inputs);
			msm_otg_del_timer(motg);
			msm_hsusb_vbus_power(motg, 0);
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
		} else if (test_bit(A_VBUS_VLD, &motg->inputs)) {
			USBH_INFO("a_vbus_vld\n");
			otg->phy->state = OTG_STATE_A_WAIT_BCON;
			if (TA_WAIT_BCON > 0)
				msm_otg_start_timer(motg, TA_WAIT_BCON,
					A_WAIT_BCON);
			msm_otg_start_host(otg, 1);
			msm_chg_enable_aca_det(motg);
			msm_chg_disable_aca_intr(motg);
			mod_timer(&motg->id_timer, ID_TIMER_FREQ);
			if (msm_chg_check_aca_intr(motg))
				work = 1;
		}
		break;
	case OTG_STATE_A_WAIT_BCON:
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs) ||
				test_bit(A_WAIT_BCON, &motg->tmouts)) {
			USBH_INFO("(id && id_a/b/c) || a_bus_drop ||"
					"a_wait_bcon_tmout\n");
			if (test_bit(A_WAIT_BCON, &motg->tmouts)) {
				pr_info("Device No Response\n");
				otg_send_event(OTG_EVENT_DEV_CONN_TMOUT);
			}
			msm_otg_del_timer(motg);
			clear_bit(A_BUS_REQ, &motg->inputs);
			clear_bit(B_CONN, &motg->inputs);
			msm_otg_start_host(otg, 0);
			if (test_bit(ID_A, &motg->inputs))
				msm_otg_notify_charger(motg, IDEV_CHG_MIN);
			else
				msm_hsusb_vbus_power(motg, 0);
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
		} else if (!test_bit(A_VBUS_VLD, &motg->inputs)) {
			USBH_INFO("!a_vbus_vld\n");
			clear_bit(B_CONN, &motg->inputs);
			msm_otg_del_timer(motg);
			msm_otg_start_host(otg, 0);
			otg->phy->state = OTG_STATE_A_VBUS_ERR;
			msm_otg_reset(otg->phy);
		} else if (test_bit(ID_A, &motg->inputs)) {
			msm_hsusb_vbus_power(motg, 0);
		} else if (!test_bit(A_BUS_REQ, &motg->inputs)) {
			if (TA_WAIT_BCON < 0)
				pm_runtime_put_sync(otg->phy->dev);
		} else if (!test_bit(ID, &motg->inputs)) {
			msm_hsusb_vbus_power(motg, 1);
		}

		if (USB_disabled && stop_usb_host != STOP_HOST_STATE) {
			USBH_INFO("[USB_disabled] disable USB Host function\n");
			stop_usb_host = TRY_STOP_HOST_STATE;
			re_enable_host = TRY_ENABLE_HOST_STATE;
			msm_otg_start_host(otg, 0);
		} else if (!USB_disabled && re_enable_host == TRY_ENABLE_HOST_STATE) {
			USBH_INFO("[USB_disabled] re-enable USB Host function\n");
			re_enable_host = DEFAULT_STATE;
			stop_usb_host = DEFAULT_STATE;
			msm_otg_start_host(otg, 1);
		}
		break;
	case OTG_STATE_A_HOST:
		dev_dbg(otg->dev, "OTG_STATE_A_HOST state\n");
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs)) {
			USBH_INFO("id_a/b/c || a_bus_drop\n");
			clear_bit(B_CONN, &motg->inputs);
			clear_bit(A_BUS_REQ, &motg->inputs);
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			msm_otg_start_host(otg, 0);
			if (!test_bit(ID_A, &motg->inputs))
				msm_hsusb_vbus_power(motg, 0);
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
		} else if (!test_bit(A_VBUS_VLD, &motg->inputs)) {
			USBH_INFO("!a_vbus_vld\n");
			clear_bit(B_CONN, &motg->inputs);
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_VBUS_ERR;
			msm_otg_start_host(otg, 0);
			msm_otg_reset(otg->phy);
		} else if (!test_bit(A_BUS_REQ, &motg->inputs)) {
			USBH_INFO("!a_bus_req\n");
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_SUSPEND;
			if (otg->host->b_hnp_enable)
				msm_otg_start_timer(motg, TA_AIDL_BDIS,
						A_AIDL_BDIS);
			else
				pm_runtime_put_sync(otg->phy->dev);
		} else if (!test_bit(B_CONN, &motg->inputs)) {
			USBH_INFO("!b_conn\n");
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_WAIT_BCON;
			if (TA_WAIT_BCON > 0)
				msm_otg_start_timer(motg, TA_WAIT_BCON,
					A_WAIT_BCON);
			if (msm_chg_check_aca_intr(motg))
				work = 1;
		} else if (test_bit(ID_A, &motg->inputs)) {
			msm_otg_del_timer(motg);
			msm_hsusb_vbus_power(motg, 0);
			if (motg->chg_type == USB_ACA_DOCK_CHARGER)
				msm_otg_notify_charger(motg,
						IDEV_ACA_CHG_MAX);
			else
				msm_otg_notify_charger(motg,
						IDEV_CHG_MIN - motg->mA_port);
		} else if (!test_bit(ID, &motg->inputs)) {
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
			motg->chg_type = USB_INVALID_CHARGER;
			msm_otg_notify_charger(motg, 0);
			msm_hsusb_vbus_power(motg, 1);
			if (USB_disabled) {
				USBH_INFO("[USB_disabled] disable USB Host function\n");
				re_enable_host = TRY_ENABLE_HOST_STATE;
				stop_usb_host = TRY_STOP_HOST_STATE;
				msm_otg_start_host(otg, 0);
			}
		}
		break;
	case OTG_STATE_A_SUSPEND:
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs) ||
				test_bit(A_AIDL_BDIS, &motg->tmouts)) {
			USBH_INFO("id_a/b/c || a_bus_drop ||"
					"a_aidl_bdis_tmout\n");
			msm_otg_del_timer(motg);
			clear_bit(B_CONN, &motg->inputs);
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			msm_otg_start_host(otg, 0);
			msm_otg_reset(otg->phy);
			if (!test_bit(ID_A, &motg->inputs))
				msm_hsusb_vbus_power(motg, 0);
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
		} else if (!test_bit(A_VBUS_VLD, &motg->inputs)) {
			USBH_INFO("!a_vbus_vld\n");
			msm_otg_del_timer(motg);
			clear_bit(B_CONN, &motg->inputs);
			otg->phy->state = OTG_STATE_A_VBUS_ERR;
			msm_otg_start_host(otg, 0);
			msm_otg_reset(otg->phy);
		} else if (!test_bit(B_CONN, &motg->inputs) &&
				otg->host->b_hnp_enable) {
			USBH_INFO("!b_conn && b_hnp_enable");
			otg->phy->state = OTG_STATE_A_PERIPHERAL;
			msm_otg_host_hnp_enable(otg, 1);
			otg->gadget->is_a_peripheral = 1;
			msm_otg_start_peripheral(otg, 1);
		} else if (!test_bit(B_CONN, &motg->inputs) &&
				!otg->host->b_hnp_enable) {
			USBH_INFO("!b_conn && !b_hnp_enable");
			set_bit(A_BUS_REQ, &motg->inputs);
			otg->phy->state = OTG_STATE_A_WAIT_BCON;
			if (TA_WAIT_BCON > 0)
				msm_otg_start_timer(motg, TA_WAIT_BCON,
					A_WAIT_BCON);

			if (!USB_disabled && re_enable_host == TRY_ENABLE_HOST_STATE) {
				USBH_INFO("[USB_disabled] re-enable USB Host function\n");
				re_enable_host = DEFAULT_STATE;
				stop_usb_host = DEFAULT_STATE;
				msm_otg_start_host(otg, 1);
			}
		} else if (test_bit(ID_A, &motg->inputs)) {
			msm_hsusb_vbus_power(motg, 0);
			msm_otg_notify_charger(motg,
					IDEV_CHG_MIN - motg->mA_port);
		} else if (!test_bit(ID, &motg->inputs)) {
			msm_otg_notify_charger(motg, 0);
			msm_hsusb_vbus_power(motg, 1);
		}
		break;
	case OTG_STATE_A_PERIPHERAL:
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs)) {
			USBH_INFO("id _f/b/c || a_bus_drop\n");
			
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			msm_otg_start_peripheral(otg, 0);
			otg->gadget->is_a_peripheral = 0;
			msm_otg_start_host(otg, 0);
			msm_otg_reset(otg->phy);
			if (!test_bit(ID_A, &motg->inputs))
				msm_hsusb_vbus_power(motg, 0);
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
		} else if (!test_bit(A_VBUS_VLD, &motg->inputs)) {
			USBH_INFO("!a_vbus_vld\n");
			
			msm_otg_del_timer(motg);
			otg->phy->state = OTG_STATE_A_VBUS_ERR;
			msm_otg_start_peripheral(otg, 0);
			otg->gadget->is_a_peripheral = 0;
			msm_otg_start_host(otg, 0);
		} else if (test_bit(A_BIDL_ADIS, &motg->tmouts)) {
			USBH_INFO("a_bidl_adis_tmout\n");
			msm_otg_start_peripheral(otg, 0);
			otg->gadget->is_a_peripheral = 0;
			otg->phy->state = OTG_STATE_A_WAIT_BCON;
			set_bit(A_BUS_REQ, &motg->inputs);
			msm_otg_host_hnp_enable(otg, 0);
			if (TA_WAIT_BCON > 0)
				msm_otg_start_timer(motg, TA_WAIT_BCON,
					A_WAIT_BCON);
		} else if (test_bit(ID_A, &motg->inputs)) {
			msm_hsusb_vbus_power(motg, 0);
			msm_otg_notify_charger(motg,
					IDEV_CHG_MIN - motg->mA_port);
		} else if (!test_bit(ID, &motg->inputs)) {
			USBH_INFO("!id\n");
			msm_otg_notify_charger(motg, 0);
			msm_hsusb_vbus_power(motg, 1);
		}
		break;
	case OTG_STATE_A_WAIT_VFALL:
		if (test_bit(A_WAIT_VFALL, &motg->tmouts)) {
			clear_bit(A_VBUS_VLD, &motg->inputs);
			otg->phy->state = OTG_STATE_A_IDLE;
			work = 1;
		}
		break;
	case OTG_STATE_A_VBUS_ERR:
		if ((test_bit(ID, &motg->inputs) &&
				!test_bit(ID_A, &motg->inputs)) ||
				test_bit(A_BUS_DROP, &motg->inputs) ||
				test_bit(A_CLR_ERR, &motg->inputs)) {
			otg->phy->state = OTG_STATE_A_WAIT_VFALL;
			if (!test_bit(ID_A, &motg->inputs))
				msm_hsusb_vbus_power(motg, 0);
			msm_otg_start_timer(motg, TA_WAIT_VFALL, A_WAIT_VFALL);
			motg->chg_state = USB_CHG_STATE_UNDEFINED;
			motg->chg_type = USB_INVALID_CHARGER;
			msm_otg_notify_charger(motg, 0);
		}
		break;
	default:
		break;
	}
	mutex_unlock(&smwork_sem);
	if (work)
		queue_work(system_nrt_wq, &motg->sm_work);
}

static irqreturn_t msm_otg_irq(int irq, void *data)
{
	struct msm_otg *motg = data;
	struct usb_otg *otg = motg->phy.otg;
	u32 otgsc = 0, usbsts, pc;
	bool work = 0;
	irqreturn_t ret = IRQ_HANDLED;

	if (atomic_read(&motg->in_lpm)) {
		pr_debug("OTG IRQ: in LPM\n");
		disable_irq_nosync(irq);
		motg->async_int = 1;
		if (atomic_read(&motg->pm_suspended))
			motg->sm_work_pending = true;
		else
			pm_request_resume(otg->phy->dev);
		return IRQ_HANDLED;
	}

	usbsts = readl(USB_USBSTS);
	otgsc = readl(USB_OTGSC);

	if (!(otgsc & OTG_OTGSTS_MASK) && !(usbsts & OTG_USBSTS_MASK))
		return IRQ_NONE;

	if ((otgsc & OTGSC_IDIS) && (otgsc & OTGSC_IDIE)) {
		if (otgsc & OTGSC_ID) {
			pr_debug("Id set\n");
			set_bit(ID, &motg->inputs);
		} else {
			pr_debug("Id clear\n");
			set_bit(A_BUS_REQ, &motg->inputs);
			clear_bit(ID, &motg->inputs);
			msm_chg_enable_aca_det(motg);
		}
		writel_relaxed(otgsc, USB_OTGSC);
		work = 1;
	} else if (otgsc & OTGSC_DPIS) {
		pr_debug("DPIS detected\n");
		writel_relaxed(otgsc, USB_OTGSC);
		set_bit(A_SRP_DET, &motg->inputs);
		set_bit(A_BUS_REQ, &motg->inputs);
		work = 1;
	} else if (otgsc & OTGSC_BSVIS) {
		writel_relaxed(otgsc, USB_OTGSC);
	} else if (usbsts & STS_PCI) {
		pc = readl_relaxed(USB_PORTSC);
		pr_debug("portsc = %x\n", pc);
		ret = IRQ_NONE;
		work = 1;
		switch (otg->phy->state) {
		case OTG_STATE_A_SUSPEND:
			if (otg->host->b_hnp_enable && (pc & PORTSC_CSC) &&
					!(pc & PORTSC_CCS)) {
				pr_debug("B_CONN clear\n");
				clear_bit(B_CONN, &motg->inputs);
				msm_otg_del_timer(motg);
			}
			break;
		case OTG_STATE_A_PERIPHERAL:
			msm_otg_del_timer(motg);
			work = 0;
			break;
		case OTG_STATE_B_WAIT_ACON:
			if ((pc & PORTSC_CSC) && (pc & PORTSC_CCS)) {
				pr_debug("A_CONN set\n");
				set_bit(A_CONN, &motg->inputs);
				
				msm_otg_del_timer(motg);
			}
			break;
		case OTG_STATE_B_HOST:
			if ((pc & PORTSC_CSC) && !(pc & PORTSC_CCS)) {
				pr_debug("A_CONN clear\n");
				clear_bit(A_CONN, &motg->inputs);
				msm_otg_del_timer(motg);
			}
			break;
		case OTG_STATE_A_WAIT_BCON:
			if (TA_WAIT_BCON < 0)
				set_bit(A_BUS_REQ, &motg->inputs);
		default:
			work = 0;
			break;
		}
	} else if (usbsts & STS_URI) {
		ret = IRQ_NONE;
		switch (otg->phy->state) {
		case OTG_STATE_A_PERIPHERAL:
			msm_otg_del_timer(motg);
			work = 0;
			break;
		default:
			work = 0;
			break;
		}
	} else if (usbsts & STS_SLI) {
		ret = IRQ_NONE;
		work = 0;
		switch (otg->phy->state) {
		case OTG_STATE_B_PERIPHERAL:
			if (otg->gadget->b_hnp_enable) {
				set_bit(A_BUS_SUSPEND, &motg->inputs);
				set_bit(B_BUS_REQ, &motg->inputs);
				work = 1;
			}
			break;
		case OTG_STATE_A_PERIPHERAL:
			msm_otg_start_timer(motg, TA_BIDL_ADIS,
					A_BIDL_ADIS);
			break;
		default:
			break;
		}
	} else if ((usbsts & PHY_ALT_INT)) {
		writel_relaxed(PHY_ALT_INT, USB_USBSTS);
		if (msm_chg_check_aca_intr(motg))
			work = 1;
		ret = IRQ_HANDLED;
	}
	if (work)
		queue_work(system_nrt_wq, &motg->sm_work);

	return ret;
}


static void ac_detect_expired_work(struct work_struct *w)
{
	u32 delay = 0;
	struct msm_otg *motg = the_msm_otg;
	struct usb_phy *usb_phy = &motg->phy;

	USBH_INFO("%s: count = %d, connect_type = %d\n", __func__,
			motg->ac_detect_count, motg->connect_type);

	if (motg->connect_type == CONNECT_TYPE_USB || motg->ac_detect_count >= 5)
		return;

	
	if ((readl(USB_PORTSC) & PORTSC_LS) != PORTSC_LS) {
#ifdef CONFIG_CABLE_DETECT_ACCESSORY
		if (cable_get_accessory_type() == DOCK_STATE_CAR
			||cable_get_accessory_type() == DOCK_STATE_AUDIO_DOCK) {
				USBH_INFO("car/audio dock mode charger\n");
				motg->chg_type = USB_DCP_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_AC;
				motg->ac_detect_count = 0;

				msm_otg_start_peripheral(usb_phy->otg, 0);
				usb_phy->state = OTG_STATE_B_IDLE;
				queue_work(system_nrt_wq, &motg->sm_work);
				queue_work(motg->usb_wq, &motg->notifier_work);
				return;
		}
		{
			int dock_result = check_three_pogo_dock();
			if (dock_result == 2) {
				USBH_INFO("three pogo dock AC type\n");
				motg->chg_type = USB_DCP_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_AC;
				motg->ac_detect_count = 0;

				msm_otg_start_peripheral(usb_phy->otg, 0);
				usb_phy->state = OTG_STATE_B_IDLE;
				queue_work(system_nrt_wq, &motg->sm_work);
				queue_work(motg->usb_wq, &motg->notifier_work);
				return;
			} else if (dock_result == 1) {
				USBH_INFO("three pogo dock USB type\n");
				motg->chg_type = USB_INVALID_CHARGER;
				motg->chg_state = USB_CHG_STATE_DETECTED;
				motg->connect_type = CONNECT_TYPE_NONE;
				motg->ac_detect_count = 0;

				msm_otg_start_peripheral(usb_phy->otg, 0);
				usb_phy->state = OTG_STATE_B_IDLE;
				queue_work(system_nrt_wq, &motg->sm_work);
				queue_work(motg->usb_wq, &motg->notifier_work);
				return;
			}
		}
#endif
		if (motg->ac_detect_count++ < 5)
			delay = 2 * HZ;

		queue_delayed_work(system_nrt_wq, &motg->ac_detect_work, delay);
	} else {
		USBH_INFO("AC charger\n");
		motg->chg_type = USB_DCP_CHARGER;
		motg->chg_state = USB_CHG_STATE_DETECTED;
		motg->connect_type = CONNECT_TYPE_AC;
		motg->ac_detect_count = 0;

		msm_otg_start_peripheral(usb_phy->otg, 0);
		usb_phy->state = OTG_STATE_B_IDLE;
		queue_work(system_nrt_wq, &motg->sm_work);

		queue_work(motg->usb_wq, &motg->notifier_work);
	}
}

#ifndef CONFIG_CABLE_DETECT_8X60
static void htc_vbus_notify(int online)
{
	cable_detection_vbus_irq_handler();
}
#endif

int msm_otg_get_vbus_state(void)
{
	return htc_otg_vbus;
}

void msm_otg_set_vbus_state(int online)
{
	static bool init;
	struct msm_otg *motg = the_msm_otg;
	struct usb_otg *otg = motg->phy.otg;
	USBH_INFO("%s: %d\n", __func__, online);

	htc_otg_vbus = online;
	
	if (online && otg->phy->state >= OTG_STATE_A_IDLE)
		return;

	if (online) {
		pr_debug("PMIC: BSV set\n");
		set_bit(B_SESS_VLD, &motg->inputs);

		
		if (motg->pdata->usb_uart_switch)
			motg->pdata->usb_uart_switch(0);
	} else {
		pr_debug("PMIC: BSV clear\n");
		clear_bit(B_SESS_VLD, &motg->inputs);

		
		if (motg->pdata->usb_uart_switch)
			motg->pdata->usb_uart_switch(1);
	}

	if (!init) {
		init = true;
		complete(&pmic_vbus_init);
		pr_debug("PMIC: BSV init complete\n");
		return;
	}

	wake_lock_timeout(&motg->cable_detect_wlock, 3 * HZ);
	if (atomic_read(&motg->pm_suspended))
		motg->sm_work_pending = true;
	else
		queue_work(system_nrt_wq, &motg->sm_work);
}


#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
void msm_otg_set_id_state(int id)
{
	struct msm_otg *motg = the_msm_otg;

	if (id) {
		pr_debug("PMIC: ID set\n");
		set_bit(ID, &motg->inputs);
	} else {
		pr_debug("PMIC: ID clear\n");
		clear_bit(ID, &motg->inputs);
	}

	if (motg->phy.state != OTG_STATE_UNDEFINED) {
		
		wake_lock_timeout(&motg->cable_detect_wlock, 3 * HZ);
		schedule_work(&motg->sm_work);
	}
}

static void usb_host_cable_detect(bool cable_in)
{
	if (cable_in)
		msm_otg_set_id_state(0);
	else
		msm_otg_set_id_state(1);
}
#endif

void msm_otg_set_disable_usb(int disable_usb)
{
	struct msm_otg *motg = the_msm_otg;

	USB_disabled = disable_usb;

	if (!disable_usb)
		motg->chg_state = USB_CHG_STATE_UNDEFINED;
	queue_work(system_nrt_wq, &motg->sm_work);
}

static void msm_pmic_id_status_w(struct work_struct *w)
{
	struct msm_otg *motg = container_of(w, struct msm_otg,
						pmic_id_status_work.work);
	int work = 0;
	unsigned long flags;

	local_irq_save(flags);
	if (irq_read_line(motg->pdata->pmic_id_irq)) {
		if (!test_and_set_bit(ID, &motg->inputs)) {
			pr_debug("PMIC: ID set\n");
			work = 1;
		}
	} else {
		if (test_and_clear_bit(ID, &motg->inputs)) {
			pr_debug("PMIC: ID clear\n");
			set_bit(A_BUS_REQ, &motg->inputs);
			work = 1;
		}
	}

	if (work && (motg->phy.state != OTG_STATE_UNDEFINED)) {
		if (atomic_read(&motg->pm_suspended))
			motg->sm_work_pending = true;
		else
			queue_work(system_nrt_wq, &motg->sm_work);
	}
	local_irq_restore(flags);

}

#define MSM_PMIC_ID_STATUS_DELAY	5 
static irqreturn_t msm_pmic_id_irq(int irq, void *data)
{
	struct msm_otg *motg = data;

	if (!aca_id_turned_on)
		
		queue_delayed_work(system_nrt_wq, &motg->pmic_id_status_work,
				msecs_to_jiffies(MSM_PMIC_ID_STATUS_DELAY));

	return IRQ_HANDLED;
}

static int msm_otg_mode_show(struct seq_file *s, void *unused)
{
	struct msm_otg *motg = s->private;
	struct usb_phy *phy = &motg->phy;

	switch (phy->state) {
	case OTG_STATE_A_HOST:
		seq_printf(s, "host\n");
		break;
	case OTG_STATE_B_PERIPHERAL:
		seq_printf(s, "peripheral\n");
		break;
	default:
		seq_printf(s, "none\n");
		break;
	}

	return 0;
}

static int msm_otg_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_otg_mode_show, inode->i_private);
}

static ssize_t msm_otg_mode_write(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct msm_otg *motg = s->private;
	char buf[16];
	struct usb_phy *phy = &motg->phy;
	int status = count;
	enum usb_mode_type req_mode;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count))) {
		status = -EFAULT;
		goto out;
	}

	if (!strncmp(buf, "host", 4)) {
		req_mode = USB_HOST;
	} else if (!strncmp(buf, "peripheral", 10)) {
		req_mode = USB_PERIPHERAL;
	} else if (!strncmp(buf, "none", 4)) {
		req_mode = USB_NONE;
	} else {
		status = -EINVAL;
		goto out;
	}

	USB_INFO("%s: %s\n", __func__, (req_mode == USB_HOST)?"host"
			:(req_mode == USB_PERIPHERAL)?"peripheral":"none");

	switch (req_mode) {
	case USB_NONE:
		switch (phy->state) {
		case OTG_STATE_A_HOST:
		case OTG_STATE_B_PERIPHERAL:
			set_bit(ID, &motg->inputs);
			clear_bit(B_SESS_VLD, &motg->inputs);
			break;
		default:
			goto out;
		}
		break;
	case USB_PERIPHERAL:
		switch (phy->state) {
		case OTG_STATE_B_IDLE:
		case OTG_STATE_A_HOST:
			set_bit(ID, &motg->inputs);
			set_bit(B_SESS_VLD, &motg->inputs);
			break;
		default:
			goto out;
		}
		break;
	case USB_HOST:
		switch (phy->state) {
		case OTG_STATE_B_IDLE:
		case OTG_STATE_B_PERIPHERAL:
			clear_bit(ID, &motg->inputs);
			break;
		default:
			goto out;
		}
		break;
	default:
		goto out;
	}

	pm_runtime_resume(phy->dev);
	queue_work(system_nrt_wq, &motg->sm_work);
out:
	return status;
}

const struct file_operations msm_otg_mode_fops = {
	.open = msm_otg_mode_open,
	.read = seq_read,
	.write = msm_otg_mode_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msm_otg_show_otg_state(struct seq_file *s, void *unused)
{
	struct msm_otg *motg = s->private;
	struct usb_phy *phy = &motg->phy;

	seq_printf(s, "%s\n", otg_state_string(phy->state));
	return 0;
}

static int msm_otg_otg_state_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_otg_show_otg_state, inode->i_private);
}

const struct file_operations msm_otg_state_fops = {
	.open = msm_otg_otg_state_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msm_otg_show_chg_type(struct seq_file *s, void *unused)
{
	struct msm_otg *motg = s->private;

	seq_printf(s, "%s\n", chg_to_string(motg->chg_type));
	return 0;
}

static int msm_otg_chg_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_otg_show_chg_type, inode->i_private);
}

const struct file_operations msm_otg_chg_fops = {
	.open = msm_otg_chg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msm_otg_aca_show(struct seq_file *s, void *unused)
{
	if (debug_aca_enabled)
		seq_printf(s, "enabled\n");
	else
		seq_printf(s, "disabled\n");

	return 0;
}

static int msm_otg_aca_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_otg_aca_show, inode->i_private);
}

static ssize_t msm_otg_aca_write(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	char buf[8];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enable", 6))
		debug_aca_enabled = true;
	else
		debug_aca_enabled = false;

	return count;
}

const struct file_operations msm_otg_aca_fops = {
	.open = msm_otg_aca_open,
	.read = seq_read,
	.write = msm_otg_aca_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msm_otg_bus_show(struct seq_file *s, void *unused)
{
	if (debug_bus_voting_enabled)
		seq_printf(s, "enabled\n");
	else
		seq_printf(s, "disabled\n");

	return 0;
}

static int msm_otg_bus_open(struct inode *inode, struct file *file)
{
	return single_open(file, msm_otg_bus_show, inode->i_private);
}

static ssize_t msm_otg_bus_write(struct file *file, const char __user *ubuf,
				size_t count, loff_t *ppos)
{
	char buf[8];
	int ret;
	struct seq_file *s = file->private_data;
	struct msm_otg *motg = s->private;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enable", 6)) {
		
		debug_bus_voting_enabled = true;
	} else {
		debug_bus_voting_enabled = false;
		if (motg->bus_perf_client) {
			ret = msm_bus_scale_client_update_request(
					motg->bus_perf_client, 0);
			if (ret)
				dev_err(motg->phy.dev, "%s: Failed to devote "
					   "for bus bw %d\n", __func__, ret);
		}
	}

	return count;
}

const struct file_operations msm_otg_bus_fops = {
	.open = msm_otg_bus_open,
	.read = seq_read,
	.write = msm_otg_bus_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct dentry *msm_otg_dbg_root;

static int msm_otg_debugfs_init(struct msm_otg *motg)
{
	struct dentry *msm_otg_dentry;

	msm_otg_dbg_root = debugfs_create_dir("msm_otg", NULL);

	if (!msm_otg_dbg_root || IS_ERR(msm_otg_dbg_root))
		return -ENODEV;

	if (motg->pdata->mode == USB_OTG &&
		motg->pdata->otg_control == OTG_USER_CONTROL) {

		msm_otg_dentry = debugfs_create_file("mode", S_IRUGO |
			S_IWUSR, msm_otg_dbg_root, motg,
			&msm_otg_mode_fops);

		if (!msm_otg_dentry) {
			debugfs_remove(msm_otg_dbg_root);
			msm_otg_dbg_root = NULL;
			return -ENODEV;
		}
	}

	msm_otg_dentry = debugfs_create_file("chg_type", S_IRUGO,
		msm_otg_dbg_root, motg,
		&msm_otg_chg_fops);

	if (!msm_otg_dentry) {
		debugfs_remove_recursive(msm_otg_dbg_root);
		return -ENODEV;
	}

	msm_otg_dentry = debugfs_create_file("aca", S_IRUGO | S_IWUSR,
		msm_otg_dbg_root, motg,
		&msm_otg_aca_fops);

	if (!msm_otg_dentry) {
		debugfs_remove_recursive(msm_otg_dbg_root);
		return -ENODEV;
	}

	msm_otg_dentry = debugfs_create_file("bus_voting", S_IRUGO | S_IWUSR,
		msm_otg_dbg_root, motg,
		&msm_otg_bus_fops);

	if (!msm_otg_dentry) {
		debugfs_remove_recursive(msm_otg_dbg_root);
		return -ENODEV;
	}

	msm_otg_dentry = debugfs_create_file("otg_state", S_IRUGO,
				msm_otg_dbg_root, motg, &msm_otg_state_fops);

	if (!msm_otg_dentry) {
		debugfs_remove_recursive(msm_otg_dbg_root);
		return -ENODEV;
	}
	return 0;
}

static void msm_otg_debugfs_cleanup(void)
{
	debugfs_remove_recursive(msm_otg_dbg_root);
}

#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
static struct t_usb_host_status_notifier usb_host_status_notifier = {
	.name = "usb_host",
	.func = usb_host_cable_detect,
};
#endif

static u64 msm_otg_dma_mask = DMA_BIT_MASK(64);
static struct platform_device *msm_otg_add_pdev(
		struct platform_device *ofdev, const char *name)
{
	struct platform_device *pdev;
	const struct resource *res = ofdev->resource;
	unsigned int num = ofdev->num_resources;
	int retval;

	pdev = platform_device_alloc(name, -1);
	if (!pdev) {
		retval = -ENOMEM;
		goto error;
	}

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &msm_otg_dma_mask;

	if (num) {
		retval = platform_device_add_resources(pdev, res, num);
		if (retval)
			goto error;
	}

	retval = platform_device_add(pdev);
	if (retval)
		goto error;

	return pdev;

error:
	platform_device_put(pdev);
	return ERR_PTR(retval);
}

static int msm_otg_setup_devices(struct platform_device *ofdev,
		enum usb_mode_type mode, bool init)
{
	const char *gadget_name = "msm_hsusb";
	const char *host_name = "msm_hsusb_host";
	static struct platform_device *gadget_pdev;
	static struct platform_device *host_pdev;
	int retval = 0;

	if (!init) {
		if (gadget_pdev)
			platform_device_unregister(gadget_pdev);
		if (host_pdev)
			platform_device_unregister(host_pdev);
		return 0;
	}

	switch (mode) {
	case USB_OTG:
		
	case USB_PERIPHERAL:
		gadget_pdev = msm_otg_add_pdev(ofdev, gadget_name);
		if (IS_ERR(gadget_pdev)) {
			retval = PTR_ERR(gadget_pdev);
			break;
		}
		if (mode == USB_PERIPHERAL)
			break;
		
	case USB_HOST:
		host_pdev = msm_otg_add_pdev(ofdev, host_name);
		if (IS_ERR(host_pdev)) {
			retval = PTR_ERR(host_pdev);
			if (mode == USB_OTG)
				platform_device_unregister(gadget_pdev);
		}
		break;
	default:
		break;
	}

	return retval;
}

struct msm_otg_platform_data *msm_otg_dt_to_pdata(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct msm_otg_platform_data *pdata;
	int len = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("unable to allocate platform data\n");
		return NULL;
	}
	of_get_property(node, "qcom,hsusb-otg-phy-init-seq", &len);
	if (len) {
		pdata->phy_init_seq = devm_kzalloc(&pdev->dev, len, GFP_KERNEL);
		if (!pdata->phy_init_seq)
			return NULL;
		of_property_read_u32_array(node, "qcom,hsusb-otg-phy-init-seq",
				pdata->phy_init_seq,
				len/sizeof(*pdata->phy_init_seq));
	}
	of_property_read_u32(node, "qcom,hsusb-otg-power-budget",
				&pdata->power_budget);
	of_property_read_u32(node, "qcom,hsusb-otg-mode",
				&pdata->mode);
	of_property_read_u32(node, "qcom,hsusb-otg-otg-control",
				&pdata->otg_control);
	of_property_read_u32(node, "qcom,hsusb-otg-default-mode",
				&pdata->default_mode);
	of_property_read_u32(node, "qcom,hsusb-otg-phy-type",
				&pdata->phy_type);
	of_property_read_u32(node, "qcom,hsusb-otg-pmic-id-irq",
				&pdata->pmic_id_irq);
	return pdata;
}


static const char *event_string(enum usb_otg_event event)
{
	switch (event) {
	case OTG_EVENT_DEV_CONN_TMOUT:
		return "DEV_CONN_TMOUT";
	case OTG_EVENT_NO_RESP_FOR_HNP_ENABLE:
		return "NO_RESP_FOR_HNP_ENABLE";
	case OTG_EVENT_HUB_NOT_SUPPORTED:
		return "HUB_NOT_SUPPORTED";
	case OTG_EVENT_DEV_NOT_SUPPORTED:
		return "DEV_NOT_SUPPORTED";
	case OTG_EVENT_HNP_FAILED:
		return "HNP_FAILED";
	case OTG_EVENT_NO_RESP_FOR_SRP:
		return "NO_RESP_FOR_SRP";
	case OTG_EVENT_INSUFFICIENT_POWER:
		return "DEV_NOT_SUPPORTED";
	default:
		return "UNDEFINED";
	}
}

static int msm_otg_send_event(struct usb_phy *phy,
				enum usb_otg_event event)
{
	char module_name[16];
	char udev_event[128];
	char *envp[] = { module_name, udev_event, NULL };
	int ret;
	
	switch (event) {
	case OTG_EVENT_INSUFFICIENT_POWER:
	case OTG_EVENT_DEV_NOT_SUPPORTED:
#if 1
			USBH_DEBUG("sending %s event\n", event_string(event));
#endif
			snprintf(module_name, 16, "MODULE=%s", DRIVER_NAME);
			snprintf(udev_event, 128, "EVENT=%s", event_string(event));
			ret = kobject_uevent_env(&phy->dev->kobj, KOBJ_CHANGE, envp);
#if 1
			if (ret < 0)
				USBH_ERR("uevent sending failed with ret = %d\n", ret);
#endif
		break;
	default:
		return 0;
		break;
	}
	return ret;
}


static int msm_otg_send_event2(struct usb_otg *otg, enum usb_otg_event event){
	return msm_otg_send_event(otg->phy,event);
}

static int __init msm_otg_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct msm_otg *motg;
	struct usb_phy *phy;
	struct msm_otg_platform_data *pdata;

	dev_info(&pdev->dev, "msm_otg probe\n");

	if (pdev->dev.of_node) {
		dev_dbg(&pdev->dev, "device tree enabled\n");
		pdata = msm_otg_dt_to_pdata(pdev);
		if (!pdata)
			return -ENOMEM;
		ret = msm_otg_setup_devices(pdev, pdata->mode, true);
		if (ret) {
			dev_err(&pdev->dev, "devices setup failed\n");
			return ret;
		}
	} else if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "No platform data given. Bailing out\n");
		return -ENODEV;
	} else {
		pdata = pdev->dev.platform_data;
	}

	motg = kzalloc(sizeof(struct msm_otg), GFP_KERNEL);
	if (!motg) {
		dev_err(&pdev->dev, "unable to allocate msm_otg\n");
		return -ENOMEM;
	}

	motg->phy.otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
	if (!motg->phy.otg) {
		dev_err(&pdev->dev, "unable to allocate usb_otg\n");
		ret = -ENOMEM;
		goto free_motg;
	}

	the_msm_otg = motg;
	motg->pdata = pdata;
	motg->connect_type_ready = 0;
	phy = &motg->phy;
	phy->dev = &pdev->dev;
	motg->reset_phy_before_lpm = pdata->reset_phy_before_lpm;
	if (aca_enabled() && motg->pdata->otg_control != OTG_PMIC_CONTROL) {
		dev_err(&pdev->dev, "ACA can not be enabled without PMIC\n");
		ret = -EINVAL;
		goto free_otg;
	}

	
	motg->reset_counter = 0;

	
	motg->phy_reset_clk = clk_get(&pdev->dev, "phy_clk");
	if (IS_ERR(motg->phy_reset_clk))
		dev_err(&pdev->dev, "failed to get phy_clk\n");

	motg->clk = clk_get(&pdev->dev, "alt_core_clk");
	if (IS_ERR(motg->clk))
		dev_dbg(&pdev->dev, "alt_core_clk is not present\n");
	else
		clk_set_rate(motg->clk, 60000000);

	motg->core_clk = clk_get(&pdev->dev, "core_clk");
	if (IS_ERR(motg->core_clk)) {
		motg->core_clk = NULL;
		dev_err(&pdev->dev, "failed to get core_clk\n");
		ret = PTR_ERR(motg->core_clk);
		goto put_clk;
	}
	clk_set_rate(motg->core_clk, INT_MAX);

	motg->pclk = clk_get(&pdev->dev, "iface_clk");
	if (IS_ERR(motg->pclk)) {
		dev_err(&pdev->dev, "failed to get iface_clk\n");
		ret = PTR_ERR(motg->pclk);
		goto put_core_clk;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get platform resource mem\n");
		ret = -ENODEV;
		goto put_pclk;
	}

	motg->regs = ioremap(res->start, resource_size(res));
	if (!motg->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto put_pclk;
	}
	dev_info(&pdev->dev, "OTG regs = %p\n", motg->regs);

	motg->irq = platform_get_irq(pdev, 0);
	if (!motg->irq) {
		dev_err(&pdev->dev, "platform_get_irq failed\n");
		ret = -ENODEV;
		goto free_regs;
	}

	motg->xo_handle = msm_xo_get(MSM_XO_TCXO_D0, "usb");
	if (IS_ERR(motg->xo_handle)) {
		dev_err(&pdev->dev, "%s not able to get the handle "
			"to vote for TCXO D0 buffer\n", __func__);
		ret = PTR_ERR(motg->xo_handle);
		goto free_regs;
	}

	ret = msm_xo_mode_vote(motg->xo_handle, MSM_XO_MODE_ON);
	if (ret) {
		dev_err(&pdev->dev, "%s failed to vote for TCXO "
			"D0 buffer%d\n", __func__, ret);
		goto free_xo_handle;
	}

	clk_prepare_enable(motg->pclk);

	motg->vdd_type = VDDCX_CORNER;
	hsusb_vddcx = devm_regulator_get(motg->phy.dev, "hsusb_vdd_dig");
	if (IS_ERR(hsusb_vddcx)) {
		hsusb_vddcx = devm_regulator_get(motg->phy.dev, "HSUSB_VDDCX");
		if (IS_ERR(hsusb_vddcx)) {
			if (motg->pdata && motg->pdata->vddcx_name)
				hsusb_vddcx = regulator_get(motg->phy.dev,
						motg->pdata->vddcx_name);
			if (IS_ERR(hsusb_vddcx)) {
				dev_err(motg->phy.dev, "unable to get hsusb vddcx\n");
				ret = PTR_ERR(hsusb_vddcx);
				goto devote_xo_handle;
			}
		}
		motg->vdd_type = VDDCX;
	}

	ret = msm_hsusb_config_vddcx(1);
	if (ret) {
		dev_err(&pdev->dev, "hsusb vddcx configuration failed\n");
		goto devote_xo_handle;
	}

	ret = regulator_enable(hsusb_vddcx);
	if (ret) {
		dev_err(&pdev->dev, "unable to enable the hsusb vddcx\n");
		goto free_config_vddcx;
	}

	ret = msm_hsusb_ldo_init(motg, 1);
	if (ret) {
		dev_err(&pdev->dev, "hsusb vreg configuration failed\n");
		goto free_hsusb_vddcx;
	}

	if (pdata->mhl_enable) {
		mhl_usb_hs_switch = devm_regulator_get(motg->phy.dev,
							"mhl_usb_hs_switch");
		if (IS_ERR(mhl_usb_hs_switch)) {
			dev_err(&pdev->dev, "Unable to get mhl_usb_hs_switch\n");
			ret = PTR_ERR(mhl_usb_hs_switch);
			goto free_ldo_init;
		}
	}

	ret = msm_hsusb_ldo_enable(motg, 1);
	if (ret) {
		dev_err(&pdev->dev, "hsusb vreg enable failed\n");
		goto free_ldo_init;
	}
	clk_prepare_enable(motg->core_clk);

	writel(0, USB_USBINTR);
	writel(0, USB_OTGSC);
	
	mb();

	motg->usb_wq = create_singlethread_workqueue("msm_hsusb");
	if (motg->usb_wq == 0) {
		USB_ERR("fail to create workqueue\n");
		goto free_ldo_init;
	}

	wake_lock_init(&motg->wlock, WAKE_LOCK_SUSPEND, "msm_otg");
	wake_lock_init(&motg->cable_detect_wlock, WAKE_LOCK_SUSPEND, "msm_usb_cable");
	msm_otg_init_timer(motg);
	INIT_WORK(&motg->sm_work, msm_otg_sm_work);
	INIT_WORK(&motg->usb_disable_work, usb_disable_work);
	INIT_WORK(&motg->notifier_work, send_usb_connect_notify);
	INIT_DELAYED_WORK(&motg->ac_detect_work, ac_detect_expired_work);
	INIT_DELAYED_WORK(&motg->chg_work, msm_chg_detect_work);
	INIT_DELAYED_WORK(&motg->pmic_id_status_work, msm_pmic_id_status_w);
	setup_timer(&motg->id_timer, msm_otg_id_timer_func,
				(unsigned long) motg);
	motg->ac_detect_count = 0;

	ret = request_irq(motg->irq, msm_otg_irq, IRQF_SHARED,
					"msm_otg", motg);
	if (ret) {
		dev_err(&pdev->dev, "request irq failed\n");
		goto destroy_wlock;
	}

	phy->init = msm_otg_reset;
	phy->set_power = msm_otg_set_power;
	phy->set_suspend = msm_otg_set_suspend;
	phy->notify_usb_attached = msm_otg_notify_usb_attached;
	phy->notify_usb_disabled = msm_otg_notify_usb_disabled;
	phy->io_ops = &msm_otg_io_ops;
	phy->send_event = msm_otg_send_event;
	phy->otg->send_event = msm_otg_send_event2;
	phy->otg->phy = &motg->phy;
	phy->otg->set_host = msm_otg_set_host;
	phy->otg->set_peripheral = msm_otg_set_peripheral;
	phy->otg->start_hnp = msm_otg_start_hnp;
	phy->otg->start_srp = msm_otg_start_srp;

	ret = usb_set_transceiver(&motg->phy);
	if (ret) {
		dev_err(&pdev->dev, "usb_set_transceiver failed\n");
		goto free_irq;
	}

	if (motg->pdata->mode == USB_OTG &&
		motg->pdata->otg_control == OTG_PMIC_CONTROL) {
		if (motg->pdata->pmic_id_irq) {
			ret = request_irq(motg->pdata->pmic_id_irq,
						msm_pmic_id_irq,
						IRQF_TRIGGER_RISING |
						IRQF_TRIGGER_FALLING,
						"msm_otg", motg);
			if (ret) {
				dev_err(&pdev->dev, "request irq failed for PMIC ID\n");
				goto remove_phy;
			}
		} else {
			
			dev_dbg(&pdev->dev, "PMIC IRQ for ID notifications doesn't exist. Maybe monitor id pin by GPIO");
		}
	}

	msm_hsusb_mhl_switch_enable(motg, 1);

	platform_set_drvdata(pdev, motg);
	device_init_wakeup(&pdev->dev, 1);
	motg->mA_port = IUNIT;

	ret = msm_otg_debugfs_init(motg);
	if (ret)
		dev_dbg(&pdev->dev, "mode debugfs file is"
			"not available\n");

	if (motg->pdata->otg_control == OTG_PMIC_CONTROL) {
#ifdef CONFIG_CABLE_DETECT_8X60
		pm8921_charger_register_vbus_sn(&msm_otg_set_vbus_state);
#else
		pm8921_charger_register_vbus_sn(&htc_vbus_notify);
#endif
	}

#if (defined(CONFIG_USB_OTG) && defined(CONFIG_USB_OTG_HOST))
	usb_host_detect_register_notifier(&usb_host_status_notifier);
#endif

	if (motg->pdata->phy_type == SNPS_28NM_INTEGRATED_PHY) {
#if 0
		if (motg->pdata->otg_control == OTG_PMIC_CONTROL &&
			(!(motg->pdata->mode == USB_OTG) ||
			 motg->pdata->pmic_id_irq))
			motg->caps =  ALLOW_PHY_RETENTION;
#endif

		if (motg->pdata->otg_control == OTG_PMIC_CONTROL)
			motg->caps = ALLOW_PHY_RETENTION;
		if (motg->pdata->otg_control == OTG_PHY_CONTROL)
			motg->caps = ALLOW_PHY_RETENTION;
		if (is_msm_otg_support_power_collapse(motg)) {
			motg->caps |= ALLOW_PHY_POWER_COLLAPSE;
			USBH_DEBUG("%s support power collapse\n", __func__);
		}
	}



	if (motg->pdata->enable_lpm_on_dev_suspend)
		motg->caps |= ALLOW_LPM_ON_DEV_SUSPEND;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	if (motg->pdata->bus_scale_table) {
		motg->bus_perf_client =
		    msm_bus_scale_register_client(motg->pdata->bus_scale_table);
		if (!motg->bus_perf_client)
			dev_err(motg->phy.dev, "%s: Failed to register BUS "
						"scaling client!!\n", __func__);
		else
			debug_bus_voting_enabled = true;
	}

	return 0;

remove_phy:
	usb_set_transceiver(NULL);
free_irq:
	free_irq(motg->irq, motg);
destroy_wlock:
	wake_lock_destroy(&motg->wlock);
	wake_lock_destroy(&motg->cable_detect_wlock);
	clk_disable_unprepare(motg->core_clk);
	msm_hsusb_ldo_enable(motg, 0);
free_ldo_init:
	msm_hsusb_ldo_init(motg, 0);
free_hsusb_vddcx:
	regulator_disable(hsusb_vddcx);
free_config_vddcx:
	regulator_set_voltage(hsusb_vddcx,
		vdd_val[motg->vdd_type][VDD_NONE],
		vdd_val[motg->vdd_type][VDD_MAX]);
devote_xo_handle:
	clk_disable_unprepare(motg->pclk);
	msm_xo_mode_vote(motg->xo_handle, MSM_XO_MODE_OFF);
free_xo_handle:
	msm_xo_put(motg->xo_handle);
free_regs:
	iounmap(motg->regs);
put_pclk:
	clk_put(motg->pclk);
put_core_clk:
	clk_put(motg->core_clk);
put_clk:
	if (!IS_ERR(motg->clk))
		clk_put(motg->clk);
	if (!IS_ERR(motg->phy_reset_clk))
		clk_put(motg->phy_reset_clk);
free_otg:
	kfree(motg->phy.otg);
free_motg:
	kfree(motg);
	return ret;
}

static int __devexit msm_otg_remove(struct platform_device *pdev)
{
	struct msm_otg *motg = platform_get_drvdata(pdev);
	struct usb_otg *otg = motg->phy.otg;
	int cnt = 0;

	if (otg->host || otg->gadget)
		return -EBUSY;

	USB_INFO("%s\n", __func__);

	if (pdev->dev.of_node)
		msm_otg_setup_devices(pdev, motg->pdata->mode, false);
	if (motg->pdata->otg_control == OTG_PMIC_CONTROL)
		pm8921_charger_unregister_vbus_sn(0);
	msm_otg_debugfs_cleanup();
	cancel_delayed_work_sync(&motg->chg_work);
	cancel_delayed_work_sync(&motg->pmic_id_status_work);
	cancel_work_sync(&motg->sm_work);

	pm_runtime_resume(&pdev->dev);

	device_init_wakeup(&pdev->dev, 0);
	pm_runtime_disable(&pdev->dev);
	wake_lock_destroy(&motg->wlock);
	wake_lock_destroy(&motg->cable_detect_wlock);

	msm_hsusb_mhl_switch_enable(motg, 0);
	if (motg->pdata->pmic_id_irq)
		free_irq(motg->pdata->pmic_id_irq, motg);
	usb_set_transceiver(NULL);
	free_irq(motg->irq, motg);

	ulpi_read(otg->phy, 0x14);
	ulpi_write(otg->phy, 0x08, 0x09);

	writel(readl(USB_PORTSC) | PORTSC_PHCD, USB_PORTSC);
	while (cnt < PHY_SUSPEND_TIMEOUT_USEC) {
		if (readl(USB_PORTSC) & PORTSC_PHCD)
			break;
		udelay(1);
		cnt++;
	}
	if (cnt >= PHY_SUSPEND_TIMEOUT_USEC)
		dev_err(otg->phy->dev, "Unable to suspend PHY\n");

	clk_disable_unprepare(motg->pclk);
	clk_disable_unprepare(motg->core_clk);
	msm_xo_put(motg->xo_handle);
	msm_hsusb_ldo_enable(motg, 0);
	msm_hsusb_ldo_init(motg, 0);
	regulator_disable(hsusb_vddcx);
	regulator_set_voltage(hsusb_vddcx,
		vdd_val[motg->vdd_type][VDD_NONE],
		vdd_val[motg->vdd_type][VDD_MAX]);

	iounmap(motg->regs);
	pm_runtime_set_suspended(&pdev->dev);

	if (!IS_ERR(motg->phy_reset_clk))
		clk_put(motg->phy_reset_clk);
	clk_put(motg->pclk);
	if (!IS_ERR(motg->clk))
		clk_put(motg->clk);
	clk_put(motg->core_clk);

	if (motg->bus_perf_client)
		msm_bus_scale_unregister_client(motg->bus_perf_client);

	kfree(motg->phy.otg);
	kfree(motg);
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int msm_otg_runtime_idle(struct device *dev)
{
	struct msm_otg *motg = dev_get_drvdata(dev);
	struct usb_phy *phy = &motg->phy;

	dev_dbg(dev, "OTG runtime idle\n");

	if (phy->state == OTG_STATE_UNDEFINED)
		return -EAGAIN;
	else
		return 0;
}

static int msm_otg_runtime_suspend(struct device *dev)
{
	struct msm_otg *motg = dev_get_drvdata(dev);

	dev_dbg(dev, "OTG runtime suspend\n");
	return msm_otg_suspend(motg);
}

static int msm_otg_runtime_resume(struct device *dev)
{
	struct msm_otg *motg = dev_get_drvdata(dev);

	dev_dbg(dev, "OTG runtime resume\n");
	pm_runtime_get_noresume(dev);
	return msm_otg_resume(motg);
}
#endif

#ifdef CONFIG_PM_SLEEP
static int msm_otg_pm_suspend(struct device *dev)
{
	int ret = 0;
	struct msm_otg *motg = dev_get_drvdata(dev);

	dev_dbg(dev, "OTG PM suspend\n");

	atomic_set(&motg->pm_suspended, 1);
	ret = msm_otg_suspend(motg);
	if (ret)
		atomic_set(&motg->pm_suspended, 0);

	return ret;
}

static int msm_otg_pm_resume(struct device *dev)
{
	int ret = 0;
	struct msm_otg *motg = dev_get_drvdata(dev);

	dev_dbg(dev, "OTG PM resume\n");

	atomic_set(&motg->pm_suspended, 0);
	if (motg->sm_work_pending) {
		motg->sm_work_pending = false;

		pm_runtime_get_noresume(dev);
		ret = msm_otg_resume(motg);

		
		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);

		queue_work(system_nrt_wq, &motg->sm_work);
	}

	return ret;
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops msm_otg_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(msm_otg_pm_suspend, msm_otg_pm_resume)
	SET_RUNTIME_PM_OPS(msm_otg_runtime_suspend, msm_otg_runtime_resume,
				msm_otg_runtime_idle)
};
#endif

static struct of_device_id msm_otg_dt_match[] = {
	{	.compatible = "qcom,hsusb-otg",
	},
	{}
};

static struct platform_driver msm_otg_driver = {
	.remove = __devexit_p(msm_otg_remove),
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &msm_otg_dev_pm_ops,
#endif
		.of_match_table = msm_otg_dt_match,
	},
};

static int __init msm_otg_init(void)
{
	return platform_driver_probe(&msm_otg_driver, msm_otg_probe);
}

static void __exit msm_otg_exit(void)
{
	platform_driver_unregister(&msm_otg_driver);
}

module_init(msm_otg_init);
module_exit(msm_otg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM USB transceiver driver");
