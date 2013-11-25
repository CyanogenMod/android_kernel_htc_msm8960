/*
 * attribute_container.c - implementation of a simple container for classes
 *
 * Copyright (c) 2005 - James Bottomley <James.Bottomley@steeleye.com>
 *
 * This file is licensed under GPLv2
 *
 * The basic idea here is to enable a device to be attached to an
 * aritrary numer of classes without having to allocate storage for them.
 * Instead, the contained classes select the devices they need to attach
 * to via a matching function.
 */

#include <linux/attribute_container.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "base.h"

struct internal_container {
	struct klist_node node;
	struct attribute_container *cont;
	struct device classdev;
};

static void internal_container_klist_get(struct klist_node *n)
{
	struct internal_container *ic =
		container_of(n, struct internal_container, node);
	get_device(&ic->classdev);
}

static void internal_container_klist_put(struct klist_node *n)
{
	struct internal_container *ic =
		container_of(n, struct internal_container, node);
	put_device(&ic->classdev);
}


struct attribute_container *
attribute_container_classdev_to_container(struct device *classdev)
{
	struct internal_container *ic =
		container_of(classdev, struct internal_container, classdev);
	return ic->cont;
}
EXPORT_SYMBOL_GPL(attribute_container_classdev_to_container);

static LIST_HEAD(attribute_container_list);

static DEFINE_MUTEX(attribute_container_mutex);

int
attribute_container_register(struct attribute_container *cont)
{
	INIT_LIST_HEAD(&cont->node);
	klist_init(&cont->containers,internal_container_klist_get,
		   internal_container_klist_put);
		
	mutex_lock(&attribute_container_mutex);
	list_add_tail(&cont->node, &attribute_container_list);
	mutex_unlock(&attribute_container_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(attribute_container_register);

int
attribute_container_unregister(struct attribute_container *cont)
{
	int retval = -EBUSY;
	mutex_lock(&attribute_container_mutex);
	spin_lock(&cont->containers.k_lock);
	if (!list_empty(&cont->containers.k_list))
		goto out;
	retval = 0;
	list_del(&cont->node);
 out:
	spin_unlock(&cont->containers.k_lock);
	mutex_unlock(&attribute_container_mutex);
	return retval;
		
}
EXPORT_SYMBOL_GPL(attribute_container_unregister);

static void attribute_container_release(struct device *classdev)
{
	struct internal_container *ic 
		= container_of(classdev, struct internal_container, classdev);
	struct device *dev = classdev->parent;

	kfree(ic);
	put_device(dev);
}

void
attribute_container_add_device(struct device *dev,
			       int (*fn)(struct attribute_container *,
					 struct device *,
					 struct device *))
{
	struct attribute_container *cont;

	mutex_lock(&attribute_container_mutex);
	list_for_each_entry(cont, &attribute_container_list, node) {
		struct internal_container *ic;

		if (attribute_container_no_classdevs(cont))
			continue;

		if (!cont->match(cont, dev))
			continue;

		ic = kzalloc(sizeof(*ic), GFP_KERNEL);
		if (!ic) {
			dev_printk(KERN_ERR, dev, "failed to allocate class container\n");
			continue;
		}

		ic->cont = cont;
		device_initialize(&ic->classdev);
		ic->classdev.parent = get_device(dev);
		ic->classdev.class = cont->class;
		cont->class->dev_release = attribute_container_release;
		dev_set_name(&ic->classdev, dev_name(dev));
		if (fn)
			fn(cont, dev, &ic->classdev);
		else
			attribute_container_add_class_device(&ic->classdev);
		klist_add_tail(&ic->node, &cont->containers);
	}
	mutex_unlock(&attribute_container_mutex);
}

#define klist_for_each_entry(pos, head, member, iter) \
	for (klist_iter_init(head, iter); (pos = ({ \
		struct klist_node *n = klist_next(iter); \
		n ? container_of(n, typeof(*pos), member) : \
			({ klist_iter_exit(iter) ; NULL; }); \
	}) ) != NULL; )
			

void
attribute_container_remove_device(struct device *dev,
				  void (*fn)(struct attribute_container *,
					     struct device *,
					     struct device *))
{
	struct attribute_container *cont;

	mutex_lock(&attribute_container_mutex);
	list_for_each_entry(cont, &attribute_container_list, node) {
		struct internal_container *ic;
		struct klist_iter iter;

		if (attribute_container_no_classdevs(cont))
			continue;

		if (!cont->match(cont, dev))
			continue;

		klist_for_each_entry(ic, &cont->containers, node, &iter) {
			if (dev != ic->classdev.parent)
				continue;
			klist_del(&ic->node);
			if (fn)
				fn(cont, dev, &ic->classdev);
			else {
				attribute_container_remove_attrs(&ic->classdev);
				device_unregister(&ic->classdev);
			}
		}
	}
	mutex_unlock(&attribute_container_mutex);
}

void
attribute_container_device_trigger(struct device *dev, 
				   int (*fn)(struct attribute_container *,
					     struct device *,
					     struct device *))
{
	struct attribute_container *cont;

	mutex_lock(&attribute_container_mutex);
	list_for_each_entry(cont, &attribute_container_list, node) {
		struct internal_container *ic;
		struct klist_iter iter;

		if (!cont->match(cont, dev))
			continue;

		if (attribute_container_no_classdevs(cont)) {
			fn(cont, dev, NULL);
			continue;
		}

		klist_for_each_entry(ic, &cont->containers, node, &iter) {
			if (dev == ic->classdev.parent)
				fn(cont, dev, &ic->classdev);
		}
	}
	mutex_unlock(&attribute_container_mutex);
}

void
attribute_container_trigger(struct device *dev,
			    int (*fn)(struct attribute_container *,
				      struct device *))
{
	struct attribute_container *cont;

	mutex_lock(&attribute_container_mutex);
	list_for_each_entry(cont, &attribute_container_list, node) {
		if (cont->match(cont, dev))
			fn(cont, dev);
	}
	mutex_unlock(&attribute_container_mutex);
}

int
attribute_container_add_attrs(struct device *classdev)
{
	struct attribute_container *cont =
		attribute_container_classdev_to_container(classdev);
	struct device_attribute **attrs = cont->attrs;
	int i, error;

	BUG_ON(attrs && cont->grp);

	if (!attrs && !cont->grp)
		return 0;

	if (cont->grp)
		return sysfs_create_group(&classdev->kobj, cont->grp);

	for (i = 0; attrs[i]; i++) {
		sysfs_attr_init(&attrs[i]->attr);
		error = device_create_file(classdev, attrs[i]);
		if (error)
			return error;
	}

	return 0;
}

int
attribute_container_add_class_device(struct device *classdev)
{
	int error = device_add(classdev);
	if (error)
		return error;
	return attribute_container_add_attrs(classdev);
}

int
attribute_container_add_class_device_adapter(struct attribute_container *cont,
					     struct device *dev,
					     struct device *classdev)
{
	return attribute_container_add_class_device(classdev);
}

void
attribute_container_remove_attrs(struct device *classdev)
{
	struct attribute_container *cont =
		attribute_container_classdev_to_container(classdev);
	struct device_attribute **attrs = cont->attrs;
	int i;

	if (!attrs && !cont->grp)
		return;

	if (cont->grp) {
		sysfs_remove_group(&classdev->kobj, cont->grp);
		return ;
	}

	for (i = 0; attrs[i]; i++)
		device_remove_file(classdev, attrs[i]);
}

void
attribute_container_class_device_del(struct device *classdev)
{
	attribute_container_remove_attrs(classdev);
	device_del(classdev);
}

struct device *
attribute_container_find_class_device(struct attribute_container *cont,
				      struct device *dev)
{
	struct device *cdev = NULL;
	struct internal_container *ic;
	struct klist_iter iter;

	klist_for_each_entry(ic, &cont->containers, node, &iter) {
		if (ic->classdev.parent == dev) {
			cdev = &ic->classdev;
			
			klist_iter_exit(&iter);
			break;
		}
	}

	return cdev;
}
EXPORT_SYMBOL_GPL(attribute_container_find_class_device);
