#ifndef _SCSI_SCSI_TCQ_H
#define _SCSI_SCSI_TCQ_H

#include <linux/blkdev.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>

#define MSG_SIMPLE_TAG	0x20
#define MSG_HEAD_TAG	0x21
#define MSG_ORDERED_TAG	0x22
#define MSG_ACA_TAG	0x24	

#define SCSI_NO_TAG	(-1)    


#ifdef CONFIG_BLOCK

static inline int scsi_get_tag_type(struct scsi_device *sdev)
{
	if (!sdev->tagged_supported)
		return 0;
	if (sdev->ordered_tags)
		return MSG_ORDERED_TAG;
	if (sdev->simple_tags)
		return MSG_SIMPLE_TAG;
	return 0;
}

static inline void scsi_set_tag_type(struct scsi_device *sdev, int tag)
{
	switch (tag) {
	case MSG_ORDERED_TAG:
		sdev->ordered_tags = 1;
		
	case MSG_SIMPLE_TAG:
		sdev->simple_tags = 1;
		break;
	case 0:
		
	default:
		sdev->ordered_tags = 0;
		sdev->simple_tags = 0;
		break;
	}
}
static inline void scsi_activate_tcq(struct scsi_device *sdev, int depth)
{
	if (!sdev->tagged_supported)
		return;

	if (!blk_queue_tagged(sdev->request_queue))
		blk_queue_init_tags(sdev->request_queue, depth,
				    sdev->host->bqt);

	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), depth);
}

static inline void scsi_deactivate_tcq(struct scsi_device *sdev, int depth)
{
	if (blk_queue_tagged(sdev->request_queue))
		blk_queue_free_tags(sdev->request_queue);
	scsi_adjust_queue_depth(sdev, 0, depth);
}

static inline int scsi_populate_tag_msg(struct scsi_cmnd *cmd, char *msg)
{
        struct request *req = cmd->request;

        if (blk_rq_tagged(req)) {
		*msg++ = MSG_SIMPLE_TAG;
        	*msg++ = req->tag;
        	return 2;
	}

	return 0;
}

static inline struct scsi_cmnd *scsi_find_tag(struct scsi_device *sdev, int tag)
{

        struct request *req;

        if (tag != SCSI_NO_TAG) {
        	req = blk_queue_find_tag(sdev->request_queue, tag);
	        return req ? (struct scsi_cmnd *)req->special : NULL;
	}

	
	return sdev->current_cmnd;
}

static inline int scsi_init_shared_tag_map(struct Scsi_Host *shost, int depth)
{
	if (!shost->bqt) {
		shost->bqt = blk_init_tags(depth);
		if (!shost->bqt)
			return -ENOMEM;
	}

	return 0;
}

static inline struct scsi_cmnd *scsi_host_find_tag(struct Scsi_Host *shost,
						int tag)
{
	struct request *req;

	if (tag != SCSI_NO_TAG) {
		req = blk_map_queue_find_tag(shost->bqt, tag);
		return req ? (struct scsi_cmnd *)req->special : NULL;
	}
	return NULL;
}

#endif 
#endif 
