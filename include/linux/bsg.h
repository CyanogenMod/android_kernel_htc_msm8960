#ifndef BSG_H
#define BSG_H

#include <linux/types.h>

#define BSG_PROTOCOL_SCSI		0

#define BSG_SUB_PROTOCOL_SCSI_CMD	0
#define BSG_SUB_PROTOCOL_SCSI_TMF	1
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT	2

#define BSG_FLAG_Q_AT_TAIL 0x10 

struct sg_io_v4 {
	__s32 guard;		
	__u32 protocol;		
	__u32 subprotocol;	

	__u32 request_len;	
	__u64 request;		
	__u64 request_tag;	
	__u32 request_attr;	
	__u32 request_priority;	
	__u32 request_extra;	
	__u32 max_response_len;	
	__u64 response;		

        
	__u32 dout_iovec_count;	
	__u32 dout_xfer_len;	
	__u32 din_iovec_count;	
	__u32 din_xfer_len;	
	__u64 dout_xferp;	
	__u64 din_xferp;	

	__u32 timeout;		
	__u32 flags;		
	__u64 usr_ptr;		
	__u32 spare_in;		

	__u32 driver_status;	
	__u32 transport_status;	
	__u32 device_status;	
	__u32 retry_delay;	
	__u32 info;		
	__u32 duration;		
	__u32 response_len;	/* [o] bytes of response actually written */
	__s32 din_resid;	
	__s32 dout_resid;	
	__u64 generated_tag;	
	__u32 spare_out;	

	__u32 padding;
};

#ifdef __KERNEL__

#if defined(CONFIG_BLK_DEV_BSG)
struct bsg_class_device {
	struct device *class_dev;
	struct device *parent;
	int minor;
	struct request_queue *queue;
	struct kref ref;
	void (*release)(struct device *);
};

extern int bsg_register_queue(struct request_queue *q,
			      struct device *parent, const char *name,
			      void (*release)(struct device *));
extern void bsg_unregister_queue(struct request_queue *);
#else
static inline int bsg_register_queue(struct request_queue *q,
				     struct device *parent, const char *name,
				     void (*release)(struct device *))
{
	return 0;
}
static inline void bsg_unregister_queue(struct request_queue *q)
{
}
#endif

#endif 

#endif
