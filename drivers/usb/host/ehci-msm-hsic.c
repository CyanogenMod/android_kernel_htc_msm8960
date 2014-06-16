/* ehci-msm-hsic.c - HSUSB Host Controller Driver Implementation
 *
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * Partly derived from ehci-fsl.c and ehci-hcd.c
 * Copyright (c) 2000-2004 by David Brownell
 * Copyright (c) 2005 MontaVista Software
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/wakelock.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <linux/usb/msm_hsusb_hw.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>

#include <mach/msm_bus.h>
#include <mach/clk.h>
#include <mach/msm_iomap.h>
#include <mach/msm_xo.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>
#include <mach/rpm-regulator.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/pm_qos.h>
#include <linux/rtc.h>
#include <mach/board_htc.h>
#include <linux/pm_qos.h>
#include <mach/cpuidle.h>
#include <linux/sched.h>

#define MSM_USB_BASE (hcd->regs)
#define USB_REG_START_OFFSET 0x90
#define USB_REG_END_OFFSET 0x250
#if 0
void (*set_htc_monitor_resume_state_fp)(void) = NULL;
EXPORT_SYMBOL(set_htc_monitor_resume_state_fp);
#endif

static const struct usb_device_id usb1_1[] = {
	{ USB_DEVICE(0x5c6, 0x9048),
	.driver_info = 0 },
	{ USB_DEVICE(0x5c6, 0x908A),
	.driver_info = 0 },
	{}
};

struct usb_hcd *mdm_hsic_usb_hcd = NULL;
struct usb_device *mdm_usb1_1_usbdev = NULL;
struct device *mdm_usb1_1_dev = NULL;
struct device *msm_hsic_host_dev = NULL;
#define HSIC_PM_MON_DELAY 5000
static struct delayed_work mdm_hsic_pm_monitor_delayed_work;
static struct delayed_work register_usb_notification_work;
void mdm_hsic_print_pm_info(void);

static void mdm_hsic_print_usb_dev_pm_info(struct usb_device *udev);
static void mdm_hsic_print_interface_pm_info(struct usb_device *udev);
static bool usb_device_recongnized = false;
unsigned long  mdm_hsic_phy_resume_jiffies = 0;
unsigned long  mdm_hsic_phy_active_total_ms = 0;
static bool usb_pm_debug_enabled = false;
#define HSIC_GPIO_CHECK_DELAY 5000
static struct delayed_work  ehci_gpio_check_wq;

#define RESUME_RETRY_LIMIT		3
#define RESUME_SIGNAL_TIME_USEC		(21 * 1000)
#define RESUME_SIGNAL_TIME_SOF_USEC	(23 * 1000)

static struct workqueue_struct  *ehci_wq;
struct ehci_timer {
#define GPT_LD(p)	((p) & 0x00FFFFFF)
	u32	gptimer0_ld;
#define GPT_RUN		BIT(31)
#define GPT_RESET	BIT(30)
#define GPT_MODE	BIT(24)
#define GPT_CNT(p)	((p) & 0x00FFFFFF)
	u32	gptimer0_ctrl;

	u32	gptimer1_ld;
	u32	gptimer1_ctrl;
};

static unsigned long long msm_hsic_wakeup_irq_timestamp = 0;
static unsigned long long msm_hsic_suspend_timestamp = 0;

#define HSIC_WAKEUP_CHECK_MIN_THRESHOLD	2
#define HSIC_WAKEUP_CHECK_MAX_THRESHOLD	7
#define NSEC_HSIC_WAKEUP_CHECK_THRESHOLD	(HSIC_WAKEUP_CHECK_MIN_THRESHOLD * NSEC_PER_SEC)

static void htc_hsic_dump_system_busy_info(void)
{
    extern unsigned long acpuclk_get_rate(int cpu);
    extern void htc_kernel_top(void);
    int cpu;

    pr_info("%s: Prepare to dump stack...\n", __func__);
    dump_stack();
    pr_info("\n ### Show Blocked State ###\n");
    show_state_filter(TASK_UNINTERRUPTIBLE);

    for_each_online_cpu(cpu)
        pr_info("cpu %d, acpuclk rate: %lu kHz\n", cpu,
            acpuclk_get_rate(cpu));

    htc_kernel_top();
}

void htc_hsic_wakeup_check(unsigned long long timestamp)
{
	static unsigned long long last_hsic_wakeup_check = 0;
	static unsigned long long last_hsic_suspend_check = 0;
	static unsigned int hsic_wakeup_check_count = HSIC_WAKEUP_CHECK_MIN_THRESHOLD;
	static unsigned int hsic_suspend_check_count = HSIC_WAKEUP_CHECK_MIN_THRESHOLD;

	if (!(get_radio_flag() & 0x0008))
		return;

	if (mdm_usb1_1_dev) {
		if (mdm_usb1_1_dev->power.runtime_status != RPM_ACTIVE) {
			if (msm_hsic_wakeup_irq_timestamp) {
				if (timestamp - msm_hsic_wakeup_irq_timestamp > (unsigned long long)NSEC_HSIC_WAKEUP_CHECK_THRESHOLD) {
					if (timestamp - last_hsic_wakeup_check > (unsigned long long)NSEC_PER_SEC) {
						if (hsic_wakeup_check_count <= HSIC_WAKEUP_CHECK_MAX_THRESHOLD) {
							pr_info("\n%s: HSIC remote wakeup was blocked for more than %d seconds!\n",
								__func__, hsic_wakeup_check_count++);

							htc_hsic_dump_system_busy_info();
						}
						last_hsic_wakeup_check = sched_clock();
					}
				}
			}
		}
		else {
			
			if (hsic_wakeup_check_count != HSIC_WAKEUP_CHECK_MIN_THRESHOLD) {
				pr_info("%s: remote wakeup complete\n", __func__);
			}
			msm_hsic_wakeup_irq_timestamp = 0;
			hsic_wakeup_check_count	= HSIC_WAKEUP_CHECK_MIN_THRESHOLD;
		}
	}

    if (msm_hsic_suspend_timestamp != 0) {
        if (timestamp - msm_hsic_suspend_timestamp > (unsigned long long)NSEC_HSIC_WAKEUP_CHECK_THRESHOLD) {
            if (timestamp - last_hsic_suspend_check > (unsigned long long)NSEC_PER_SEC) {
                if (hsic_suspend_check_count <= HSIC_WAKEUP_CHECK_MAX_THRESHOLD) {
                    pr_info("\n%s: HSIC remote suspend was blocked for more than %d seconds!\n",
                        __func__, hsic_suspend_check_count++);

                    htc_hsic_dump_system_busy_info();
                }
                last_hsic_suspend_check = sched_clock();
            }
        }
    }
    else {
        hsic_suspend_check_count = HSIC_WAKEUP_CHECK_MIN_THRESHOLD;
    }
}
EXPORT_SYMBOL(htc_hsic_wakeup_check);

struct msm_hsic_hcd {
	struct ehci_hcd		ehci;
	spinlock_t              wakeup_lock;
	struct device		*dev;
	struct clk		*ahb_clk;
	struct clk		*core_clk;
	struct clk		*alt_core_clk;
	struct clk		*phy_clk;
	struct clk		*cal_clk;
	struct regulator	*hsic_vddcx;
	bool			async_int;
	atomic_t                in_lpm;
	struct wake_lock	wlock;
	int			peripheral_status_irq;
	int			wakeup_irq;
	int			wakeup_gpio;
	int			ready_gpio;
	bool			wakeup_irq_enabled;
	atomic_t		pm_usage_cnt;
	uint32_t		bus_perf_client;
	uint32_t		wakeup_int_cnt;
	enum usb_vdd_type	vdd_type;

	struct work_struct	bus_vote_w;
	bool			bus_vote;

	
	struct ehci_timer __iomem *timer;
	struct completion	gpt0_completion;
	struct completion	rt_completion;
	int 		resume_status;
	int 		resume_again;
	int			bus_reset;
	int			reset_again;
	ktime_t			resume_start_t;

	struct pm_qos_request pm_qos_req_dma;
	struct task_struct	*resume_thread;
	
	struct pm_qos_request pm_qos_req_dma_htc;
	
};

static void ehci_hsic_prevent_sleep(struct msm_hsic_hcd *mehci)
{
	s32 latency;
	if (!in_interrupt()) {
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s+\n", __func__);
		latency =  msm_cpuidle_get_deep_idle_latency();
		if (!latency)
			latency = 2;
		pm_qos_update_request(&mehci->pm_qos_req_dma_htc, latency);
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s-\n", __func__);
	}
}

static void ehci_hsic_allow_sleep(struct msm_hsic_hcd *mehci)
{
	if (!in_interrupt()) {
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s+\n", __func__);
		pm_qos_update_request(&mehci->pm_qos_req_dma_htc, PM_QOS_DEFAULT_VALUE);
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s-\n", __func__);
	}
}

bool ehci_hsic_is_2nd_enum_done(void)
{
	return (mdm_usb1_1_usbdev ? true : false);
}
EXPORT_SYMBOL_GPL(ehci_hsic_is_2nd_enum_done);

extern int subsystem_restart(const char *name);
struct msm_hsic_hcd *__mehci;

static bool debug_bus_voting_enabled = true;

static unsigned int enable_dbg_log = 1;
module_param(enable_dbg_log, uint, S_IRUGO | S_IWUSR);
static unsigned int ep_addr_rxdbg_mask = 9;
module_param(ep_addr_rxdbg_mask, uint, S_IRUGO | S_IWUSR);
static unsigned int ep_addr_txdbg_mask = 9;
module_param(ep_addr_txdbg_mask, uint, S_IRUGO | S_IWUSR);

#define DBG_MSG_LEN   100UL

#define DBG_MAX_MSG   256UL

#define TIME_BUF_LEN  20

enum event_type {
	EVENT_UNDEF = -1,
	URB_SUBMIT,
	URB_COMPLETE,
	EVENT_NONE,
};

#define EVENT_STR_LEN	5
void __iomem *clk_regs;

static char *event_to_str(enum event_type e)
{
	switch (e) {
	case URB_SUBMIT:
		return "S";
	case URB_COMPLETE:
		return "C";
	case EVENT_NONE:
		return "NONE";
	default:
		return "UNDEF";
	}
}

static enum event_type str_to_event(const char *name)
{
	if (!strncasecmp("S", name, EVENT_STR_LEN))
		return URB_SUBMIT;
	if (!strncasecmp("C", name, EVENT_STR_LEN))
		return URB_COMPLETE;
	if (!strncasecmp("", name, EVENT_STR_LEN))
		return EVENT_NONE;

	return EVENT_UNDEF;
}

static struct {
	char     (buf[DBG_MAX_MSG])[DBG_MSG_LEN];   
	unsigned idx;   
	rwlock_t lck;   
} dbg_hsic_ctrl = {
	.idx = 0,
	.lck = __RW_LOCK_UNLOCKED(lck)
};

static struct {
	char     (buf[DBG_MAX_MSG])[DBG_MSG_LEN];   
	unsigned idx;   
	rwlock_t lck;   
} dbg_hsic_data = {
	.idx = 0,
	.lck = __RW_LOCK_UNLOCKED(lck)
};

static void dbg_inc(unsigned *idx)
{
	*idx = (*idx + 1) & (DBG_MAX_MSG-1);
}

static char *get_timestamp(char *tbuf)
{
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000)/1000;
	scnprintf(tbuf, TIME_BUF_LEN, "[%5lu.%06lu] ", (unsigned long)t,
		nanosec_rem);
	return tbuf;
}

static int allow_dbg_log(int ep_addr)
{
	int dir, num;

	dir = ep_addr & USB_DIR_IN ? USB_DIR_IN : USB_DIR_OUT;
	num = ep_addr & ~USB_DIR_IN;
	num = 1 << num;

	if ((dir == USB_DIR_IN) && (num & ep_addr_rxdbg_mask))
		return 1;
	if ((dir == USB_DIR_OUT) && (num & ep_addr_txdbg_mask))
		return 1;

	return 0;
}

void dbg_log_event(struct urb *urb, char * event, unsigned extra)
{
	unsigned long flags;
	int ep_addr;
	char tbuf[TIME_BUF_LEN];

	if (!enable_dbg_log)
		return;

	if (!urb) {
		write_lock_irqsave(&dbg_hsic_ctrl.lck, flags);
		scnprintf(dbg_hsic_ctrl.buf[dbg_hsic_ctrl.idx], DBG_MSG_LEN,
			"%s: %s : %u\n", get_timestamp(tbuf), event, extra);
		dbg_inc(&dbg_hsic_ctrl.idx);
		write_unlock_irqrestore(&dbg_hsic_ctrl.lck, flags);
		return;
	}

	ep_addr = urb->ep->desc.bEndpointAddress;
	if (!allow_dbg_log(ep_addr))
		return;

	if ((ep_addr & 0x0f) == 0x0) {
		
		if (!str_to_event(event)) {
			write_lock_irqsave(&dbg_hsic_ctrl.lck, flags);
			scnprintf(dbg_hsic_ctrl.buf[dbg_hsic_ctrl.idx],
				DBG_MSG_LEN, "%s: [%s : %p]:[%s] "
				  "%02x %02x %04x %04x %04x  %u %d\n",
				  get_timestamp(tbuf), event, urb,
				  (ep_addr & USB_DIR_IN) ? "in" : "out",
				  urb->setup_packet[0], urb->setup_packet[1],
				  (urb->setup_packet[3] << 8) |
				  urb->setup_packet[2],
				  (urb->setup_packet[5] << 8) |
				  urb->setup_packet[4],
				  (urb->setup_packet[7] << 8) |
				  urb->setup_packet[6],
				  urb->transfer_buffer_length, urb->status);

			dbg_inc(&dbg_hsic_ctrl.idx);
			write_unlock_irqrestore(&dbg_hsic_ctrl.lck, flags);
		} else {
			write_lock_irqsave(&dbg_hsic_ctrl.lck, flags);
			scnprintf(dbg_hsic_ctrl.buf[dbg_hsic_ctrl.idx],
				DBG_MSG_LEN, "%s: [%s : %p]:[%s] %u %d\n",
				  get_timestamp(tbuf), event, urb,
				  (ep_addr & USB_DIR_IN) ? "in" : "out",
				  urb->actual_length, extra);

			dbg_inc(&dbg_hsic_ctrl.idx);
			write_unlock_irqrestore(&dbg_hsic_ctrl.lck, flags);
		}
	} else {
		write_lock_irqsave(&dbg_hsic_data.lck, flags);
		scnprintf(dbg_hsic_data.buf[dbg_hsic_data.idx], DBG_MSG_LEN,
			  "%s: [%s : %p]:ep%d[%s]  %u %d\n",
			  get_timestamp(tbuf), event, urb, ep_addr & 0x0f,
			  (ep_addr & USB_DIR_IN) ? "in" : "out",
			  str_to_event(event) ? urb->actual_length :
			  urb->transfer_buffer_length,
			  str_to_event(event) ?  extra : urb->status);

		dbg_inc(&dbg_hsic_data.idx);
		write_unlock_irqrestore(&dbg_hsic_data.lck, flags);
	}
}
EXPORT_SYMBOL(dbg_log_event);

static int in_progress;

static void do_restart(struct work_struct *dummy)
{
#ifdef CONFIG_ARCH_APQ8064
	
	
	int err_fatal=0;
	
	int mdm2ap_status = 0;
	
	
	extern int mdm_common_htc_get_mdm2ap_errfatal_level(void);
	extern int mdm_common_htc_get_mdm2ap_status_level(void);

	err_fatal = mdm_common_htc_get_mdm2ap_errfatal_level();
	mdm2ap_status = mdm_common_htc_get_mdm2ap_status_level();

	pr_info("%s: inprocess: %d, err_fatal:%d, system_state: %d, mdm2ap_status: %d \n", __func__, in_progress, err_fatal, system_state, mdm2ap_status);
	if(!in_progress && !err_fatal && mdm2ap_status == 1 && system_state == SYSTEM_RUNNING){
		in_progress = 1;
		pr_info("%s: do SSR-!\n", __func__);
		subsystem_restart("external_modem");
	} else if (!in_progress && err_fatal && mdm2ap_status == 1 && system_state == SYSTEM_RUNNING ) {
		
		pr_info("%s: schedule ehci_gpio_check_wq to check err_fatal state\n", __func__);
		schedule_delayed_work(&ehci_gpio_check_wq, msecs_to_jiffies(HSIC_GPIO_CHECK_DELAY));
	}
#endif 
}

static DECLARE_DELAYED_WORK(htc_ehci_do_mdm_restart_delay_work, do_restart);
void htc_ehci_trigger_mdm_restart(void)
{
	pr_info("%s[%d]\n", __func__, __LINE__);
	schedule_delayed_work_on(0, &htc_ehci_do_mdm_restart_delay_work, msecs_to_jiffies(10));
}
EXPORT_SYMBOL_GPL(htc_ehci_trigger_mdm_restart);

static inline struct msm_hsic_hcd *hcd_to_hsic(struct usb_hcd *hcd)
{
	return (struct msm_hsic_hcd *) (hcd->hcd_priv);
}

static inline struct usb_hcd *hsic_to_hcd(struct msm_hsic_hcd *mehci)
{
	return container_of((void *) mehci, struct usb_hcd, hcd_priv);
}


static void dump_qh_qtd(struct usb_hcd *hcd)
{
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct ehci_hcd *ehci = &mehci->ehci;
	struct ehci_qh	*head, *qh;
	struct ehci_qh_hw *hw;
	struct ehci_qtd *qtd;
	struct list_head *entry, *tmp;
	struct ehci_qtd	*qtd1;

	pr_info("-----------------------------DUMPING EHCI QHs & QTDs-------------------------------\n");

	head = ehci->async;
	hw = head->hw;
	pr_info("Current QH: %p\n",head);
	pr_info("Overlay:\n next %08x, info %x %x, cur qtd %x\n", hw->hw_next, hw->hw_info1, hw->hw_info2, hw->hw_current);
	qtd =  (struct ehci_qtd *)&hw->hw_qtd_next;
	pr_info("Current QTD: %p\n",qtd);
	pr_info("td %p, next qtd %08x %08x, token %08x p0=%08x\n", qtd,
		hc32_to_cpup(ehci, &qtd->hw_next),
		hc32_to_cpup(ehci, &qtd->hw_alt_next),
		hc32_to_cpup(ehci, &qtd->hw_token),
		hc32_to_cpup(ehci, &qtd->hw_buf [0]));
	if (qtd->hw_buf [1])
		pr_info("  p1=%08x p2=%08x p3=%08x p4=%08x\n",
			hc32_to_cpup(ehci, &qtd->hw_buf[1]),
			hc32_to_cpup(ehci, &qtd->hw_buf[2]),
			hc32_to_cpup(ehci, &qtd->hw_buf[3]),
			hc32_to_cpup(ehci, &qtd->hw_buf[4]));
	pr_info("---------------------------LIST OF QTDs for current QH: %p---------------------------------\n", head);
	list_for_each_safe (entry, tmp, &head->qtd_list) {

	qtd1 = list_entry (entry, struct ehci_qtd, qtd_list);
	pr_info("td %p, next qtd %08x %08x, token %08x p0=%08x\n", qtd1,
			hc32_to_cpup(ehci, &qtd1->hw_next),
			hc32_to_cpup(ehci, &qtd1->hw_alt_next),
			hc32_to_cpup(ehci, &qtd1->hw_token),
			hc32_to_cpup(ehci, &qtd1->hw_buf [0]));
	if (qtd->hw_buf [1])
		pr_info("  p1=%08x p2=%08x p3=%08x p4=%08x\n",
				hc32_to_cpup(ehci, &qtd1->hw_buf[1]),
				hc32_to_cpup(ehci, &qtd1->hw_buf[2]),
				hc32_to_cpup(ehci, &qtd1->hw_buf[3]),
				hc32_to_cpup(ehci, &qtd1->hw_buf[4]));

	}
	qh = head->qh_next.qh;
	while (qh){
		pr_info("---------------------------QH %p--------------------------\n",qh);
		hw = qh->hw;
		pr_info("Current QH: %p\n",qh);
		pr_info("Overlay:\n next %08x, info %x %x, cur qtd %x\n", hw->hw_next, hw->hw_info1, hw->hw_info2, hw->hw_current);

		qtd = (struct ehci_qtd *)&hw->hw_qtd_next;
		pr_info("Current QTD: %p\n",qtd);
		pr_info("td %p, next qtd %08x %08x, token %08x p0=%08x\n", qtd,
				hc32_to_cpup(ehci, &qtd->hw_next),
				hc32_to_cpup(ehci, &qtd->hw_alt_next),
				hc32_to_cpup(ehci, &qtd->hw_token),
				hc32_to_cpup(ehci, &qtd->hw_buf [0]));
		if (qtd->hw_buf [1])
			pr_info("  p1=%08x p2=%08x p3=%08x p4=%08x\n",
					hc32_to_cpup(ehci, &qtd->hw_buf[1]),
					hc32_to_cpup(ehci, &qtd->hw_buf[2]),
					hc32_to_cpup(ehci, &qtd->hw_buf[3]),
					hc32_to_cpup(ehci, &qtd->hw_buf[4]));

		list_for_each_safe (entry, tmp, &head->qtd_list) {

		qtd1 = list_entry (entry, struct ehci_qtd, qtd_list);
		pr_info("td %p, next qtd %08x %08x, token %08x p0=%08x\n", qtd1,
				hc32_to_cpup(ehci, &qtd1->hw_next),
				hc32_to_cpup(ehci, &qtd1->hw_alt_next),
				hc32_to_cpup(ehci, &qtd1->hw_token),
				hc32_to_cpup(ehci, &qtd1->hw_buf [0]));
		if (qtd->hw_buf [1])
			pr_info("  p1=%08x p2=%08x p3=%08x p4=%08x\n",
					hc32_to_cpup(ehci, &qtd1->hw_buf[1]),
					hc32_to_cpup(ehci, &qtd1->hw_buf[2]),
					hc32_to_cpup(ehci, &qtd1->hw_buf[3]),
					hc32_to_cpup(ehci, &qtd1->hw_buf[4]));

		}

		qh = qh->qh_next.qh;
	}
}


static void dump_hsic_regs(struct usb_hcd *hcd)
{
	int i;
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (atomic_read(&mehci->in_lpm))
		return;

	for (i = USB_REG_START_OFFSET; i <= USB_REG_END_OFFSET; i += 0x10)
		pr_info("%p: %08x\t%08x\t%08x\t%08x\n", hcd->regs + i,
				readl_relaxed(hcd->regs + i),
				readl_relaxed(hcd->regs + i + 4),
				readl_relaxed(hcd->regs + i + 8),
				readl_relaxed(hcd->regs + i + 0xc));

}

#define LOG_WITH_TIMESTAMP(x...) do { \
struct timespec ts; \
struct rtc_time tm; \
getnstimeofday(&ts); \
rtc_time_to_tm(ts.tv_sec, &tm); \
printk(KERN_INFO "[HSIC] " x); \
printk(" at %lld (%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", \
ktime_to_ns(ktime_get()), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, \
tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec); \
} while (0)

static void mdm_hsic_gpio_check_func(struct work_struct *work)
{
#ifdef CONFIG_ARCH_APQ8064
	int err_fatal=0;
	int mdm2ap_status = 0;

	extern int mdm_common_htc_get_mdm2ap_errfatal_level(void);
	extern int mdm_common_htc_get_mdm2ap_status_level(void);

	err_fatal = mdm_common_htc_get_mdm2ap_errfatal_level();
	mdm2ap_status = mdm_common_htc_get_mdm2ap_status_level();

	pr_info("%s: inprocess: %d, err_fatal:%d, system_state: %d, mdm2ap_status: %d \n", __func__, in_progress, err_fatal, system_state, mdm2ap_status);
	if (!in_progress && err_fatal && mdm2ap_status == 1 && system_state == SYSTEM_RUNNING ) {
		in_progress = 1;
		pr_info("%s: do SSR-!\n", __func__);
		subsystem_restart("external_modem");
	}
#endif 
}

static void mdm_hsic_pm_monitor_func(struct work_struct *work)
{
	struct usb_device *udev = mdm_usb1_1_usbdev;
	struct usb_interface *intf;	int status;
	if (udev == NULL)
		return;
	mdm_hsic_print_pm_info();
	if (jiffies_to_msecs(jiffies - ACCESS_ONCE(udev->dev.power.last_busy)) > 3000 &&
			udev->dev.power.timer_expires == 0)	{
		pr_info("%s(%d) activate timer by get put interface !!!\n", __func__, __LINE__);
		intf = usb_ifnum_to_if(udev, 0);
		if (intf) {
			usb_lock_device(udev);
			status = usb_autopm_get_interface(intf);
			if (status == 0) {
				pr_info("%s(%d) usb_autopm_get_interface OK\n", __func__, __LINE__);
				usb_autopm_put_interface(intf);
			} else {
				pr_info("%s(%d) error status:%d\n", __func__, __LINE__, status);
			}
			usb_unlock_device(udev);
		} else {
			pr_info("%s(%d) usb_ifnum_to_if error\n", __func__, __LINE__);
		}
	}
	schedule_delayed_work(&mdm_hsic_pm_monitor_delayed_work, msecs_to_jiffies(HSIC_PM_MON_DELAY));

}
static void mdm_hsic_print_interface_pm_info(struct usb_device *udev)
{
	struct usb_interface *intf;
	int	i = 0, n = 0;
	if (udev == NULL)
		return;
	if (udev->actconfig) {
		n = udev->actconfig->desc.bNumInterfaces;
		for (i = 0; i <= n - 1; i++) {
			intf = udev->actconfig->interface[i];
			
#ifdef HTC_PM_DBG
			if (usb_pm_debug_enabled) {
				if (i == 5)
					printk("\n");
				if ((i >= 5) && (i<=8)) 
					printk("%d>LB:%8lx,LC:%10u LCD:%10u, PM_CNT: %d,", i,
						ACCESS_ONCE(intf->last_busy_jiffies),
						intf->busy_cnt,
						intf->data_busy_cnt, atomic_read(&intf->pm_usage_cnt));
				else
					printk("%d>LB:%8lx,LC:%10u, PM_CNT: %d", i,
						ACCESS_ONCE(intf->last_busy_jiffies),
						intf->busy_cnt, atomic_read(&intf->pm_usage_cnt));
			}
#endif
		}
		printk("\n");
	}
}
static void mdm_hsic_print_usb_dev_pm_info(struct usb_device *udev)
{
	if (udev != NULL) {
		struct device *dev = &(udev->dev);
#ifdef HTC_PM_DBG
		if (usb_pm_debug_enabled) {
			dev_info(&udev->dev, "[HSIC_PM_DBG] is_suspend:%d usage_count:%d last_busy:%8lx auto_suspend_timer_set:%d timer_expires:%8lx jiffies:%lx\n",
				udev->is_suspend,
				atomic_read(&(udev->dev.power.usage_count)),
				ACCESS_ONCE(udev->dev.power.last_busy),
				udev->auto_suspend_timer_set,
				dev->power.timer_expires,
				jiffies);
			mdm_hsic_print_interface_pm_info(udev);
		}
#endif	
	}
}

static void mdm_hsic_usb_device_add_handler(struct usb_device *udev)
{
	struct usb_interface *intf = usb_ifnum_to_if(udev, 0);
	const struct usb_device_id *usb1_1_id;
	if (intf == NULL)
		return;

	pr_info("%s(%d) USB device added %d <%s %s>\n", __func__, __LINE__, udev->devnum, udev->manufacturer, udev->product);
	usb1_1_id = usb_match_id(intf, usb1_1);
	if (usb1_1_id) {
		usb_device_recongnized = true;
		pr_info("%s: usb 1-1 found \n", __func__);
		mdm_usb1_1_usbdev = udev;
		mdm_usb1_1_dev = &(udev->dev);

#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD){
			htc_pm_runtime_enable_hsic_dbg(&udev->dev, 1);
		}
		if (udev->dev.parent) {
			if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD){
				htc_pm_runtime_enable_hsic_dbg(udev->dev.parent, 1);
			}
			if (udev->dev.parent->parent) {
				if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD){
					htc_pm_runtime_enable_hsic_dbg(udev->dev.parent->parent, 1);
				}
				if (!strncmp(dev_name(udev->dev.parent->parent), "msm_hsic_host", 13)) {
					msm_hsic_host_dev = udev->dev.parent->parent;
					pr_info("%s: msm_hsic_host_dev:%x \n", __func__, (unsigned int)msm_hsic_host_dev);
				}
			}
		}
#endif	
	}
}
static void mdm_hsic_usb_device_remove_handler(struct usb_device *udev)
{
	if (mdm_usb1_1_usbdev == udev) {
		usb_device_recongnized = false;
		pr_info("Remove device %d <%s %s>\n", udev->devnum,	udev->manufacturer, udev->product);
		mdm_usb1_1_usbdev = NULL;
		mdm_usb1_1_dev = NULL;
	}
}
static int mdm_hsic_usb_notify(struct notifier_block *self, unsigned long action,	void *blob)
{

	switch (action)	{
		case USB_DEVICE_ADD:
			mdm_hsic_usb_device_add_handler(blob);
			break;
		case USB_DEVICE_REMOVE:
			mdm_hsic_usb_device_remove_handler(blob);
			break;
		}
	return NOTIFY_OK;
}
struct notifier_block mdm_hsic_usb_nb = {
	.notifier_call = mdm_hsic_usb_notify,
};
void mdm_hsic_print_pm_info(void)
{
	mdm_hsic_print_usb_dev_pm_info(mdm_usb1_1_usbdev);
}EXPORT_SYMBOL_GPL(mdm_hsic_print_pm_info);

static void register_usb_notification_func(struct work_struct *work)
{
	usb_register_notify(&mdm_hsic_usb_nb);
}




#define ULPI_IO_TIMEOUT_USEC	(10 * 1000)

#define USB_PHY_VDD_DIG_VOL_NONE	0 
#define USB_PHY_VDD_DIG_VOL_MIN		1000000 
#define USB_PHY_VDD_DIG_VOL_MAX		1320000 

#define HSIC_DBG1_REG		0x38

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

static int msm_hsic_init_vddcx(struct msm_hsic_hcd *mehci, int init)
{
	int ret = 0;
	int none_vol, min_vol, max_vol;

	if (!mehci->hsic_vddcx) {
		mehci->vdd_type = VDDCX_CORNER;
		mehci->hsic_vddcx = devm_regulator_get(mehci->dev,
			"hsic_vdd_dig");
		if (IS_ERR(mehci->hsic_vddcx)) {
			mehci->hsic_vddcx = devm_regulator_get(mehci->dev,
				"HSIC_VDDCX");
			if (IS_ERR(mehci->hsic_vddcx)) {
				dev_err(mehci->dev, "unable to get hsic vddcx\n");
				return PTR_ERR(mehci->hsic_vddcx);
			}
			mehci->vdd_type = VDDCX;
		}
	}

	none_vol = vdd_val[mehci->vdd_type][VDD_NONE];
	min_vol = vdd_val[mehci->vdd_type][VDD_MIN];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	if (!init)
		goto disable_reg;

	ret = regulator_set_voltage(mehci->hsic_vddcx, min_vol, max_vol);
	if (ret) {
		dev_err(mehci->dev, "unable to set the voltage"
				"for hsic vddcx\n");
		return ret;
	}

	ret = regulator_enable(mehci->hsic_vddcx);
	if (ret) {
		dev_err(mehci->dev, "unable to enable hsic vddcx\n");
		goto reg_enable_err;
	}

	return 0;

disable_reg:
	regulator_disable(mehci->hsic_vddcx);
reg_enable_err:
	regulator_set_voltage(mehci->hsic_vddcx, none_vol, max_vol);

	return ret;

}

static int __maybe_unused ulpi_read(struct msm_hsic_hcd *mehci, u32 reg)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	
	
	int cnt = 0;
	

	
	writel_relaxed(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
	       USB_ULPI_VIEWPORT);

	
	
	
	
	
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		dev_err(mehci->dev, "ulpi_read timeout\n");
		return -ETIMEDOUT;
	}
	

	return ULPI_DATA_READ(readl_relaxed(USB_ULPI_VIEWPORT));
}

static int ulpi_write(struct msm_hsic_hcd *mehci, u32 val, u32 reg)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0;

	
	writel_relaxed(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val),
	       USB_ULPI_VIEWPORT);

	
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		dev_err(mehci->dev, "ulpi_write: timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int ehci_hsic_int_latency(struct usb_hcd *hcd, int latency)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 cmd;
	unsigned long flags;
	int f01 = 0;

	if (latency < 0 || latency > 6)
		return -EINVAL;

#if defined(CONFIG_MACH_M7_WLV)
	if (latency == 6)
		latency = 5;
#endif

	f01 = 1;
	spin_lock_irqsave(&ehci->lock, flags);
	f01 = 2;
	dbg_log_event(NULL, "ITC Modify", latency);
	f01 = 3;
	pr_info("[%s] ITC modify %d\n", __func__,latency);


	f01 = 4;
	cmd = ehci_readl(ehci, &ehci->regs->command);
	f01 = 5;
	cmd &= ~CMD_ITC;
	cmd |= 1 << (16 + latency);
	f01 = 6;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	f01 = 7;
	spin_unlock_irqrestore(&ehci->lock, flags);
	f01 = 8;

	return 0;
}

static int msm_hsic_config_gpios(struct msm_hsic_hcd *mehci, int gpio_en)
{
	int rc = 0;
	struct msm_hsic_host_platform_data *pdata;
	static int gpio_status;

	pdata = mehci->dev->platform_data;

	if (!pdata || !pdata->strobe || !pdata->data)
		return rc;

	if (gpio_status == gpio_en)
		return 0;

	gpio_status = gpio_en;

	if (!gpio_en)
		goto free_gpio;

	rc = gpio_request(pdata->strobe, "HSIC_STROBE_GPIO");
	if (rc < 0) {
		dev_err(mehci->dev, "gpio request failed for HSIC STROBE\n");
		return rc;
	}

	rc = gpio_request(pdata->data, "HSIC_DATA_GPIO");
	if (rc < 0) {
		dev_err(mehci->dev, "gpio request failed for HSIC DATA\n");
		goto free_strobe;
		}

	if (mehci->wakeup_gpio) {
		rc = gpio_request(mehci->wakeup_gpio, "HSIC_WAKEUP_GPIO");
		if (rc < 0) {
			dev_err(mehci->dev, "gpio request failed for HSIC WAKEUP\n");
			goto free_data;
		}
	}

	return 0;

free_gpio:
	if (mehci->wakeup_gpio)
		gpio_free(mehci->wakeup_gpio);
free_data:
	gpio_free(pdata->data);
free_strobe:
	gpio_free(pdata->strobe);

	return rc;
}

static void msm_hsic_clk_reset(struct msm_hsic_hcd *mehci)
{
	int ret;

	ret = clk_reset(mehci->core_clk, CLK_RESET_ASSERT);
	if (ret) {
		dev_err(mehci->dev, "hsic clk assert failed:%d\n", ret);
		return;
	}
	clk_disable(mehci->core_clk);

	ret = clk_reset(mehci->core_clk, CLK_RESET_DEASSERT);
	if (ret)
		dev_err(mehci->dev, "hsic clk deassert failed:%d\n", ret);

	usleep_range(10000, 12000);

	clk_enable(mehci->core_clk);
}

#define HSIC_STROBE_GPIO_PAD_CTL	(MSM_TLMM_BASE+0x20C0)
#define HSIC_DATA_GPIO_PAD_CTL		(MSM_TLMM_BASE+0x20C4)
#define HSIC_CAL_PAD_CTL       (MSM_TLMM_BASE+0x20C8)
#define HSIC_LV_MODE		0x04
#define HSIC_PAD_CALIBRATION	0xA8
#define HSIC_GPIO_PAD_VAL	0x0A1EBE10
#define LINK_RESET_TIMEOUT_USEC		(250 * 1000)
static int msm_hsic_reset(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int ret;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;

	msm_hsic_clk_reset(mehci);

	
	writel_relaxed(0x80000000, USB_PORTSC);

	mb();

	if (pdata && pdata->strobe && pdata->data) {

		
		writel_relaxed(HSIC_LV_MODE, HSIC_CAL_PAD_CTL);

		mb();

		ulpi_write(mehci, 0xFF, 0x33);

		
		ulpi_write(mehci, HSIC_PAD_CALIBRATION, 0x30);

		
		ret = msm_hsic_config_gpios(mehci, 1);
		if (ret) {
			dev_err(mehci->dev, " gpio configuarion failed\n");
			return ret;
		}
		
		writel_relaxed(HSIC_GPIO_PAD_VAL, HSIC_STROBE_GPIO_PAD_CTL);
		writel_relaxed(HSIC_GPIO_PAD_VAL, HSIC_DATA_GPIO_PAD_CTL);

		mb();

		
		ulpi_write(mehci, 0x01, 0x31);
	} else {

		
		ret = ulpi_write(mehci, 3, HSIC_DBG1_REG);
		if (ret) {
			pr_err("%s: Unable to program length of connect "
			      "signaling\n", __func__);
		}

		ulpi_write(mehci, 0xFF, 0x33);

		
		ulpi_write(mehci, 0xA9, 0x30);
	}

	
	ulpi_write(mehci, ULPI_IFC_CTRL_AUTORESUME, ULPI_CLR(ULPI_IFC_CTRL));

	return 0;
}

#define PHY_SUSPEND_TIMEOUT_USEC	(500 * 1000)
#define PHY_RESUME_TIMEOUT_USEC		(100 * 1000)

#ifdef CONFIG_PM_SLEEP
static int msm_hsic_suspend(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0, ret;
	u32 val;
	int none_vol, max_vol;
#ifdef HTC_PM_DBG
	unsigned long  elapsed_ms = 0;
	static unsigned int suspend_cnt = 0;
#endif

	if (atomic_read(&mehci->in_lpm)) {
		dev_err(mehci->dev, "%s called in lpm\n", __func__);
		return 0;
	}

	disable_irq(hcd->irq);

	
	if (test_bit(HCD_FLAG_WAKEUP_PENDING, &hcd->flags) ||
	    readl_relaxed(USB_PORTSC) & PORT_RESUME) {
		dev_err(mehci->dev, "wakeup pending, aborting suspend\n");
		enable_irq(hcd->irq);
		return -EBUSY;
	}

	
	msm_hsic_suspend_timestamp = sched_clock();
	

	val = readl_relaxed(USB_PORTSC);
	val &= ~PORT_RWC_BITS;
	val |= PORTSC_PHCD;
	writel_relaxed(val, USB_PORTSC);
	while (cnt < PHY_SUSPEND_TIMEOUT_USEC) {
		if (readl_relaxed(USB_PORTSC) & PORTSC_PHCD)
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= PHY_SUSPEND_TIMEOUT_USEC) {
		dev_err(mehci->dev, "Unable to suspend PHY\n");
		msm_hsic_config_gpios(mehci, 0);
		msm_hsic_reset(mehci);
	}

	writel_relaxed(readl_relaxed(USB_USBCMD) | ASYNC_INTR_CTRL |
				ULPI_STP_CTRL, USB_USBCMD);

	mb();

	clk_disable_unprepare(mehci->core_clk);
	clk_disable_unprepare(mehci->phy_clk);
	clk_disable_unprepare(mehci->cal_clk);
	clk_disable_unprepare(mehci->ahb_clk);

	none_vol = vdd_val[mehci->vdd_type][VDD_NONE];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	ret = regulator_set_voltage(mehci->hsic_vddcx, none_vol, max_vol);
	if (ret < 0)
		dev_err(mehci->dev, "unable to set vddcx voltage for VDD MIN\n");

	if (mehci->bus_perf_client && debug_bus_voting_enabled) {
			mehci->bus_vote = false;
			queue_work(ehci_wq, &mehci->bus_vote_w);
	}

	atomic_set(&mehci->in_lpm, 1);
	enable_irq(hcd->irq);

	mehci->wakeup_irq_enabled = 1;
	enable_irq_wake(mehci->wakeup_irq);
	enable_irq(mehci->wakeup_irq);
	
#ifdef HTC_PM_DBG
	if (usb_pm_debug_enabled) {
		if (mdm_hsic_phy_resume_jiffies != 0) {
			elapsed_ms = jiffies_to_msecs(jiffies - mdm_hsic_phy_resume_jiffies);
			mdm_hsic_phy_active_total_ms += elapsed_ms ;
			LOG_WITH_TIMESTAMP("%s elapsed_ms: %lu ms, total: %lu", __func__, elapsed_ms, mdm_hsic_phy_active_total_ms);
		}
		suspend_cnt++;
		if (elapsed_ms > 30000 || suspend_cnt >= 10) {
			suspend_cnt = 0;
			mdm_hsic_print_pm_info();
		}
	}
#endif
	if (usb_device_recongnized)
		cancel_delayed_work(&mdm_hsic_pm_monitor_delayed_work);
	
	wake_unlock(&mehci->wlock);

	
	ehci_hsic_allow_sleep(mehci);
	

	
	msm_hsic_suspend_timestamp = 0;
	

	dev_info(mehci->dev, "HSIC-USB in low power mode\n");

	return 0;
}

static int msm_hsic_resume(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0, ret;
	unsigned temp;
	int min_vol, max_vol;
	unsigned long flags;

	if (!atomic_read(&mehci->in_lpm)) {
		dev_err(mehci->dev, "%s called in !in_lpm\n", __func__);
		return 0;
	}

	spin_lock_irqsave(&mehci->wakeup_lock, flags);
	if (mehci->wakeup_irq_enabled) {
		disable_irq_wake(mehci->wakeup_irq);
		disable_irq_nosync(mehci->wakeup_irq);
		mehci->wakeup_irq_enabled = 0;
	}
	spin_unlock_irqrestore(&mehci->wakeup_lock, flags);

	wake_lock(&mehci->wlock);
	
	ehci_hsic_prevent_sleep(mehci);
	

	
	mdm_hsic_phy_resume_jiffies = jiffies;
	if (usb_device_recongnized)
		schedule_delayed_work(&mdm_hsic_pm_monitor_delayed_work, msecs_to_jiffies(HSIC_PM_MON_DELAY));
	

	if (mehci->bus_perf_client && debug_bus_voting_enabled) {
			mehci->bus_vote = true;
			queue_work(ehci_wq, &mehci->bus_vote_w);
	}

	min_vol = vdd_val[mehci->vdd_type][VDD_MIN];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	ret = regulator_set_voltage(mehci->hsic_vddcx, min_vol, max_vol);
	if (ret < 0)
		dev_err(mehci->dev, "unable to set nominal vddcx voltage (no VDD MIN)\n");

	clk_prepare_enable(mehci->core_clk);
	clk_prepare_enable(mehci->phy_clk);
	clk_prepare_enable(mehci->cal_clk);
	clk_prepare_enable(mehci->ahb_clk);

	temp = readl_relaxed(USB_USBCMD);
	temp &= ~ASYNC_INTR_CTRL;
	temp &= ~ULPI_STP_CTRL;
	writel_relaxed(temp, USB_USBCMD);

	if (!(readl_relaxed(USB_PORTSC) & PORTSC_PHCD))
		goto skip_phy_resume;

	temp = readl_relaxed(USB_PORTSC);
	temp &= ~(PORT_RWC_BITS | PORTSC_PHCD);
	writel_relaxed(temp, USB_PORTSC);
	while (cnt < PHY_RESUME_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_PORTSC) & PORTSC_PHCD) &&
			(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_SYNC_STATE))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= PHY_RESUME_TIMEOUT_USEC) {
		dev_err(mehci->dev, "Unable to resume USB. Reset the hsic\n");
		msm_hsic_config_gpios(mehci, 0);
		msm_hsic_reset(mehci);
	}

skip_phy_resume:

	usb_hcd_resume_root_hub(hcd);

	atomic_set(&mehci->in_lpm, 0);

	if (mehci->async_int) {
		mehci->async_int = false;
		
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s(%d): pm_runtime_put_noidle\n", __func__, __LINE__);
		
		pm_runtime_put_noidle(mehci->dev);
		enable_irq(hcd->irq);
	}

	
	spin_lock_irqsave(&mehci->wakeup_lock, flags);
	

	if (atomic_read(&mehci->pm_usage_cnt)) {
		atomic_set(&mehci->pm_usage_cnt, 0);
		
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			pr_info("%s(%d): pm_runtime_put_noidle\n", __func__, __LINE__);
		
		pm_runtime_put_noidle(mehci->dev);
	}

	
	spin_unlock_irqrestore(&mehci->wakeup_lock, flags);
	

	dev_info(mehci->dev, "HSIC-USB exited from low power mode\n");

	return 0;
}
#endif

static void ehci_hsic_bus_vote_w(struct work_struct *w)
{
	struct msm_hsic_hcd *mehci =
			container_of(w, struct msm_hsic_hcd, bus_vote_w);
	int ret;

	ret = msm_bus_scale_client_update_request(mehci->bus_perf_client,
			mehci->bus_vote);
	if (ret)
		dev_err(mehci->dev, "%s: Failed to vote for bus bandwidth %d\n",
				__func__, ret);
}

static int msm_hsic_reset_done(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 __iomem *status_reg = &ehci->regs->port_status[0];
	int ret;

	ehci_writel(ehci, ehci_readl(ehci, status_reg) & ~(PORT_RWC_BITS |
					PORT_RESET), status_reg);

	ret = handshake(ehci, status_reg, PORT_RESET, 0, 1 * 1000);

	if (ret)
		pr_err("reset handshake failed in %s\n", __func__);
	else
		ehci_writel(ehci, ehci_readl(ehci, &ehci->regs->command) |
				CMD_RUN, &ehci->regs->command);

	return ret;
}

#define STS_GPTIMER0_INTERRUPT	BIT(24)
static irqreturn_t msm_hsic_irq(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	u32			status;
	int			ret = 0;

	if (atomic_read(&mehci->in_lpm)) {
		disable_irq_nosync(hcd->irq);
		dev_dbg(mehci->dev, "phy async intr\n");
		mehci->async_int = true;
		pm_runtime_get(mehci->dev);
		return IRQ_HANDLED;
	}

	status = ehci_readl(ehci, &ehci->regs->status);

	if (status & STS_GPTIMER0_INTERRUPT) {
		int timeleft;

		dbg_log_event(NULL, "FPR: gpt0_isr", mehci->bus_reset);

		timeleft = GPT_CNT(ehci_readl(ehci,
						 &mehci->timer->gptimer1_ctrl));
		if (!mehci->bus_reset) {
			if (ktime_us_delta(ktime_get(), mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
				dbg_log_event(NULL, "FPR: GPT prog invalid",
						timeleft);
				pr_err("HSIC GPT timer prog invalid\n");
				timeleft = 0;
			}
		}
		if (timeleft) {
			if (mehci->bus_reset) {
				ret = msm_hsic_reset_done(hcd);
				if (ret) {
					mehci->reset_again = 1;
					dbg_log_event(NULL, "RESET: fail", 0);
				}
			} else {
				writel_relaxed(readl_relaxed(
					&ehci->regs->command) | CMD_RUN,
					&ehci->regs->command);
				if (ktime_us_delta(ktime_get(),
					mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
					dbg_log_event(NULL,
						"FPR: resume prog invalid",
						timeleft);
					pr_err("HSIC resume fail. retrying\n");
					mehci->resume_again = 1;
				}
			}
		} else {
			if (mehci->bus_reset)
				mehci->reset_again = 1;
			else
				mehci->resume_again = 1;
		}

		dbg_log_event(NULL, "FPR: timeleft", timeleft);

		complete(&mehci->gpt0_completion);
		ehci_writel(ehci, STS_GPTIMER0_INTERRUPT, &ehci->regs->status);
	}

	return ehci_irq(hcd);
}

static int ehci_hsic_reset(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	int retval;

	mehci->timer = USB_HS_GPTIMER_BASE;
	ehci->caps = USB_CAPLENGTH;
	ehci->regs = USB_CAPLENGTH +
		HC_LENGTH(ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));
	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	hcd->has_tt = 1;
	ehci->sbrn = HCD_USB2;

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	retval = ehci_reset(ehci);
	if (retval)
		return retval;

	
	writel_relaxed(0, USB_AHBBURST);
	
	writel_relaxed(0x08, USB_AHBMODE);
	
	writel_relaxed(0x13, USB_USBMODE);

	ehci_port_power(ehci, 1);
	return 0;
}

static int ehci_hsic_urb_enqueue(struct usb_hcd *hcd, struct urb *urb,
		gfp_t mem_flags)
{
	dbg_log_event(urb, event_to_str(URB_SUBMIT), 0);
	return ehci_urb_enqueue(hcd, urb, mem_flags);
}

#define RESET_RETRY_LIMIT 3
#define RESET_SIGNAL_TIME_SOF_USEC (50 * 1000)
#define RESET_SIGNAL_TIME_USEC (20 * 1000)
static void ehci_hsic_reset_sof_bug_handler(struct usb_hcd *hcd, u32 val)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;
	u32 __iomem *status_reg = &ehci->regs->port_status[0];
	u32 cmd;
	unsigned long flags;
	int retries = 0, ret, cnt = RESET_SIGNAL_TIME_USEC;

	if (pdata && pdata->swfi_latency)
		pm_qos_update_request(&mehci->pm_qos_req_dma,
			pdata->swfi_latency + 1);

	mehci->bus_reset = 1;

	
	cmd = ehci_readl(ehci, &ehci->regs->command);
	cmd &= ~CMD_RUN;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	ret = handshake(ehci, &ehci->regs->status, STS_HALT,
			STS_HALT, 16 * 125);
	if (ret) {
		pr_err("halt handshake fatal error\n");
		dbg_log_event(NULL, "HALT: fatal", 0);
		goto fail;
	}

retry:
	retries++;
	dbg_log_event(NULL, "RESET: start", retries);
	pr_debug("reset begin %d\n", retries);
	mehci->reset_again = 0;
	spin_lock_irqsave(&ehci->lock, flags);
	ehci_writel(ehci, val, status_reg);
	ehci_writel(ehci, GPT_LD(RESET_SIGNAL_TIME_USEC - 1),
			&mehci->timer->gptimer0_ld);
	ehci_writel(ehci, GPT_RESET | GPT_RUN,
			&mehci->timer->gptimer0_ctrl);
	ehci_writel(ehci, INTR_MASK | STS_GPTIMER0_INTERRUPT,
			&ehci->regs->intr_enable);

	ehci_writel(ehci, GPT_LD(RESET_SIGNAL_TIME_SOF_USEC - 1),
			&mehci->timer->gptimer1_ld);
	ehci_writel(ehci, GPT_RESET | GPT_RUN,
			&mehci->timer->gptimer1_ctrl);

	spin_unlock_irqrestore(&ehci->lock, flags);
	wait_for_completion(&mehci->gpt0_completion);

	if (!mehci->reset_again)
		goto done;

	if (handshake(ehci, status_reg, PORT_RESET, 0, 10 * 1000)) {
		pr_err("reset handshake fatal error\n");
		dbg_log_event(NULL, "RESET: fatal", retries);
		goto fail;
	}

	if (retries < RESET_RETRY_LIMIT)
		goto retry;

	
	pr_info("RESET in tight loop\n");
	dbg_log_event(NULL, "RESET: tight", 0);

	spin_lock_irqsave(&ehci->lock, flags);
	ehci_writel(ehci, val, status_reg);
	while (cnt--)
		udelay(1);

	ret = msm_hsic_reset_done(hcd);
	spin_unlock_irqrestore(&ehci->lock, flags);
	if (ret) {
		pr_err("RESET in tight loop failed\n");
		dbg_log_event(NULL, "RESET: tight failed", 0);
		goto fail;
	}

done:
	dbg_log_event(NULL, "RESET: done", retries);
	pr_debug("reset completed\n");
fail:
	mehci->bus_reset = 0;
	if (pdata && pdata->swfi_latency)
		pm_qos_update_request(&mehci->pm_qos_req_dma,
			PM_QOS_DEFAULT_VALUE);
}

static int ehci_hsic_bus_suspend(struct usb_hcd *hcd)
{
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (!(readl_relaxed(USB_PORTSC) & PORT_PE)) {
		dbg_log_event(NULL, "RH suspend attempt failed", 0);
		dev_err(mehci->dev, "%s:port is not enabled skip suspend\n",
				__func__);
		return -EAGAIN;
	}

	dbg_log_event(NULL, "Suspend RH", 0);
	pr_info("%s: Suspend RH\n", __func__);
	return ehci_bus_suspend(hcd);
}

static int msm_hsic_resume_thread(void *data)
{
	struct msm_hsic_hcd *mehci = data;
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	struct ehci_hcd 	*ehci = hcd_to_ehci(hcd);
	u32 		temp;
	unsigned long		resume_needed = 0;
	int			retry_cnt = 0;
	int			tight_resume = 0;
	int			tight_count = 0;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;
	s32 next_latency = 0;
	ktime_t now;
	s64 mdiff;

while (!kthread_should_stop()) {
	resume_needed = 0;
	retry_cnt = 0;
	tight_resume = 0;
	tight_count = 0;
	next_latency = 0;

	dbg_log_event(NULL, "Resume RH", 0);

	
	pr_info("%s : Resume RH\n", __func__);
	if (pdata && pdata->swfi_latency) {
		next_latency = pdata->swfi_latency + 1;
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);
		next_latency = PM_QOS_DEFAULT_VALUE;
	}

	
	now = ktime_get();
	mdiff = ktime_to_us(ktime_sub(now,ehci->last_susp_resume));
	if (mdiff < 10000) {
		usleep_range(10000, 10000);

		
		pr_info("%s[%d] usleep_range 10000 end", __func__, __LINE__);
	}

	spin_lock_irq(&ehci->lock);
	if (!HCD_HW_ACCESSIBLE(hcd)) {
		mehci->resume_status = -ESHUTDOWN;
		goto exit;
	}

	if (unlikely(ehci->debug)) {
		if (!dbgp_reset_prep())
			ehci->debug = NULL;
		else
			dbgp_external_startup();
	}

	writel_relaxed(0, &ehci->regs->intr_enable);

	
	writel_relaxed(0, &ehci->regs->segment);
	writel_relaxed(ehci->periodic_dma, &ehci->regs->frame_list);
	writel_relaxed((u32) ehci->async->qh_dma, &ehci->regs->async_next);

	
	if (ehci->resume_sof_bug)
		ehci->command &= ~CMD_RUN;

	
	writel_relaxed(ehci->command, &ehci->regs->command);

	
resume_again:
	if (retry_cnt >= RESUME_RETRY_LIMIT) {
		pr_info("retry count(%d) reached max, resume in tight loop\n",
					retry_cnt);
		tight_resume = 1;
	}

	temp = readl_relaxed(&ehci->regs->port_status[0]);
	temp &= ~(PORT_RWC_BITS | PORT_WAKE_BITS);
	if (test_bit(0, &ehci->bus_suspended) && (temp & PORT_SUSPEND)) {
		temp |= PORT_RESUME;
		set_bit(0, &resume_needed);
	}
	mehci->resume_start_t = ktime_get();
	dbg_log_event(NULL, "FPR: Set", temp);
	writel_relaxed(temp, &ehci->regs->port_status[0]);

	if (ehci->resume_sof_bug && resume_needed) {
		if (!tight_resume) {
			mehci->resume_again = 0;
			writel_relaxed(GPT_LD(RESUME_SIGNAL_TIME_USEC - 1),
					&mehci->timer->gptimer0_ld);
			writel_relaxed(GPT_RESET | GPT_RUN,
					&mehci->timer->gptimer0_ctrl);
			writel_relaxed(INTR_MASK | STS_GPTIMER0_INTERRUPT,
					&ehci->regs->intr_enable);

			writel_relaxed(GPT_LD(RESUME_SIGNAL_TIME_SOF_USEC - 1),
					&mehci->timer->gptimer1_ld);
			writel_relaxed(GPT_RESET | GPT_RUN,
				&mehci->timer->gptimer1_ctrl);
			dbg_log_event(NULL, "GPT timer prog done", 0);

			spin_unlock_irq(&ehci->lock);
			wait_for_completion(&mehci->gpt0_completion);
			spin_lock_irq(&ehci->lock);
		} else {
			dbg_log_event(NULL, "FPR: Tightloop", tight_count);
			
			mdelay(22);
			writel_relaxed(readl_relaxed(&ehci->regs->command) |
					CMD_RUN, &ehci->regs->command);
			if (ktime_us_delta(ktime_get(), mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
				dbg_log_event(NULL, "FPR: Tightloop fail", 0);
				if (++tight_count > 3) {
					pr_err("HSIC resume failed\n");
					mehci->resume_status = -ENODEV;
					goto exit;
				}
				pr_err("FPR tight loop fail %d\n", tight_count);
				mehci->resume_again = 1;
			} else {
				dbg_log_event(NULL, "FPR: Tightloop done", 0);
			}
		}

		if (mehci->resume_again) {
			int temp;

			dbg_log_event(NULL, "FPR: Re-Resume", retry_cnt);
			pr_info("FPR: retry count: %d\n", retry_cnt);
			spin_unlock_irq(&ehci->lock);
			temp = readl_relaxed(&ehci->regs->command);
			if (temp & CMD_RUN) {
				temp &= ~CMD_RUN;
				writel_relaxed(temp, &ehci->regs->command);
				dbg_log_event(NULL, "FPR: R/S cleared", 0);
			}
			temp = readl_relaxed(&ehci->regs->port_status[0]);
			temp &= ~PORT_RWC_BITS;
			temp |= PORT_SUSPEND;
			writel_relaxed(temp, &ehci->regs->port_status[0]);
			usleep_range(5000, 5000);
			dbg_log_event(NULL,
				"FPR: RResume",
				ehci_readl(ehci, &ehci->regs->port_status[0]));
			spin_lock_irq(&ehci->lock);
			mehci->resume_again = 0;
			retry_cnt++;
			goto resume_again;
		}
	}

	ehci->command |= CMD_RUN;
	dbg_log_event(NULL, "FPR: RT-Done", 0);
	mehci->resume_status = 1;
exit:
	spin_unlock_irq(&ehci->lock);
	complete(&mehci->rt_completion);
	if (next_latency)
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);


	__set_current_state(TASK_UNINTERRUPTIBLE);
	schedule();
	__set_current_state(TASK_RUNNING);
	}
	pr_info("%s Exit!\n", __func__);
	return 0;
}

static int ehci_hsic_bus_resume(struct usb_hcd *hcd)
{
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct ehci_hcd 	*ehci = hcd_to_ehci(hcd);
	u32 		temp;
	int ret = 0;
	

	mehci->resume_status = 0;
	
	
	ret = wake_up_process(mehci->resume_thread);

	#if 0
	if (IS_ERR(resume_thread)) {
		pr_err("Error creating resume thread:%lu\n",
				PTR_ERR(resume_thread));
		return PTR_ERR(resume_thread);
	}
	#endif
	wait_for_completion(&mehci->rt_completion);

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		pr_info("%s[%d] resume_status:%d", __func__, __LINE__, mehci->resume_status);

	if (mehci->resume_status < 0)
		return mehci->resume_status;

	dbg_log_event(NULL, "FPR: Wokeup", 0);
	spin_lock_irq(&ehci->lock);
	(void) ehci_readl(ehci, &ehci->regs->command);

	temp = 0;
	if (ehci->async->qh_next.qh)
		temp |= CMD_ASE;
	if (ehci->periodic_sched)
		temp |= CMD_PSE;
	if (temp) {
		pr_info("%s: ASE enabled during resume!!!\n", __func__);
		dbg_log_event(NULL, "ASE set during resume", 0);
		ehci->command |= temp;
		ehci_writel(ehci, ehci->command, &ehci->regs->command);
	}

	ehci->next_statechange = jiffies + msecs_to_jiffies(5);
	hcd->state = HC_STATE_RUNNING;
	ehci->rh_state = EHCI_RH_RUNNING;

	
	ehci_writel(ehci, INTR_MASK, &ehci->regs->intr_enable);

	spin_unlock_irq(&ehci->lock);

	return 0;
}

static struct hc_driver msm_hsic_driver = {
	.description		= hcd_name,
	.product_desc		= "Qualcomm EHCI Host Controller using HSIC",
	.hcd_priv_size		= sizeof(struct msm_hsic_hcd),

	.irq			= msm_hsic_irq,
	.flags			= HCD_USB2 | HCD_MEMORY | HCD_OLD_ENUM,

	.reset			= ehci_hsic_reset,
	.start			= ehci_run,

	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	.urb_enqueue		= ehci_hsic_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,
	.clear_tt_buffer_complete	 = ehci_clear_tt_buffer_complete,

	.get_frame_number	= ehci_get_frame,

	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.bus_suspend		= ehci_hsic_bus_suspend,
	.bus_resume		= ehci_hsic_bus_resume,

	.set_int_latency	= ehci_hsic_int_latency,

	.log_urb_complete	= dbg_log_event,
	.dump_regs		= dump_hsic_regs,
	.dump_qh_qtd		=  dump_qh_qtd,

	.reset_sof_bug_handler  = ehci_hsic_reset_sof_bug_handler,
};

static int msm_hsic_init_clocks(struct msm_hsic_hcd *mehci, u32 init)
{
	int ret = 0;

	if (!init)
		goto put_clocks;

	mehci->core_clk = clk_get(mehci->dev, "core_clk");
	if (IS_ERR(mehci->core_clk)) {
		dev_err(mehci->dev, "failed to get core_clk\n");
		ret = PTR_ERR(mehci->core_clk);
		return ret;
	}

	mehci->alt_core_clk = clk_get(mehci->dev, "alt_core_clk");
	if (IS_ERR(mehci->alt_core_clk)) {
		dev_err(mehci->dev, "failed to core_clk\n");
		ret = PTR_ERR(mehci->alt_core_clk);
		goto put_core_clk;
	}

	mehci->phy_clk = clk_get(mehci->dev, "phy_clk");
	if (IS_ERR(mehci->phy_clk)) {
		dev_err(mehci->dev, "failed to get phy_clk\n");
		ret = PTR_ERR(mehci->phy_clk);
		goto put_alt_core_clk;
	}

	
	mehci->cal_clk = clk_get(mehci->dev, "cal_clk");
	if (IS_ERR(mehci->cal_clk)) {
		dev_err(mehci->dev, "failed to get cal_clk\n");
		ret = PTR_ERR(mehci->cal_clk);
		goto put_phy_clk;
	}
	clk_set_rate(mehci->cal_clk, 10000000);

	
	mehci->ahb_clk = clk_get(mehci->dev, "iface_clk");
	if (IS_ERR(mehci->ahb_clk)) {
		dev_err(mehci->dev, "failed to get iface_clk\n");
		ret = PTR_ERR(mehci->ahb_clk);
		goto put_cal_clk;
	}

	clk_prepare_enable(mehci->core_clk);
	clk_prepare_enable(mehci->phy_clk);
	clk_prepare_enable(mehci->cal_clk);
	clk_prepare_enable(mehci->ahb_clk);

	return 0;

put_clocks:
	if (!atomic_read(&mehci->in_lpm)) {
		clk_disable_unprepare(mehci->core_clk);
		clk_disable_unprepare(mehci->phy_clk);
		clk_disable_unprepare(mehci->cal_clk);
		clk_disable_unprepare(mehci->ahb_clk);
	}
	clk_put(mehci->ahb_clk);
put_cal_clk:
	clk_put(mehci->cal_clk);
put_phy_clk:
	clk_put(mehci->phy_clk);
put_alt_core_clk:
	clk_put(mehci->alt_core_clk);
put_core_clk:
	clk_put(mehci->core_clk);

	return ret;
}
static irqreturn_t hsic_peripheral_status_change(int irq, void *dev_id)
{
	struct msm_hsic_hcd *mehci = dev_id;

	pr_debug("%s: mehci:%p dev_id:%p\n", __func__, mehci, dev_id);

	if (mehci)
		msm_hsic_config_gpios(mehci, 0);

	return IRQ_HANDLED;
}

static irqreturn_t msm_hsic_wakeup_irq(int irq, void *data)
{
	struct msm_hsic_hcd *mehci = data;
	int ret = 0;

	
	msm_hsic_wakeup_irq_timestamp = sched_clock();
	

	mehci->wakeup_int_cnt++;
	dbg_log_event(NULL, "Remote Wakeup IRQ", mehci->wakeup_int_cnt);
	
	LOG_WITH_TIMESTAMP("%s: hsic remote wakeup interrupt cnt: %u ",
			__func__, mehci->wakeup_int_cnt);
#if 0
	
	if ( set_htc_monitor_resume_state_fp ) {
		set_htc_monitor_resume_state_fp();
	}
	
#endif
	

	wake_lock(&mehci->wlock);

	spin_lock(&mehci->wakeup_lock);
	if (mehci->wakeup_irq_enabled) {
		mehci->wakeup_irq_enabled = 0;
		disable_irq_wake(irq);
		disable_irq_nosync(irq);
	}
	spin_unlock(&mehci->wakeup_lock);

	if (!atomic_read(&mehci->pm_usage_cnt)) {
		
		ret = pm_runtime_get(mehci->dev);

		
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			dev_info(mehci->dev, "%s: hsic pm_runtime_get return: %d\n",
					__func__, ret);
		


		
		spin_lock(&mehci->wakeup_lock);
		

		if ((ret == 1) || (ret == -EINPROGRESS))
			pm_runtime_put_noidle(mehci->dev);
		else
			atomic_set(&mehci->pm_usage_cnt, 1);

		
		if (!atomic_read(&mehci->in_lpm)) {
			pr_info("%s(%d): mehci->in_lpm==0 !!!\n", __func__, __LINE__);
			if (atomic_read(&mehci->pm_usage_cnt)) {
				atomic_set(&mehci->pm_usage_cnt, 0);
				pr_info("%s(%d): pm_runtime_put_noidle !!!\n", __func__, __LINE__);
				pm_runtime_put_noidle(mehci->dev);
			}
		}
		

		
		spin_unlock(&mehci->wakeup_lock);
		
	}

	return IRQ_HANDLED;
}

static int ehci_hsic_msm_bus_show(struct seq_file *s, void *unused)
{
	if (debug_bus_voting_enabled)
		seq_printf(s, "enabled\n");
	else
		seq_printf(s, "disabled\n");

	return 0;
}

static int ehci_hsic_msm_bus_open(struct inode *inode, struct file *file)
{
	return single_open(file, ehci_hsic_msm_bus_show, inode->i_private);
}

static ssize_t ehci_hsic_msm_bus_write(struct file *file,
	const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[8];
	int ret;
	struct seq_file *s = file->private_data;
	struct msm_hsic_hcd *mehci = s->private;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "enable", 6)) {
		
		debug_bus_voting_enabled = true;
	} else {
		debug_bus_voting_enabled = false;
		if (mehci->bus_perf_client) {
			ret = msm_bus_scale_client_update_request(
					mehci->bus_perf_client, 0);
			if (ret)
				dev_err(mehci->dev, "%s: Failed to devote "
					   "for bus bw %d\n", __func__, ret);
		}
	}

	return count;
}

const struct file_operations ehci_hsic_msm_bus_fops = {
	.open = ehci_hsic_msm_bus_open,
	.read = seq_read,
	.write = ehci_hsic_msm_bus_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ehci_hsic_msm_wakeup_cnt_show(struct seq_file *s, void *unused)
{
	struct msm_hsic_hcd *mehci = s->private;

	seq_printf(s, "%u\n", mehci->wakeup_int_cnt);

	return 0;
}

static int ehci_hsic_msm_wakeup_cnt_open(struct inode *inode, struct file *f)
{
	return single_open(f, ehci_hsic_msm_wakeup_cnt_show, inode->i_private);
}

const struct file_operations ehci_hsic_msm_wakeup_cnt_fops = {
	.open = ehci_hsic_msm_wakeup_cnt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ehci_hsic_msm_data_events_show(struct seq_file *s, void *unused)
{
	unsigned long	flags;
	unsigned	i;

	read_lock_irqsave(&dbg_hsic_data.lck, flags);

	i = dbg_hsic_data.idx;
	for (dbg_inc(&i); i != dbg_hsic_data.idx; dbg_inc(&i)) {
		if (!strnlen(dbg_hsic_data.buf[i], DBG_MSG_LEN))
			continue;
		seq_printf(s, "%s\n", dbg_hsic_data.buf[i]);
	}

	read_unlock_irqrestore(&dbg_hsic_data.lck, flags);

	return 0;
}

static int ehci_hsic_msm_data_events_open(struct inode *inode, struct file *f)
{
	return single_open(f, ehci_hsic_msm_data_events_show, inode->i_private);
}

const struct file_operations ehci_hsic_msm_dbg_data_fops = {
	.open = ehci_hsic_msm_data_events_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ehci_hsic_msm_ctrl_events_show(struct seq_file *s, void *unused)
{
	unsigned long	flags;
	unsigned	i;

	read_lock_irqsave(&dbg_hsic_ctrl.lck, flags);

	i = dbg_hsic_ctrl.idx;
	for (dbg_inc(&i); i != dbg_hsic_ctrl.idx; dbg_inc(&i)) {
		if (!strnlen(dbg_hsic_ctrl.buf[i], DBG_MSG_LEN))
			continue;
		seq_printf(s, "%s\n", dbg_hsic_ctrl.buf[i]);
	}

	read_unlock_irqrestore(&dbg_hsic_ctrl.lck, flags);

	return 0;
}

static int ehci_hsic_msm_ctrl_events_open(struct inode *inode, struct file *f)
{
	return single_open(f, ehci_hsic_msm_ctrl_events_show, inode->i_private);
}

const struct file_operations ehci_hsic_msm_dbg_ctrl_fops = {
	.open = ehci_hsic_msm_ctrl_events_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct dentry *ehci_hsic_msm_dbg_root;
static int ehci_hsic_msm_debugfs_init(struct msm_hsic_hcd *mehci)
{
	struct dentry *ehci_hsic_msm_dentry;

	ehci_hsic_msm_dbg_root = debugfs_create_dir("ehci_hsic_msm_dbg", NULL);

	if (!ehci_hsic_msm_dbg_root || IS_ERR(ehci_hsic_msm_dbg_root))
		return -ENODEV;

	ehci_hsic_msm_dentry = debugfs_create_file("bus_voting",
		S_IRUGO | S_IWUSR,
		ehci_hsic_msm_dbg_root, mehci,
		&ehci_hsic_msm_bus_fops);

	if (!ehci_hsic_msm_dentry) {
		debugfs_remove_recursive(ehci_hsic_msm_dbg_root);
		return -ENODEV;
	}

	ehci_hsic_msm_dentry = debugfs_create_file("wakeup_cnt",
		S_IRUGO,
		ehci_hsic_msm_dbg_root, mehci,
		&ehci_hsic_msm_wakeup_cnt_fops);

	if (!ehci_hsic_msm_dentry) {
		debugfs_remove_recursive(ehci_hsic_msm_dbg_root);
		return -ENODEV;
	}

	ehci_hsic_msm_dentry = debugfs_create_file("show_ctrl_events",
		S_IRUGO,
		ehci_hsic_msm_dbg_root, mehci,
		&ehci_hsic_msm_dbg_ctrl_fops);

	if (!ehci_hsic_msm_dentry) {
		debugfs_remove_recursive(ehci_hsic_msm_dbg_root);
		return -ENODEV;
	}

	ehci_hsic_msm_dentry = debugfs_create_file("show_data_events",
		S_IRUGO,
		ehci_hsic_msm_dbg_root, mehci,
		&ehci_hsic_msm_dbg_data_fops);

	if (!ehci_hsic_msm_dentry) {
		debugfs_remove_recursive(ehci_hsic_msm_dbg_root);
		return -ENODEV;
	}

	return 0;
}

static void ehci_hsic_msm_debugfs_cleanup(void)
{
	debugfs_remove_recursive(ehci_hsic_msm_dbg_root);
}

static int __devinit ehci_hsic_msm_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct resource *res;
	struct msm_hsic_hcd *mehci;
	struct msm_hsic_host_platform_data *pdata;
	int ret;

	dev_dbg(&pdev->dev, "ehci_msm-hsic probe\n");

	
	if (get_radio_flag() & 0x0001)
		usb_pm_debug_enabled = true;
	

	if (pdev->dev.parent)
		pm_runtime_get_sync(pdev->dev.parent);

	hcd = usb_create_hcd(&msm_hsic_driver, &pdev->dev,
				dev_name(&pdev->dev));
	if (!hcd) {
		dev_err(&pdev->dev, "Unable to create HCD\n");
		return  -ENOMEM;
	}

	hcd_to_bus(hcd)->skip_resume = true;

	hcd->irq = platform_get_irq(pdev, 0);
	if (hcd->irq < 0) {
		dev_err(&pdev->dev, "Unable to get IRQ resource\n");
		ret = hcd->irq;
		goto put_hcd;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get memory resource\n");
		ret = -ENODEV;
		goto put_hcd;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto put_hcd;
	}

	mehci = hcd_to_hsic(hcd);
	mehci->dev = &pdev->dev;

	spin_lock_init(&mehci->wakeup_lock);

	mehci->ehci.susp_sof_bug = 1;
	mehci->ehci.reset_sof_bug = 1;

	mehci->ehci.resume_sof_bug = 1;

	mehci->ehci.max_log2_irq_thresh = 6;

#if defined(CONFIG_MACH_M7_WLV)
	mehci->ehci.max_log2_irq_thresh = 5;
#endif

	INIT_WORK(&hcd->ssr_work,do_restart);

	res = platform_get_resource_byname(pdev,
			IORESOURCE_IRQ,
			"peripheral_status_irq");
	if (res)
		mehci->peripheral_status_irq = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO, "wakeup");
	if (res) {
		mehci->wakeup_gpio = res->start;
		mehci->wakeup_irq = MSM_GPIO_TO_INT(res->start);
		dev_dbg(mehci->dev, "wakeup_irq: %d\n", mehci->wakeup_irq);
	}


	ret = msm_hsic_init_clocks(mehci, 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize clocks\n");
		ret = -ENODEV;
		goto unmap;
	}

	ret = msm_hsic_init_vddcx(mehci, 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize VDDCX\n");
		ret = -ENODEV;
		goto deinit_clocks;
	}

	init_completion(&mehci->rt_completion);
	init_completion(&mehci->gpt0_completion);
	ret = msm_hsic_reset(mehci);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize PHY\n");
		goto deinit_vddcx;
	}

	ehci_wq = create_singlethread_workqueue("ehci_wq");
	if (!ehci_wq) {
		dev_err(&pdev->dev, "unable to create workqueue\n");
		ret = -ENOMEM;
		goto deinit_vddcx;
	}

	INIT_WORK(&mehci->bus_vote_w, ehci_hsic_bus_vote_w);

	INIT_DELAYED_WORK(&ehci_gpio_check_wq, mdm_hsic_gpio_check_func);

	ret = usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
	if (ret) {
		dev_err(&pdev->dev, "unable to register HCD\n");
		goto unconfig_gpio;
	}

	device_init_wakeup(&pdev->dev, 1);
	wake_lock_init(&mehci->wlock, WAKE_LOCK_SUSPEND, dev_name(&pdev->dev));
	wake_lock(&mehci->wlock);

	
	pm_qos_add_request(&mehci->pm_qos_req_dma_htc, PM_QOS_CPU_DMA_LATENCY,
				PM_QOS_DEFAULT_VALUE);
	pr_info("%s[%d] pm_qos_add_request pm_qos_req_dma_htc\n", __func__, __LINE__);
	

	if (mehci->peripheral_status_irq) {
		ret = request_threaded_irq(mehci->peripheral_status_irq,
			NULL, hsic_peripheral_status_change,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
						| IRQF_SHARED,
			"hsic_peripheral_status", mehci);
		if (ret)
			dev_err(&pdev->dev, "%s:request_irq:%d failed:%d",
				__func__, mehci->peripheral_status_irq, ret);
	}

	
	if (mehci->wakeup_irq) {
		irq_set_status_flags(mehci->wakeup_irq, IRQ_NOAUTOEN);

		ret = request_irq(mehci->wakeup_irq, msm_hsic_wakeup_irq,
				IRQF_TRIGGER_HIGH,
				"msm_hsic_wakeup", mehci);

		if (ret) {
			dev_err(&pdev->dev, "request_irq(%d) failed: %d\n",
					mehci->wakeup_irq, ret);
			mehci->wakeup_irq = 0;
		}
	}

	ret = ehci_hsic_msm_debugfs_init(mehci);
	if (ret)
		dev_dbg(&pdev->dev, "mode debugfs file is"
			"not available\n");

	pdata = mehci->dev->platform_data;
	if (pdata && pdata->bus_scale_table) {
		mehci->bus_perf_client =
		    msm_bus_scale_register_client(pdata->bus_scale_table);
		
		if (mehci->bus_perf_client) {
				mehci->bus_vote = true;
				queue_work(ehci_wq, &mehci->bus_vote_w);
		} else {
			dev_err(&pdev->dev, "%s: Failed to register BUS "
						"scaling client!!\n", __func__);
		}
	}
	mehci->resume_thread = kthread_create_on_node(msm_hsic_resume_thread, mehci, -1, "hsic_resume_thread");
	if (IS_ERR(mehci->resume_thread)) {
		pr_err("Error creating resume thread:%lu\n",
				PTR_ERR(mehci->resume_thread));
	}
	__mehci = mehci;

	if (pdata && pdata->swfi_latency)
		pm_qos_add_request(&mehci->pm_qos_req_dma,
			PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	if (pdev->dev.parent)
		pm_runtime_put_sync(pdev->dev.parent);

	
	INIT_DELAYED_WORK(&mdm_hsic_pm_monitor_delayed_work, mdm_hsic_pm_monitor_func);
	INIT_DELAYED_WORK(&register_usb_notification_work, register_usb_notification_func);
	schedule_delayed_work(&register_usb_notification_work, msecs_to_jiffies(10));
	mdm_hsic_usb_hcd = hcd;
	pr_info("%s(%d): mdm_hsic_usb_hcd:0x%x\n", __func__, __LINE__, (unsigned int)mdm_hsic_usb_hcd);
	

	in_progress = 0;

	return 0;

unconfig_gpio:
	destroy_workqueue(ehci_wq);
	msm_hsic_config_gpios(mehci, 0);
deinit_vddcx:
	msm_hsic_init_vddcx(mehci, 0);
deinit_clocks:
	msm_hsic_init_clocks(mehci, 0);
unmap:
	iounmap(hcd->regs);
put_hcd:
	usb_put_hcd(hcd);

	return ret;
}

#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
extern int mdm_is_in_restart;
static void dbg_hsic_usage_count_delay_work_fn(struct work_struct *work)
{
    panic("msm_hsic_host usage count is not 0 !!!");
}
static DECLARE_DELAYED_WORK(dbg_hsic_usage_count_delay_work, dbg_hsic_usage_count_delay_work_fn);
#endif 

static int __devexit ehci_hsic_msm_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;

	if (pdata && pdata->swfi_latency)
		pm_qos_remove_request(&mehci->pm_qos_req_dma);

	if (mehci->peripheral_status_irq)
		free_irq(mehci->peripheral_status_irq, mehci);

	if (mehci->wakeup_irq) {
		if (mehci->wakeup_irq_enabled)
			disable_irq_wake(mehci->wakeup_irq);
		free_irq(mehci->wakeup_irq, mehci);
	}

	kthread_stop(mehci->resume_thread);
	mehci->bus_vote = false;
	cancel_work_sync(&mehci->bus_vote_w);
	cancel_delayed_work_sync(&ehci_gpio_check_wq);
	
	cancel_delayed_work_sync(&mdm_hsic_pm_monitor_delayed_work);
	cancel_delayed_work_sync(&register_usb_notification_work);
	usb_unregister_notify(&mdm_hsic_usb_nb);
	mdm_hsic_usb_hcd = NULL;
	pr_info("%s(%d): mdm_hsic_usb_hcd:0x%x\n", __func__, __LINE__, (unsigned int)mdm_hsic_usb_hcd);
	

	if (mehci->bus_perf_client)
		msm_bus_scale_unregister_client(mehci->bus_perf_client);

	ehci_hsic_msm_debugfs_cleanup();
	device_init_wakeup(&pdev->dev, 0);
	pm_runtime_set_suspended(&pdev->dev);

	destroy_workqueue(ehci_wq);

	usb_remove_hcd(hcd);
	msm_hsic_config_gpios(mehci, 0);
	msm_hsic_init_vddcx(mehci, 0);

	msm_hsic_init_clocks(mehci, 0);
	wake_lock_destroy(&mehci->wlock);

	
	pm_qos_remove_request(&mehci->pm_qos_req_dma_htc);
	pr_info("%s[%d] pm_qos_remove_request pm_qos_req_dma_htc\n", __func__, __LINE__);
	

	iounmap(hcd->regs);
	usb_put_hcd(hcd);

	
	#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (pdev) {
		int usage_count = atomic_read(&(pdev->dev.power.usage_count));
		dev_info(&(pdev->dev), "%s[%d] usage_count is [%d] msm_hsic_host_dev:0x%p &pdev->dev:0x%p mdm_is_in_restart:%d\n",
			__func__, __LINE__, usage_count, msm_hsic_host_dev, &(pdev->dev), mdm_is_in_restart);

		if (mdm_is_in_restart && usage_count != 0) {
			pr_info("%s[%d] !!! usage_count:%d is not 0 !!!\n", __func__, __LINE__, usage_count);
			atomic_set(&(pdev->dev.power.usage_count), 0);
		}
	}
	#endif
	

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int msm_hsic_pm_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	dev_info(dev, "ehci-msm-hsic PM suspend\n");

	dbg_log_event(NULL, "PM Suspend", 0);

	if (!atomic_read(&mehci->in_lpm)) {
		dev_info(dev, "abort suspend\n");
		dbg_log_event(NULL, "PM Suspend abort", 0);
		return -EBUSY;
	}

	if (device_may_wakeup(dev))
		enable_irq_wake(hcd->irq);

	return 0;
}

static int msm_hsic_pm_resume(struct device *dev)
{
	int ret;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	dev_dbg(dev, "ehci-msm-hsic PM resume\n");
	dbg_log_event(NULL, "PM Resume", 0);

	if (device_may_wakeup(dev))
		disable_irq_wake(hcd->irq);

	if (hcd_to_bus(hcd)->skip_resume)
	{
		if (!atomic_read(&mehci->pm_usage_cnt) &&
				pm_runtime_suspended(dev))
		{
			
			if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD) {
				dev_info(dev, "skip ehci-msm-hsic PM resume\n");
			}
			
			return 0;
		}
	}

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		dev_info(dev, "ehci-msm-hsic PM resume\n");
	

	ret = msm_hsic_resume(mehci);
	if (ret)
		return ret;

	
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int msm_hsic_runtime_idle(struct device *dev)
{
	dev_info(dev, "EHCI runtime idle\n");
	return 0;
}

static int msm_hsic_runtime_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	dev_info(dev, "EHCI runtime suspend\n");

	dbg_log_event(NULL, "Run Time PM Suspend", 0);

	return msm_hsic_suspend(mehci);
}

static int msm_hsic_runtime_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	dev_info(dev, "EHCI runtime resume\n");

	dbg_log_event(NULL, "Run Time PM Resume", 0);

	return msm_hsic_resume(mehci);
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops msm_hsic_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(msm_hsic_pm_suspend, msm_hsic_pm_resume)
	SET_RUNTIME_PM_OPS(msm_hsic_runtime_suspend, msm_hsic_runtime_resume,
				msm_hsic_runtime_idle)
};
#endif

static struct platform_driver ehci_msm_hsic_driver = {
	.probe	= ehci_hsic_msm_probe,
	.remove	= __devexit_p(ehci_hsic_msm_remove),
	.driver = {
		.name = "msm_hsic_host",
#ifdef CONFIG_PM
		.pm = &msm_hsic_dev_pm_ops,
#endif
	},
};
