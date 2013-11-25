/* linux/include/asm-arm/arch-msm/hsusb.h
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 * Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __ASM_ARCH_MSM_HSUSB_H
#define __ASM_ARCH_MSM_HSUSB_H

#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include <linux/wakelock.h>
#include <mach/board.h>
#include <linux/pm_qos.h>
#include <linux/hrtimer.h>

#define MSM_PIPE_ID_MASK		(0x1F)
#define MSM_TX_PIPE_ID_OFS		(16)
#define MSM_SPS_MODE			BIT(5)
#define MSM_IS_FINITE_TRANSFER		BIT(6)
#define MSM_PRODUCER			BIT(7)
#define MSM_DISABLE_WB			BIT(8)
#define MSM_ETD_IOC			BIT(9)
#define MSM_INTERNAL_MEM		BIT(10)
#define MSM_VENDOR_ID			BIT(16)

enum usb_mode_type {
	USB_NONE = 0,
	USB_PERIPHERAL,
	USB_HOST,
	USB_OTG,
};

enum otg_control_type {
	OTG_NO_CONTROL = 0,
	OTG_PHY_CONTROL,
	OTG_PMIC_CONTROL,
	OTG_USER_CONTROL,
};

enum msm_usb_phy_type {
	INVALID_PHY = 0,
	CI_45NM_INTEGRATED_PHY,
	SNPS_28NM_INTEGRATED_PHY,
};

#define IDEV_CHG_MAX	1500
#define IDEV_CHG_MIN	500
#define IUNIT		100

#define IDEV_ACA_CHG_MAX	1500
#define IDEV_ACA_CHG_LIMIT	500

enum usb_chg_state {
	USB_CHG_STATE_UNDEFINED = 0,
	USB_CHG_STATE_WAIT_FOR_DCD,
	USB_CHG_STATE_DCD_DONE,
	USB_CHG_STATE_PRIMARY_DONE,
	USB_CHG_STATE_SECONDARY_DONE,
	USB_CHG_STATE_DETECTED,
};

enum usb_chg_type {
	USB_INVALID_CHARGER = 0,
	USB_SDP_CHARGER,
	USB_DCP_CHARGER,
	USB_CDP_CHARGER,
	USB_ACA_A_CHARGER,
	USB_ACA_B_CHARGER,
	USB_ACA_C_CHARGER,
	USB_ACA_DOCK_CHARGER,
	USB_PROPRIETARY_CHARGER,
};

enum usb_vdd_type {
	VDDCX_CORNER = 0,
	VDDCX,
	VDD_TYPE_MAX,
};

enum usb_vdd_value {
	VDD_NONE = 0,
	VDD_MIN,
	VDD_MAX,
	VDD_VAL_MAX,
};

#define POWER_COLLAPSE_LDO3V3	(1 << 0) 
#define POWER_COLLAPSE_LDO1V8	(1 << 1) 
struct msm_otg_platform_data {
	int *phy_init_seq;
	int (*vbus_power)(bool on);
	unsigned power_budget;
	enum usb_mode_type mode;
	enum otg_control_type otg_control;
	enum usb_mode_type default_mode;
	enum msm_usb_phy_type phy_type;
	void (*setup_gpio)(enum usb_otg_state state);
	int pmic_id_irq;
	bool mhl_enable;
	char *ldo_3v3_name;
	char *ldo_1v8_name;
	char *vddcx_name;
	bool disable_reset_on_disconnect;
	bool enable_lpm_on_dev_suspend;
	bool core_clk_always_on_workaround;
	struct msm_bus_scale_pdata *bus_scale_table;
	int reset_phy_before_lpm;
	void (*usb_uart_switch)(int uart);
	int ldo_power_collapse;
};

#define TA_WAIT_VRISE	100	
#define TA_WAIT_VFALL	500	

#ifdef CONFIG_USB_OTG
	
#define TA_WAIT_BCON	-1	
#else
#define TA_WAIT_BCON	-1
#endif

#define TA_AIDL_BDIS	500	
#define TA_BIDL_ADIS	155	
#define TB_SRP_FAIL	6000	
#define TB_ASE0_BRST	200	

#define TB_SRP_INIT	2000	

#define TA_TST_MAINT	10100	
#define TB_TST_SRP	3000	
#define TB_TST_CONFIG	300


#define A_WAIT_VRISE	0
#define A_WAIT_VFALL	1
#define A_WAIT_BCON	2
#define A_AIDL_BDIS	3
#define A_BIDL_ADIS	4
#define B_SRP_FAIL	5
#define B_ASE0_BRST	6
#define A_TST_MAINT	7
#define B_TST_SRP	8
#define B_TST_CONFIG	9

struct msm_otg {
	struct usb_phy phy;
	struct msm_otg_platform_data *pdata;
	int irq;
	struct clk *clk;
	struct clk *pclk;
	struct clk *phy_reset_clk;
	struct clk *core_clk;
	void __iomem *regs;
#define ID		0
#define B_SESS_VLD	1
#define ID_A		2
#define ID_B		3
#define ID_C		4
#define A_BUS_DROP	5
#define A_BUS_REQ	6
#define A_SRP_DET	7
#define A_VBUS_VLD	8
#define B_CONN		9
#define ADP_CHANGE	10
#define POWER_UP	11
#define A_CLR_ERR	12
#define A_BUS_RESUME	13
#define A_BUS_SUSPEND	14
#define A_CONN		15
#define B_BUS_REQ	16
	unsigned long inputs;
	struct work_struct sm_work;
	bool sm_work_pending;
	atomic_t pm_suspended;
	atomic_t in_lpm;
	int async_int;
	unsigned cur_power;
	struct delayed_work chg_work;
	struct delayed_work pmic_id_status_work;
	enum usb_chg_state chg_state;
	enum usb_chg_type chg_type;
	unsigned dcd_time;
	struct wake_lock wlock, usb_otg_wlock;
	struct wake_lock cable_detect_wlock;
	struct notifier_block usbdev_nb;
	unsigned mA_port;
	struct timer_list id_timer;
	unsigned long caps;
	struct msm_xo_voter *xo_handle;
	uint32_t bus_perf_client;
#define ALLOW_PHY_POWER_COLLAPSE	BIT(0)
#define ALLOW_PHY_RETENTION		BIT(1)
#define ALLOW_LPM_ON_DEV_SUSPEND	    BIT(2)
	unsigned long lpm_flags;
#define PHY_PWR_COLLAPSED		BIT(0)
#define PHY_RETENTIONED			BIT(1)
#define XO_SHUTDOWN			BIT(2)
	struct work_struct notifier_work;
	enum usb_connect_type connect_type;
	int connect_type_ready;
	struct workqueue_struct *usb_wq;
	struct delayed_work ac_detect_work;
	struct work_struct usb_disable_work;
	int ac_detect_count;
	int reset_phy_before_lpm;
	int reset_counter;
	unsigned long b_last_se0_sess;
	unsigned long tmouts;
	u8 active_tmout;
	struct hrtimer timer;
	enum usb_vdd_type vdd_type;
	struct delayed_work init_work;
};

struct msm_hsic_host_platform_data {
	unsigned strobe;
	unsigned data;
	struct msm_bus_scale_pdata *bus_scale_table;
	u32 swfi_latency;
};

struct msm_usb_host_platform_data {
	unsigned int power_budget;
	unsigned int dock_connect_irq;
};

struct msm_hsic_peripheral_platform_data {
	bool core_clk_always_on_workaround;
};

struct usb_bam_pipe_connect {
	u32 src_phy_addr;
	int src_pipe_index;
	u32 dst_phy_addr;
	int dst_pipe_index;
	u32 data_fifo_base_offset;
	u32 data_fifo_size;
	u32 desc_fifo_base_offset;
	u32 desc_fifo_size;
};

struct msm_usb_bam_platform_data {
	struct usb_bam_pipe_connect *connections;
	int usb_active_bam;
	int usb_bam_num_pipes;
};

enum usb_bam {
	HSUSB_BAM = 0,
	HSIC_BAM,
};

int msm_ep_config(struct usb_ep *ep);
int msm_ep_unconfig(struct usb_ep *ep);
int msm_data_fifo_config(struct usb_ep *ep, u32 addr, u32 size);

#endif
