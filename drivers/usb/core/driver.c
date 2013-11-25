/*
 * drivers/usb/driver.c - most of the driver model stuff for usb
 *
 * (C) Copyright 2005 Greg Kroah-Hartman <gregkh@suse.de>
 *
 * based on drivers/usb/usb.c which had the following copyrights:
 *	(C) Copyright Linus Torvalds 1999
 *	(C) Copyright Johannes Erdfelt 1999-2001
 *	(C) Copyright Andreas Gal 1999
 *	(C) Copyright Gregory P. Smith 1999
 *	(C) Copyright Deti Fliegl 1999 (new USB architecture)
 *	(C) Copyright Randy Dunlap 2000
 *	(C) Copyright David Brownell 2000-2004
 *	(C) Copyright Yggdrasil Computing, Inc. 2000
 *		(usb_device_id matching changes by Adam J. Richter)
 *	(C) Copyright Greg Kroah-Hartman 2002-2003
 *
 * NOTE! This is not actually a driver at all, rather this is
 * just a collection of helper routines that implement the
 * matching, probing, releasing, suspending and resuming for
 * real drivers.
 *
 */

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include <linux/usb/hcd.h>
#include <mach/board_htc.h>  
#include "usb.h"


#ifdef CONFIG_HOTPLUG

ssize_t usb_store_new_id(struct usb_dynids *dynids,
			 struct device_driver *driver,
			 const char *buf, size_t count)
{
	struct usb_dynid *dynid;
	u32 idVendor = 0;
	u32 idProduct = 0;
	unsigned int bInterfaceClass = 0;
	int fields = 0;
	int retval = 0;

	fields = sscanf(buf, "%x %x %x", &idVendor, &idProduct,
					&bInterfaceClass);
	if (fields < 2)
		return -EINVAL;

	dynid = kzalloc(sizeof(*dynid), GFP_KERNEL);
	if (!dynid)
		return -ENOMEM;

	INIT_LIST_HEAD(&dynid->node);
	dynid->id.idVendor = idVendor;
	dynid->id.idProduct = idProduct;
	dynid->id.match_flags = USB_DEVICE_ID_MATCH_DEVICE;
	if (fields == 3) {
		dynid->id.bInterfaceClass = (u8)bInterfaceClass;
		dynid->id.match_flags |= USB_DEVICE_ID_MATCH_INT_CLASS;
	}

	spin_lock(&dynids->lock);
	list_add_tail(&dynid->node, &dynids->list);
	spin_unlock(&dynids->lock);

	retval = driver_attach(driver);

	if (retval)
		return retval;
	return count;
}
EXPORT_SYMBOL_GPL(usb_store_new_id);

static ssize_t store_new_id(struct device_driver *driver,
			    const char *buf, size_t count)
{
	struct usb_driver *usb_drv = to_usb_driver(driver);

	return usb_store_new_id(&usb_drv->dynids, driver, buf, count);
}
static DRIVER_ATTR(new_id, S_IWUSR, NULL, store_new_id);

static ssize_t
store_remove_id(struct device_driver *driver, const char *buf, size_t count)
{
	struct usb_dynid *dynid, *n;
	struct usb_driver *usb_driver = to_usb_driver(driver);
	u32 idVendor = 0;
	u32 idProduct = 0;
	int fields = 0;
	int retval = 0;

	fields = sscanf(buf, "%x %x", &idVendor, &idProduct);
	if (fields < 2)
		return -EINVAL;

	spin_lock(&usb_driver->dynids.lock);
	list_for_each_entry_safe(dynid, n, &usb_driver->dynids.list, node) {
		struct usb_device_id *id = &dynid->id;
		if ((id->idVendor == idVendor) &&
		    (id->idProduct == idProduct)) {
			list_del(&dynid->node);
			kfree(dynid);
			retval = 0;
			break;
		}
	}
	spin_unlock(&usb_driver->dynids.lock);

	if (retval)
		return retval;
	return count;
}
static DRIVER_ATTR(remove_id, S_IWUSR, NULL, store_remove_id);

static int usb_create_newid_files(struct usb_driver *usb_drv)
{
	int error = 0;

	if (usb_drv->no_dynamic_id)
		goto exit;

	if (usb_drv->probe != NULL) {
		error = driver_create_file(&usb_drv->drvwrap.driver,
					   &driver_attr_new_id);
		if (error == 0) {
			error = driver_create_file(&usb_drv->drvwrap.driver,
					&driver_attr_remove_id);
			if (error)
				driver_remove_file(&usb_drv->drvwrap.driver,
						&driver_attr_new_id);
		}
	}
exit:
	return error;
}

static void usb_remove_newid_files(struct usb_driver *usb_drv)
{
	if (usb_drv->no_dynamic_id)
		return;

	if (usb_drv->probe != NULL) {
		driver_remove_file(&usb_drv->drvwrap.driver,
				&driver_attr_remove_id);
		driver_remove_file(&usb_drv->drvwrap.driver,
				   &driver_attr_new_id);
	}
}

static void usb_free_dynids(struct usb_driver *usb_drv)
{
	struct usb_dynid *dynid, *n;

	spin_lock(&usb_drv->dynids.lock);
	list_for_each_entry_safe(dynid, n, &usb_drv->dynids.list, node) {
		list_del(&dynid->node);
		kfree(dynid);
	}
	spin_unlock(&usb_drv->dynids.lock);
}
#else
static inline int usb_create_newid_files(struct usb_driver *usb_drv)
{
	return 0;
}

static void usb_remove_newid_files(struct usb_driver *usb_drv)
{
}

static inline void usb_free_dynids(struct usb_driver *usb_drv)
{
}
#endif

static const struct usb_device_id *usb_match_dynamic_id(struct usb_interface *intf,
							struct usb_driver *drv)
{
	struct usb_dynid *dynid;

	spin_lock(&drv->dynids.lock);
	list_for_each_entry(dynid, &drv->dynids.list, node) {
		if (usb_match_one_id(intf, &dynid->id)) {
			spin_unlock(&drv->dynids.lock);
			return &dynid->id;
		}
	}
	spin_unlock(&drv->dynids.lock);
	return NULL;
}


static int usb_probe_device(struct device *dev)
{
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);
	struct usb_device *udev = to_usb_device(dev);
	int error = 0;

	dev_dbg(dev, "%s\n", __func__);

	

	if (!udriver->supports_autosuspend)
		error = usb_autoresume_device(udev);

	if (!error)
		error = udriver->probe(udev);


#ifdef HTC_PM_DBG
	if (!strncmp(dev_name(&udev->dev), "1-1", 3) && ( get_radio_flag() & 0x0001 ) ) {
		udev->enable_pm_debug = 1;
	} else {
		udev->enable_pm_debug = 0;
	}
	dev_info(&udev->dev, "udev(%s)->enable_pm_debug=[%d]\n", dev_name(&udev->dev), udev->enable_pm_debug);
#endif

	return error;
}

static int usb_unbind_device(struct device *dev)
{
	struct usb_device *udev = to_usb_device(dev);
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);

	udriver->disconnect(udev);
	if (!udriver->supports_autosuspend)
		usb_autosuspend_device(udev);
	return 0;
}

static void usb_cancel_queued_reset(struct usb_interface *iface)
{
	if (iface->reset_running == 0)
		cancel_work_sync(&iface->reset_ws);
}

static int usb_probe_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *udev = interface_to_usbdev(intf);
	const struct usb_device_id *id;
	int error = -ENODEV;

	dev_dbg(dev, "%s\n", __func__);

	intf->needs_binding = 0;

	if (usb_device_is_owned(udev))
		return error;

	if (udev->authorized == 0) {
		dev_err(&intf->dev, "Device is not authorized for usage\n");
		return error;
	}

	id = usb_match_id(intf, driver->id_table);
	if (!id)
		id = usb_match_dynamic_id(intf, driver);
	if (!id)
		return error;

	dev_dbg(dev, "%s - got id\n", __func__);

	error = usb_autoresume_device(udev);
	if (error)
		return error;

	intf->condition = USB_INTERFACE_BINDING;

	pm_runtime_set_active(dev);
	pm_suspend_ignore_children(dev, false);
	if (driver->supports_autosuspend)
		pm_runtime_enable(dev);

	
	if (intf->needs_altsetting0) {
		error = usb_set_interface(udev, intf->altsetting[0].
				desc.bInterfaceNumber, 0);
		if (error < 0)
			goto err;
		intf->needs_altsetting0 = 0;
	}

	error = driver->probe(intf, id);
	if (error)
		goto err;

	intf->condition = USB_INTERFACE_BOUND;
	dev_info(dev, "%s[%d] needs_remote_wakeup:%d\n", __func__, __LINE__, intf->needs_remote_wakeup);
	usb_autosuspend_device(udev);
	return error;

 err:
	intf->needs_remote_wakeup = 0;
	intf->condition = USB_INTERFACE_UNBOUND;
	usb_cancel_queued_reset(intf);

	
	if (driver->supports_autosuspend)
		pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);

	usb_autosuspend_device(udev);
	return error;
}

static int usb_unbind_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *udev;
	int error, r;

	intf->condition = USB_INTERFACE_UNBINDING;

	
	udev = interface_to_usbdev(intf);
	error = usb_autoresume_device(udev);

	if (!driver->soft_unbind)
		usb_disable_interface(udev, intf, false);

	driver->disconnect(intf);
	usb_cancel_queued_reset(intf);

	if (intf->cur_altsetting->desc.bAlternateSetting == 0) {
		usb_enable_interface(udev, intf, false);
	} else if (!error && !intf->dev.power.is_prepared) {
		r = usb_set_interface(udev, intf->altsetting[0].
				desc.bInterfaceNumber, 0);
		if (r < 0)
			intf->needs_altsetting0 = 1;
	} else {
		intf->needs_altsetting0 = 1;
	}
	usb_set_intfdata(intf, NULL);

	intf->condition = USB_INTERFACE_UNBOUND;
	intf->needs_remote_wakeup = 0;

	
	if (driver->supports_autosuspend)
		pm_runtime_disable(dev);
	pm_runtime_set_suspended(dev);

	
	for (r = atomic_read(&intf->pm_usage_cnt); r > 0; --r)
		usb_autopm_put_interface_no_suspend(intf);
	atomic_set(&intf->pm_usage_cnt, 0);

	if (!error)
		usb_autosuspend_device(udev);

	return 0;
}

int usb_driver_claim_interface(struct usb_driver *driver,
				struct usb_interface *iface, void *priv)
{
	struct device *dev = &iface->dev;
	int retval = 0;

	if (dev->driver)
		return -EBUSY;

	dev->driver = &driver->drvwrap.driver;
	usb_set_intfdata(iface, priv);
	iface->needs_binding = 0;

	iface->condition = USB_INTERFACE_BOUND;

	pm_suspend_ignore_children(dev, false);
	if (driver->supports_autosuspend)
		pm_runtime_enable(dev);
	else
		pm_runtime_set_active(dev);

	if (device_is_registered(dev))
		retval = device_bind_driver(dev);

	return retval;
}
EXPORT_SYMBOL_GPL(usb_driver_claim_interface);

void usb_driver_release_interface(struct usb_driver *driver,
					struct usb_interface *iface)
{
	struct device *dev = &iface->dev;

	
	if (!dev->driver || dev->driver != &driver->drvwrap.driver)
		return;

	
	if (iface->condition != USB_INTERFACE_BOUND)
		return;
	iface->condition = USB_INTERFACE_UNBINDING;

	if (device_is_registered(dev)) {
		device_release_driver(dev);
	} else {
		device_lock(dev);
		usb_unbind_interface(dev);
		dev->driver = NULL;
		device_unlock(dev);
	}
}
EXPORT_SYMBOL_GPL(usb_driver_release_interface);

int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}

int usb_match_one_id(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	struct usb_host_interface *intf;
	struct usb_device *dev;

	
	if (id == NULL)
		return 0;

	intf = interface->cur_altsetting;
	dev = interface_to_usbdev(interface);

	if (!usb_match_device(dev, id))
		return 0;

	if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC &&
			!(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			(id->match_flags & (USB_DEVICE_ID_MATCH_INT_CLASS |
				USB_DEVICE_ID_MATCH_INT_SUBCLASS |
				USB_DEVICE_ID_MATCH_INT_PROTOCOL)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
	    (id->bInterfaceClass != intf->desc.bInterfaceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
	    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
	    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
		return 0;

	return 1;
}
EXPORT_SYMBOL_GPL(usb_match_one_id);

const struct usb_device_id *usb_match_id(struct usb_interface *interface,
					 const struct usb_device_id *id)
{
	
	if (id == NULL)
		return NULL;

	for (; id->idVendor || id->idProduct || id->bDeviceClass ||
	       id->bInterfaceClass || id->driver_info; id++) {
		if (usb_match_one_id(interface, id))
			return id;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(usb_match_id);

static int usb_device_match(struct device *dev, struct device_driver *drv)
{
	
	if (is_usb_device(dev)) {

		
		if (!is_usb_device_driver(drv))
			return 0;

		
		return 1;

	} else if (is_usb_interface(dev)) {
		struct usb_interface *intf;
		struct usb_driver *usb_drv;
		const struct usb_device_id *id;

		
		if (is_usb_device_driver(drv))
			return 0;

		intf = to_usb_interface(dev);
		usb_drv = to_usb_driver(drv);

		id = usb_match_id(intf, usb_drv->id_table);
		if (id)
			return 1;

		id = usb_match_dynamic_id(intf, usb_drv);
		if (id)
			return 1;
	}

	return 0;
}

#ifdef	CONFIG_HOTPLUG
static int usb_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct usb_device *usb_dev;

	if (is_usb_device(dev)) {
		usb_dev = to_usb_device(dev);
	} else if (is_usb_interface(dev)) {
		struct usb_interface *intf = to_usb_interface(dev);

		usb_dev = interface_to_usbdev(intf);
	} else {
		return 0;
	}

	if (usb_dev->devnum < 0) {
		
		pr_debug("usb %s: already deleted?\n", dev_name(dev));
		return -ENODEV;
	}
	if (!usb_dev->bus) {
		pr_debug("usb %s: bus removed?\n", dev_name(dev));
		return -ENODEV;
	}

#ifdef	CONFIG_USB_DEVICEFS
	if (add_uevent_var(env, "DEVICE=/proc/bus/usb/%03d/%03d",
			   usb_dev->bus->busnum, usb_dev->devnum))
		return -ENOMEM;
#endif

	
	if (add_uevent_var(env, "PRODUCT=%x/%x/%x",
			   le16_to_cpu(usb_dev->descriptor.idVendor),
			   le16_to_cpu(usb_dev->descriptor.idProduct),
			   le16_to_cpu(usb_dev->descriptor.bcdDevice)))
		return -ENOMEM;

	
	if (add_uevent_var(env, "TYPE=%d/%d/%d",
			   usb_dev->descriptor.bDeviceClass,
			   usb_dev->descriptor.bDeviceSubClass,
			   usb_dev->descriptor.bDeviceProtocol))
		return -ENOMEM;

	return 0;
}

#else

static int usb_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return -ENODEV;
}
#endif	

int usb_register_device_driver(struct usb_device_driver *new_udriver,
		struct module *owner)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_udriver->drvwrap.for_devices = 1;
	new_udriver->drvwrap.driver.name = (char *) new_udriver->name;
	new_udriver->drvwrap.driver.bus = &usb_bus_type;
	new_udriver->drvwrap.driver.probe = usb_probe_device;
	new_udriver->drvwrap.driver.remove = usb_unbind_device;
	new_udriver->drvwrap.driver.owner = owner;

	retval = driver_register(&new_udriver->drvwrap.driver);

	if (!retval) {
		pr_info("%s: registered new device driver %s\n",
			usbcore_name, new_udriver->name);
		usbfs_update_special();
	} else {
		printk(KERN_ERR "%s: error %d registering device "
			"	driver %s\n",
			usbcore_name, retval, new_udriver->name);
	}

	return retval;
}
EXPORT_SYMBOL_GPL(usb_register_device_driver);

void usb_deregister_device_driver(struct usb_device_driver *udriver)
{
	pr_info("%s: deregistering device driver %s\n",
			usbcore_name, udriver->name);

	driver_unregister(&udriver->drvwrap.driver);
	usbfs_update_special();
}
EXPORT_SYMBOL_GPL(usb_deregister_device_driver);

int usb_register_driver(struct usb_driver *new_driver, struct module *owner,
			const char *mod_name)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_driver->drvwrap.for_devices = 0;
	new_driver->drvwrap.driver.name = (char *) new_driver->name;
	new_driver->drvwrap.driver.bus = &usb_bus_type;
	new_driver->drvwrap.driver.probe = usb_probe_interface;
	new_driver->drvwrap.driver.remove = usb_unbind_interface;
	new_driver->drvwrap.driver.owner = owner;
	new_driver->drvwrap.driver.mod_name = mod_name;
	spin_lock_init(&new_driver->dynids.lock);
	INIT_LIST_HEAD(&new_driver->dynids.list);

	retval = driver_register(&new_driver->drvwrap.driver);
	if (retval)
		goto out;

	usbfs_update_special();

	retval = usb_create_newid_files(new_driver);
	if (retval)
		goto out_newid;

	pr_info("%s: registered new interface driver %s\n",
			usbcore_name, new_driver->name);

out:
	return retval;

out_newid:
	driver_unregister(&new_driver->drvwrap.driver);

	printk(KERN_ERR "%s: error %d registering interface "
			"	driver %s\n",
			usbcore_name, retval, new_driver->name);
	goto out;
}
EXPORT_SYMBOL_GPL(usb_register_driver);

void usb_deregister(struct usb_driver *driver)
{
	pr_info("%s: deregistering interface driver %s\n",
			usbcore_name, driver->name);

	usb_remove_newid_files(driver);
	driver_unregister(&driver->drvwrap.driver);
	usb_free_dynids(driver);

	usbfs_update_special();
}
EXPORT_SYMBOL_GPL(usb_deregister);

void usb_forced_unbind_intf(struct usb_interface *intf)
{
	struct usb_driver *driver = to_usb_driver(intf->dev.driver);

	dev_dbg(&intf->dev, "forced unbind\n");
	usb_driver_release_interface(driver, intf);

	
	intf->needs_binding = 1;
}

void usb_rebind_intf(struct usb_interface *intf)
{
	int rc;

	
	if (intf->dev.driver)
		usb_forced_unbind_intf(intf);

	
	if (!intf->dev.power.is_prepared) {
		intf->needs_binding = 0;
		rc = device_attach(&intf->dev);
		if (rc < 0)
			dev_warn(&intf->dev, "rebind failed: %d\n", rc);
	}
}

#ifdef CONFIG_PM

static void unbind_no_pm_drivers_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;
	struct usb_driver	*drv;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];

			if (intf->dev.driver) {
				drv = to_usb_driver(intf->dev.driver);
				if (!drv->suspend || !drv->resume)
					usb_forced_unbind_intf(intf);
			}
		}
	}
}

static void unbind_no_reset_resume_drivers_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			if (intf->dev.driver && intf->needs_binding)
				usb_forced_unbind_intf(intf);
		}
	}
}

static void do_rebind_interfaces(struct usb_device *udev)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			if (intf->needs_binding)
				usb_rebind_intf(intf);
		}
	}
}

static int usb_suspend_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	
	if (udev->dev.driver)
		udriver = to_usb_device_driver(udev->dev.driver);
	else {
		udev->do_remote_wakeup = 0;
		udriver = &usb_generic_driver;
	}
	status = udriver->suspend(udev, msg);

 done:
	dev_dbg(&udev->dev, "%s: status %d\n", __func__, status);
	
	if (status != 0)
		dev_err(&udev->dev, "%s: status %d\n", __func__, status);
	
	return status;
}

static int usb_resume_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED)
		goto done;

	
	if (udev->dev.driver == NULL) {
		status = -ENOTCONN;
		goto done;
	}

	if (!PMSG_IS_AUTO(msg) && udev->parent && udev->bus->hs_companion)
		device_pm_wait_for_dev(&udev->dev,
				&udev->bus->hs_companion->root_hub->dev);

	if (udev->quirks & USB_QUIRK_RESET_RESUME)
		udev->reset_resume = 1;

	udriver = to_usb_device_driver(udev->dev.driver);
	status = udriver->resume(udev, msg);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_suspend_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED ||
			intf->condition == USB_INTERFACE_UNBOUND)
		goto done;
	driver = to_usb_driver(intf->dev.driver);

	
	status = driver->suspend(intf, msg);
	if (status && !PMSG_IS_AUTO(msg))
		dev_err(&intf->dev, "suspend error %d\n", status);

 done:
	if (get_radio_flag() & 0x1)
		dev_info(&intf->dev, "%s: status %d\n", __func__, status);

	
	if (status != 0)
		dev_err(&intf->dev, "%s: status %d\n", __func__, status);
	
	return status;
}

static int usb_resume_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg, int reset_resume)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED)
		goto done;

	
	if (intf->condition == USB_INTERFACE_UNBINDING)
		goto done;

	
	if (intf->condition == USB_INTERFACE_UNBOUND) {

		
		if (intf->needs_altsetting0 && !intf->dev.power.is_prepared) {
			usb_set_interface(udev, intf->altsetting[0].
					desc.bInterfaceNumber, 0);
			intf->needs_altsetting0 = 0;
		}
		goto done;
	}

	
	if (intf->needs_binding) {
		dev_err(&intf->dev, "%s needs_binding:%d, goto done!!!\n", __func__, intf->needs_binding);   
		goto done;
    }
	driver = to_usb_driver(intf->dev.driver);

	if (reset_resume) {
		if (driver->reset_resume) {

			status = driver->reset_resume(intf);

			if (status)
				dev_err(&intf->dev, "%s error %d\n",
						"reset_resume", status);
		} else {
			intf->needs_binding = 1;
			dev_warn(&intf->dev, "no %s for driver %s?\n",
					"reset_resume", driver->name);
		}
	} else {
		status = driver->resume(intf);
		if (status)
			dev_err(&intf->dev, "resume error %d\n", status);
	}

done:
	if (get_radio_flag() & 0x1)
		dev_info(&intf->dev, "%s: status %d\n", __func__, status);

	
	if (status != 0)
		dev_err(&intf->dev, "%s: status %d\n", __func__, status);
	

	
	return status;
}

static int usb_suspend_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i = 0, n = 0;
	struct usb_interface	*intf;

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		dev_info(&udev->dev, "%s+\n", __func__);

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	
	if (udev->actconfig) {
		n = udev->actconfig->desc.bNumInterfaces;
		
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
			dev_info(&udev->dev, "%s[%d] suspend intf n:%d\n", __func__, __LINE__, n);

		for (i = n - 1; i >= 0; --i) {
			intf = udev->actconfig->interface[i];
			status = usb_suspend_interface(udev, intf, msg);

			
			if (!PMSG_IS_AUTO(msg))
				status = 0;
			if (status != 0) {
				
				dev_err(&udev->dev, "%s: usb_suspend_interface %d failed:: = %d\n", __func__, i, status);
				break;
			}
		}
	}
	if (status == 0) {
		status = usb_suspend_device(udev, msg);

		if (udev->parent && !PMSG_IS_AUTO(msg))
			status = 0;
		if (status == -EPROTO) {
			int r = 0;
			dev_info(&udev->dev, "%s:  status=[%d], call usb_resume_device+\n", __func__, status);
			r = usb_resume_device(udev, msg);
			dev_info(&udev->dev, "%s:  usb_resume_device-, r=[%d]\n", __func__, r);
		}
	}

	
	if (status != 0) {
		
		dev_info(&udev->dev, "%s:  suspend failed:: n = %d\n", __func__, n);

		msg.event ^= (PM_EVENT_SUSPEND | PM_EVENT_RESUME);
		while (++i < n) {
			intf = udev->actconfig->interface[i];
			usb_resume_interface(udev, intf, msg, 0);
		}

	} else {
		udev->can_submit = 0;
		for (i = 0; i < 16; ++i) {
			usb_hcd_flush_endpoint(udev, udev->ep_out[i]);
			usb_hcd_flush_endpoint(udev, udev->ep_in[i]);
		}
	}

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		dev_info(&udev->dev, "%s-: status %d\n", __func__, status);

	return status;
}

static int usb_resume_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i;
	struct usb_interface	*intf;

	if (udev->state == USB_STATE_NOTATTACHED) {
		status = -ENODEV;
		goto done;
	}
	udev->can_submit = 1;

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		dev_info(&udev->dev, "%s+\n", __func__);

	
	if (udev->state == USB_STATE_SUSPENDED || udev->reset_resume) {
		status = usb_resume_device(udev, msg);
	}

	
	if (status == 0 && udev->actconfig) {
		
		int n = udev->actconfig->desc.bNumInterfaces;
		dev_info(&udev->dev, "%s[%d] resume intf n:%d\n", __func__, __LINE__, n);
		

		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];
			usb_resume_interface(udev, intf, msg,
					udev->reset_resume);
		}
	}
	usb_mark_last_busy(udev);

 done:

 #ifdef HTC_PM_DBG
	udev->auto_suspend_timer_set = 0;
 #endif

	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);

	
	if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
		dev_info(&udev->dev, "%s-: status %d\n", __func__, status);

	if (!status)
		udev->reset_resume = 0;
	return status;
}

#ifdef CONFIG_USB_OTG


#define DO_UNBIND	0
#define DO_REBIND	1

static void do_unbind_rebind(struct usb_device *udev, int action)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;
	struct usb_driver	*drv;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			switch (action) {
			case DO_UNBIND:
				if (intf->dev.driver) {
					drv = to_usb_driver(intf->dev.driver);
					if (!drv->suspend || !drv->resume)
						usb_forced_unbind_intf(intf);
				}
				break;
			case DO_REBIND:
				if (intf->needs_binding)
					usb_rebind_intf(intf);
				break;
			}
		}
	}
}

void usb_hnp_polling_work(struct work_struct *work)
{
	int ret;
	struct usb_bus *bus =
		container_of(work, struct usb_bus, hnp_polling.work);
	struct usb_device *udev = bus->root_hub->children[bus->otg_port - 1];
	u8 *status = NULL;

	if (bus->quick_hnp) {
		bus->quick_hnp = 0;
		goto start_hnp;
	}

	status = kmalloc(sizeof(*status), GFP_KERNEL);
	if (!status)
		goto reschedule;

	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
		USB_REQ_GET_STATUS, USB_DIR_IN | USB_RECIP_DEVICE,
		0, OTG_STATUS_SELECTOR, status, sizeof(*status),
		USB_CTRL_GET_TIMEOUT);
	if (ret < 0) {
		
		dev_info(&udev->dev, "HNP polling failed. status %d\n", ret);
		goto out;
	}

	if (!(*status & (1 << HOST_REQUEST_FLAG)))
		goto reschedule;

start_hnp:
	do_unbind_rebind(udev, DO_UNBIND);
	udev->do_remote_wakeup = device_may_wakeup(&udev->dev);
	ret = usb_suspend_both(udev, PMSG_USER_SUSPEND);
	if (ret)
		dev_info(&udev->dev, "suspend failed\n");
	goto out;

reschedule:
	schedule_delayed_work(&bus->hnp_polling,
		msecs_to_jiffies(THOST_REQ_POLL));
out:
	kfree(status);
}
#endif

static void choose_wakeup(struct usb_device *udev, pm_message_t msg)
{
	int	w;

	if (msg.event == PM_EVENT_FREEZE || msg.event == PM_EVENT_QUIESCE) {
		if (udev->state != USB_STATE_SUSPENDED)
			udev->do_remote_wakeup = 0;
		return;
	}

	w = device_may_wakeup(&udev->dev);

	if (udev->state == USB_STATE_SUSPENDED && w != udev->do_remote_wakeup)
		pm_runtime_resume(&udev->dev);
	udev->do_remote_wakeup = w;
}

int usb_suspend(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev = to_usb_device(dev);

	if (udev->bus->skip_resume) {
		if (udev->state == USB_STATE_SUSPENDED) {
			
			if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD) {
				dev_info(dev, "skip suspend\n");
			}
			
			return 0;
		} else {
			dev_err(dev, "abort suspend\n");
			return -EBUSY;
		}
	}

	unbind_no_pm_drivers_interfaces(udev);

	choose_wakeup(udev, msg);
	return usb_suspend_both(udev, msg);
}

int usb_resume_complete(struct device *dev)
{
	struct usb_device *udev = to_usb_device(dev);

	if (udev->state != USB_STATE_NOTATTACHED)
		do_rebind_interfaces(udev);
	return 0;
}

int usb_resume(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;

	if (udev->bus->skip_resume) {
		
		if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD) {
			dev_info(&udev->dev, "skip_resume\n");
		}
		
		return 0;
	}

	pm_runtime_get_sync(dev->parent);
	status = usb_resume_both(udev, msg);
	if (status == 0) {
		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
		unbind_no_reset_resume_drivers_interfaces(udev);
	}
	pm_runtime_put_sync(dev->parent);

	if (status == -ENODEV || status == -ESHUTDOWN)
		status = 0;
	return status;
}

#endif 

#ifdef CONFIG_USB_SUSPEND

void usb_enable_autosuspend(struct usb_device *udev)
{
	pm_runtime_allow(&udev->dev);
}
EXPORT_SYMBOL_GPL(usb_enable_autosuspend);

void usb_disable_autosuspend(struct usb_device *udev)
{
	pm_runtime_forbid(&udev->dev);
}
EXPORT_SYMBOL_GPL(usb_disable_autosuspend);

void usb_autosuspend_device(struct usb_device *udev)
{
	int	status;

	usb_mark_last_busy(udev);
	status = pm_runtime_put_sync_autosuspend(&udev->dev);
	dev_vdbg(&udev->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&udev->dev.power.usage_count),
			status);
}

int usb_autoresume_device(struct usb_device *udev)
{
	int	status;

	status = pm_runtime_get_sync(&udev->dev);

	if (status < 0)
		pm_runtime_put_sync(&udev->dev);
	dev_vdbg(&udev->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&udev->dev.power.usage_count),
			status);
	if (status > 0)
		status = 0;
	return status;
}

void usb_autopm_put_interface(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status;


#ifdef HTC_PM_DBG
	if ( udev && udev->enable_pm_debug && intf ) {
		usb_mark_intf_last_busy(intf, false);
	}
#endif

	usb_mark_last_busy(udev);
	atomic_dec(&intf->pm_usage_cnt);
	status = pm_runtime_put_sync(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface);

void usb_autopm_put_interface_async(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status;

#ifdef HTC_PM_DBG
	if ( udev && udev->enable_pm_debug && intf ) {
		usb_mark_intf_last_busy(intf, false);
	}
#endif

	usb_mark_last_busy(udev);
	atomic_dec(&intf->pm_usage_cnt);
	status = pm_runtime_put(&intf->dev);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface_async);

void usb_autopm_put_interface_no_suspend(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);

#ifdef HTC_PM_DBG
	if ( udev && udev->enable_pm_debug && intf ) {
		usb_mark_intf_last_busy(intf, false);
	}
#endif

	usb_mark_last_busy(udev);
	atomic_dec(&intf->pm_usage_cnt);
	pm_runtime_put_noidle(&intf->dev);
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface_no_suspend);

int usb_autopm_get_interface(struct usb_interface *intf)
{
	int	status;

	status = pm_runtime_get_sync(&intf->dev);
	if (status < 0)
		pm_runtime_put_sync(&intf->dev);
	else
		atomic_inc(&intf->pm_usage_cnt);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
	if (status > 0)
		status = 0;
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface);

int usb_autopm_get_interface_async(struct usb_interface *intf)
{
	int	status;

	status = pm_runtime_get(&intf->dev);
	if (status < 0 && status != -EINPROGRESS)
		pm_runtime_put_noidle(&intf->dev);
	else
		atomic_inc(&intf->pm_usage_cnt);
	dev_vdbg(&intf->dev, "%s: cnt %d -> %d\n",
			__func__, atomic_read(&intf->dev.power.usage_count),
			status);
	if (status > 0 || status == -EINPROGRESS)
		status = 0;
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface_async);

void usb_autopm_get_interface_no_resume(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);

#ifdef HTC_PM_DBG
	if ( udev && udev->enable_pm_debug && intf ) {
		usb_mark_intf_last_busy(intf, false);
	}
#endif

	usb_mark_last_busy(udev);
	atomic_inc(&intf->pm_usage_cnt);
	pm_runtime_get_noresume(&intf->dev);
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface_no_resume);

static int autosuspend_check(struct usb_device *udev)
{
	int			w, i;
	struct usb_interface	*intf;

	w = 0;
	if (udev->actconfig) {
		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];

			if (intf->dev.power.disable_depth)
				continue;
			if (atomic_read(&intf->dev.power.usage_count) > 0) {
				dev_info(&intf->dev, "%s: EBUSY, cnt %d\n",
				__func__, atomic_read(&intf->dev.power.usage_count));
				return -EBUSY;
			}
			w |= intf->needs_remote_wakeup;

			if (udev->quirks & USB_QUIRK_RESET_RESUME) {
				struct usb_driver *driver;

				driver = to_usb_driver(intf->dev.driver);
				if (!driver->reset_resume ||
						intf->needs_remote_wakeup)
					return -EOPNOTSUPP;
			}
		}
	}
	if (w && !device_can_wakeup(&udev->dev)) {
		dev_dbg(&udev->dev, "remote wakeup needed for autosuspend\n");
		return -EOPNOTSUPP;
	}
	udev->do_remote_wakeup = w;
	return 0;
}

int usb_runtime_suspend(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;
	
	int ret = 0;
	

	
	ret = autosuspend_check(udev);
	
	if (ret != 0) {
		dev_info(dev, "%s : autosuspend_check ret %d\n", __func__, ret);
		return -EAGAIN;
	}
	status = usb_suspend_both(udev, PMSG_AUTO_SUSPEND);

	
	if (status == -EAGAIN || status == -EBUSY)
		usb_mark_last_busy(udev);

	if (status != 0)
		return -EBUSY;
	return status;
}

int usb_runtime_resume(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			status;

	status = usb_resume_both(udev, PMSG_AUTO_RESUME);
	return status;
}

int usb_runtime_idle(struct device *dev)
{
	struct usb_device	*udev = to_usb_device(dev);

	if (autosuspend_check(udev) == 0)
		pm_runtime_autosuspend(dev);
	return 0;
}

int usb_set_usb2_hardware_lpm(struct usb_device *udev, int enable)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	int ret = -EPERM;

	if (hcd->driver->set_usb2_hw_lpm) {
		ret = hcd->driver->set_usb2_hw_lpm(hcd, udev, enable);
		if (!ret)
			udev->usb2_hw_lpm_enabled = enable;
	}

	return ret;
}

#endif 

int usb_set_interrupt_latency(struct usb_device *udev, int latency)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	int ret = -EOPNOTSUPP;
	pr_info("[%s]usb_set_interrupt_latency+\n", __func__);

	if (hcd->driver->set_int_latency) {
		ret = hcd->driver->set_int_latency(hcd, latency);
	} else {
		pr_info("[%s] set_int_latency is null\n", __func__);
	}

	pr_info("[%s] usb_set_interrupt_latency-, r=[%d]\n", __func__, ret);
	return ret;
}

struct bus_type usb_bus_type = {
	.name =		"usb",
	.match =	usb_device_match,
	.uevent =	usb_uevent,
};
