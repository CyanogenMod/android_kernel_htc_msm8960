/*
 * drivers/base/power/runtime.c - Helper functions for device runtime PM
 *
 * Copyright (c) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 * Copyright (C) 2010 Alan Stern <stern@rowland.harvard.edu>
 *
 * This file is released under the GPLv2.
 */

#include <linux/sched.h>
#include <linux/export.h>
#include <linux/pm_runtime.h>
#include <trace/events/rpm.h>
#include "power.h"
#include <linux/usb.h>
#include <mach/board_htc.h>

#if defined(CONFIG_USB_EHCI_MSM_HSIC)
extern struct device *mdm_usb1_1_dev;
extern struct device *msm_hsic_host_dev;
#endif	

#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
extern int mdm_is_in_restart;
#endif 

static int rpm_resume(struct device *dev, int rpmflags);
static int rpm_suspend(struct device *dev, int rpmflags);

void update_pm_runtime_accounting(struct device *dev)
{
	unsigned long now = jiffies;
	unsigned long delta;

	delta = now - dev->power.accounting_timestamp;

	dev->power.accounting_timestamp = now;

	if (dev->power.disable_depth > 0)
		return;

	if (dev->power.runtime_status == RPM_SUSPENDED)
		dev->power.suspended_jiffies += delta;
	else
		dev->power.active_jiffies += delta;
}

static void __update_runtime_status(struct device *dev, enum rpm_status status)
{
	update_pm_runtime_accounting(dev);
	dev->power.runtime_status = status;
}

static void pm_runtime_deactivate_timer(struct device *dev)
{
	if (dev->power.timer_expires > 0) {
		del_timer(&dev->power.suspend_timer);
		dev->power.timer_expires = 0;
	}
}

static void pm_runtime_cancel_pending(struct device *dev)
{
	pm_runtime_deactivate_timer(dev);
	dev->power.request = RPM_REQ_NONE;
}

unsigned long pm_runtime_autosuspend_expiration(struct device *dev)
{
	int autosuspend_delay;
	long elapsed;
	unsigned long last_busy;
	unsigned long expires = 0;

	if (!dev->power.use_autosuspend)
		goto out;

	autosuspend_delay = ACCESS_ONCE(dev->power.autosuspend_delay);
	if (autosuspend_delay < 0)
		goto out;

	last_busy = ACCESS_ONCE(dev->power.last_busy);
	elapsed = jiffies - last_busy;
	if (elapsed < 0)
		goto out;	

	expires = last_busy + msecs_to_jiffies(autosuspend_delay);
	if (autosuspend_delay >= 1000)
		expires = round_jiffies(expires);
	expires += !expires;
	if (elapsed >= expires - last_busy)
		expires = 0;	

 out:
	return expires;
}
EXPORT_SYMBOL_GPL(pm_runtime_autosuspend_expiration);

static int rpm_check_suspend_allowed(struct device *dev)
{
	int retval = 0;

	if (dev->power.runtime_error)
		retval = -EINVAL;
	else if (dev->power.disable_depth > 0)
		retval = -EACCES;
	else if (atomic_read(&dev->power.usage_count) > 0)
		retval = -EAGAIN;
	else if (!pm_children_suspended(dev))
		retval = -EBUSY;

	
	else if ((dev->power.deferred_resume
			&& dev->power.runtime_status == RPM_SUSPENDING)
	    || (dev->power.request_pending
			&& dev->power.request == RPM_REQ_RESUME))
		retval = -EAGAIN;
	else if (dev->power.runtime_status == RPM_SUSPENDED)
		retval = 1;

	return retval;
}

static int __rpm_callback(int (*cb)(struct device *), struct device *dev)
	__releases(&dev->power.lock) __acquires(&dev->power.lock)
{
	int retval;

	if (dev->power.irq_safe)
		spin_unlock(&dev->power.lock);
	else
		spin_unlock_irq(&dev->power.lock);

	retval = cb(dev);

	if (dev->power.irq_safe)
		spin_lock(&dev->power.lock);
	else
		spin_lock_irq(&dev->power.lock);

	return retval;
}

static int rpm_idle(struct device *dev, int rpmflags)
{
	int (*callback)(struct device *);
	int retval;

	trace_rpm_idle(dev, rpmflags);
	retval = rpm_check_suspend_allowed(dev);
	
	
	#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
		dev_info(dev,"%s: rpm_check_suspend_allowed return %d\n", __func__, retval);
	}
	#endif	
	
	
	if (retval < 0)
		;	

	
	else if (dev->power.runtime_status != RPM_ACTIVE)
		retval = -EAGAIN;

	else if (dev->power.request_pending &&
	    dev->power.request > RPM_REQ_IDLE)
		retval = -EAGAIN;

	
	else if (dev->power.idle_notification)
		retval = -EINPROGRESS;
	if (retval)
		goto out;

	
	dev->power.request = RPM_REQ_NONE;

	if (dev->power.no_callbacks) {
		
		retval = rpm_suspend(dev, rpmflags);
		goto out;
	}

	
	if (rpmflags & RPM_ASYNC) {
		dev->power.request = RPM_REQ_IDLE;
		if (!dev->power.request_pending) {
			dev->power.request_pending = true;
			queue_work(pm_wq, &dev->power.work);
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (msm_hsic_host_dev == dev && dev && dev->power.htc_hsic_dbg_enable) {
				dev_info(dev," %s: queue work for msm_hsic_host suspend %d\n", __func__, retval);
			}
			#endif	
			
		}
		goto out;
	}

	dev->power.idle_notification = true;

	if (dev->pm_domain)
		callback = dev->pm_domain->ops.runtime_idle;
	else if (dev->type && dev->type->pm)
		callback = dev->type->pm->runtime_idle;
	else if (dev->class && dev->class->pm)
		callback = dev->class->pm->runtime_idle;
	else if (dev->bus && dev->bus->pm)
		callback = dev->bus->pm->runtime_idle;
	else
		callback = NULL;

	if (!callback && dev->driver && dev->driver->pm)
		callback = dev->driver->pm->runtime_idle;

	if (callback)
		__rpm_callback(callback, dev);

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable && mdm_usb1_1_dev != dev)
		dev_info(dev, "%s[%d] __rpm_callback(%x)\n", __func__, __LINE__, (unsigned int)callback);
	#endif	
	
	

	dev->power.idle_notification = false;
	wake_up_all(&dev->power.wait_queue);

 out:
	trace_rpm_return_int(dev, _THIS_IP_, retval);

	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (msm_hsic_host_dev == dev && (get_radio_flag() & 0x0001)) {
		dev_info(dev," %s: retval:%d\n", __func__, retval);
	}
	#endif	
	

	return retval;
}

static int rpm_callback(int (*cb)(struct device *), struct device *dev)
{
	int retval;

	if (!cb)
		return -ENOSYS;

	retval = __rpm_callback(cb, dev);

	dev->power.runtime_error = retval;
	return retval != -EACCES ? retval : -EIO;
}

struct rpm_qos_data {
	ktime_t time_now;
	s64 constraint_ns;
};

static int rpm_update_qos_constraint(struct device *dev, void *data)
{
	struct rpm_qos_data *qos = data;
	unsigned long flags;
	s64 delta_ns;
	int ret = 0;

	spin_lock_irqsave(&dev->power.lock, flags);

	if (dev->power.max_time_suspended_ns < 0)
		goto out;

	delta_ns = dev->power.max_time_suspended_ns -
		ktime_to_ns(ktime_sub(qos->time_now, dev->power.suspend_time));
	if (delta_ns <= 0) {
		ret = -EBUSY;
		goto out;
	}

	if (qos->constraint_ns > delta_ns || qos->constraint_ns == 0)
		qos->constraint_ns = delta_ns;

 out:
	spin_unlock_irqrestore(&dev->power.lock, flags);

	return ret;
}

static int rpm_suspend(struct device *dev, int rpmflags)
	__releases(&dev->power.lock) __acquires(&dev->power.lock)
{
	int (*callback)(struct device *);
	struct device *parent = NULL;
	struct rpm_qos_data qos;
	int retval;
	int retval_parent_idle = 0;

	trace_rpm_suspend(dev, rpmflags);

 repeat:
	retval = rpm_check_suspend_allowed(dev);

	if (retval < 0)
		;	

	
	else if (dev->power.runtime_status == RPM_RESUMING &&
	    !(rpmflags & RPM_ASYNC))
		retval = -EAGAIN;
	if (retval)
		goto out;

	
	if ((rpmflags & RPM_AUTO)
	    && dev->power.runtime_status != RPM_SUSPENDING) {
		unsigned long expires = pm_runtime_autosuspend_expiration(dev);

		if (expires != 0) {
			
			dev->power.request = RPM_REQ_NONE;

			if (!(dev->power.timer_expires && time_before_eq(
			    dev->power.timer_expires, expires))) {
			    
#ifdef HTC_PM_DBG
#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
				struct usb_device *udev = NULL;
				extern struct usb_device *mdm_usb1_1_usbdev;
				extern struct device *mdm_usb1_1_dev;

				if (mdm_usb1_1_dev == dev)
					udev = mdm_usb1_1_usbdev;

				if (udev) {
					if (!(udev->auto_suspend_timer_set)) {
						udev->auto_suspend_timer_set = 1;
						if (dev && dev->power.htc_hsic_dbg_enable)
							dev_info(dev, "%s[%d] dev->power.timer_expires=%lx, expires=%lx\n",
								__func__, __LINE__, dev->power.timer_expires, expires);
					}
				}
#endif
#endif
				
				dev->power.timer_expires = expires;
				mod_timer(&dev->power.suspend_timer, expires);
			}
			dev->power.timer_autosuspends = 1;
			goto out;
		}
	}

	
	pm_runtime_cancel_pending(dev);

	if (dev->power.runtime_status == RPM_SUSPENDING) {
		DEFINE_WAIT(wait);

		if (rpmflags & (RPM_ASYNC | RPM_NOWAIT)) {
			retval = -EINPROGRESS;
			goto out;
		}

		if (dev->power.irq_safe) {
			spin_unlock(&dev->power.lock);

			cpu_relax();

			spin_lock(&dev->power.lock);
			goto repeat;
		}

		
		for (;;) {
			prepare_to_wait(&dev->power.wait_queue, &wait,
					TASK_UNINTERRUPTIBLE);
			if (dev->power.runtime_status != RPM_SUSPENDING)
				break;

			spin_unlock_irq(&dev->power.lock);

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && dev->power.htc_hsic_dbg_enable)
				dev_info(dev, "%s[%d] schedule+\n", __func__, __LINE__);
			#endif	
			
			

			schedule();

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && dev->power.htc_hsic_dbg_enable)
				dev_info(dev, "%s[%d] schedule-\n", __func__, __LINE__);
			#endif	
			
			

			spin_lock_irq(&dev->power.lock);
		}
		finish_wait(&dev->power.wait_queue, &wait);
		goto repeat;
	}

	dev->power.deferred_resume = false;
	if (dev->power.no_callbacks)
		goto no_callback;	

	
	if (rpmflags & RPM_ASYNC) {
		dev->power.request = (rpmflags & RPM_AUTO) ?
		    RPM_REQ_AUTOSUSPEND : RPM_REQ_SUSPEND;
		if (!dev->power.request_pending) {
			dev->power.request_pending = true;

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] queue_work request:%x\n", __func__, __LINE__, dev->power.request);
		#endif	
		
		

			queue_work(pm_wq, &dev->power.work);
		}
		goto out;
	}

	qos.constraint_ns = __dev_pm_qos_read_value(dev);
	if (qos.constraint_ns < 0) {
		
		retval = -EPERM;
		goto out;
	}
	qos.constraint_ns *= NSEC_PER_USEC;
	qos.time_now = ktime_get();

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x0001))
		dev_info(dev, "%s[%d] runtime_status RPM_SUSPENDING\n", __func__, __LINE__);
	#endif	
	
	

	__update_runtime_status(dev, RPM_SUSPENDING);

	if (!dev->power.ignore_children) {
		if (dev->power.irq_safe)
			spin_unlock(&dev->power.lock);
		else
			spin_unlock_irq(&dev->power.lock);

		retval = device_for_each_child(dev, &qos,
					       rpm_update_qos_constraint);

		if (dev->power.irq_safe)
			spin_lock(&dev->power.lock);
		else
			spin_lock_irq(&dev->power.lock);

		if (retval)
			goto fail;
	}

	dev->power.suspend_time = qos.time_now;
	dev->power.max_time_suspended_ns = qos.constraint_ns ? : -1;

	if (dev->pm_domain)
		callback = dev->pm_domain->ops.runtime_suspend;
	else if (dev->type && dev->type->pm)
		callback = dev->type->pm->runtime_suspend;
	else if (dev->class && dev->class->pm)
		callback = dev->class->pm->runtime_suspend;
	else if (dev->bus && dev->bus->pm)
		callback = dev->bus->pm->runtime_suspend;
	else
		callback = NULL;

	if (!callback && dev->driver && dev->driver->pm)
		callback = dev->driver->pm->runtime_suspend;

	retval = rpm_callback(callback, dev);

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable)
		dev_info(dev, "%s[%d] rpm_callback(%x) retval:%d\n", __func__, __LINE__, (unsigned int)callback, retval);
	#endif	
	
	

	if (retval)
		goto fail;

 no_callback:
	__update_runtime_status(dev, RPM_SUSPENDED);

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x0001))
		dev_info(dev, "%s[%d] runtime_status RPM_SUSPENDED\n", __func__, __LINE__);
	#endif	
	
	

	pm_runtime_deactivate_timer(dev);

	if (dev->parent) {
		parent = dev->parent;
		atomic_add_unless(&parent->power.child_count, -1, 0);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (parent && msm_hsic_host_dev == parent && (get_radio_flag() & 0x0001)) {
			dev_info(parent, "%s[%d]child_count[%d]\n", __func__, __LINE__, atomic_read(&parent->power.child_count));
		}
		#endif	
		
		
	}
	wake_up_all(&dev->power.wait_queue);

	if (dev->power.deferred_resume) {

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] rpm_resume+ deferred_resume:%d\n", __func__, __LINE__, dev->power.deferred_resume);
		#endif	
		
		

		rpm_resume(dev, 0);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] rpm_resume- deferred_resume:%d\n", __func__, __LINE__, dev->power.deferred_resume);
		#endif	
		
		

		retval = -EAGAIN;
		goto out;
	}

	
	if (parent && !parent->power.ignore_children && !dev->power.irq_safe) {
		spin_unlock(&dev->power.lock);

		spin_lock(&parent->power.lock);

		retval_parent_idle = rpm_idle(parent, RPM_ASYNC);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (msm_hsic_host_dev == parent) {
			if (retval_parent_idle) {
				dev_info(dev, "%s[%d] rpm_idle parent failed:%d\n", __func__, __LINE__, retval_parent_idle);
				dev_info(parent, "%s[%d] runtime_error[%d] disable_depth[%d] usage_count[%d] child_count[%d]\n", __func__, __LINE__,
					parent->power.runtime_error, parent->power.disable_depth, atomic_read(&parent->power.usage_count), atomic_read(&parent->power.child_count));
				dev_info(parent, "%s[%d] deferred_resume[%d] runtime_status[%d] request_pending[%d] request[%d]\n", __func__, __LINE__,
					parent->power.deferred_resume, parent->power.runtime_status, parent->power.request_pending, parent->power.request);
			}
		}
		#endif	
		
		

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] rpm_idle parent ret:%d\n", __func__, __LINE__, retval_parent_idle);
		#endif	
		
		

		spin_unlock(&parent->power.lock);

		spin_lock(&dev->power.lock);
	}
#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	else {
		if (dev && dev->power.htc_hsic_dbg_enable) {
			dev_info(dev, "%s[%d] parent:%x\n", __func__, __LINE__, (unsigned int)parent);
			if (parent)
				dev_info(parent, "%s[%d] ignore_children:%d\n", __func__, __LINE__, parent->power.ignore_children);
			dev_info(dev, "%s[%d] irq_safe:%x\n", __func__, __LINE__, dev->power.irq_safe);
		}
	}
#endif	

 out:
	trace_rpm_return_int(dev, _THIS_IP_, retval);

	return retval;

 fail:
	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable)
		dev_info(dev, "%s[%d] runtime_status RPM_ACTIVE !!!\n", __func__, __LINE__);
	#endif	
	
	
	__update_runtime_status(dev, RPM_ACTIVE);
	dev->power.suspend_time = ktime_set(0, 0);
	dev->power.max_time_suspended_ns = -1;
	dev->power.deferred_resume = false;
	wake_up_all(&dev->power.wait_queue);

	if (retval == -EAGAIN || retval == -EBUSY) {
		dev->power.runtime_error = 0;

		if ((rpmflags & RPM_AUTO) &&
		    pm_runtime_autosuspend_expiration(dev) != 0)
			goto repeat;
	} else {
		pm_runtime_cancel_pending(dev);
	}
	goto out;
}

static int rpm_resume(struct device *dev, int rpmflags)
	__releases(&dev->power.lock) __acquires(&dev->power.lock)
{
	int (*callback)(struct device *);
	struct device *parent = NULL;
	int retval = 0;
	int log_enable = 0;

	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (msm_hsic_host_dev == dev && (get_radio_flag() & 0x0001)) {
		log_enable = 1;
	}
	#endif	
	

	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] rpmflags=[0x%x], runtime_error=[%d], disable_depth=[%d], timer_autosuspends=[%d], runtime_status=[%d], irq_safe=[%d]\n", __func__, __LINE__,
		rpmflags, dev->power.runtime_error, dev->power.disable_depth, dev->power.timer_autosuspends, dev->power.runtime_status, dev->power.irq_safe);

	trace_rpm_resume(dev, rpmflags);

 repeat:
	if (dev->power.runtime_error)
		retval = -EINVAL;
	else if (dev->power.disable_depth > 0)
		retval = -EACCES;
	if (retval)
		goto out;

	dev->power.request = RPM_REQ_NONE;
	if (!dev->power.timer_autosuspends)
		pm_runtime_deactivate_timer(dev);

	if (dev->power.runtime_status == RPM_ACTIVE) {
		retval = 1;
		goto out;
	}

	if (dev->power.runtime_status == RPM_RESUMING
	    || dev->power.runtime_status == RPM_SUSPENDING) {
		DEFINE_WAIT(wait);
		if (rpmflags & (RPM_ASYNC | RPM_NOWAIT)) {
			if (dev->power.runtime_status == RPM_SUSPENDING) {
				dev->power.deferred_resume = true;
			} else {
				retval = -EINPROGRESS;
			}
			goto out;
		}

		if (dev->power.irq_safe) {
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_unlock\n", __func__, __LINE__);
			spin_unlock(&dev->power.lock);
			cpu_relax();

			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_lock+\n", __func__, __LINE__);
			spin_lock(&dev->power.lock);
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_lock-\n", __func__, __LINE__);
			goto repeat;
		}

		
		for (;;) {
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] prepare_to_wait+\n", __func__, __LINE__);
			prepare_to_wait(&dev->power.wait_queue, &wait,
					TASK_UNINTERRUPTIBLE);
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] prepare_to_wait-\n", __func__, __LINE__);
			if (dev->power.runtime_status != RPM_RESUMING
			    && dev->power.runtime_status != RPM_SUSPENDING) {
				break;
			}

			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_unlock_irq\n", __func__, __LINE__);
			spin_unlock_irq(&dev->power.lock);


			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && dev->power.htc_hsic_dbg_enable)
				dev_info(dev, "%s[%d] schedule+\n", __func__, __LINE__);
			#endif	
			
			

			schedule();

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && dev->power.htc_hsic_dbg_enable)
				dev_info(dev, "%s[%d] schedule-\n", __func__, __LINE__);
			#endif	
			
			

			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_lock_irq+\n", __func__, __LINE__);
			spin_lock_irq(&dev->power.lock);
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_lock_irq-\n", __func__, __LINE__);
		}
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] finish_wait+\n", __func__, __LINE__);
		finish_wait(&dev->power.wait_queue, &wait);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] finish_wait-\n", __func__, __LINE__);
		goto repeat;
	}
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] no_callbacks=[0x%x], parent=[0x%x], dev->parent=[0x%x]\n", __func__, __LINE__,
		dev->power.no_callbacks, (uint)parent, (uint)dev->parent);
	if (dev->power.no_callbacks && !parent && dev->parent) {
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock_nested+\n", __func__, __LINE__);
		spin_lock_nested(&dev->parent->power.lock, SINGLE_DEPTH_NESTING);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock_nested-\n", __func__, __LINE__);
		if (dev->parent->power.disable_depth > 0
		    || dev->parent->power.ignore_children
		    || dev->parent->power.runtime_status == RPM_ACTIVE) {
			atomic_inc(&dev->parent->power.child_count);

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (parent && msm_hsic_host_dev == parent && (get_radio_flag() & 0x0001)) {
				dev_info(parent, "%s[%d]child_count[%d]\n", __func__, __LINE__, atomic_read(&parent->power.child_count));
			}
			#endif	
			
			

			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] spin_unlock\n", __func__, __LINE__);
			spin_unlock(&dev->parent->power.lock);
			goto no_callback;	
		}
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_unlock\n", __func__, __LINE__);
		spin_unlock(&dev->parent->power.lock);
	}

	
	if (rpmflags & RPM_ASYNC) {
		dev->power.request = RPM_REQ_RESUME;
		if (!dev->power.request_pending) {
			dev->power.request_pending = true;

			if (!strncmp(dev_name(dev), "msm_hsic_host", 13)) {
				
				
				#if defined(CONFIG_USB_EHCI_MSM_HSIC)
				if (dev && dev->power.htc_hsic_dbg_enable)
					pr_info("%s: RT PM work. %s(0x%x) \n", __FUNCTION__, dev_name(dev), (unsigned int)dev);
				#endif	
				
				

				
				#if defined(CONFIG_USB_EHCI_MSM_HSIC)
				if (dev != msm_hsic_host_dev) {
					pr_info("%s: dev(0x%x) msm_hsic_host_dev(0x%x) \n", __FUNCTION__, (unsigned int)dev, (unsigned int)msm_hsic_host_dev);
				}
				#endif	
				

				queue_work(pm_rt_wq, &dev->power.work);
			} else {
				queue_work(pm_wq, &dev->power.work);
			}
		}
		retval = 0;
		goto out;
	}

	if (!parent && dev->parent) {
		parent = dev->parent;
		if (dev->power.irq_safe)
			goto skip_parent;
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_unlock\n", __func__, __LINE__);
		spin_unlock(&dev->power.lock);

		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] pm_runtime_get_noresume+\n", __func__, __LINE__);
		pm_runtime_get_noresume(parent);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] pm_runtime_get_noresume-\n", __func__, __LINE__);

		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock+\n", __func__, __LINE__);
		spin_lock(&parent->power.lock);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock-\n", __func__, __LINE__);
		if (!parent->power.disable_depth
		    && !parent->power.ignore_children) {
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] rpm_resume+\n", __func__, __LINE__);
			rpm_resume(parent, 0);
			if ( log_enable == 1 )
				dev_info(dev, "%s[%d] rpm_resume-\n", __func__, __LINE__);
			if (parent->power.runtime_status != RPM_ACTIVE)
				retval = -EBUSY;
		}
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_unlock\n", __func__, __LINE__);
		spin_unlock(&parent->power.lock);

		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock+\n", __func__, __LINE__);
		spin_lock(&dev->power.lock);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock-\n", __func__, __LINE__);

		if (retval)
			goto out;
		goto repeat;
	}
 skip_parent:

	if (dev->power.no_callbacks)
		goto no_callback;	

	dev->power.suspend_time = ktime_set(0, 0);
	dev->power.max_time_suspended_ns = -1;

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x0001))
		dev_info(dev, "%s[%d] runtime_status RPM_RESUMING\n", __func__, __LINE__);
	#endif	
	
	

	__update_runtime_status(dev, RPM_RESUMING);

	if (dev->pm_domain)
		callback = dev->pm_domain->ops.runtime_resume;
	else if (dev->type && dev->type->pm)
		callback = dev->type->pm->runtime_resume;
	else if (dev->class && dev->class->pm)
		callback = dev->class->pm->runtime_resume;
	else if (dev->bus && dev->bus->pm)
		callback = dev->bus->pm->runtime_resume;
	else
		callback = NULL;

	if (!callback && dev->driver && dev->driver->pm)
		callback = dev->driver->pm->runtime_resume;

	retval = rpm_callback(callback, dev);

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable)
		dev_info(dev, "%s[%d] rpm_callback(%x) ret:%d\n", __func__, __LINE__, (unsigned int)callback, retval);
	#endif	
	
	

	if (retval) {
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] runtime_status RPM_SUSPENDED\n", __func__, __LINE__);
		#endif	
		
		

		__update_runtime_status(dev, RPM_SUSPENDED);

		pm_runtime_cancel_pending(dev);
	} else {
 no_callback:
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x0001))
			dev_info(dev, "%s[%d] runtime_status RPM_ACTIVE\n", __func__, __LINE__);
		#endif	
		
		

		__update_runtime_status(dev, RPM_ACTIVE);
		if (parent)
			atomic_inc(&parent->power.child_count);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (parent && msm_hsic_host_dev == parent && (get_radio_flag() & 0x0001)) {
			dev_info(parent, "%s[%d]child_count[%d]\n", __func__, __LINE__, atomic_read(&parent->power.child_count));
		}
		#endif	
		
		
	}
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] wake_up_all+\n", __func__, __LINE__);
	wake_up_all(&dev->power.wait_queue);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] wake_up_all-\n", __func__, __LINE__);
	if (!retval) {
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] rpm_idle+\n", __func__, __LINE__);
		rpm_idle(dev, RPM_ASYNC);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] rpm_idle-\n", __func__, __LINE__);
	}

 out:
	if (parent && !dev->power.irq_safe) {
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_unlock_irq\n", __func__, __LINE__);
		spin_unlock_irq(&dev->power.lock);

		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] pm_runtime_put+\n", __func__, __LINE__);
		pm_runtime_put(parent);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] pm_runtime_put-\n", __func__, __LINE__);

		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock_irq+\n", __func__, __LINE__);
		spin_lock_irq(&dev->power.lock);
		if ( log_enable == 1 )
			dev_info(dev, "%s[%d] spin_lock_irq-\n", __func__, __LINE__);
	}

	trace_rpm_return_int(dev, _THIS_IP_, retval);
	return retval;
}

static void pm_runtime_work(struct work_struct *work)
{
	struct device *dev = container_of(work, struct device, power.work);
	enum rpm_request req;
	int log_enable = 0;

	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (msm_hsic_host_dev == dev && (get_radio_flag() & 0x0001)) {
		log_enable = 1;
	}
	#endif	
	

	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] request=[%d], request_pending=[%d]\n", __func__, __LINE__, dev->power.request, dev->power.request_pending);
	spin_lock_irq(&dev->power.lock);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] spin_lock_irq+\n", __func__, __LINE__);
	if (!dev->power.request_pending)
		goto out;
	req = dev->power.request;
	dev->power.request = RPM_REQ_NONE;
	dev->power.request_pending = false;

	switch (req) {
	case RPM_REQ_NONE:
		break;
	case RPM_REQ_IDLE:
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable && mdm_usb1_1_dev != dev)
			dev_info(dev, "%s[%d] RPM_REQ_IDLE rpm_idle+\n", __func__, __LINE__);
		#endif	
		
		
		rpm_idle(dev, RPM_NOWAIT);
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable && mdm_usb1_1_dev != dev)
			dev_info(dev, "%s[%d] RPM_REQ_IDLE rpm_idle-\n", __func__, __LINE__);
		#endif	
		
		
		break;
	case RPM_REQ_SUSPEND:
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_SUSPEND rpm_suspend+\n", __func__, __LINE__);
		#endif	
		
		

		rpm_suspend(dev, RPM_NOWAIT);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_SUSPEND rpm_suspend-\n", __func__, __LINE__);
		#endif	
		
		
		break;
	case RPM_REQ_AUTOSUSPEND:
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_AUTOSUSPEND rpm_suspend+\n", __func__, __LINE__);
		#endif	
		
		

		rpm_suspend(dev, RPM_NOWAIT | RPM_AUTO);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_AUTOSUSPEND rpm_suspend-\n", __func__, __LINE__);
		#endif	
		
		
		break;
	case RPM_REQ_RESUME:
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_RESUME rpm_resume+\n", __func__, __LINE__);
		#endif	
		
		
		rpm_resume(dev, RPM_NOWAIT);
		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable)
			dev_info(dev, "%s[%d] RPM_REQ_RESUME rpm_resume-\n", __func__, __LINE__);
		#endif	
		
		
		break;
	}

 out:
	spin_unlock_irq(&dev->power.lock);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] spin_lock_irq-\n", __func__, __LINE__);
}

static void pm_suspend_timer_fn(unsigned long data)
{
	struct device *dev = (struct device *)data;
	unsigned long flags;
	unsigned long expires;

	spin_lock_irqsave(&dev->power.lock, flags);

	expires = dev->power.timer_expires;

	
	if (expires > 0 && !time_after(expires, jiffies)) {
		dev->power.timer_expires = 0;

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x1))
			dev_info(dev, "%s[%d] rpm_suspend+\n", __func__, __LINE__);
		#endif	
		
		

		rpm_suspend(dev, dev->power.timer_autosuspends ?
		    (RPM_ASYNC | RPM_AUTO) : RPM_ASYNC);

		
		
		#if defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x1))
			dev_info(dev, "%s[%d] rpm_suspend-\n", __func__, __LINE__);
		#endif	
		
		

	}

	spin_unlock_irqrestore(&dev->power.lock, flags);
}

int pm_schedule_suspend(struct device *dev, unsigned int delay)
{
	unsigned long flags;
	int retval;

	spin_lock_irqsave(&dev->power.lock, flags);

	if (!delay) {
		retval = rpm_suspend(dev, RPM_ASYNC);
		goto out;
	}

	retval = rpm_check_suspend_allowed(dev);
	if (retval)
		goto out;

	
	pm_runtime_cancel_pending(dev);

	dev->power.timer_expires = jiffies + msecs_to_jiffies(delay);
	dev->power.timer_expires += !dev->power.timer_expires;
	dev->power.timer_autosuspends = 0;
	mod_timer(&dev->power.suspend_timer, dev->power.timer_expires);

 out:
	spin_unlock_irqrestore(&dev->power.lock, flags);

	return retval;
}
EXPORT_SYMBOL_GPL(pm_schedule_suspend);

int __pm_runtime_idle(struct device *dev, int rpmflags)
{
	unsigned long flags;
	int retval;

	might_sleep_if(!(rpmflags & RPM_ASYNC) && !dev->power.irq_safe);

	if (rpmflags & RPM_GET_PUT) {
		if (!atomic_dec_and_test(&dev->power.usage_count)) {
			
			
			#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
				dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
					atomic_read(&dev->power.usage_count));
			}
			#endif	
			
			

			return 0;
		}

		
		
		#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
			dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
				atomic_read(&dev->power.usage_count));
		}
		#endif	
		
		
	}

	spin_lock_irqsave(&dev->power.lock, flags);
	retval = rpm_idle(dev, rpmflags);
	spin_unlock_irqrestore(&dev->power.lock, flags);

	return retval;
}
EXPORT_SYMBOL_GPL(__pm_runtime_idle);

int __pm_runtime_suspend(struct device *dev, int rpmflags)
{
	unsigned long flags;
	int retval;

	might_sleep_if(!(rpmflags & RPM_ASYNC) && !dev->power.irq_safe);

	if (rpmflags & RPM_GET_PUT) {
		if (!atomic_dec_and_test(&dev->power.usage_count)) {
			
			
			#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
				dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
					atomic_read(&dev->power.usage_count));
			}
			#endif	
			
			
			return 0;
		}

		
		
		#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
			dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
				atomic_read(&dev->power.usage_count));
		}
		#endif	
		
		
	}

	spin_lock_irqsave(&dev->power.lock, flags);
	retval = rpm_suspend(dev, rpmflags);
	spin_unlock_irqrestore(&dev->power.lock, flags);

	return retval;
}
EXPORT_SYMBOL_GPL(__pm_runtime_suspend);

int __pm_runtime_resume(struct device *dev, int rpmflags)
{
	unsigned long flags;
	int retval;
	int log_enable = 0;

	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (msm_hsic_host_dev == dev && (get_radio_flag() & 0x0001)) {
		log_enable = 1;
	}
	#endif	
	

	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] rpmflags=[%d], power.irq_safe=[%d], might_sleep_if=[%d]\n", __func__, __LINE__, rpmflags, dev->power.irq_safe, (!(rpmflags & RPM_ASYNC) && !dev->power.irq_safe));

	might_sleep_if(!(rpmflags & RPM_ASYNC) && !dev->power.irq_safe);

	if (rpmflags & RPM_GET_PUT) {
		atomic_inc(&dev->power.usage_count);

		
		
		#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
		if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
			dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
				atomic_read(&dev->power.usage_count));
		}
		#endif	
		
		
	}

	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] spin_lock_irqsave+\n", __func__, __LINE__);
	spin_lock_irqsave(&dev->power.lock, flags);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] rpm_resume+\n", __func__, __LINE__);
	retval = rpm_resume(dev, rpmflags);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] rpm_resume-\n", __func__, __LINE__);
	spin_unlock_irqrestore(&dev->power.lock, flags);
	if ( log_enable == 1 )
		dev_info(dev, "%s[%d] spin_lock_irqsave-\n", __func__, __LINE__);

	return retval;
}
EXPORT_SYMBOL_GPL(__pm_runtime_resume);

int __pm_runtime_set_status(struct device *dev, unsigned int status)
{
	struct device *parent = dev->parent;
	unsigned long flags;
	bool notify_parent = false;
	int error = 0;

	if (status != RPM_ACTIVE && status != RPM_SUSPENDED)
		return -EINVAL;

	spin_lock_irqsave(&dev->power.lock, flags);

	if (!dev->power.runtime_error && !dev->power.disable_depth) {
		error = -EAGAIN;
		goto out;
	}

	if (dev->power.runtime_status == status)
		goto out_set;

	if (status == RPM_SUSPENDED) {
		
		if (parent) {
			atomic_add_unless(&parent->power.child_count, -1, 0);

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (parent && msm_hsic_host_dev == parent && (get_radio_flag() & 0x0001)) {
				dev_info(parent, "%s[%d]child_count[%d]\n", __func__, __LINE__, atomic_read(&parent->power.child_count));
			}
			#endif	
			
			

			notify_parent = !parent->power.ignore_children;
		}
		goto out_set;
	}

	if (parent) {
		spin_lock_nested(&parent->power.lock, SINGLE_DEPTH_NESTING);

		if (!parent->power.disable_depth
		    && !parent->power.ignore_children
		    && parent->power.runtime_status != RPM_ACTIVE)
			error = -EBUSY;
		else if (dev->power.runtime_status == RPM_SUSPENDED) {
			atomic_inc(&parent->power.child_count);

			
			
			#if defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (parent && msm_hsic_host_dev == parent && (get_radio_flag() & 0x0001)) {
				dev_info(parent, "%s[%d]child_count[%d]\n", __func__, __LINE__, atomic_read(&parent->power.child_count));
			}
			#endif	
			
			
		}

		spin_unlock(&parent->power.lock);

		if (error)
			goto out;
	}

 out_set:
	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && dev->power.htc_hsic_dbg_enable && (get_radio_flag() & 0x0001))
		dev_info(dev, "%s[%d] runtime_status %d\n", __func__, __LINE__, status);
	#endif	
	
	
	__update_runtime_status(dev, status);
	dev->power.runtime_error = 0;
 out:
	spin_unlock_irqrestore(&dev->power.lock, flags);

	if (notify_parent)
		pm_request_idle(parent);

	return error;
}
EXPORT_SYMBOL_GPL(__pm_runtime_set_status);

static void __pm_runtime_barrier(struct device *dev)
{
	pm_runtime_deactivate_timer(dev);

	if (dev->power.request_pending) {
		dev->power.request = RPM_REQ_NONE;
		spin_unlock_irq(&dev->power.lock);

		cancel_work_sync(&dev->power.work);

		spin_lock_irq(&dev->power.lock);
		dev->power.request_pending = false;
	}

	if (dev->power.runtime_status == RPM_SUSPENDING
	    || dev->power.runtime_status == RPM_RESUMING
	    || dev->power.idle_notification) {
		DEFINE_WAIT(wait);

		
		for (;;) {
			prepare_to_wait(&dev->power.wait_queue, &wait,
					TASK_UNINTERRUPTIBLE);
			if (dev->power.runtime_status != RPM_SUSPENDING
			    && dev->power.runtime_status != RPM_RESUMING
			    && !dev->power.idle_notification)
				break;
			spin_unlock_irq(&dev->power.lock);

			schedule();

			spin_lock_irq(&dev->power.lock);
		}
		finish_wait(&dev->power.wait_queue, &wait);
	}
}

int pm_runtime_barrier(struct device *dev)
{
	int retval = 0;

	pm_runtime_get_noresume(dev);
	spin_lock_irq(&dev->power.lock);

	if (dev->power.request_pending
	    && dev->power.request == RPM_REQ_RESUME) {
		rpm_resume(dev, 0);
		retval = 1;
	}

	__pm_runtime_barrier(dev);

	spin_unlock_irq(&dev->power.lock);
	pm_runtime_put_noidle(dev);

	return retval;
}
EXPORT_SYMBOL_GPL(pm_runtime_barrier);

void __pm_runtime_disable(struct device *dev, bool check_resume)
{
	spin_lock_irq(&dev->power.lock);

	if (dev->power.disable_depth > 0) {
		dev->power.disable_depth++;
		goto out;
	}

	if (check_resume && dev->power.request_pending
	    && dev->power.request == RPM_REQ_RESUME) {
		pm_runtime_get_noresume(dev);

		rpm_resume(dev, 0);

		pm_runtime_put_noidle(dev);
	}

	if (!dev->power.disable_depth++)
		__pm_runtime_barrier(dev);

 out:
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(__pm_runtime_disable);

void pm_runtime_enable(struct device *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->power.lock, flags);

	if (dev->power.disable_depth > 0)
		dev->power.disable_depth--;
	else
		dev_warn(dev, "Unbalanced %s!\n", __func__);

	spin_unlock_irqrestore(&dev->power.lock, flags);
}
EXPORT_SYMBOL_GPL(pm_runtime_enable);

void pm_runtime_forbid(struct device *dev)
{
	spin_lock_irq(&dev->power.lock);
	if (!dev->power.runtime_auto)
		goto out;

	dev->power.runtime_auto = false;
	atomic_inc(&dev->power.usage_count);

	
	
	#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
		dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
			atomic_read(&dev->power.usage_count));
	}
	#endif	
	
	

	rpm_resume(dev, 0);

 out:
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(pm_runtime_forbid);

void pm_runtime_allow(struct device *dev)
{
	spin_lock_irq(&dev->power.lock);
	if (dev->power.runtime_auto)
		goto out;

	dev->power.runtime_auto = true;
	if (atomic_dec_and_test(&dev->power.usage_count))
		rpm_idle(dev, RPM_AUTO);

	
	
	#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
	if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
		dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
			atomic_read(&dev->power.usage_count));
	}
	#endif	
	
	

 out:
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(pm_runtime_allow);

void pm_runtime_no_callbacks(struct device *dev)
{
	spin_lock_irq(&dev->power.lock);
	dev->power.no_callbacks = 1;
	spin_unlock_irq(&dev->power.lock);
	if (device_is_registered(dev))
		rpm_sysfs_remove(dev);
}
EXPORT_SYMBOL_GPL(pm_runtime_no_callbacks);

void pm_runtime_irq_safe(struct device *dev)
{
	if (dev->parent)
		pm_runtime_get_sync(dev->parent);
	spin_lock_irq(&dev->power.lock);
	dev->power.irq_safe = 1;
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(pm_runtime_irq_safe);

static void update_autosuspend(struct device *dev, int old_delay, int old_use)
{
	int delay = dev->power.autosuspend_delay;

	
	if (dev->power.use_autosuspend && delay < 0) {

		
		if (!old_use || old_delay >= 0) {
			atomic_inc(&dev->power.usage_count);

			
			
			#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
				dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
					atomic_read(&dev->power.usage_count));
			}
			#endif	
			
			

			rpm_resume(dev, 0);
		}
	}

	
	else {

		
		if (old_use && old_delay < 0) {
			atomic_dec(&dev->power.usage_count);

			
			
			#if defined(CONFIG_ARCH_APQ8064) && defined(CONFIG_USB_EHCI_MSM_HSIC)
			if (dev && msm_hsic_host_dev == dev && (mdm_is_in_restart || (get_radio_flag() & 0x0001))) {
				dev_info(dev, "%s[%d] usage_count[%d]\n", __func__, __LINE__,
					atomic_read(&dev->power.usage_count));
			}
			#endif	
			
			
		}

		
		rpm_idle(dev, RPM_AUTO);
	}
}

void pm_runtime_set_autosuspend_delay(struct device *dev, int delay)
{
	int old_delay, old_use;

	spin_lock_irq(&dev->power.lock);
	old_delay = dev->power.autosuspend_delay;
	old_use = dev->power.use_autosuspend;
	dev->power.autosuspend_delay = delay;
	update_autosuspend(dev, old_delay, old_use);
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(pm_runtime_set_autosuspend_delay);

void __pm_runtime_use_autosuspend(struct device *dev, bool use)
{
	int old_delay, old_use;

	spin_lock_irq(&dev->power.lock);
	old_delay = dev->power.autosuspend_delay;
	old_use = dev->power.use_autosuspend;
	dev->power.use_autosuspend = use;
	update_autosuspend(dev, old_delay, old_use);
	spin_unlock_irq(&dev->power.lock);
}
EXPORT_SYMBOL_GPL(__pm_runtime_use_autosuspend);

void pm_runtime_init(struct device *dev)
{
	dev->power.runtime_status = RPM_SUSPENDED;
	dev->power.idle_notification = false;

	dev->power.disable_depth = 1;
	atomic_set(&dev->power.usage_count, 0);

	dev->power.runtime_error = 0;

	atomic_set(&dev->power.child_count, 0);
	pm_suspend_ignore_children(dev, false);
	dev->power.runtime_auto = true;

	dev->power.request_pending = false;
	dev->power.request = RPM_REQ_NONE;
	dev->power.deferred_resume = false;
	dev->power.accounting_timestamp = jiffies;
	INIT_WORK(&dev->power.work, pm_runtime_work);

	dev->power.timer_expires = 0;
	setup_timer(&dev->power.suspend_timer, pm_suspend_timer_fn,
			(unsigned long)dev);

	dev->power.suspend_time = ktime_set(0, 0);
	dev->power.max_time_suspended_ns = -1;

	init_waitqueue_head(&dev->power.wait_queue);

	
	
	#if defined(CONFIG_USB_EHCI_MSM_HSIC)
	dev->power.htc_hsic_dbg_enable = 0;
	#endif	
	
	
}

void pm_runtime_remove(struct device *dev)
{
	__pm_runtime_disable(dev, false);

	
	if (dev->power.runtime_status == RPM_ACTIVE)
		pm_runtime_set_suspended(dev);
	if (dev->power.irq_safe && dev->parent)
		pm_runtime_put_sync(dev->parent);
}

void pm_runtime_update_max_time_suspended(struct device *dev, s64 delta_ns)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->power.lock, flags);

	if (delta_ns > 0 && dev->power.max_time_suspended_ns > 0) {
		if (dev->power.max_time_suspended_ns > delta_ns)
			dev->power.max_time_suspended_ns -= delta_ns;
		else
			dev->power.max_time_suspended_ns = 0;
	}

	spin_unlock_irqrestore(&dev->power.lock, flags);
}
