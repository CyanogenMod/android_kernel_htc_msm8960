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
 */


#ifndef _SPS_H_
#define _SPS_H_

#include <linux/types.h>	

#define SPS_DEV_HANDLE_MEM       ((u32)0x7ffffffful)


#define SPS_DEV_HANDLE_INVALID   ((u32)0)

#define SPS_IRQ_INVALID          0

#define SPS_ADDR_INVALID      0

#define SPS_CLASS_INVALID     ((u32)-1)

#define SPS_CONFIG_DEFAULT       0

#define SPS_DMA_THRESHOLD_DEFAULT   0

#define SPS_IOVEC_FLAG_INT  0x8000  
#define SPS_IOVEC_FLAG_EOT  0x4000  
#define SPS_IOVEC_FLAG_EOB  0x2000  
#define SPS_IOVEC_FLAG_NWD  0x1000  
#define SPS_IOVEC_FLAG_CMD  0x0800  
#define SPS_IOVEC_FLAG_LOCK  0x0400  
#define SPS_IOVEC_FLAG_UNLOCK  0x0200  
#define SPS_IOVEC_FLAG_IMME 0x0100  
#define SPS_IOVEC_FLAG_NO_SUBMIT 0x0002  
#define SPS_IOVEC_FLAG_DEFAULT   0x0001  


#define SPS_BAM_OPT_ENABLE_AT_BOOT  1UL
#define SPS_BAM_OPT_IRQ_DISABLED    (1UL << 1)
#define SPS_BAM_OPT_BAMDMA          (1UL << 2)
#define SPS_BAM_OPT_IRQ_WAKEUP      (1UL << 3)


#define SPS_BAM_MGR_DEVICE_REMOTE   1UL
#define SPS_BAM_MGR_MULTI_EE        (1UL << 1)
#define SPS_BAM_MGR_PIPE_NO_ALLOC   (1UL << 2)
#define SPS_BAM_MGR_PIPE_NO_CONFIG  (1UL << 3)
#define SPS_BAM_MGR_PIPE_NO_CTRL    (1UL << 4)
#define SPS_BAM_MGR_NONE            \
	(SPS_BAM_MGR_DEVICE_REMOTE | SPS_BAM_MGR_PIPE_NO_ALLOC | \
	 SPS_BAM_MGR_PIPE_NO_CONFIG | SPS_BAM_MGR_PIPE_NO_CTRL)
#define SPS_BAM_MGR_LOCAL           0
#define SPS_BAM_MGR_LOCAL_SHARED    SPS_BAM_MGR_MULTI_EE
#define SPS_BAM_MGR_REMOTE_SHARED   \
	(SPS_BAM_MGR_DEVICE_REMOTE | SPS_BAM_MGR_MULTI_EE | \
	 SPS_BAM_MGR_PIPE_NO_ALLOC)
#define SPS_BAM_MGR_ACCESS_MASK     SPS_BAM_MGR_NONE

#define SPS_BAM_NUM_EES             4
#define SPS_BAM_SEC_DO_NOT_CONFIG   0
#define SPS_BAM_SEC_DO_CONFIG       0x0A434553

enum sps_mode {
	SPS_MODE_SRC = 0,  
	SPS_MODE_DEST,	   
};


enum sps_option {
	SPS_O_DESC_DONE = 0x00000001,  
	SPS_O_INACTIVE  = 0x00000002,  
	SPS_O_WAKEUP    = 0x00000004,  
	SPS_O_OUT_OF_DESC = 0x00000008,
	SPS_O_ERROR     = 0x00000010,  
	SPS_O_EOT       = 0x00000020,  

	
	SPS_O_STREAMING = 0x00010000,  
	
	SPS_O_IRQ_MTI   = 0x00020000,
	/* NWD bit written with EOT for BAM2BAM producer pipe */
	SPS_O_WRITE_NWD   = 0x00040000,

	
	
	SPS_O_POLL      = 0x01000000,
	
	SPS_O_NO_Q      = 0x02000000,
	SPS_O_FLOWOFF   = 0x04000000,  
	
	SPS_O_WAKEUP_IS_ONESHOT = 0x08000000,
	SPS_O_ACK_TRANSFERS = 0x10000000,
	
	SPS_O_AUTO_ENABLE = 0x20000000,
	
	SPS_O_NO_EP_SYNC = 0x40000000,
};

enum sps_dma_priority {
	SPS_DMA_PRI_DEFAULT = 0,
	SPS_DMA_PRI_LOW,
	SPS_DMA_PRI_MED,
	SPS_DMA_PRI_HIGH,
};

enum sps_owner {
	SPS_OWNER_LOCAL = 0x1,	
	SPS_OWNER_REMOTE = 0x2,	
};

enum sps_event {
	SPS_EVENT_INVALID = 0,

	SPS_EVENT_EOT,		
	SPS_EVENT_DESC_DONE,	
	SPS_EVENT_OUT_OF_DESC,	
	SPS_EVENT_WAKEUP,	
	SPS_EVENT_FLOWOFF,	
	SPS_EVENT_INACTIVE,	
	SPS_EVENT_ERROR,	
	SPS_EVENT_MAX,
};

enum sps_trigger {
	
	SPS_TRIGGER_CALLBACK = 0,
	
	SPS_TRIGGER_WAIT,
};

enum sps_flow_off {
	SPS_FLOWOFF_FORCED = 0,	
	
	SPS_FLOWOFF_GRACEFUL,
};

enum sps_mem {
	SPS_MEM_LOCAL = 0,  
	SPS_MEM_UC,	    
};

enum sps_timer_op {
	SPS_TIMER_OP_CONFIG = 0,
	SPS_TIMER_OP_RESET,
	SPS_TIMER_OP_READ,
};

enum sps_timer_mode {
	SPS_TIMER_MODE_ONESHOT = 0,
};

enum sps_callback_case {
	SPS_CALLBACK_BAM_ERROR_IRQ = 1,     
	SPS_CALLBACK_BAM_HRESP_ERR_IRQ,	    
};

enum sps_command_type {
	SPS_WRITE_COMMAND = 0,
	SPS_READ_COMMAND,
};

struct sps_iovec {
	u32 addr;
	u32 size:16;
	u32 flags:16;
};

/**
 * This data type corresponds to the native Command Element
 * supported by SPS hardware
 *
 * @addr - register address.
 * @command - command type.
 * @data - for write command: content to be written into peripheral register.
 *         for read command: dest addr to write peripheral register value to.
 * @mask - register mask.
 * @reserved - for future usage.
 *
 */
struct sps_command_element {
	u32 addr:24;
	u32 command:8;
	u32 data;
	u32 mask;
	u32 reserved;
};

struct sps_bam_pipe_sec_config_props {
	u32 pipe_mask;
	u32 vmid;
};

struct sps_bam_sec_config_props {
	
	struct sps_bam_pipe_sec_config_props ees[SPS_BAM_NUM_EES];
};

struct sps_bam_props {

	

	u32 options;
	u32 phys_addr;
	void *virt_addr;
	u32 virt_size;
	u32 irq;
	u32 num_pipes;
	u32 summing_threshold;

	

	u32 periph_class;
	u32 periph_dev_id;
	u32 periph_phys_addr;
	void *periph_virt_addr;
	u32 periph_virt_size;

	

	u32 event_threshold;
	u32 desc_size;
	u32 data_size;
	u32 desc_mem_id;
	u32 data_mem_id;

	
	void (*callback)(enum sps_callback_case, void *);
	void *user;

	

	u32 manage;
	u32 restricted_pipes;
	u32 ee;

	

	u32 irq_gen_addr;

	

	u32 sec_config;
	struct sps_bam_sec_config_props *p_sec_config_props;
};

struct sps_mem_buffer {
	void *base;
	u32 phys_base;
	u32 size;
	u32 min_size;
};

struct sps_connect {
	u32 source;
	u32 src_pipe_index;
	u32 destination;
	u32 dest_pipe_index;

	enum sps_mode mode;

	u32 config;

	enum sps_option options;

	struct sps_mem_buffer desc;
	struct sps_mem_buffer data;

	u32 event_thresh;

	u32 lock_group;

	

	u32 irq_gen_addr;
	u32 irq_gen_data;

	u32 sps_reserved;

};

struct sps_satellite {
	u32 dev;
	u32 pipe_index;

	u32 config;
	enum sps_option options;

};

struct sps_alloc_dma_chan {
	u32 dev;

	

	u32 threshold;
	enum sps_dma_priority priority;

	u32 src_owner;
	u32 dest_owner;

};

struct sps_dma_chan {
	u32 dev;
	u32 dest_pipe_index;
	u32 src_pipe_index;
};

/**
 * This struct is an argument passed payload when triggering a callback event
 * object registered for an SPS connection end point.
 *
 * @user - Pointer registered with sps_register_event().
 *
 * @event_id - Which event.
 *
 * @iovec - The associated I/O vector. If the end point is a system-mode
 * producer, the size will reflect the actual number of bytes written to the
 * buffer by the pipe. NOTE: If this I/O vector was part of a set submitted to
 * sps_transfer(), then the vector array itself will be	updated with all of
 * the actual counts.
 *
 * @user - Pointer registered with the transfer.
 *
 */
struct sps_event_notify {
	void *user;

	enum sps_event event_id;

	

	union {
		
		struct {
			u32 mask;
		} irq;

		

		struct {
			struct sps_iovec iovec;
			void *user;
		} transfer;

		

		struct {
			u32 status;
		} err;

	} data;
};

struct sps_register_event {
	enum sps_option options;
	enum sps_trigger mode;
	struct completion *xfer_done;
	void (*callback)(struct sps_event_notify *notify);
	void *user;
};

struct sps_transfer {
	u32 iovec_phys;
	struct sps_iovec *iovec;
	u32 iovec_count;
	void *user;
};

struct sps_timer_ctrl {
	enum sps_timer_op op;

	enum sps_timer_mode mode;
	u32 timeout_msec;
};

struct sps_timer_result {
	u32 current_timer;
};


struct sps_pipe;	

#ifdef CONFIG_SPS
/**
 * Register a BAM device
 *
 * This function registers a BAM device with the SPS driver. For each
 *peripheral that includes a BAM, the peripheral driver must register
 * the BAM with the SPS driver.
 *
 * A requirement is that the peripheral driver must remain attached
 * to the SPS driver until the BAM is deregistered. Otherwise, the
 * system may attempt to unload the SPS driver. BAM registrations would
 * be lost.
 *
 * @bam_props - Pointer to struct for BAM device properties.
 *
 * @dev_handle - Device handle will be written to this location (output).
 *
 * @return 0 on success, negative value on error
 *
 */
int sps_register_bam_device(const struct sps_bam_props *bam_props,
			    u32 *dev_handle);

int sps_deregister_bam_device(u32 dev_handle);

struct sps_pipe *sps_alloc_endpoint(void);

int sps_free_endpoint(struct sps_pipe *h);

int sps_get_config(struct sps_pipe *h, struct sps_connect *config);

int sps_alloc_mem(struct sps_pipe *h, enum sps_mem mem,
		  struct sps_mem_buffer *mem_buffer);

int sps_free_mem(struct sps_pipe *h, struct sps_mem_buffer *mem_buffer);

int sps_connect(struct sps_pipe *h, struct sps_connect *connect);

int sps_disconnect(struct sps_pipe *h);

int sps_register_event(struct sps_pipe *h, struct sps_register_event *reg);

int sps_transfer_one(struct sps_pipe *h, u32 addr, u32 size,
		     void *user, u32 flags);

int sps_get_event(struct sps_pipe *h, struct sps_event_notify *event);

int sps_get_iovec(struct sps_pipe *h, struct sps_iovec *iovec);

int sps_flow_on(struct sps_pipe *h);

int sps_flow_off(struct sps_pipe *h, enum sps_flow_off mode);

int sps_transfer(struct sps_pipe *h, struct sps_transfer *transfer);

int sps_is_pipe_empty(struct sps_pipe *h, u32 *empty);

int sps_device_reset(u32 dev);

int sps_set_config(struct sps_pipe *h, struct sps_connect *config);

int sps_set_owner(struct sps_pipe *h, enum sps_owner owner,
		  struct sps_satellite *connect);

/**
 * Allocate a BAM DMA channel
 *
 * This function allocates a BAM DMA channel. A "BAM DMA" is a special
 * DMA peripheral with a BAM front end. The DMA peripheral acts as a conduit
 * for data to flow into a consumer pipe and then out of a producer pipe.
 * It's primarily purpose is to serve as a path for interprocessor communication
 * that allows each processor to control and protect it's own memory space.
 *
 * @alloc - Pointer to struct for BAM DMA channel allocation properties.
 *
 * @chan - Allocated channel information will be written to this
 *  location (output).
 *
 * @return 0 on success, negative value on error
 *
 */
int sps_alloc_dma_chan(const struct sps_alloc_dma_chan *alloc,
		       struct sps_dma_chan *chan);

int sps_free_dma_chan(struct sps_dma_chan *chan);

u32 sps_dma_get_bam_handle(void);

void sps_dma_free_bam_handle(u32 h);


int sps_get_free_count(struct sps_pipe *h, u32 *count);

int sps_timer_ctrl(struct sps_pipe *h,
		   struct sps_timer_ctrl *timer_ctrl,
		   struct sps_timer_result *timer_result);

int sps_phy2h(u32 phys_addr, u32 *handle);

int sps_setup_bam2bam_fifo(struct sps_mem_buffer *mem_buffer,
		  u32 addr, u32 size, int use_offset);

int sps_get_unused_desc_num(struct sps_pipe *h, u32 *desc_num);

int sps_get_bam_debug_info(u32 dev, u32 option);

#else
static inline int sps_register_bam_device(const struct sps_bam_props
			*bam_props, u32 *dev_handle)
{
	return -EPERM;
}

static inline int sps_deregister_bam_device(u32 dev_handle)
{
	return -EPERM;
}

static inline struct sps_pipe *sps_alloc_endpoint(void)
{
	return NULL;
}

static inline int sps_free_endpoint(struct sps_pipe *h)
{
	return -EPERM;
}

static inline int sps_get_config(struct sps_pipe *h, struct sps_connect *config)
{
	return -EPERM;
}

static inline int sps_alloc_mem(struct sps_pipe *h, enum sps_mem mem,
		  struct sps_mem_buffer *mem_buffer)
{
	return -EPERM;
}

static inline int sps_free_mem(struct sps_pipe *h,
				struct sps_mem_buffer *mem_buffer)
{
	return -EPERM;
}

static inline int sps_connect(struct sps_pipe *h, struct sps_connect *connect)
{
	return -EPERM;
}

static inline int sps_disconnect(struct sps_pipe *h)
{
	return -EPERM;
}

static inline int sps_register_event(struct sps_pipe *h,
					struct sps_register_event *reg)
{
	return -EPERM;
}

static inline int sps_transfer_one(struct sps_pipe *h, u32 addr, u32 size,
		     void *user, u32 flags)
{
	return -EPERM;
}

static inline int sps_get_event(struct sps_pipe *h,
				struct sps_event_notify *event)
{
	return -EPERM;
}

static inline int sps_get_iovec(struct sps_pipe *h, struct sps_iovec *iovec)
{
	return -EPERM;
}

static inline int sps_flow_on(struct sps_pipe *h)
{
	return -EPERM;
}

static inline int sps_flow_off(struct sps_pipe *h, enum sps_flow_off mode)
{
	return -EPERM;
}

static inline int sps_transfer(struct sps_pipe *h,
				struct sps_transfer *transfer)
{
	return -EPERM;
}

static inline int sps_is_pipe_empty(struct sps_pipe *h, u32 *empty)
{
	return -EPERM;
}

static inline int sps_device_reset(u32 dev)
{
	return -EPERM;
}

static inline int sps_set_config(struct sps_pipe *h, struct sps_connect *config)
{
	return -EPERM;
}

static inline int sps_set_owner(struct sps_pipe *h, enum sps_owner owner,
		  struct sps_satellite *connect)
{
	return -EPERM;
}

static inline int sps_get_free_count(struct sps_pipe *h, u32 *count)
{
	return -EPERM;
}

static inline int sps_alloc_dma_chan(const struct sps_alloc_dma_chan *alloc,
		       struct sps_dma_chan *chan)
{
	return -EPERM;
}

static inline int sps_free_dma_chan(struct sps_dma_chan *chan)
{
	return -EPERM;
}

static inline u32 sps_dma_get_bam_handle(void)
{
	return 0;
}

static inline void sps_dma_free_bam_handle(u32 h)
{
}

static inline int sps_timer_ctrl(struct sps_pipe *h,
		   struct sps_timer_ctrl *timer_ctrl,
		   struct sps_timer_result *timer_result)
{
	return -EPERM;
}

static inline int sps_phy2h(u32 phys_addr, u32 *handle)
{
	return -EPERM;
}

static inline int sps_setup_bam2bam_fifo(struct sps_mem_buffer *mem_buffer,
		  u32 addr, u32 size, int use_offset)
{
	return -EPERM;
}

static inline int sps_get_unused_desc_num(struct sps_pipe *h, u32 *desc_num)
{
	return -EPERM;
}
#endif

#endif 
