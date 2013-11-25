/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/mfd/pmic8058.h>
#include <linux/jiffies.h>
#include <linux/suspend.h>
#include <linux/percpu.h>
#include <linux/interrupt.h>
#include <asm/fiq.h>
#include <asm/hardware/gic.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <mach/msm_iomap.h>
#include <asm/mach-types.h>
#include <mach/scm.h>
#include <mach/socinfo.h>
#include <mach/board_htc.h>
#include <mach/msm_watchdog.h>
#include "timer.h"
#include "htc_watchdog_monitor.h"

#define MODULE_NAME "msm_watchdog"

#define TCSR_WDT_CFG	0x30

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_STS	0x44
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

#define WDT_HZ		32768

#define HTC_WATCHDOG_TOP_SCHED_DUMP 0

struct msm_watchdog_dump msm_dump_cpu_ctx;

static void __iomem *msm_tmr0_base;

static unsigned long delay_time;
static unsigned long bark_time;
static unsigned long long last_pet;
static bool has_vic;
static atomic_t watchdog_bark_counter = ATOMIC_INIT(0);

static int enable = 1;
module_param(enable, int, 0);

static int runtime_disable;
static DEFINE_MUTEX(disable_lock);
static int wdog_enable_set(const char *val, struct kernel_param *kp);
module_param_call(runtime_disable, wdog_enable_set, param_get_int,
			&runtime_disable, 0644);

static int appsbark;
module_param(appsbark, int, 0);

static int appsbark_fiq;

static int print_all_stacks = 1;
module_param(print_all_stacks, int,  S_IRUGO | S_IWUSR);

static struct msm_watchdog_pdata __percpu **percpu_pdata;

static void pet_watchdog_work(struct work_struct *work);
static void init_watchdog_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(dogwork_struct, pet_watchdog_work);
static DECLARE_WORK(init_dogwork_struct, init_watchdog_work);

void msm_wdog_bark_fin(void)
{
	pr_crit("\nApps Watchdog bark received - Calling Panic\n");
	panic("Apps Watchdog Bark received\n");
}

static unsigned int last_irqs[NR_IRQS];
void wtd_dump_irqs(unsigned int dump)
{
	int n;
	if (dump) {
		pr_err("\nWatchdog dump irqs:\n");
		pr_err("irqnr       total  since-last   status  name\n");
	}
	for (n = 1; n < NR_IRQS; n++) {
		struct irqaction *act = irq_desc[n].action;
		if (!act && !kstat_irqs(n))
			continue;
		if (dump) {
			pr_err("%5d: %10u %11u  %s\n", n,
				kstat_irqs(n),
				kstat_irqs(n) - last_irqs[n],
				(act && act->name) ? act->name : "???");
		}
		last_irqs[n] = kstat_irqs(n);
	}
}
EXPORT_SYMBOL(wtd_dump_irqs);

#define APPS_WDOG_FOOT_PRINT_MAGIC	0xACBDFE00
#define APPS_WDOG_FOOT_PRINT_EN		(APPS_WDOG_FOOT_PRINT_BASE + 0x0)
#define APPS_WDOD_FOOT_PRINT_PET	(APPS_WDOG_FOOT_PRINT_BASE + 0x4)
#define MPM_SCLK_COUNT_VAL    0x0024
void set_WDT_EN_footprint(unsigned WDT_ENABLE)
{
	unsigned *status = (unsigned *)APPS_WDOG_FOOT_PRINT_EN;
	*status = (APPS_WDOG_FOOT_PRINT_MAGIC | WDT_ENABLE);
	mb();
}

int check_WDT_EN_footprint(void)
{
	unsigned *status = (unsigned *)APPS_WDOG_FOOT_PRINT_EN;
	return *status & 0xFF;
}

void set_dog_pet_footprint(void)
{
	unsigned *time = (unsigned *)APPS_WDOD_FOOT_PRINT_PET;
	uint32_t t = __raw_readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
	*time = t;

	mb();
}

#ifdef CONFIG_APQ8064_ONLY 
uint32_t mpm_get_timetick(void)
{
	volatile uint32_t i = 0;
	volatile uint32_t tick;
	volatile uint32_t tick_count;

	tick = __raw_readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
	for (i; i < 3; i++)
	{
	  tick_count = __raw_readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
	  if (tick != tick_count)
	  {
		i = 0;
		tick = __raw_readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
	  }
	}
	mb();
	return tick;
}
#endif

#define PET_CHECK_THRESHOLD	12
#define NSEC_PER_THRESHOLD	PET_CHECK_THRESHOLD * NSEC_PER_SEC
static int pet_check_counter = PET_CHECK_THRESHOLD;
static unsigned long long last_pet_check;

extern void show_pending_work_on_gcwq(void);

void msm_watchdog_check_pet(unsigned long long timestamp)
{
	if (!enable || !msm_tmr0_base || !check_WDT_EN_footprint())
		return;

	if (timestamp - last_pet > (unsigned long long)NSEC_PER_THRESHOLD) {
		if (timestamp - last_pet_check > (unsigned long long)NSEC_PER_SEC) {
			last_pet_check = timestamp;
			pr_info("\n%s: MSM watchdog was blocked for more than %d seconds!\n",
				__func__, pet_check_counter++);
			pr_info("%s: Prepare to dump stack...\n",
				__func__);
			dump_stack();
			pr_info("%s: Prepare to dump pending works on global workqueue...\n",
				__func__);
			show_pending_work_on_gcwq();
			pr_info("\n ### Show Blocked State ###\n");
			show_state_filter(TASK_UNINTERRUPTIBLE);
		}
	}
}
EXPORT_SYMBOL(msm_watchdog_check_pet);

int msm_watchdog_suspend(struct device *dev)
{
	if (!enable || !msm_tmr0_base)
		return 0;

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	mb();
	set_WDT_EN_footprint(0);
	printk(KERN_DEBUG "msm_watchdog_suspend\n");
	return 0;
}
EXPORT_SYMBOL(msm_watchdog_suspend);

int msm_watchdog_resume(struct device *dev)
{
	if (!enable || !msm_tmr0_base)
		return 0;

	__raw_writel(1, msm_tmr0_base + WDT0_EN);
	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	last_pet = sched_clock();
	mb();
	set_dog_pet_footprint();
	set_WDT_EN_footprint(1);
	printk(KERN_DEBUG "msm_watchdog_resume\n");
	mb();
	return 0;
}
EXPORT_SYMBOL(msm_watchdog_resume);

static void msm_watchdog_stop(void)
{
	if (msm_tmr0_base) {
		__raw_writel(1, msm_tmr0_base + WDT0_RST);
		__raw_writel(0, msm_tmr0_base + WDT0_EN);
		set_WDT_EN_footprint(0);
		printk(KERN_INFO "msm_watchdog_stop");
	}
}

static int panic_wdog_handler(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	if (panic_timeout == 0) {
		__raw_writel(0, msm_tmr0_base + WDT0_EN);
		mb();
		set_WDT_EN_footprint(0);
	} else {
		__raw_writel(WDT_HZ * (panic_timeout + 4),
				msm_tmr0_base + WDT0_BARK_TIME);
		__raw_writel(WDT_HZ * (panic_timeout + 4),
				msm_tmr0_base + WDT0_BITE_TIME);
		__raw_writel(1, msm_tmr0_base + WDT0_RST);
	}
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_wdog_handler,
};

struct wdog_disable_work_data {
	struct work_struct work;
	struct completion complete;
};

static void wdog_disable_work(struct work_struct *work)
{
	struct wdog_disable_work_data *work_data =
		container_of(work, struct wdog_disable_work_data, work);
	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	mb();
	if (has_vic) {
		free_irq(WDT0_ACCSCSSNBARK_INT, 0);
	} else {
		disable_percpu_irq(WDT0_ACCSCSSNBARK_INT);
		if (!appsbark_fiq) {
			free_percpu_irq(WDT0_ACCSCSSNBARK_INT,
					percpu_pdata);
			free_percpu(percpu_pdata);
		}
	}
	enable = 0;
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_blk);
	cancel_delayed_work(&dogwork_struct);
	
	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	complete(&work_data->complete);
	pr_info("MSM Watchdog deactivated.\n");
}

static int wdog_enable_set(const char *val, struct kernel_param *kp)
{
	int ret = 0;
	int old_val = runtime_disable;
	struct wdog_disable_work_data work_data;

	mutex_lock(&disable_lock);
	if (!enable) {
		printk(KERN_INFO "MSM Watchdog is not active.\n");
		ret = -EINVAL;
		goto done;
	}

	ret = param_set_int(val, kp);
	if (ret)
		goto done;

	if (runtime_disable == 1) {
		if (old_val)
			goto done;
		init_completion(&work_data.complete);
		INIT_WORK_ONSTACK(&work_data.work, wdog_disable_work);
		schedule_work_on(0, &work_data.work);
		wait_for_completion(&work_data.complete);
	} else {
		runtime_disable = old_val;
		ret = -EINVAL;
	}
done:
	mutex_unlock(&disable_lock);
	return ret;
}

unsigned min_slack_ticks = UINT_MAX;
unsigned long long min_slack_ns = ULLONG_MAX;

void pet_watchdog(void)
{
	int slack;
	unsigned long long time_ns;
	unsigned long long slack_ns;
	unsigned long long bark_time_ns = bark_time * 1000000ULL;

	if (!enable)
		return;

	slack = __raw_readl(msm_tmr0_base + WDT0_STS) >> 3;
	slack = ((bark_time*WDT_HZ)/1000) - slack;
	if (slack < min_slack_ticks)
		min_slack_ticks = slack;
	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	time_ns = sched_clock();
	slack_ns = (last_pet + bark_time_ns) - time_ns;
	if (slack_ns < min_slack_ns)
		min_slack_ns = slack_ns;
	last_pet = time_ns;
#if HTC_WATCHDOG_TOP_SCHED_DUMP
       htc_watchdog_top_stat();
#endif
}
EXPORT_SYMBOL(pet_watchdog);

static void pet_watchdog_work(struct work_struct *work)
{
	pet_watchdog();
	set_dog_pet_footprint();
	pet_check_counter = PET_CHECK_THRESHOLD;
	if (enable)
		schedule_delayed_work_on(0, &dogwork_struct, delay_time);

	wtd_dump_irqs(0);
}

static int msm_watchdog_remove(struct platform_device *pdev)
{
	if (enable && msm_tmr0_base) {
		__raw_writel(0, msm_tmr0_base + WDT0_EN);
		mb();
		set_WDT_EN_footprint(0);
		if (has_vic) {
			free_irq(WDT0_ACCSCSSNBARK_INT, 0);
		} else {
			disable_percpu_irq(WDT0_ACCSCSSNBARK_INT);
			if (!appsbark_fiq) {
				free_percpu_irq(WDT0_ACCSCSSNBARK_INT,
						percpu_pdata);
				free_percpu(percpu_pdata);
			}
		}
		enable = 0;
		
		__raw_writel(0, msm_tmr0_base + WDT0_EN);
	}
	printk(KERN_INFO "MSM Watchdog Exit - Deactivated\n");
	return 0;
}

static irqreturn_t wdog_bark_handler(int irq, void *dev_id)
{
	unsigned long nanosec_rem;
	unsigned long long t = sched_clock();
	struct task_struct *tsk;

	
	if (atomic_add_return(1, &watchdog_bark_counter) != 1)
		return IRQ_HANDLED;

	nanosec_rem = do_div(t, 1000000000);
	printk(KERN_INFO "[K] Watchdog bark! Now = %lu.%06lu\n", (unsigned long) t,
		nanosec_rem / 1000);

	nanosec_rem = do_div(last_pet, 1000000000);
	printk(KERN_INFO "[K] Watchdog last pet at %lu.%06lu\n", (unsigned long)
		last_pet, nanosec_rem / 1000);

	if (print_all_stacks) {

		
		msm_watchdog_suspend(NULL);

		
		htc_watchdog_top_stat();
		
		print_modules();
		__show_regs(get_irq_regs());

		wtd_dump_irqs(1);

		dump_stack();

		printk(KERN_INFO "Stack trace dump:\n");

		for_each_process(tsk) {
			printk(KERN_INFO "\nPID: %d, Name: %s\n",
				tsk->pid, tsk->comm);
			show_stack(tsk, NULL);
		}

		
		printk(KERN_INFO "\n### Show Blocked State ###\n");
		show_state_filter(TASK_UNINTERRUPTIBLE);
#ifdef TRACING_WORKQUEUE_HISTORY
		print_workqueue();
#endif
		msm_watchdog_resume(NULL);
	}

	panic("Apps watchdog bark received!");
	return IRQ_HANDLED;
}

#define SCM_SET_REGSAVE_CMD 0x2

static void configure_bark_dump(void)
{
	int ret;
	struct {
		unsigned addr;
		int len;
	} cmd_buf;

	if (!appsbark) {
		cmd_buf.addr = MSM_TZ_DOG_BARK_REG_SAVE_PHYS;
		cmd_buf.len  = MSM_TZ_DOG_BARK_REG_SAVE_SIZE;

		ret = scm_call(SCM_SVC_UTIL, SCM_SET_REGSAVE_CMD,
			       &cmd_buf, sizeof(cmd_buf), NULL, 0);
		if (ret)
			pr_err("Setting register save address failed.\n"
			       "Registers won't be dumped on a dog "
			       "bite\n");
		else
			pr_info("%s: dogbark processed by TZ side, regsave address = 0x%X\n",
					__func__, cmd_buf.addr);
	} else
		pr_info("%s: dogbark processed by apps side\n", __func__);
}

struct fiq_handler wdog_fh = {
	.name = MODULE_NAME,
};

static void set_bark_bite_time(u64 timeout, int wait)
{
	int counter = wait;
	u64 bark_time = timeout;
	u64 bite_time = timeout + 3*WDT_HZ;

	__raw_writel(bark_time, msm_tmr0_base + WDT0_BARK_TIME);
	while(__raw_readl(msm_tmr0_base + WDT0_BARK_TIME) != bark_time
			&& counter != 0) {
		mdelay(1);
		counter--;
	}

	if (counter == 0) {
		pr_err("%s: error setting WDT0_BARK_TIME to %llu. value is = %d\n",
			__func__, bark_time, __raw_readl(msm_tmr0_base + WDT0_BARK_TIME));
	} else {
		pr_info("%s: successfully set WDT0_BARK_TIME to %llu in %d milliseconds!\n",
			__func__, bark_time, wait - counter);
	}

	counter = wait;
	__raw_writel(bite_time, msm_tmr0_base + WDT0_BITE_TIME);
	while(__raw_readl(msm_tmr0_base + WDT0_BITE_TIME) != bite_time
			&& counter != 0) {
		mdelay(1);
		counter--;
	}

	if (counter == 0) {
		pr_err("%s: error setting WDT0_BITE_TIME to %llu. value is = %d\n",
			__func__, bite_time, __raw_readl(msm_tmr0_base + WDT0_BITE_TIME));
	} else {
		pr_info("%s: successfully set WDT0_BITE_TIME to %llu in %d milliseconds!\n",
			__func__, bite_time, wait - counter);
	}
}

static void init_watchdog_work(struct work_struct *work)
{
	u64 timeout = (bark_time * WDT_HZ)/1000;
	void *stack;
	int ret;

	if (has_vic) {
		ret = request_irq(WDT0_ACCSCSSNBARK_INT, wdog_bark_handler, 0,
				  "apps_wdog_bark", NULL);
		if (ret)
			return;
	} else if (appsbark_fiq) {
		claim_fiq(&wdog_fh);
		set_fiq_handler(&msm_wdog_fiq_start, msm_wdog_fiq_length);
		stack = (void *)__get_free_pages(GFP_KERNEL, THREAD_SIZE_ORDER);
		if (!stack) {
			pr_info("No free pages available - %s fails\n",
					__func__);
			return;
		}

		msm_wdog_fiq_setup(stack);
		gic_set_irq_secure(WDT0_ACCSCSSNBARK_INT);
	} else {
		percpu_pdata = alloc_percpu(struct msm_watchdog_pdata *);
		if (!percpu_pdata) {
			pr_err("%s: memory allocation failed for percpu data\n",
					__func__);
			return;
		}

		
		ret = request_percpu_irq(WDT0_ACCSCSSNBARK_INT,
			wdog_bark_handler, "apps_wdog_bark", percpu_pdata);
		if (ret) {
			free_percpu(percpu_pdata);
			return;
		}
	}

	configure_bark_dump();

	
	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	__raw_writel(1, msm_tmr0_base + WDT0_RST);

	set_bark_bite_time(timeout, 1000);

	schedule_delayed_work_on(0, &dogwork_struct, delay_time);

	atomic_notifier_chain_register(&panic_notifier_list,
				       &panic_blk);

	__raw_writel(1, msm_tmr0_base + WDT0_EN);
	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	set_dog_pet_footprint();
	set_WDT_EN_footprint(1);
	last_pet = sched_clock();

	if (!has_vic)
		enable_percpu_irq(WDT0_ACCSCSSNBARK_INT, IRQ_TYPE_EDGE_RISING);

	htc_watchdog_monitor_init();

	printk(KERN_INFO "MSM Watchdog Initialized\n");

	return;
}

static int msm_watchdog_probe(struct platform_device *pdev)
{
	struct msm_watchdog_pdata *pdata = pdev->dev.platform_data;

	msm_tmr0_base = msm_timer_get_timer0_base();

	
	if (get_kernel_flag() & KERNEL_FLAG_WATCHDOG_ENABLE)
		enable = 0;

	if (!enable || !pdata || !pdata->pet_time || !pdata->bark_time) {
		
		msm_watchdog_stop();
		printk(KERN_INFO "MSM Watchdog Not Initialized\n");
		return -ENODEV;
	}

	bark_time = pdata->bark_time;
	has_vic = pdata->has_vic;
	
	if (!pdata->has_secure || get_kernel_flag() & KERNEL_FLAG_APPSBARK) {
		appsbark = 1;
		appsbark_fiq = pdata->use_kernel_fiq;
	}

	if (cpu_is_msm9615())
		__raw_writel(0xF, MSM_TCSR_BASE + TCSR_WDT_CFG);

	if (pdata->needs_expired_enable)
		__raw_writel(0x1, MSM_CLK_CTL_BASE + 0x3820);

	delay_time = msecs_to_jiffies(pdata->pet_time);
	schedule_work_on(0, &init_dogwork_struct);
	return 0;
}

static struct platform_driver msm_watchdog_driver = {
	.probe = msm_watchdog_probe,
	.remove = msm_watchdog_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int init_watchdog(void)
{
	return platform_driver_register(&msm_watchdog_driver);
}

late_initcall(init_watchdog);
MODULE_DESCRIPTION("MSM Watchdog Driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
