/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/export.h>
#include <linux/time.h>
#include <linux/sysfs.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/idr.h>

#include "kgsl.h"
#include "kgsl_log.h"
#include "kgsl_device.h"
#include "kgsl_sharedmem.h"
#include "kgsl_snapshot.h"


struct kgsl_snapshot_object {
	unsigned int gpuaddr;
	unsigned int ptbase;
	unsigned int size;
	unsigned int offset;
	int type;
	struct kgsl_mem_entry *entry;
	struct list_head node;
};

struct snapshot_obj_itr {
	void *buf;      
	int pos;        
	loff_t offset;  
	size_t remain;  
	size_t write;   /* Bytes written so far */
};

static void obj_itr_init(struct snapshot_obj_itr *itr, void *buf,
	loff_t offset, size_t remain)
{
	itr->buf = buf;
	itr->offset = offset;
	itr->remain = remain;
	itr->pos = 0;
	itr->write = 0;
}

static int obj_itr_out(struct snapshot_obj_itr *itr, void *src, int size)
{
	if (itr->remain == 0)
		return 0;

	if ((itr->pos + size) <= itr->offset)
		goto done;

	

	if (itr->offset > itr->pos) {
		src += (itr->offset - itr->pos);
		size -= (itr->offset - itr->pos);

		
		itr->pos = itr->offset;
	}

	if (size > itr->remain)
		size = itr->remain;

	memcpy(itr->buf, src, size);

	itr->buf += size;
	itr->write += size;
	itr->remain -= size;

done:
	itr->pos += size;
	return size;
}


static int snapshot_context_count(int id, void *ptr, void *data)
{
	int *count = data;
	*count = *count + 1;

	return 0;
}


static void *_ctxtptr;

static int snapshot_context_info(int id, void *ptr, void *data)
{
	struct kgsl_snapshot_linux_context *header = _ctxtptr;
	struct kgsl_context *context = ptr;
	struct kgsl_device *device;

	if (context)
		device = context->dev_priv->device;
	else
		device = (struct kgsl_device *)data;

	header->id = id;


	header->timestamp_queued = kgsl_readtimestamp(device, context,
						      KGSL_TIMESTAMP_QUEUED);
	header->timestamp_retired = kgsl_readtimestamp(device, context,
						       KGSL_TIMESTAMP_RETIRED);

	_ctxtptr += sizeof(struct kgsl_snapshot_linux_context);

	return 0;
}

static int snapshot_os(struct kgsl_device *device,
	void *snapshot, int remain, void *priv)
{
	struct kgsl_snapshot_linux *header = snapshot;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct task_struct *task;
	pid_t pid;
	int hang = (int) priv;
	int ctxtcount = 0;
	int size = sizeof(*header);


	read_lock(&device->context_lock);
	idr_for_each(&device->context_idr, snapshot_context_count, &ctxtcount);
	read_unlock(&device->context_lock);

	
	ctxtcount++;

	size += ctxtcount * sizeof(struct kgsl_snapshot_linux_context);

	
	if (remain < size) {
		SNAPSHOT_ERR_NOMEM(device, "OS");
		return 0;
	}

	memset(header, 0, sizeof(*header));

	header->osid = KGSL_SNAPSHOT_OS_LINUX;

	header->state = hang ? SNAPSHOT_STATE_HUNG : SNAPSHOT_STATE_RUNNING;

	
	strlcpy(header->release, utsname()->release, sizeof(header->release));
	strlcpy(header->version, utsname()->version, sizeof(header->version));

	
	header->seconds = get_seconds();

	
	header->power_flags = pwr->power_flags;
	header->power_level = pwr->active_pwrlevel;
	header->power_interval_timeout = pwr->interval_timeout;
	header->grpclk = kgsl_get_clkrate(pwr->grp_clks[0]);
	header->busclk = kgsl_get_clkrate(pwr->ebi1_clk);

	
	kgsl_sharedmem_readl(&device->memstore, &header->current_context,
		KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL, current_context));

	
	header->ptbase = kgsl_mmu_get_current_ptbase(&device->mmu);
	
	pid = header->pid = kgsl_mmu_get_ptname_from_ptbase(&device->mmu,
								header->ptbase);

	task = find_task_by_vpid(pid);

	if (task)
		get_task_comm(header->comm, task);

	header->ctxtcount = ctxtcount;

	_ctxtptr = snapshot + sizeof(*header);

	
	snapshot_context_info(KGSL_MEMSTORE_GLOBAL, NULL, device);

	

	read_lock(&device->context_lock);
	idr_for_each(&device->context_idr, snapshot_context_info, NULL);
	read_unlock(&device->context_lock);

	
	return size;
}

static int kgsl_snapshot_dump_indexed_regs(struct kgsl_device *device,
	void *snapshot, int remain, void *priv)
{
	struct kgsl_snapshot_indexed_registers *iregs = priv;
	struct kgsl_snapshot_indexed_regs *header = snapshot;
	unsigned int *data = snapshot + sizeof(*header);
	int i;

	if (remain < (iregs->count * 4) + sizeof(*header)) {
		SNAPSHOT_ERR_NOMEM(device, "INDEXED REGS");
		return 0;
	}

	header->index_reg = iregs->index;
	header->data_reg = iregs->data;
	header->count = iregs->count;
	header->start = iregs->start;

	for (i = 0; i < iregs->count; i++) {
		kgsl_regwrite(device, iregs->index, iregs->start + i);
		kgsl_regread(device, iregs->data, &data[i]);
	}

	return (iregs->count * 4) + sizeof(*header);
}

#define GPU_OBJ_HEADER_SZ \
	(sizeof(struct kgsl_snapshot_section_header) + \
	 sizeof(struct kgsl_snapshot_gpu_object))

static int kgsl_snapshot_dump_object(struct kgsl_device *device,
	struct kgsl_snapshot_object *obj, struct snapshot_obj_itr *itr)
{
	struct kgsl_snapshot_section_header sect;
	struct kgsl_snapshot_gpu_object header;
	int ret;

	sect.magic = SNAPSHOT_SECTION_MAGIC;
	sect.id = KGSL_SNAPSHOT_SECTION_GPU_OBJECT;


	sect.size = GPU_OBJ_HEADER_SZ + ALIGN(obj->size, 4);

	ret = obj_itr_out(itr, &sect, sizeof(sect));
	if (ret == 0)
		return 0;

	header.size = ALIGN(obj->size, 4) >> 2;
	header.gpuaddr = obj->gpuaddr;
	header.ptbase = obj->ptbase;
	header.type = obj->type;

	ret = obj_itr_out(itr, &header, sizeof(header));
	if (ret == 0)
		return 0;

	ret = obj_itr_out(itr, obj->entry->memdesc.hostptr + obj->offset,
		obj->size);
	if (ret == 0)
		return 0;

	

	if (obj->size % 4) {
		unsigned int dummy = 0;
		ret = obj_itr_out(itr, &dummy, obj->size % 4);
	}

	return ret;
}

static void kgsl_snapshot_put_object(struct kgsl_device *device,
	struct kgsl_snapshot_object *obj)
{
	list_del(&obj->node);

	obj->entry->memdesc.priv &= ~KGSL_MEMDESC_FROZEN;
	kgsl_mem_entry_put(obj->entry);

	kfree(obj);
}

int kgsl_snapshot_have_object(struct kgsl_device *device, unsigned int ptbase,
	unsigned int gpuaddr, unsigned int size)
{
	struct kgsl_snapshot_object *obj;

	list_for_each_entry(obj, &device->snapshot_obj_list, node) {
		if (obj->ptbase != ptbase)
			continue;

		if ((gpuaddr >= obj->gpuaddr) &&
			((gpuaddr + size) <= (obj->gpuaddr + obj->size)))
			return 1;
	}

	return 0;
}
EXPORT_SYMBOL(kgsl_snapshot_have_object);


int kgsl_snapshot_get_object(struct kgsl_device *device, unsigned int ptbase,
	unsigned int gpuaddr, unsigned int size, unsigned int type)
{
	struct kgsl_mem_entry *entry;
	struct kgsl_snapshot_object *obj;
	int offset;
	int ret = -EINVAL;

	if (!gpuaddr)
		return 0;

	entry = kgsl_get_mem_entry(device, ptbase, gpuaddr, size);

	if (entry == NULL) {
		KGSL_DRV_ERR(device, "Unable to find GPU buffer %8.8X\n",
				gpuaddr);
		return -EINVAL;
	}

	
	if (entry->memtype != KGSL_MEM_ENTRY_KERNEL) {
		KGSL_DRV_ERR(device,
			"Only internal GPU buffers can be frozen\n");
		goto err_put;
	}


	if (size == 0) {
		size = entry->memdesc.size;
		offset = 0;

		
		gpuaddr = entry->memdesc.gpuaddr;
	} else {
		offset = gpuaddr - entry->memdesc.gpuaddr;
	}

	if (size + offset > entry->memdesc.size) {
		KGSL_DRV_ERR(device, "Invalid size for GPU buffer %8.8X\n",
				gpuaddr);
		goto err_put;
	}

	
	list_for_each_entry(obj, &device->snapshot_obj_list, node) {
		if (obj->gpuaddr == gpuaddr && obj->ptbase == ptbase) {
			
			if (obj->size < size)
				obj->size = size;
			ret = 0;
			goto err_put;
		}
	}

	if (kgsl_memdesc_map(&entry->memdesc) == NULL) {
		KGSL_DRV_ERR(device, "Unable to map GPU buffer %X\n",
				gpuaddr);
		goto err_put;
	}

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);

	if (obj == NULL) {
		KGSL_DRV_ERR(device, "Unable to allocate memory\n");
		goto err_put;
	}

	obj->type = type;
	obj->entry = entry;
	obj->gpuaddr = gpuaddr;
	obj->ptbase = ptbase;
	obj->size = size;
	obj->offset = offset;

	list_add(&obj->node, &device->snapshot_obj_list);


	ret = (entry->memdesc.priv & KGSL_MEMDESC_FROZEN) ? 0
		: entry->memdesc.size;

	entry->memdesc.priv |= KGSL_MEMDESC_FROZEN;

	return ret;
err_put:
	kgsl_mem_entry_put(entry);
	return ret;
}
EXPORT_SYMBOL(kgsl_snapshot_get_object);

int kgsl_snapshot_dump_regs(struct kgsl_device *device, void *snapshot,
	int remain, void *priv)
{
	struct kgsl_snapshot_registers_list *list = priv;

	struct kgsl_snapshot_regs *header = snapshot;
	struct kgsl_snapshot_registers *regs;
	unsigned int *data = snapshot + sizeof(*header);
	int count = 0, i, j, k;

	

	for (i = 0; i < list->count; i++) {
		regs = &(list->registers[i]);

		for (j = 0; j < regs->count; j++) {
			int start = regs->regs[j * 2];
			int end = regs->regs[j * 2 + 1];

			count += (end - start + 1);
		}
	}

	if (remain < (count * 8) + sizeof(*header)) {
		SNAPSHOT_ERR_NOMEM(device, "REGISTERS");
		return 0;
	}


	for (i = 0; i < list->count; i++) {
		regs = &(list->registers[i]);
		for (j = 0; j < regs->count; j++) {
			unsigned int start = regs->regs[j * 2];
			unsigned int end = regs->regs[j * 2 + 1];

			for (k = start; k <= end; k++) {
				unsigned int val;

				kgsl_regread(device, k, &val);
				*data++ = k;
				*data++ = val;
			}
		}
	}

	header->count = count;

	
	return (count * 8) + sizeof(*header);
}
EXPORT_SYMBOL(kgsl_snapshot_dump_regs);

void *kgsl_snapshot_indexed_registers(struct kgsl_device *device,
		void *snapshot, int *remain,
		unsigned int index, unsigned int data, unsigned int start,
		unsigned int count)
{
	struct kgsl_snapshot_indexed_registers iregs;
	iregs.index = index;
	iregs.data = data;
	iregs.start = start;
	iregs.count = count;

	return kgsl_snapshot_add_section(device,
		 KGSL_SNAPSHOT_SECTION_INDEXED_REGS, snapshot,
		 remain, kgsl_snapshot_dump_indexed_regs, &iregs);
}
EXPORT_SYMBOL(kgsl_snapshot_indexed_registers);

int kgsl_device_snapshot(struct kgsl_device *device, int hang)
{
	struct kgsl_snapshot_header *header = device->snapshot;
	int remain = device->snapshot_maxsize - sizeof(*header);
	void *snapshot;
	struct timespec boot;
	int ret = 0;

	if (kgsl_active_count_get(device)) {
		KGSL_DRV_ERR(device, "Failed to get GPU active count");
		return -EINVAL;
	}


	if (hang && device->snapshot_frozen == 1) {
		ret = 0;
		goto done;
	}

	if (device->snapshot == NULL) {
		KGSL_DRV_ERR(device,
			"snapshot: No snapshot memory available\n");
		ret = -ENOMEM;
		goto done;
	}

	if (remain < sizeof(*header)) {
		KGSL_DRV_ERR(device,
			"snapshot: Not enough memory for the header\n");
		ret = -ENOMEM;
		goto done;
	}

	header->magic = SNAPSHOT_MAGIC;

	header->gpuid = kgsl_gpuid(device, &header->chipid);

	
	snapshot = ((void *) device->snapshot) + sizeof(*header);

	
	snapshot = kgsl_snapshot_add_section(device, KGSL_SNAPSHOT_SECTION_OS,
		snapshot, &remain, snapshot_os, (void *) hang);

	
	if (device->ftbl->snapshot)
		snapshot = device->ftbl->snapshot(device, snapshot, &remain,
			hang);


	getboottime(&boot);
	device->snapshot_timestamp = get_seconds() - boot.tv_sec;
	device->snapshot_size = (int) (snapshot - device->snapshot);

	
	device->snapshot_frozen = (hang) ? 1 : 0;

	
	KGSL_DRV_ERR(device, "snapshot created at pa %lx size %d\n",
			__pa(device->snapshot),	device->snapshot_size);
	if (hang)
		sysfs_notify(&device->snapshot_kobj, NULL, "timestamp");

done:
	kgsl_active_count_put(device);
	return ret;
}
EXPORT_SYMBOL(kgsl_device_snapshot);

struct kgsl_snapshot_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kgsl_device *device, char *buf);
	ssize_t (*store)(struct kgsl_device *device, const char *buf,
		size_t count);
};

#define to_snapshot_attr(a) \
container_of(a, struct kgsl_snapshot_attribute, attr)

#define kobj_to_device(a) \
container_of(a, struct kgsl_device, snapshot_kobj)

static ssize_t snapshot_show(struct file *filep, struct kobject *kobj,
	struct bin_attribute *attr, char *buf, loff_t off,
	size_t count)
{
	struct kgsl_device *device = kobj_to_device(kobj);
	struct kgsl_snapshot_object *obj, *tmp;
	struct kgsl_snapshot_section_header head;
	struct snapshot_obj_itr itr;
	int ret;

	if (device == NULL)
		return 0;

	
	if (device->snapshot_timestamp == 0)
		return 0;

	
	mutex_lock(&device->mutex);

	obj_itr_init(&itr, buf, off, count);

	ret = obj_itr_out(&itr, device->snapshot, device->snapshot_size);

	if (ret == 0)
		goto done;

	list_for_each_entry(obj, &device->snapshot_obj_list, node)
		kgsl_snapshot_dump_object(device, obj, &itr);

	{
		head.magic = SNAPSHOT_SECTION_MAGIC;
		head.id = KGSL_SNAPSHOT_SECTION_END;
		head.size = sizeof(head);

		obj_itr_out(&itr, &head, sizeof(head));
	}

	/*
	 * Make sure everything has been written out before destroying things.
	 * The best way to confirm this is to go all the way through without
	 * writing any bytes - so only release if we get this far and
	 * itr->write is 0
	 */

	if (itr.write == 0) {
		list_for_each_entry_safe(obj, tmp, &device->snapshot_obj_list,
			node)
			kgsl_snapshot_put_object(device, obj);

		if (device->snapshot_frozen)
			KGSL_DRV_ERR(device, "Snapshot objects released\n");

		device->snapshot_frozen = 0;
	}

done:
	mutex_unlock(&device->mutex);

	return itr.write;
}

static ssize_t timestamp_show(struct kgsl_device *device, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", device->snapshot_timestamp);
}

static ssize_t trigger_store(struct kgsl_device *device, const char *buf,
	size_t count)
{
	if (device && count > 0) {
		mutex_lock(&device->mutex);
		kgsl_device_snapshot(device, 0);
		mutex_unlock(&device->mutex);
	}

	return count;
}

static ssize_t no_panic_show(struct kgsl_device *device, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%x\n", device->snapshot_no_panic);
}

static ssize_t no_panic_store(struct kgsl_device *device, const char *buf,
    size_t count)
{
    if (device && count > 0) {
        mutex_lock(&device->mutex);
        device->snapshot_no_panic = simple_strtol(buf, NULL, 10);
        mutex_unlock(&device->mutex);
    }

    return count;
}


static struct bin_attribute snapshot_attr = {
	.attr.name = "dump",
	.attr.mode = 0444,
	.size = 0,
	.read = snapshot_show
};

#define SNAPSHOT_ATTR(_name, _mode, _show, _store) \
struct kgsl_snapshot_attribute attr_##_name = { \
	.attr = { .name = __stringify(_name), .mode = _mode }, \
	.show = _show, \
	.store = _store, \
}

SNAPSHOT_ATTR(trigger, 0600, NULL, trigger_store);
SNAPSHOT_ATTR(timestamp, 0444, timestamp_show, NULL);
SNAPSHOT_ATTR(no_panic, 0644, no_panic_show, no_panic_store);

static void snapshot_sysfs_release(struct kobject *kobj)
{
}

static ssize_t snapshot_sysfs_show(struct kobject *kobj,
	struct attribute *attr, char *buf)
{
	struct kgsl_snapshot_attribute *pattr = to_snapshot_attr(attr);
	struct kgsl_device *device = kobj_to_device(kobj);
	ssize_t ret;

	if (device && pattr->show)
		ret = pattr->show(device, buf);
	else
		ret = -EIO;

	return ret;
}

static ssize_t snapshot_sysfs_store(struct kobject *kobj,
	struct attribute *attr, const char *buf, size_t count)
{
	struct kgsl_snapshot_attribute *pattr = to_snapshot_attr(attr);
	struct kgsl_device *device = kobj_to_device(kobj);
	ssize_t ret;

	if (device && pattr->store)
		ret = pattr->store(device, buf, count);
	else
		ret = -EIO;

	return ret;
}

static const struct sysfs_ops snapshot_sysfs_ops = {
	.show = snapshot_sysfs_show,
	.store = snapshot_sysfs_store,
};

static struct kobj_type ktype_snapshot = {
	.sysfs_ops = &snapshot_sysfs_ops,
	.default_attrs = NULL,
	.release = snapshot_sysfs_release,
};


int kgsl_device_snapshot_init(struct kgsl_device *device)
{
	int ret;

	if (device->snapshot == NULL)
		device->snapshot = kzalloc(KGSL_SNAPSHOT_MEMSIZE, GFP_KERNEL);

	if (device->snapshot == NULL)
		return -ENOMEM;

	device->snapshot_maxsize = KGSL_SNAPSHOT_MEMSIZE;
	device->snapshot_timestamp = 0;

	INIT_LIST_HEAD(&device->snapshot_obj_list);

	ret = kobject_init_and_add(&device->snapshot_kobj, &ktype_snapshot,
		&device->dev->kobj, "snapshot");
	if (ret)
		goto done;

	ret = sysfs_create_bin_file(&device->snapshot_kobj, &snapshot_attr);
	if (ret)
		goto done;

	ret  = sysfs_create_file(&device->snapshot_kobj, &attr_trigger.attr);
	if (ret)
		goto done;

	ret  = sysfs_create_file(&device->snapshot_kobj, &attr_timestamp.attr);
	if (ret)
		goto done;

	ret  = sysfs_create_file(&device->snapshot_kobj, &attr_no_panic.attr);

done:
	return ret;
}
EXPORT_SYMBOL(kgsl_device_snapshot_init);


void kgsl_device_snapshot_close(struct kgsl_device *device)
{
	sysfs_remove_bin_file(&device->snapshot_kobj, &snapshot_attr);
	sysfs_remove_file(&device->snapshot_kobj, &attr_trigger.attr);
	sysfs_remove_file(&device->snapshot_kobj, &attr_timestamp.attr);
	sysfs_remove_file(&device->snapshot_kobj, &attr_no_panic.attr);

	kobject_put(&device->snapshot_kobj);

	kfree(device->snapshot);

	device->snapshot = NULL;
	device->snapshot_maxsize = 0;
	device->snapshot_timestamp = 0;
}
EXPORT_SYMBOL(kgsl_device_snapshot_close);
