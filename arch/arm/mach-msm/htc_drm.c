/* arch/arm/mach-msm/htc_drm-8x60.c
 *
 * Copyright (C) 2011 HTC Corporation.
 * Author: Eddic Hsien <eddic_hsien@htc.com>
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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#ifndef CONFIG_ARCH_MSM7X30
#include <mach/scm.h>
#else	/* CONFIG_ARCH_MSM7X30 */
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/semaphore.h>
#include <mach/msm_rpcrouter.h>
#include <mach/oem_rapi_client.h>

#define OEM_RAPI_PROG  0x3000006B
#define OEM_RAPI_VERS  0x00010001

#define OEM_RAPI_NULL_PROC                        0
#define OEM_RAPI_RPC_GLUE_CODE_INFO_REMOTE_PROC   1
#define OEM_RAPI_STREAMING_FUNCTION_PROC          2

#define OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE 128
#endif	/* CONFIG_ARCH_MSM7X30 */

#define DEVICE_NAME "htcdrm"

#define HTCDRM_IOCTL_WIDEVINE	0x2563
#define HTCDRM_IOCTL_DISCRETIX	0x2596
#define HTCDRM_IOCTL_CPRM   	0x2564

#define DEVICE_ID_LEN			32
#define WIDEVINE_KEYBOX_LEN		128
#define CPRM_KEY_LEN			188

#define HTC_DRM_DEBUG	0
#undef PDEBUG
#if HTC_DRM_DEBUG
#define PDEBUG(fmt, args...) printk(KERN_INFO "%s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)
#else
#define PDEBUG(fmt, args...) do {} while (0)
#endif

#undef PERR
#define PERR(fmt, args...) printk(KERN_ERR "%s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

#ifndef CONFIG_ARCH_MSM7X30
#define UP(S)
#else
#define UP(S) up(S)
#endif
static int htcdrm_major;
static struct class *htcdrm_class;
static const struct file_operations htcdrm_fops;

typedef struct _htc_drm_msg_s {
	int func;
	int offset;
	unsigned char *req_buf;
	int req_len;
	unsigned char *resp_buf;
	int resp_len;
} htc_drm_msg_s;

enum {
		HTC_OEMCRYPTO_STORE_KEYBOX = 1,
		HTC_OEMCRYPTO_GET_KEYBOX,
		HTC_OEMCRYPTO_IDENTIFY_DEVICE,
		HTC_OEMCRYPTO_GET_RANDOM,
		HTC_OEMCRYPTO_IS_KEYBOX_VALID,
		HTC_OEMCRYPTO_SET_ENTITLEMENT_KEY,
		HTC_OEMCRYPTO_DERIVE_CONTROL_WORD,
};


static unsigned char *htc_device_id;
static unsigned char *htc_keybox;

typedef struct _htc_drm_dix_msg_s {
	int func;
	int ret;
	unsigned char *buf;
	int buf_len;
	int flags;
} htc_drm_dix_msg_s;

typedef union {
	struct {
		void *buffer;
		int size;
	} memref;
	struct {
		unsigned int a;
		unsigned int b;
	} value;
} TEE_Param;

#define	TEE_PARAM_TYPE_NONE 			0
#define	TEE_PARAM_TYPE_VALUE_INPUT		1
#define	TEE_PARAM_TYPE_VALUE_OUTPUT		2
#define	TEE_PARAM_TYPE_VALUE_INOUT		3
#define	TEE_PARAM_TYPE_MEMREF_INPUT		5
#define	TEE_PARAM_TYPE_MEMREF_OUTPUT	6
#define	TEE_PARAM_TYPE_MEMREF_INOUT		7
#define TEE_PARAM_TYPE_GET(t, i) (((t) >> (i*4)) & 0xF)

enum {
    TEE_FUNC_TA_OpenSession = 1,
    TEE_FUNC_TA_CloseSession,
    TEE_FUNC_TA_InvokeCommand,
	TEE_FUNC_TA_CreateEntryPoint,
	TEE_FUNC_TA_DestroyEntryPoint,
	TEE_FUNC_SECURE_STORAGE_INIT,
	TEE_FUNC_SECURE_STORAGE_SYNC,
	TEE_FUNC_HTC_HEAP_INIT,
	TEE_FUNC_HTC_SMEM_INIT,
};

struct CMD_TA_OpenSession {
	unsigned int paramTypes;
	TEE_Param params[4];
	void **sessionContext;
};

struct CMD_TA_CloseSession {
    void *sessionContext;
};

struct CMD_TA_InvokeCommand {
	void *sessionContext;
	unsigned int commandID;
	unsigned int paramTypes;
	TEE_Param params[4];
};

struct CMD_SECURE_STORAGE {
	unsigned char *image_base;
	unsigned int image_size;
};

static int secure_storage_init;

#define DIS_ALLOC_TZ_HEAP 0

#if DIS_ALLOC_TZ_HEAP
#define DISCRETIX_HEAP_SIZE	(512 * 1024)
#else
#define DISCRETIX_HEAP_SIZE	0
#endif

static unsigned char *discretix_tz_heap;

#ifdef CONFIG_ARCH_MSM7X30
struct htc_keybox_dev {
	struct platform_device *pdev;
	struct cdev cdev;
	struct device *device;
	struct class *class;
	dev_t dev_num;
	unsigned char keybox_buf[OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE];
	struct semaphore sem;
} htc_keybox_dev;



static struct msm_rpc_client *rpc_client;
static uint32_t open_count;
static DEFINE_MUTEX(oem_rapi_client_lock);
/* TODO: check where to allocate memory for return */
static int oem_rapi_client_cb(struct msm_rpc_client *client,
			      struct rpc_request_hdr *req,
			      struct msm_rpc_xdr *xdr)
{
	uint32_t cb_id, accept_status;
	int rc;
	void *cb_func;
	uint32_t temp;

	struct oem_rapi_client_streaming_func_cb_arg arg;
	struct oem_rapi_client_streaming_func_cb_ret ret;

	arg.input = NULL;
	ret.out_len = NULL;
	ret.output = NULL;

	xdr_recv_uint32(xdr, &cb_id);                    /* cb_id */
	xdr_recv_uint32(xdr, &arg.event);                /* enum */
	xdr_recv_uint32(xdr, (uint32_t *)(&arg.handle)); /* handle */
	xdr_recv_uint32(xdr, &arg.in_len);               /* in_len */
	xdr_recv_bytes(xdr, (void **)&arg.input, &temp); /* input */
	xdr_recv_uint32(xdr, &arg.out_len_valid);        /* out_len */
	if (arg.out_len_valid) {
		ret.out_len = kmalloc(sizeof(*ret.out_len), GFP_KERNEL);
		if (!ret.out_len) {
			accept_status = RPC_ACCEPTSTAT_SYSTEM_ERR;
			goto oem_rapi_send_ack;
		}
	}

	xdr_recv_uint32(xdr, &arg.output_valid);         /* out */
	if (arg.output_valid) {
		xdr_recv_uint32(xdr, &arg.output_size);  /* ouput_size */

		ret.output = kmalloc(arg.output_size, GFP_KERNEL);
		if (!ret.output) {
			accept_status = RPC_ACCEPTSTAT_SYSTEM_ERR;
			goto oem_rapi_send_ack;
		}
	}

	cb_func = msm_rpc_get_cb_func(client, cb_id);
	if (cb_func) {
		rc = ((int (*)(struct oem_rapi_client_streaming_func_cb_arg *,
			       struct oem_rapi_client_streaming_func_cb_ret *))
		      cb_func)(&arg, &ret);
		if (rc)
			accept_status = RPC_ACCEPTSTAT_SYSTEM_ERR;
		else
			accept_status = RPC_ACCEPTSTAT_SUCCESS;
	} else
		accept_status = RPC_ACCEPTSTAT_SYSTEM_ERR;

 oem_rapi_send_ack:
	xdr_start_accepted_reply(xdr, accept_status);

	if (accept_status == RPC_ACCEPTSTAT_SUCCESS) {
		uint32_t temp = sizeof(uint32_t);
		xdr_send_pointer(xdr, (void **)&(ret.out_len), temp,
				 xdr_send_uint32);

		/* output */
		if (ret.output && ret.out_len)
			xdr_send_bytes(xdr, (const void **)&ret.output,
					     ret.out_len);
		else {
			temp = 0;
			xdr_send_uint32(xdr, &temp);
		}
	}
	rc = xdr_send_msg(xdr);
	if (rc)
		pr_err("%s: sending reply failed: %d\n", __func__, rc);

	kfree(arg.input);
	kfree(ret.out_len);
	kfree(ret.output);

	return 0;
}

static int oem_rapi_client_streaming_function_arg(struct msm_rpc_client *client,
						  struct msm_rpc_xdr *xdr,
						  void *data)
{
	int cb_id;
	struct oem_rapi_client_streaming_func_arg *arg = data;

	cb_id = msm_rpc_add_cb_func(client, (void *)arg->cb_func);
	if ((cb_id < 0) && (cb_id != MSM_RPC_CLIENT_NULL_CB_ID))
		return cb_id;

	xdr_send_uint32(xdr, &arg->event);                /* enum */
	xdr_send_uint32(xdr, &cb_id);                     /* cb_id */
	xdr_send_uint32(xdr, (uint32_t *)(&arg->handle)); /* handle */
	xdr_send_uint32(xdr, &arg->in_len);               /* in_len */
	xdr_send_bytes(xdr, (const void **)&arg->input,
			     &arg->in_len);                     /* input */
	xdr_send_uint32(xdr, &arg->out_len_valid);        /* out_len */
	xdr_send_uint32(xdr, &arg->output_valid);         /* output */

	/* output_size */
	if (arg->output_valid)
		xdr_send_uint32(xdr, &arg->output_size);

	return 0;
}

static int oem_rapi_client_streaming_function_ret(struct msm_rpc_client *client,
						  struct msm_rpc_xdr *xdr,
						  void *data)
{
	struct oem_rapi_client_streaming_func_ret *ret = data;
	uint32_t temp;

	/* out_len */
	xdr_recv_pointer(xdr, (void **)&(ret->out_len), sizeof(uint32_t),
			 xdr_recv_uint32);

	/* output */
	if (ret->out_len && *ret->out_len)
		xdr_recv_bytes(xdr, (void **)&ret->output, &temp);

	return 0;
}

int oem_rapi_client_streaming_function(
	struct msm_rpc_client *client,
	struct oem_rapi_client_streaming_func_arg *arg,
	struct oem_rapi_client_streaming_func_ret *ret)
{
	return msm_rpc_client_req2(client,
				   OEM_RAPI_STREAMING_FUNCTION_PROC,
				   oem_rapi_client_streaming_function_arg, arg,
				   oem_rapi_client_streaming_function_ret,
				   ret, -1);
}
EXPORT_SYMBOL(oem_rapi_client_streaming_function);

int oem_rapi_client_close(void)
{
	mutex_lock(&oem_rapi_client_lock);
	if (--open_count == 0) {
		msm_rpc_unregister_client(rpc_client);
		pr_info("%s: disconnected from remote oem rapi server\n",
			__func__);
	}
	mutex_unlock(&oem_rapi_client_lock);
	return 0;
}
EXPORT_SYMBOL(oem_rapi_client_close);

struct msm_rpc_client *oem_rapi_client_init(void)
{
	mutex_lock(&oem_rapi_client_lock);
	if (open_count == 0) {
		rpc_client = msm_rpc_register_client2("oemrapiclient",
						      OEM_RAPI_PROG,
						      OEM_RAPI_VERS, 0,
						      oem_rapi_client_cb);
		if (!IS_ERR(rpc_client))
			open_count++;
	}
	mutex_unlock(&oem_rapi_client_lock);
	return rpc_client;
}
EXPORT_SYMBOL(oem_rapi_client_init);

static ssize_t htc_keybox_read(struct htc_keybox_dev *dev, char *buf, size_t size, loff_t p)
{
	unsigned int count = size;
	int ret_rpc = 0;
	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;
	unsigned char nullbuf[OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE];

	memset(dev->keybox_buf, 56, OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE);
	memset(nullbuf, 0, OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE);

	printk(KERN_INFO "htc_keybox_read start:\n");
	if (p >= OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE)
		return count ? -ENXIO : 0;

	printk(KERN_INFO "htc_keybox_read oem_rapi_client_streaming_function start:\n");
	if (count == 0xFF) {
		arg.event = OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_DEVICE_ID;
		memset(dev->keybox_buf, 57, OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE);
		printk(KERN_INFO "htc_keybox_read: OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_DEVICE_ID\n");
	} else if (count > OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE - p) {
		count = OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE - p;
		arg.event = OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_KEYBOX;
		printk(KERN_INFO "htc_keybox_read: OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_KEYBOX\n");
	} else {
		arg.event = OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_KEYBOX;
		printk(KERN_INFO "htc_keybox_read: OEM_RAPI_CLIENT_EVENT_WIDEVINE_READ_KEYBOX\n");
	}
	arg.cb_func = NULL;
	arg.handle = (void *)0;
	arg.in_len = 0;
	arg.input = (char *)nullbuf;
	arg.out_len_valid = 1;
	arg.output_valid = 1;
	arg.output_size = OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE;
	ret.out_len = NULL;
	ret.output = NULL;

	ret_rpc = oem_rapi_client_streaming_function(rpc_client, &arg, &ret);
	if (ret_rpc) {
		printk(KERN_INFO "%s: Get data from modem failed: %d\n", __func__, ret_rpc);
		return -EFAULT;
	}
	printk(KERN_INFO "%s: Data obtained from modem %d, ", __func__, *(ret.out_len));
	memcpy(dev->keybox_buf, ret.output, *(ret.out_len));
	kfree(ret.out_len);
	kfree(ret.output);

	return 0;
}

static ssize_t htc_keybox_write(struct htc_keybox_dev *dev, const char *buf, size_t size, loff_t p)
{
	unsigned int count = size;
	int ret_rpc = 0;
	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;

	printk(KERN_INFO "htc_keybox_write start:\n");
	if (p >= OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE)
		return count ? -ENXIO : 0;
	if (count > OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE - p)
		count = OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE - p;
	printk(KERN_INFO "htc_keybox_write oem_rapi_client_streaming_function start:\n");
	arg.event = OEM_RAPI_CLIENT_EVENT_WIDEVINE_WRITE_KEYBOX;
	arg.cb_func = NULL;
	arg.handle = (void *)0;
	arg.in_len = OEM_RAPI_CLIENT_MAX_OUT_BUFF_SIZE;
	arg.input = (char *)dev->keybox_buf;
	arg.out_len_valid = 0;
	arg.output_valid = 0;
	arg.output_size = 0;
	ret.out_len = NULL;
	ret.output = NULL;

	ret_rpc = oem_rapi_client_streaming_function(rpc_client, &arg, &ret);
	if (ret_rpc) {
		printk(KERN_INFO "%s: Send data from modem failed: %d\n", __func__, ret_rpc);
		return -EFAULT;
	}
	printk(KERN_INFO "%s: Data sent to modem %s\n", __func__, dev->keybox_buf);

	return 0;
}
static struct htc_keybox_dev *keybox_dev;
#endif /* CONFIG_ARCH_MSM7X30 */

static unsigned char *htc_device_id;
static unsigned char *htc_keybox;

#define DX_SMEM_SIZE	PAGE_SIZE

static int discretix_smem_size = DX_SMEM_SIZE;
static unsigned char *discretix_smem_ptr;
static unsigned char *discretix_smem_phy;
static unsigned char *discretix_smem_area;

static DEFINE_MUTEX(dx_lock);
/* static htc_drm_dix_msg_s hdix; */

void scm_inv_range(unsigned long start, unsigned long end);

static long htcdrm_discretix_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	htc_drm_dix_msg_s hdix;
	int ret = 0, i;
	unsigned char *ptr, *data, *image, *image_u;
	unsigned char *kbuf[4], *ubuf[4];
	unsigned int sessionContext;
	unsigned long start, end;

	image_u = NULL;
	image = NULL;

	if (copy_from_user(&hdix, (void __user *)arg, sizeof(hdix))) {
		PERR("copy_from_user error (msg)");
		return -EFAULT;
	}

	PDEBUG("htcdrm_discretix_ioctl func: %x", hdix.func);

	/* check function buffer size */
	switch (hdix.func) {
	case TEE_FUNC_TA_OpenSession:
		if (hdix.buf_len != sizeof(struct CMD_TA_OpenSession)) {
			PERR("OpenSession size error");
			return -EFAULT;
		}
		break;
	case TEE_FUNC_TA_InvokeCommand:
		if (hdix.buf_len != sizeof(struct CMD_TA_InvokeCommand)) {
			PERR("InvokeCommand size error");
			return -EFAULT;
		}
		break;
	case TEE_FUNC_TA_CloseSession:
		if (hdix.buf_len != sizeof(struct CMD_TA_CloseSession)) {
			PERR("CloseSession size error");
			return -EFAULT;
		}
		break;
	case TEE_FUNC_TA_CreateEntryPoint:
		if (hdix.buf_len != 0) {
			PERR("CreateEntryPoint size error");
			return -EFAULT;
		}
		break;
	case TEE_FUNC_TA_DestroyEntryPoint:
		if (hdix.buf_len != 0) {
			PERR("DestroyEntryPoint size error");
			return -EFAULT;
		}
		break;
	case TEE_FUNC_SECURE_STORAGE_INIT:
		if (secure_storage_init)
			return 0;

	case TEE_FUNC_HTC_HEAP_INIT:
	case TEE_FUNC_HTC_SMEM_INIT:
	case TEE_FUNC_SECURE_STORAGE_SYNC:
		if (hdix.buf_len != sizeof(struct CMD_SECURE_STORAGE)) {
			PERR("SECURE_STORAGE size error");
			return -EFAULT;
		}
		break;
	default:
		PERR("func: %d error\n", hdix.func);
		return -EFAULT;
	}

	if (hdix.buf_len != 0) {
		ptr = kzalloc(hdix.buf_len, GFP_KERNEL);
		if (ptr == NULL) {
			PERR("allocate the space for data failed (%d)", hdix.buf_len);
			return -EFAULT;
		}
		if (copy_from_user(ptr, (void __user *)hdix.buf, hdix.buf_len)) {
			PERR("copy_from_user error (data)");
			kfree(ptr);
			return -EFAULT;
		}

		data = hdix.buf;
		hdix.buf = (unsigned char *)virt_to_phys(ptr);
		/*
			ptr : kernel
			data: user space
			hdix.buf: virtual address of kernel
		*/
	} else {
		data = NULL;
		ptr = NULL;
	}
	for (i = 0; i < 4; i++)
		kbuf[i] = NULL;

	PDEBUG("func = %x", hdix.func);
	switch (hdix.func) {
	case TEE_FUNC_HTC_HEAP_INIT:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			s->image_base = (unsigned char *)virt_to_phys(discretix_tz_heap);
			s->image_size = DISCRETIX_HEAP_SIZE;
		}
		break;
	case TEE_FUNC_HTC_SMEM_INIT:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			s->image_base = (unsigned char *)virt_to_phys(discretix_smem_area);
			s->image_size = discretix_smem_size;
		}
		break;
	case TEE_FUNC_SECURE_STORAGE_INIT:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			image = kzalloc(s->image_size, GFP_KERNEL);
			if (image == NULL) {
				PERR("allocate the space for fat8 image failed");
				return -1;
			}
			if (copy_from_user(image, (void __user *)s->image_base, s->image_size)) {
				PERR("copy_from_user error (image)");
				kfree(image);
				return -EFAULT;
			}
			s->image_base = (unsigned char *)virt_to_phys(image);
		}
		break;
	case TEE_FUNC_SECURE_STORAGE_SYNC:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			image = kzalloc(s->image_size, GFP_KERNEL);
			if (image == NULL) {
				PERR("allocate the space for fat8 image failed");
				return -1;
			}
			image_u = s->image_base;
			s->image_base = (unsigned char *)virt_to_phys(image);
		}
		break;
	case TEE_FUNC_TA_OpenSession:
		{
			struct CMD_TA_OpenSession *s;

			s = (struct CMD_TA_OpenSession *)ptr;
			sessionContext = 0;
			s->sessionContext = (void *)virt_to_phys(&sessionContext);
			for (i = 0; i < 4; i++) {
				int ptype;

				ubuf[i] = s->params[i].memref.buffer;
				ptype = TEE_PARAM_TYPE_GET(s->paramTypes, i);
				switch (ptype) {
				case TEE_PARAM_TYPE_MEMREF_INPUT:
				case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				case TEE_PARAM_TYPE_MEMREF_INOUT:
					if (s->params[i].memref.size != 0) {
						kbuf[i] = kzalloc(s->params[i].memref.size, GFP_KERNEL);
						if (kbuf[i] == NULL) {
							PERR("allocate the space for buffer failed (%d)", s->params[i].memref.size);
							ret = -EFAULT;
							goto discretix_error_exit;
						}
						s->params[i].memref.buffer = (unsigned char *)virt_to_phys(kbuf[i]);
					} else
						kbuf[i] = NULL;
					if ((ptype == TEE_PARAM_TYPE_MEMREF_INPUT) ||
						(ptype == TEE_PARAM_TYPE_MEMREF_INOUT)) {
						if (copy_from_user(kbuf[i], (void __user *)ubuf[i], s->params[i].memref.size)) {
							PERR("copy_from_user error (buffer)");
							ret = -EFAULT;
							goto discretix_error_exit;
						}
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	case TEE_FUNC_TA_InvokeCommand:
		{
			struct CMD_TA_InvokeCommand *s;

			s = (struct CMD_TA_InvokeCommand *)ptr;
			for (i = 0; i < 4; i++) {
				int ptype;

				ubuf[i] = s->params[i].memref.buffer;
				ptype = TEE_PARAM_TYPE_GET(s->paramTypes, i);
				switch (ptype) {
				case TEE_PARAM_TYPE_MEMREF_INPUT:
				case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				case TEE_PARAM_TYPE_MEMREF_INOUT:
					if (s->params[i].memref.size != 0) {
						kbuf[i] = kzalloc(s->params[i].memref.size, GFP_KERNEL);
						if (kbuf[i] == NULL) {
							PERR("allocate the space for buffer failed (%d)", s->params[i].memref.size);
							ret = -EFAULT;
							goto discretix_error_exit;
						}
						s->params[i].memref.buffer = (unsigned char *)virt_to_phys(kbuf[i]);
					} else
						kbuf[i] = NULL;
					if ((ptype == TEE_PARAM_TYPE_MEMREF_INPUT) ||
						(ptype == TEE_PARAM_TYPE_MEMREF_INOUT)) {
						if (copy_from_user(kbuf[i], (void __user *)ubuf[i], s->params[i].memref.size)) {
							PERR("copy_from_user error (buffer)");
							ret = -EFAULT;
							goto discretix_error_exit;
						}
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	default:
		break;
	}
    PDEBUG("##### TZ DX");
	ret = secure_3rd_party_syscall(0, (unsigned char *)&hdix, sizeof(hdix));

	/* invalid cache */
	if (ptr) {
		start = (unsigned long)ptr;
		end = start + hdix.buf_len;
		scm_inv_range(start, end);
	}

	PDEBUG("##### TZ DX ret=%d(%x)", ret, ret);
	if (ret) {
		PERR("3rd party syscall failed (%d)", ret);
		goto discretix_error_exit;
	}

	if (hdix.ret) {
		PERR("[DX] func:%x ret:%x", hdix.func, hdix.ret);
	}

	switch (hdix.func) {
	case TEE_FUNC_TA_OpenSession:
		{
			struct CMD_TA_OpenSession *s;

			s = (struct CMD_TA_OpenSession *)ptr;
			s->sessionContext = (void *)sessionContext;
			for (i = 0; i < 4; i++) {
				int ptype;

				ptype = TEE_PARAM_TYPE_GET(s->paramTypes, i);
				switch (ptype) {
				case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				case TEE_PARAM_TYPE_MEMREF_INOUT:
					if (s->params[i].memref.size != 0) {
						/* invalid cache */
						start = (unsigned long)kbuf[i];
						end = start + s->params[i].memref.size;
						scm_inv_range(start, end);
						if (copy_to_user((void __user *)ubuf[i], kbuf[i], s->params[i].memref.size)) {
							PERR("copy_to_user error (buffer)");
							ret = -EFAULT;
							goto discretix_error_exit;
						}
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	case TEE_FUNC_TA_InvokeCommand:
		{
			struct CMD_TA_InvokeCommand *s;

			s = (struct CMD_TA_InvokeCommand *)ptr;
			for (i = 0; i < 4; i++) {
				int ptype;

				ptype = TEE_PARAM_TYPE_GET(s->paramTypes, i);
				switch (ptype) {
				case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				case TEE_PARAM_TYPE_MEMREF_INOUT:
					if (s->params[i].memref.size != 0) {
						/* invalid cache */
						start = (unsigned long)kbuf[i];
						end = start + s->params[i].memref.size;
						scm_inv_range(start, end);
						if (copy_to_user((void __user *)ubuf[i], kbuf[i], s->params[i].memref.size)) {
							PERR("copy_to_user error (buffer)");
							ret = -EFAULT;
							goto discretix_error_exit;
						}
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	case TEE_FUNC_SECURE_STORAGE_INIT:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			s->image_base = image_u;
			kfree(image);
			secure_storage_init = 1;
		}
		break;
	case TEE_FUNC_SECURE_STORAGE_SYNC:
		{
			struct CMD_SECURE_STORAGE *s;

			s = (struct CMD_SECURE_STORAGE *)ptr;
			s->image_base = image_u;
			/* invalid cache */
			start = (unsigned long)image;
			end = start + s->image_size;
			scm_inv_range(start, end);
			if (copy_to_user((void __user *)image_u, image, s->image_size))
				 PERR("copy_to_user error (image)");
			kfree(image);
			PDEBUG("sync htc ssd");
		}
		break;
	default:
		break;
	}

	if (hdix.buf_len != 0) {
		if (copy_to_user((void __user *)data, ptr, hdix.buf_len)) {
			PERR("copy_to_user error (data)");
			ret = -EFAULT;
			goto discretix_error_exit;
		}
	}
	if (copy_to_user((void __user *)arg, &hdix, sizeof(hdix))) {
		PERR("copy_to_user error (msg)");
		ret = -EFAULT;
		goto discretix_error_exit;
	}

discretix_error_exit:
	for (i = 0; i < 4; i++)
		if (kbuf[i] != NULL)
			kfree(kbuf[i]);
	kfree(ptr);

	return ret;
}

static long htcdrm_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	htc_drm_msg_s hmsg;
	int ret = 0;
	unsigned char *ptr;
	static unsigned char htc_cprmkey[188]={0};
	static unsigned char emmkey[16]={0};
	static unsigned char ecmkey[32]={0};

	PDEBUG("command = %x\n", command);
	switch (command) {
	case HTCDRM_IOCTL_WIDEVINE:
		if (copy_from_user(&hmsg, (void __user *)arg, sizeof(hmsg))) {
			PERR("copy_from_user error (msg)");
			return -EFAULT;
		}
#ifdef CONFIG_ARCH_MSM7X30
		oem_rapi_client_init();
		if (down_interruptible(&keybox_dev->sem)) {
			PERR("interrupt error");
			return -EFAULT;
		}
#endif /* CONFIG_ARCH_MSM7X30 */
		PDEBUG("func = %x\n", hmsg.func);
		switch (hmsg.func) {
		case HTC_OEMCRYPTO_STORE_KEYBOX:
			if ((hmsg.req_buf == NULL) || (hmsg.req_len != WIDEVINE_KEYBOX_LEN)) {
				PERR("invalid arguments");
				UP(&keybox_dev->sem);
				return -EFAULT;
			}
			if (copy_from_user(htc_keybox, (void __user *)hmsg.req_buf, hmsg.req_len)) {
				PERR("copy_from_user error (keybox)");
				UP(&keybox_dev->sem);
				return -EFAULT;
			}
#ifdef CONFIG_ARCH_MSM7X30
			memcpy(keybox_dev->keybox_buf, htc_keybox, WIDEVINE_KEYBOX_LEN);
			ret = htc_keybox_write(keybox_dev, htc_keybox, hmsg.req_len, hmsg.offset);
			oem_rapi_client_close();
#else
			ret = secure_access_item(1, ITEM_KEYBOX_PROVISION, hmsg.req_len,
					htc_keybox);
#endif	/* CONFIG_ARCH_MSM7X30 */
			if (ret)
				PERR("provision keybox failed (%d)\n", ret);
			UP(&keybox_dev->sem);
			break;
		case HTC_OEMCRYPTO_GET_KEYBOX:
			if ((hmsg.resp_buf == NULL) || !hmsg.resp_len ||
					((hmsg.offset + hmsg.resp_len) > WIDEVINE_KEYBOX_LEN)) {
				PERR("invalid arguments");
				UP(&keybox_dev->sem);
				return -EFAULT;
			}
#ifdef CONFIG_ARCH_MSM7X30
			ret = htc_keybox_read(keybox_dev, htc_keybox, hmsg.resp_len, hmsg.offset);
			oem_rapi_client_close();
			htc_keybox = keybox_dev->keybox_buf;
#else
			ret = secure_access_item(0, ITEM_KEYBOX_DATA, WIDEVINE_KEYBOX_LEN,
					htc_keybox);
#endif	/* CONFIG_ARCH_MSM7X30 */
			if (ret)
				PERR("get keybox failed (%d)\n", ret);
			else {
				if (copy_to_user((void __user *)hmsg.resp_buf, htc_keybox + hmsg.offset, hmsg.resp_len)) {
					PERR("copy_to_user error (keybox)");
					UP(&keybox_dev->sem);
					return -EFAULT;
				}
			}
			UP(&keybox_dev->sem);
			break;
		case HTC_OEMCRYPTO_IDENTIFY_DEVICE:
			if ((hmsg.resp_buf == NULL) || ((hmsg.resp_len != DEVICE_ID_LEN) && (hmsg.resp_len != 0xFF))) {
				PERR("invalid arguments");
				UP(&keybox_dev->sem);
				return -EFAULT;
			}

#ifdef CONFIG_ARCH_MSM7X30
			ret = htc_keybox_read(keybox_dev, htc_device_id, hmsg.resp_len, hmsg.offset);
			oem_rapi_client_close();
			htc_device_id = keybox_dev->keybox_buf;
#else
			ret = secure_access_item(0, ITEM_DEVICE_ID, DEVICE_ID_LEN,
					htc_device_id);
#endif	/* CONFIG_ARCH_MSM7X30 */
			if (ret)
				PERR("get device ID failed (%d)\n", ret);
			else {
				if (copy_to_user((void __user *)hmsg.resp_buf, htc_device_id, DEVICE_ID_LEN)) {
					PERR("copy_to_user error (device ID)");
					UP(&keybox_dev->sem);
					return -EFAULT;
				}
			}
			UP(&keybox_dev->sem);
			break;
		case HTC_OEMCRYPTO_GET_RANDOM:
			if ((hmsg.resp_buf == NULL) || !hmsg.resp_len) {
				PERR("invalid arguments");
				UP(&keybox_dev->sem);
				return -EFAULT;
			}
			ptr = kzalloc(hmsg.resp_len, GFP_KERNEL);
			if (ptr == NULL) {
				PERR("allocate the space for random data failed\n");
				UP(&keybox_dev->sem);
				return -1;
			}
#ifdef CONFIG_ARCH_MSM7X30
			get_random_bytes(ptr, hmsg.resp_len);
			printk(KERN_INFO "%s: Data get from random entropy ", __func__);
#else
			get_random_bytes(ptr, hmsg.resp_len);
			/* FIXME: second time of this function call will hang
			ret = secure_access_item(0, ITEM_RAND_DATA, hmsg.resp_len, ptr);
			*/
#endif	/* CONFIG_ARCH_MSM7X30 */
			if (ret)
				PERR("get random data failed (%d)\n", ret);
			else {
				if (copy_to_user((void __user *)hmsg.resp_buf, ptr, hmsg.resp_len)) {
					PERR("copy_to_user error (random data)");
					kfree(ptr);
					UP(&keybox_dev->sem);
					return -EFAULT;
				}
			}
			UP(&keybox_dev->sem);
			kfree(ptr);
			break;
		case HTC_OEMCRYPTO_IS_KEYBOX_VALID:
#ifndef CONFIG_ARCH_MSM7X30
			ret = secure_access_item(0, ITEM_VALIDATE_KEYBOX, WIDEVINE_KEYBOX_LEN, NULL);
#endif
			UP(&keybox_dev->sem);
			return ret;
		case HTC_OEMCRYPTO_SET_ENTITLEMENT_KEY:
			if (copy_from_user(emmkey, (void __user *)hmsg.req_buf, hmsg.req_len)) {
				PERR("copy_from_user error (keybox)");
				return -EFAULT;
			}
			ret = secure_key_ladder(ITEM_SET_ENTITLEMENT_KEY,hmsg.req_len,emmkey);
			if (ret)
				PERR("set emmkey failed (%d)\n", ret);
			break;
		case HTC_OEMCRYPTO_DERIVE_CONTROL_WORD:
			if (copy_from_user(ecmkey, (void __user *)hmsg.req_buf, hmsg.req_len)) {
				PERR("copy_from_user error (keybox)");
				return -EFAULT;
			}
			ret = secure_key_ladder(ITEM_DERIVE_CONTROL_WORD,hmsg.req_len,ecmkey);
			if (ret)
				PERR("DeriveControlWord failed (%d)\n", ret);
			else {
				if (copy_to_user( (void __user *)hmsg.resp_buf , ecmkey , hmsg.resp_len)) {
					PERR("copy_to_user error (ecmkey)");
					return -EFAULT;
				}
			}
			break;
		default:
			UP(&keybox_dev->sem);
			PERR("func error\n");
			return -EFAULT;
		}
		break;

	case HTCDRM_IOCTL_DISCRETIX:
		mutex_lock(&dx_lock);
		ret = htcdrm_discretix_ioctl(file, command, arg);
		mutex_unlock(&dx_lock);
		break;

    case HTCDRM_IOCTL_CPRM:
		 if (copy_from_user(&hmsg, (void __user *)arg, sizeof(hmsg))) {
			PERR("copy_from_user error (msg)");
			return -EFAULT;
		}


        if ((hmsg.resp_buf == NULL) || !hmsg.resp_len ) {
            PERR("invalid arguments");
            return -EFAULT;
		}

        ret = secure_access_item(0, ITEM_CPRMKEY_DATA, CPRM_KEY_LEN, htc_cprmkey);

        if (ret)
            PERR("get cprmkey failed (%d)\n", ret);
        else {
            if (copy_to_user( (void __user *)hmsg.resp_buf , htc_cprmkey , hmsg.resp_len)) {
                PERR("copy_to_user error (cprmkey)");
                return -EFAULT;
            }
            }

		break;

	default:
		PERR("command error\n");
		return -EFAULT;
	}
	return ret;
}


static int htcdrm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int htcdrm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int htcdrm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;
	long length = vma->vm_end - vma->vm_start;

	/* check length - do not allow larger mappings than the number of
		pages allocated */
	if (length > discretix_smem_size)
		return -EIO;

	PDEBUG("htcdrm_mmap %lx", length);

	/* map the whole physically contiguous area in one piece */
	ret = remap_pfn_range(vma,
			vma->vm_start,
			virt_to_phys((void *)discretix_smem_area) >> PAGE_SHIFT,
			length,
			vma->vm_page_prot);

	return ret;
}


static const struct file_operations htcdrm_fops = {
	.unlocked_ioctl = htcdrm_ioctl,
	.open = htcdrm_open,
	.release = htcdrm_release,
	.mmap = htcdrm_mmap,
	.owner = THIS_MODULE,
};


static int __init htcdrm_init(void)
{
	int ret;

	htc_device_id = kzalloc(DEVICE_ID_LEN, GFP_KERNEL);
	if (htc_device_id == NULL) {
		PERR("allocate the space for device ID failed\n");
		return -1;
	}
	htc_keybox = kzalloc(WIDEVINE_KEYBOX_LEN, GFP_KERNEL);
	if (htc_keybox == NULL) {
		PERR("allocate the space for keybox failed\n");
		kfree(htc_device_id);
		return -1;
	}

	discretix_smem_ptr = kzalloc(discretix_smem_size + (2 * PAGE_SIZE), GFP_KERNEL);
	if (discretix_smem_ptr == NULL) {
		PERR("allocate the space for DX smem failed\n");
		kfree(htc_device_id);
		return -1;
	}

	/* round it up to the page bondary */
	discretix_smem_area = (unsigned char *)((((unsigned long)discretix_smem_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
	discretix_smem_phy = (unsigned char *)virt_to_phys(discretix_smem_area);

#if DIS_ALLOC_TZ_HEAP
	discretix_tz_heap = kzalloc(DISCRETIX_HEAP_SIZE, GFP_KERNEL);
	if (discretix_tz_heap == NULL) {
		PERR("allocate the space for discretix heap failed\n");
		kfree(htc_device_id);
		return -1;
	}
#endif

	ret = register_chrdev(0, DEVICE_NAME, &htcdrm_fops);
	if (ret < 0) {
		PERR("register module fail\n");
		kfree(htc_device_id);
		kfree(htc_keybox);
		return ret;
	}
	htcdrm_major = ret;

	htcdrm_class = class_create(THIS_MODULE, "htcdrm");
	device_create(htcdrm_class, NULL, MKDEV(htcdrm_major, 0), NULL, DEVICE_NAME);

#ifdef CONFIG_ARCH_MSM7X30
	keybox_dev = kzalloc(sizeof(htc_keybox_dev), GFP_KERNEL);
	if (keybox_dev == NULL) {
		PERR("allocate space for keybox_dev failed\n");
		kfree(keybox_dev);
		return -1;
	}
	sema_init(&keybox_dev->sem, 1);
#endif
	PDEBUG("register module ok\n");
	return 0;
}


static void  __exit htcdrm_exit(void)
{
	device_destroy(htcdrm_class, MKDEV(htcdrm_major, 0));
	class_unregister(htcdrm_class);
	class_destroy(htcdrm_class);
	unregister_chrdev(htcdrm_major, DEVICE_NAME);
	kfree(htc_device_id);
	kfree(htc_keybox);
#ifdef CONFIG_ARCH_MSM7X30
	kfree(keybox_dev);
#endif

#if DIS_ALLOC_TZ_HEAP
	kfree(discretix_tz_heap);
#endif
	kfree(discretix_smem_ptr);

	PDEBUG("un-registered module ok\n");
}

module_init(htcdrm_init);
module_exit(htcdrm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eddic Hsien");

