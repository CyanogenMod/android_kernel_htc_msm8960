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


#ifdef CONFIG_SPS_SUPPORT_BAMDMA

#include <linux/export.h>
#include <linux/memory.h>	

#include "spsi.h"
#include "bam.h"
#include "sps_bam.h"		
#include "sps_core.h"		


#define DMA_ENBL			(0x00000000)
#ifdef CONFIG_SPS_SUPPORT_NDP_BAM
#define DMA_REVISION			(0x00000004)
#define DMA_CONFIG			(0x00000008)
#define DMA_CHNL_CONFIG(n)		(0x00001000 + 4096 * (n))
#else
#define DMA_CHNL_CONFIG(n)		(0x00000004 + 4 * (n))
#define DMA_CONFIG			(0x00000040)
#endif


#ifdef CONFIG_SPS_SUPPORT_NDP_BAM
#define DMA_CHNL_PRODUCER_PIPE_ENABLED	0x40000
#define DMA_CHNL_CONSUMER_PIPE_ENABLED	0x20000
#endif
#define DMA_CHNL_HALT_DONE		0x10000
#define DMA_CHNL_HALT			0x1000
#define DMA_CHNL_ENABLE                 0x100
#define DMA_CHNL_ACT_THRESH             0x30
#define DMA_CHNL_WEIGHT                 0x7

#define TESTBUS_SELECT                  0x3

static inline void dma_write_reg(void *base, u32 offset, u32 val)
{
	iowrite32(val, base + offset);
	SPS_DBG("sps:bamdma: write reg 0x%x w_val 0x%x.", offset, val);
}

static inline void dma_write_reg_field(void *base, u32 offset,
				       const u32 mask, u32 val)
{
	u32 shift = find_first_bit((void *)&mask, 32);
	u32 tmp = ioread32(base + offset);

	tmp &= ~mask;		/* clear written bits */
	val = tmp | (val << shift);
	iowrite32(val, base + offset);
	SPS_DBG("sps:bamdma: write reg 0x%x w_val 0x%x.", offset, val);
}

#define DMA_MAX_PIPES         ((BAM_MAX_PIPES / 2) * 2)

#define MAX_BAM_DMA_DEVICES   1

#define MAX_BAM_DMA_BAMS      1

#define DMA_PIPES_STATE_DIFF     0
#define DMA_PIPES_BOTH_DISABLED  1
#define DMA_PIPES_BOTH_ENABLED   2

#define DMA_PIPE_IS_DEST(p)   (((p) & 1) == 0)
#define DMA_PIPE_IS_SRC(p)    (((p) & 1) != 0)

enum bamdma_pipe_state {
	PIPE_INACTIVE = 0,
	PIPE_ACTIVE
};

enum bamdma_chan_state {
	DMA_CHAN_STATE_FREE = 0,
	DMA_CHAN_STATE_ALLOC_EXT,	
	DMA_CHAN_STATE_ALLOC_INT	
};

struct bamdma_chan {
	
	enum bamdma_chan_state state;

	
	u32 threshold;
	enum sps_dma_priority priority;

	
	enum bam_dma_thresh_dma thresh;
	enum bam_dma_weight_dma weight;

};

struct bamdma_device {
	
	int enabled;
	int local;

	
	struct sps_bam *bam;

	
	u32 h;

	
	void *virt_addr;
	int virtual_mapped;
	u32 phys_addr;
	void *hwio;

	
	u32 num_pipes;
	enum bamdma_pipe_state pipes[DMA_MAX_PIPES];
	struct bamdma_chan chans[DMA_MAX_PIPES / 2];

};

static struct bamdma_device bam_dma_dev[MAX_BAM_DMA_DEVICES];
static struct mutex bam_dma_lock;

static int num_bams;
static u32 bam_handles[MAX_BAM_DMA_BAMS];

static struct bamdma_device *sps_dma_find_device(u32 h)
{
	return &bam_dma_dev[0];
}

static int sps_dma_device_enable(struct bamdma_device *dev)
{
	if (dev->enabled)
		return 0;

	if (dev->local)
		dma_write_reg(dev->virt_addr, DMA_ENBL, 1);

	
	if (sps_bam_enable(dev->bam)) {
		SPS_ERR("sps:Failed to enable BAM DMA's BAM: %x",
			dev->phys_addr);
		return SPS_ERROR;
	}

	dev->enabled = true;

	return 0;
}

static int sps_dma_device_disable(struct bamdma_device *dev)
{
	u32 pipe_index;

	if (!dev->enabled)
		return 0;

	
	for (pipe_index = 0; pipe_index < dev->num_pipes; pipe_index++) {
		if (dev->pipes[pipe_index] != PIPE_INACTIVE)
			break;
	}

	if (pipe_index < dev->num_pipes) {
		SPS_ERR("sps:Fail to disable BAM-DMA %x:channels are active",
			dev->phys_addr);
		return SPS_ERROR;
	}

	dev->enabled = false;

	
	if (sps_bam_disable(dev->bam)) {
		SPS_ERR("sps:Fail to disable BAM-DMA BAM:%x", dev->phys_addr);
		return SPS_ERROR;
	}

	
	if (dev->local)
		
		dma_write_reg(dev->virt_addr, DMA_ENBL, 0);

	return 0;
}

int sps_dma_device_init(u32 h)
{
	struct bamdma_device *dev;
	struct sps_bam_props *props;
	u32 chan;
	int result = SPS_ERROR;

	mutex_lock(&bam_dma_lock);

	
	dev = NULL;
	if (bam_dma_dev[0].bam != NULL) {
		SPS_ERR("sps:BAM-DMA BAM device is already initialized.");
		goto exit_err;
	} else {
		dev = &bam_dma_dev[0];
	}

	
	memset(dev, 0, sizeof(*dev));
	dev->h = h;
	dev->bam = sps_h2bam(h);

	if (dev->bam == NULL) {
		SPS_ERR("sps:BAM-DMA BAM device is not found "
				"from the handle.");
		goto exit_err;
	}

	
	props = &dev->bam->props;
	dev->phys_addr = props->periph_phys_addr;
	if (props->periph_virt_addr != NULL) {
		dev->virt_addr = props->periph_virt_addr;
		dev->virtual_mapped = false;
	} else {
		if (props->periph_virt_size == 0) {
			SPS_ERR("sps:Unable to map BAM DMA IO memory: %x %x",
			 dev->phys_addr, props->periph_virt_size);
			goto exit_err;
		}

		dev->virt_addr = ioremap(dev->phys_addr,
					  props->periph_virt_size);
		if (dev->virt_addr == NULL) {
			SPS_ERR("sps:Unable to map BAM DMA IO memory: %x %x",
				dev->phys_addr, props->periph_virt_size);
			goto exit_err;
		}
		dev->virtual_mapped = true;
	}
	dev->hwio = (void *) dev->virt_addr;

	
	if ((props->manage & SPS_BAM_MGR_DEVICE_REMOTE) == 0) {
		SPS_DBG2("sps:BAM-DMA is controlled locally: %x",
			dev->phys_addr);
		dev->local = true;
	} else {
		SPS_DBG2("sps:BAM-DMA is controlled remotely: %x",
			dev->phys_addr);
		dev->local = false;
	}

	if (sps_dma_device_enable(dev))
		goto exit_err;

	dev->num_pipes = dev->bam->props.num_pipes;

	
	if (dev->local)
		for (chan = 0; chan < (dev->num_pipes / 2); chan++) {
			dma_write_reg_field(dev->virt_addr,
					    DMA_CHNL_CONFIG(chan),
					    DMA_CHNL_ENABLE, 0);
		}

	result = 0;
exit_err:
	if (result) {
		if (dev != NULL) {
			if (dev->virtual_mapped)
				iounmap(dev->virt_addr);

			dev->bam = NULL;
		}
	}

	mutex_unlock(&bam_dma_lock);

	return result;
}

int sps_dma_device_de_init(u32 h)
{
	struct bamdma_device *dev;
	u32 pipe_index;
	u32 chan;
	int result = 0;

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device(h);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: not registered: %x", h);
		result = SPS_ERROR;
		goto exit_err;
	}

	
	for (chan = 0; chan < dev->num_pipes / 2; chan++) {
		if (dev->chans[chan].state != DMA_CHAN_STATE_FREE) {
			SPS_ERR("sps:BAM-DMA: channel not free: %d", chan);
			result = SPS_ERROR;
			dev->chans[chan].state = DMA_CHAN_STATE_FREE;
		}
	}
	for (pipe_index = 0; pipe_index < dev->num_pipes; pipe_index++) {
		if (dev->pipes[pipe_index] != PIPE_INACTIVE) {
			SPS_ERR("sps:BAM-DMA: pipe not inactive: %d",
					pipe_index);
			result = SPS_ERROR;
			dev->pipes[pipe_index] = PIPE_INACTIVE;
		}
	}

	
	if (sps_dma_device_disable(dev))
		result = SPS_ERROR;

	dev->h = BAM_HANDLE_INVALID;
	dev->bam = NULL;
	if (dev->virtual_mapped)
		iounmap(dev->virt_addr);

exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}

int sps_dma_init(const struct sps_bam_props *bam_props)
{
	struct sps_bam_props props;
	const struct sps_bam_props *bam_reg;
	u32 h;

	
	memset(&bam_dma_dev, 0, sizeof(bam_dma_dev));
	num_bams = 0;
	memset(bam_handles, 0, sizeof(bam_handles));

	
	mutex_init(&bam_dma_lock);

	
	if (bam_props == NULL)
		return 0;

	if (bam_props->phys_addr) {
		
		bam_reg = bam_props;
		if ((bam_props->options & SPS_BAM_OPT_BAMDMA) &&
		    (bam_props->manage & SPS_BAM_MGR_MULTI_EE) == 0) {
			SPS_DBG("sps:Setting multi-EE options for BAM-DMA: %x",
				bam_props->phys_addr);
			props = *bam_props;
			props.manage |= SPS_BAM_MGR_MULTI_EE;
			bam_reg = &props;
		}

		
		if (sps_register_bam_device(bam_reg, &h)) {
			SPS_ERR("sps:Fail to register BAM-DMA BAM device: "
				"phys 0x%0x", bam_props->phys_addr);
			return SPS_ERROR;
		}

		
		if (num_bams < MAX_BAM_DMA_BAMS) {
			bam_handles[num_bams] = h;
			num_bams++;
		} else {
			SPS_ERR("sps:BAM-DMA: BAM limit exceeded: %d",
					num_bams);
			return SPS_ERROR;
		}
	} else {
		SPS_ERR("sps:BAM-DMA phys_addr is zero.");
		return SPS_ERROR;
	}


	return 0;
}

void sps_dma_de_init(void)
{
	int n;

	
	for (n = 0; n < num_bams; n++)
		sps_deregister_bam_device(bam_handles[n]);

	
	memset(&bam_dma_dev, 0, sizeof(bam_dma_dev));
	num_bams = 0;
	memset(bam_handles, 0, sizeof(bam_handles));
}

int sps_alloc_dma_chan(const struct sps_alloc_dma_chan *alloc,
		       struct sps_dma_chan *chan_info)
{
	struct bamdma_device *dev;
	struct bamdma_chan *chan;
	u32 pipe_index;
	enum bam_dma_thresh_dma thresh = (enum bam_dma_thresh_dma) 0;
	enum bam_dma_weight_dma weight = (enum bam_dma_weight_dma) 0;
	int result = SPS_ERROR;

	if (alloc == NULL || chan_info == NULL) {
		SPS_ERR("sps:sps_alloc_dma_chan. invalid parameters");
		return SPS_ERROR;
	}

	
	if (alloc->threshold != SPS_DMA_THRESHOLD_DEFAULT) {
		if (alloc->threshold >= 512)
			thresh = BAM_DMA_THRESH_512;
		else if (alloc->threshold >= 256)
			thresh = BAM_DMA_THRESH_256;
		else if (alloc->threshold >= 128)
			thresh = BAM_DMA_THRESH_128;
		else
			thresh = BAM_DMA_THRESH_64;
	}

	weight = alloc->priority;

	if ((u32)alloc->priority > (u32)BAM_DMA_WEIGHT_HIGH) {
		SPS_ERR("sps:BAM-DMA: invalid priority: %x", alloc->priority);
		return SPS_ERROR;
	}

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device(alloc->dev);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: invalid BAM handle: %x", alloc->dev);
		goto exit_err;
	}

	
	for (pipe_index = 0, chan = dev->chans;
	      pipe_index < dev->num_pipes; pipe_index += 2, chan++) {
		if (chan->state == DMA_CHAN_STATE_FREE) {
			
			if (dev->pipes[pipe_index] != PIPE_INACTIVE ||
			    dev->pipes[pipe_index + 1] != PIPE_INACTIVE) {
				SPS_ERR("sps:BAM-DMA: channel %d state "
					"error:%d %d",
					pipe_index / 2, dev->pipes[pipe_index],
				 dev->pipes[pipe_index + 1]);
				goto exit_err;
			}
			break; 
		}
	}

	if (pipe_index >= dev->num_pipes) {
		SPS_ERR("sps:BAM-DMA: no free channel. num_pipes = %d",
			dev->num_pipes);
		goto exit_err;
	}

	chan->state = DMA_CHAN_STATE_ALLOC_EXT;

	
	chan = &dev->chans[pipe_index / 2];
	chan->threshold = alloc->threshold;
	chan->thresh = thresh;
	chan->priority = alloc->priority;
	chan->weight = weight;

	SPS_DBG2("sps:sps_alloc_dma_chan. pipe %d.\n", pipe_index);

	
	chan_info->dev = dev->h;
	
	chan_info->dest_pipe_index = pipe_index;
	
	chan_info->src_pipe_index = pipe_index + 1;

	result = 0;
exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}
EXPORT_SYMBOL(sps_alloc_dma_chan);

int sps_free_dma_chan(struct sps_dma_chan *chan)
{
	struct bamdma_device *dev;
	u32 pipe_index;
	int result = 0;

	if (chan == NULL) {
		SPS_ERR("sps:sps_free_dma_chan. chan is NULL");
		return SPS_ERROR;
	}

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device(chan->dev);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: invalid BAM handle: %x", chan->dev);
		result = SPS_ERROR;
		goto exit_err;
	}

	
	pipe_index = chan->dest_pipe_index;
	if (pipe_index >= dev->num_pipes || ((pipe_index & 1)) ||
	    (pipe_index + 1) != chan->src_pipe_index) {
		SPS_ERR("sps:sps_free_dma_chan. Invalid pipe indices."
			"num_pipes=%d.dest=%d.src=%d.",
			dev->num_pipes,
			chan->dest_pipe_index,
			chan->src_pipe_index);
		result = SPS_ERROR;
		goto exit_err;
	}

	
	if (dev->chans[pipe_index / 2].state != DMA_CHAN_STATE_ALLOC_EXT ||
	    dev->pipes[pipe_index] != PIPE_INACTIVE ||
	    dev->pipes[pipe_index + 1] != PIPE_INACTIVE) {
		SPS_ERR("sps:BAM-DMA: attempt to free active chan %d: %d %d",
			pipe_index / 2, dev->pipes[pipe_index],
			dev->pipes[pipe_index + 1]);
		result = SPS_ERROR;
		goto exit_err;
	}

	
	dev->chans[pipe_index / 2].state = DMA_CHAN_STATE_FREE;

exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}
EXPORT_SYMBOL(sps_free_dma_chan);

static u32 sps_dma_check_pipes(struct bamdma_device *dev, u32 pipe_index)
{
	u32 pipe_in;
	u32 pipe_out;
	int enabled_in;
	int enabled_out;
	u32 check;

	pipe_in = pipe_index & ~1;
	pipe_out = pipe_in + 1;
	enabled_in = bam_pipe_is_enabled(dev->bam->base, pipe_in);
	enabled_out = bam_pipe_is_enabled(dev->bam->base, pipe_out);

	if (!enabled_in && !enabled_out)
		check = DMA_PIPES_BOTH_DISABLED;
	else if (enabled_in && enabled_out)
		check = DMA_PIPES_BOTH_ENABLED;
	else
		check = DMA_PIPES_STATE_DIFF;

	return check;
}

int sps_dma_pipe_alloc(void *bam_arg, u32 pipe_index, enum sps_mode dir)
{
	struct sps_bam *bam = bam_arg;
	struct bamdma_device *dev;
	struct bamdma_chan *chan;
	u32 channel;
	int result = SPS_ERROR;

	if (bam == NULL) {
		SPS_ERR("sps:BAM context is NULL");
		return SPS_ERROR;
	}

	
	if ((DMA_PIPE_IS_DEST(pipe_index) && dir != SPS_MODE_DEST) ||
	    (DMA_PIPE_IS_SRC(pipe_index) && dir != SPS_MODE_SRC)) {
		SPS_ERR("sps:BAM-DMA: wrong direction for BAM %x pipe %d",
			bam->props.phys_addr, pipe_index);
		return SPS_ERROR;
	}

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device((u32) bam);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: invalid BAM: %x",
			bam->props.phys_addr);
		goto exit_err;
	}
	if (pipe_index >= dev->num_pipes) {
		SPS_ERR("sps:BAM-DMA: BAM %x invalid pipe: %d",
			bam->props.phys_addr, pipe_index);
		goto exit_err;
	}
	if (dev->pipes[pipe_index] != PIPE_INACTIVE) {
		SPS_ERR("sps:BAM-DMA: BAM %x pipe %d already active",
			bam->props.phys_addr, pipe_index);
		goto exit_err;
	}

	
	dev->pipes[pipe_index] = PIPE_ACTIVE;

	
	channel = pipe_index / 2;
	chan = &dev->chans[channel];
	if (chan->state != DMA_CHAN_STATE_ALLOC_EXT &&
	    chan->state != DMA_CHAN_STATE_ALLOC_INT) {
		chan->state = DMA_CHAN_STATE_ALLOC_INT;
	}

	result = 0;
exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}

int sps_dma_pipe_enable(void *bam_arg, u32 pipe_index)
{
	struct sps_bam *bam = bam_arg;
	struct bamdma_device *dev;
	struct bamdma_chan *chan;
	u32 channel;
	int result = SPS_ERROR;

	SPS_DBG2("sps:sps_dma_pipe_enable.pipe %d", pipe_index);

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device((u32) bam);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: invalid BAM");
		goto exit_err;
	}
	if (pipe_index >= dev->num_pipes) {
		SPS_ERR("sps:BAM-DMA: BAM %x invalid pipe: %d",
			bam->props.phys_addr, pipe_index);
		goto exit_err;
	}
	if (dev->pipes[pipe_index] != PIPE_ACTIVE) {
		SPS_ERR("sps:BAM-DMA: BAM %x pipe %d not active",
			bam->props.phys_addr, pipe_index);
		goto exit_err;
	}

	if (DMA_PIPE_IS_DEST(pipe_index)) {
		
		channel = pipe_index / 2;
		chan = &dev->chans[channel];

		if (chan->threshold != SPS_DMA_THRESHOLD_DEFAULT)
			dma_write_reg_field(dev->virt_addr,
					    DMA_CHNL_CONFIG(channel),
					    DMA_CHNL_ACT_THRESH,
					    chan->thresh);

		if (chan->priority != SPS_DMA_PRI_DEFAULT)
			dma_write_reg_field(dev->virt_addr,
					    DMA_CHNL_CONFIG(channel),
					    DMA_CHNL_WEIGHT,
					    chan->weight);

		dma_write_reg_field(dev->virt_addr,
				    DMA_CHNL_CONFIG(channel),
				    DMA_CHNL_ENABLE, 1);
	}

	result = 0;
exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}

static int sps_dma_deactivate_pipe_atomic(struct bamdma_device *dev,
					  struct sps_bam *bam,
					  u32 pipe_index)
{
	u32 channel;

	if (dev->bam != bam)
		return SPS_ERROR;
	if (pipe_index >= dev->num_pipes)
		return SPS_ERROR;
	if (dev->pipes[pipe_index] != PIPE_ACTIVE)
		return SPS_ERROR;	

	SPS_DBG2("sps:BAM-DMA: deactivate pipe %d", pipe_index);

	
	dev->pipes[pipe_index] = PIPE_INACTIVE;

	channel = pipe_index / 2;
	dma_write_reg_field(dev->virt_addr, DMA_CHNL_CONFIG(channel),
			    DMA_CHNL_ENABLE, 0);

	
	if (sps_dma_check_pipes(dev, pipe_index) == DMA_PIPES_BOTH_DISABLED) {
		
		if (dev->chans[channel].state == DMA_CHAN_STATE_ALLOC_INT)
			dev->chans[channel].state = DMA_CHAN_STATE_FREE;
	}

	return 0;
}

int sps_dma_pipe_free(void *bam_arg, u32 pipe_index)
{
	struct bamdma_device *dev;
	struct sps_bam *bam = bam_arg;
	int result;

	mutex_lock(&bam_dma_lock);

	dev = sps_dma_find_device((u32) bam);
	if (dev == NULL) {
		SPS_ERR("sps:BAM-DMA: invalid BAM");
		result = SPS_ERROR;
		goto exit_err;
	}

	result = sps_dma_deactivate_pipe_atomic(dev, bam, pipe_index);

exit_err:
	mutex_unlock(&bam_dma_lock);

	return result;
}

u32 sps_dma_get_bam_handle(void)
{
	return (u32) bam_dma_dev[0].bam;
}
EXPORT_SYMBOL(sps_dma_get_bam_handle);

void sps_dma_free_bam_handle(u32 h)
{
}
EXPORT_SYMBOL(sps_dma_free_bam_handle);

#endif 
