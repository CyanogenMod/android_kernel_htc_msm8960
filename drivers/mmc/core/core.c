/*
 *  linux/drivers/mmc/core/core.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  SD support Copyright (C) 2004 Ian Molton, All Rights Reserved.
 *  Copyright (C) 2005-2008 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/scatterlist.h>
#include <linux/log2.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/fault-inject.h>
#include <linux/random.h>
#include <linux/wakelock.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/statfs.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include "core.h"
#include "bus.h"
#include "host.h"
#include "sdio_bus.h"

#include "mmc_ops.h"
#include "sd_ops.h"
#include "sdio_ops.h"

#define MMC_BKOPS_MAX_TIMEOUT    (4 * 60 * 1000) 

#define CREATE_TRACE_POINTS
#include <trace/events/mmcio.h>

static struct workqueue_struct *workqueue;
struct workqueue_struct *stats_workqueue = NULL;

static struct wake_lock mmc_removal_work_wake_lock;

struct timer_list sd_remove_tout_timer;
bool use_spi_crc = 1;
module_param(use_spi_crc, bool, 0);

#ifdef CONFIG_MMC_UNSAFE_RESUME
bool mmc_assume_removable;
#else
bool mmc_assume_removable = 1;
#endif
EXPORT_SYMBOL(mmc_assume_removable);
module_param_named(removable, mmc_assume_removable, bool, 0644);
MODULE_PARM_DESC(
	removable,
	"MMC/SD cards are removable and may be removed during suspend");
#ifdef SD_DEBOUNCE_DEBUG
extern int mmc_is_sd_host(struct mmc_host *mmc);
#endif

static int stats_interval = MMC_STATS_INTERVAL;
#define K(x) ((x) << (PAGE_SHIFT - 10))
void mmc_stats(struct work_struct *work)
{
	struct mmc_host *host =
		container_of(work, struct mmc_host, stats_work.work);
	unsigned long rtime, wtime;
	unsigned long rbytes, wbytes, rcnt, wcnt;
	unsigned long wperf = 0, rperf = 0;
	unsigned long flags;
	u64 val;
	struct kstatfs stat;
	unsigned long free = 0;
	int reset_low_perf_data = 1;
	
	unsigned long rtime_rand = 0, wtime_rand = 0;
	unsigned long rbytes_rand = 0, wbytes_rand = 0, rcnt_rand = 0, wcnt_rand = 0;
	unsigned long wperf_rand = 0, rperf_rand = 0;
	
	if (!host || !host->perf_enable || !stats_workqueue)
		return;

	spin_lock_irqsave(&host->lock, flags);

	rbytes = host->perf.rbytes_drv;
	wbytes = host->perf.wbytes_drv;
	rcnt = host->perf.rcount;
	wcnt = host->perf.wcount;
	rtime = (unsigned long)ktime_to_us(host->perf.rtime_drv);
	wtime = (unsigned long)ktime_to_us(host->perf.wtime_drv);
	host->perf.rbytes_drv = host->perf.wbytes_drv = 0;
	host->perf.rcount = host->perf.wcount = 0;
	host->perf.rtime_drv = ktime_set(0, 0);
	host->perf.wtime_drv = ktime_set(0, 0);
	
	if (host->debug_mask & MMC_DEBUG_RANDOM_RW) {
		rbytes_rand = host->perf.rbytes_drv_rand;
		wbytes_rand = host->perf.wbytes_drv_rand;
		rcnt_rand = host->perf.rcount_rand;
		wcnt_rand = host->perf.wcount_rand;
		rtime_rand = (unsigned long)ktime_to_us(host->perf.rtime_drv_rand);
		wtime_rand = (unsigned long)ktime_to_us(host->perf.wtime_drv_rand);

		host->perf.rbytes_drv_rand = host->perf.wbytes_drv_rand = 0;
		host->perf.rcount_rand = host->perf.wcount_rand = 0;
		host->perf.rtime_drv_rand = ktime_set(0, 0);
		host->perf.wtime_drv_rand = ktime_set(0, 0);

	}
	

	spin_unlock_irqrestore(&host->lock, flags);

	if (wtime) {
		val = ((u64)wbytes / 1024) * 1000000;
		do_div(val, wtime);
		wperf = (unsigned long)val;
	}
	if (rtime) {
		val = ((u64)rbytes / 1024) * 1000000;
		do_div(val, rtime);
		rperf = (unsigned long)val;
	}

	if (host->debug_mask & MMC_DEBUG_FREE_SPACE) {
		struct file *file;
		file = filp_open("/data", O_RDONLY, 0);
		if (!IS_ERR(file)) {
			vfs_statfs(&file->f_path, &stat);
			filp_close(file, NULL);
			free = (unsigned long)stat.f_bfree;
			free /= 256; 
		}
	}

	
	wtime /= 1000;
	rtime /= 1000;

	
	if (!wtime)
		reset_low_perf_data = 0;
	else if (wperf < 100) {
		host->perf.lp_duration += stats_interval; 
		host->perf.wbytes_low_perf += wbytes;
		host->perf.wtime_low_perf += wtime; 
		if (host->perf.lp_duration >= 600000) {
			unsigned long perf;
			val = ((u64)host->perf.wbytes_low_perf / 1024) * 1000;
			do_div(val, host->perf.wtime_low_perf);
			perf = (unsigned long)val;
			pr_err("%s Statistics: write %lu KB in %lu ms, perf %lu KB/s, duration %lu sec\n",
					mmc_hostname(host), host->perf.wbytes_low_perf / 1024,
					host->perf.wtime_low_perf,
					perf, host->perf.lp_duration / 1000);
		} else
			reset_low_perf_data = 0;
	}
	if (host->perf.lp_duration && reset_low_perf_data) {
		host->perf.lp_duration = 0;
		host->perf.wbytes_low_perf = 0;
		host->perf.wtime_low_perf = 0;
	}

	
	if ((wtime > 500) || (wtime && (stats_interval == MMC_STATS_LOG_INTERVAL)))  {
		#if 0
		pr_info("%s Statistics: dirty %luKB, writeback %luKB\n", mmc_hostname(host),
				K(global_page_state(NR_FILE_DIRTY)), K(global_page_state(NR_WRITEBACK)));
		#endif
		if (host->debug_mask & MMC_DEBUG_FREE_SPACE)
			pr_info("%s Statistics: write %lu KB in %lu ms, perf %lu KB/s, rq %lu, /data free %lu MB\n",
					mmc_hostname(host), wbytes / 1024, wtime, wperf, wcnt, free);
		else
			pr_info("%s Statistics: write %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
					mmc_hostname(host), wbytes / 1024, wtime, wperf, wcnt);
	}
	if ((rtime > 500) || (rtime && (stats_interval == MMC_STATS_LOG_INTERVAL)))  {
		if (host->debug_mask & MMC_DEBUG_FREE_SPACE)
			pr_info("%s Statistics: read %lu KB in %lu ms, perf %lu KB/s, rq %lu, /data free %lu MB\n",
					mmc_hostname(host), rbytes / 1024, rtime, rperf, rcnt, free);
		else
			pr_info("%s Statistics: read %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
					mmc_hostname(host), rbytes / 1024, rtime, rperf, rcnt);
	}

	
	if (host->debug_mask & MMC_DEBUG_RANDOM_RW) {
		if (wtime_rand) {
			val = ((u64)wbytes_rand / 1024) * 1000000;
			do_div(val, wtime_rand);
			wperf_rand = (unsigned long)val;
		}
		if (rtime_rand) {
			val = ((u64)rbytes_rand / 1024) * 1000000;
			do_div(val, rtime_rand);
			rperf_rand = (unsigned long)val;
		}
		wtime_rand /= 1000;
		rtime_rand /= 1000;
		if (wperf_rand && wtime_rand) {
			if (host->debug_mask & MMC_DEBUG_FREE_SPACE)
				pr_info("%s Statistics: random write %lu KB in %lu ms, perf %lu KB/s, rq %lu, /data free %lu MB\n",
						mmc_hostname(host), wbytes_rand / 1024, wtime_rand, wperf_rand, wcnt_rand, free);
			else
				pr_info("%s Statistics: random write %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
						mmc_hostname(host), wbytes_rand / 1024, wtime_rand, wperf_rand, wcnt_rand);
		}
		if (rperf_rand && rtime_rand) {
			if (host->debug_mask & MMC_DEBUG_FREE_SPACE)
				pr_info("%s Statistics: random read %lu KB in %lu ms, perf %lu KB/s, rq %lu, /data free %lu MB\n",
						mmc_hostname(host), rbytes_rand / 1024, rtime_rand, rperf_rand, rcnt_rand, free);
			else
				pr_info("%s Statistics: random read %lu KB in %lu ms, perf %lu KB/s, rq %lu\n",
						mmc_hostname(host), rbytes_rand / 1024, rtime_rand, rperf_rand, rcnt_rand);
		}
	}
	

	if (host->debug_mask & MMC_DEBUG_MEMORY) {
		struct sysinfo mi;
		long cached;
		si_meminfo(&mi);
		cached = global_page_state(NR_FILE_PAGES) -
			total_swapcache_pages - mi.bufferram;
		pr_info("meminfo: total %lu KB, free %lu KB, buffers %lu KB, cached %lu KB \n",
				K(mi.totalram),
				K(mi.freeram),
				K(mi.bufferram),
				K(cached));
	}

	queue_delayed_work(stats_workqueue, &host->stats_work, msecs_to_jiffies(stats_interval));
	return;
}
int mmc_schedule_card_removal_work(struct delayed_work *work,
                                    unsigned long delay)
{
       wake_lock(&mmc_removal_work_wake_lock);
       return queue_delayed_work(workqueue, work, delay);
}

static int mmc_schedule_delayed_work(struct delayed_work *work,
				     unsigned long delay)
{
	return queue_delayed_work(workqueue, work, delay);
}

static void mmc_flush_scheduled_work(void)
{
	flush_workqueue(workqueue);
}

#ifdef CONFIG_FAIL_MMC_REQUEST

static void mmc_should_fail_request(struct mmc_host *host,
				    struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	static const int data_errors[] = {
		-ETIMEDOUT,
		-EILSEQ,
		-EIO,
	};

	if (!data)
		return;

	if (cmd->error || data->error ||
	    !should_fail(&host->fail_mmc_request, data->blksz * data->blocks))
		return;

	data->error = data_errors[random32() % ARRAY_SIZE(data_errors)];
	data->bytes_xfered = (random32() % (data->bytes_xfered >> 9)) << 9;
}

#else 

static inline void mmc_should_fail_request(struct mmc_host *host,
					   struct mmc_request *mrq)
{
}

#endif 

void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
	struct mmc_command *cmd = mrq->cmd;
	int err = cmd->error;
	ktime_t diff;

	if (err && cmd->retries && mmc_host_is_spi(host)) {
		if (cmd->resp[0] & R1_SPI_ILLEGAL_COMMAND)
			cmd->retries = 0;
	}

	if (err && cmd->retries && !mmc_card_removed(host->card)) {
		if (mrq->done)
			mrq->done(mrq);
	} else {
		mmc_should_fail_request(host, mrq);

		

		pr_debug("%s: req done (CMD%u): %d: %08x %08x %08x %08x\n",
			mmc_hostname(host), cmd->opcode, err,
			cmd->resp[0], cmd->resp[1],
			cmd->resp[2], cmd->resp[3]);

		if (mrq->data) {
			if (host->perf_enable) {
				unsigned long flags;
				diff = ktime_sub(ktime_get(), host->perf.start);
				if (host->tp_enable)
					trace_mmc_request_done(&host->class_dev,
							cmd->opcode, mrq->cmd->arg,
							mrq->data->blocks, ktime_to_ms(diff));
				spin_lock_irqsave(&host->lock, flags);
				if (mrq->data->flags == MMC_DATA_READ) {
					host->perf.rbytes_drv +=
							mrq->data->bytes_xfered;
					host->perf.rtime_drv =
						ktime_add(host->perf.rtime_drv,
							diff);
					host->perf.rcount++;
					if (host->debug_mask & MMC_DEBUG_RANDOM_RW) {
						if (mrq->data->bytes_xfered <= 32*1024) {
							host->perf.rbytes_drv_rand +=
								mrq->data->bytes_xfered;
							host->perf.rtime_drv_rand =
								ktime_add(host->perf.rtime_drv_rand,
										diff);
							host->perf.rcount_rand++;
						}
					}
				} else {
					host->perf.wbytes_drv +=
						mrq->data->bytes_xfered;
					host->perf.wtime_drv =
						ktime_add(host->perf.wtime_drv,
							diff);
					host->perf.wcount++;
					if (host->debug_mask & MMC_DEBUG_RANDOM_RW) {
						if (mrq->data->bytes_xfered <= 32*1024) {
							host->perf.wbytes_drv_rand +=
								mrq->data->bytes_xfered;
							host->perf.wtime_drv_rand =
								ktime_add(host->perf.wtime_drv_rand,
										diff);
							host->perf.wcount_rand++;
						}
					}
				}
				spin_unlock_irqrestore(&host->lock, flags);
			}
			pr_debug("%s:     %d bytes transferred: %d\n",
				mmc_hostname(host),
				mrq->data->bytes_xfered, mrq->data->error);
		}

		if (mrq->stop) {
			pr_debug("%s:     (CMD%u): %d: %08x %08x %08x %08x\n",
				mmc_hostname(host), mrq->stop->opcode,
				mrq->stop->error,
				mrq->stop->resp[0], mrq->stop->resp[1],
				mrq->stop->resp[2], mrq->stop->resp[3]);
		}

		if (mrq->done)
			mrq->done(mrq);

		mmc_host_clk_release(host);
	}
}

EXPORT_SYMBOL(mmc_request_done);

static void
mmc_start_request(struct mmc_host *host, struct mmc_request *mrq)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned int i, sz;
	struct scatterlist *sg;
#endif

	if (mrq->sbc) {
		pr_debug("<%s: starting CMD%u arg %08x flags %08x>\n",
			 mmc_hostname(host), mrq->sbc->opcode,
			 mrq->sbc->arg, mrq->sbc->flags);
	}

	pr_debug("%s: starting CMD%u arg %08x flags %08x\n",
		 mmc_hostname(host), mrq->cmd->opcode,
		 mrq->cmd->arg, mrq->cmd->flags);

	if (mrq->data) {
		pr_debug("%s:     blksz %d blocks %d flags %08x "
			"tsac %d ms nsac %d\n",
			mmc_hostname(host), mrq->data->blksz,
			mrq->data->blocks, mrq->data->flags,
			mrq->data->timeout_ns / 1000000,
			mrq->data->timeout_clks);
		if (host->tp_enable)
			trace_mmc_req_start(&host->class_dev, mrq->cmd->opcode,
				mrq->cmd->arg, mrq->data->blocks);
	}

	if (mrq->stop) {
		pr_debug("%s:     CMD%u arg %08x flags %08x\n",
			 mmc_hostname(host), mrq->stop->opcode,
			 mrq->stop->arg, mrq->stop->flags);
	}

	WARN_ON(!host->claimed);

	mrq->cmd->error = 0;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		BUG_ON(mrq->data->blksz > host->max_blk_size);
		BUG_ON(mrq->data->blocks > host->max_blk_count);
		BUG_ON(mrq->data->blocks * mrq->data->blksz >
			host->max_req_size);

#ifdef CONFIG_MMC_DEBUG
		sz = 0;
		for_each_sg(mrq->data->sg, sg, mrq->data->sg_len, i)
			sz += sg->length;
		BUG_ON(sz != mrq->data->blocks * mrq->data->blksz);
#endif

		mrq->cmd->data = mrq->data;
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
		if (mrq->stop) {
			mrq->data->stop = mrq->stop;
			mrq->stop->error = 0;
			mrq->stop->mrq = mrq;
		}
		if (host->perf_enable)
			host->perf.start = ktime_get();
	}
	mmc_host_clk_hold(host);
	
	host->ops->request(host, mrq);
}

int mmc_card_start_sanitize(struct mmc_card *card)
{
	int err = 0;
	unsigned long flags;
	struct mmc_host *host = card->host;
	if (!(card->ext_csd.sec_feature_support & EXT_CSD_SEC_SANITIZE)) {
		pr_info("%s: sanitize not support\n", mmc_hostname(host));
		return -1;
	}

	mmc_claim_host(host);
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_SANITIZE_START, 1, 0);
	if (err)
		pr_err("%s: error %d starting sanitize\n",
			   mmc_hostname(host), err);
	else {
		pr_info("%s: start sanitize\n", mmc_hostname(host));
		spin_lock_irqsave(&host->lock, flags);
		mmc_card_set_doing_sanitize(card);
		spin_unlock_irqrestore(&host->lock, flags);
	}
	mmc_release_host(host);

	return err;
}

int mmc_card_start_bkops(struct mmc_card *card)
{
	int err = 0;
	unsigned long flags;
	struct mmc_host *host = card->host;

	mmc_claim_host(host);
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BKOPS_START, 1, 0);
	if (err)
		pr_err("%s: error %d starting bkops\n",
			   mmc_hostname(host), err);
	else {
		pr_info("%s: start bkops\n", mmc_hostname(host));
		spin_lock_irqsave(&host->lock, flags);
		mmc_card_set_doing_bkops(card);
		spin_unlock_irqrestore(&host->lock, flags);
	}
	mmc_release_host(host);
	return err;
}

int mmc_card_stop_work(struct mmc_card *card, int work, int *complete)
{
	int err = -1;
	u32 status;
	unsigned long flags;
	struct mmc_host *host = card->host;

	mmc_claim_host(host);
	err = mmc_send_status(card, &status);
	mmc_release_host(host);
	if (err) {
		pr_err("%s: Get card status fail, work %d err %d\n",
			mmc_hostname(host), work, err);
		goto out;
	}
	if (R1_CURRENT_STATE(status) == R1_STATE_PRG) {
		err = mmc_interrupt_hpi(card);
		if (err)
			pr_err("%s: send hpi fail, work %d err %d\n",
				mmc_hostname(host), work, err);
	} else
		*complete = 1;
out:
	if (work == MMC_WORK_BKOPS) {
		pr_info("%s: bkops %s\n", mmc_hostname(host),
			*complete ? "completed" : "interrupted");
		spin_lock_irqsave(&host->lock, flags);
		mmc_card_clr_doing_bkops(card);
		spin_unlock_irqrestore(&host->lock, flags);
	} else if (work == MMC_WORK_SANITIZE) {
		pr_info("%s: sanitize %s\n", mmc_hostname(host),
			*complete ? "completed" : "interrupted");
		spin_lock_irqsave(&host->lock, flags);
		mmc_card_clr_doing_sanitize(card);
		spin_unlock_irqrestore(&host->lock, flags);
	}
	return err;
}

void mmc_start_bkops(struct mmc_card *card)
{
	int err;
	unsigned long flags;
	int timeout;
	int is_storage_encrypting = 0;
	int urgent_bkops = 0;
	int do_bkops = 0;

	BUG_ON(!card);
	if (!card->ext_csd.bkops_en || !(card->host->caps2 & MMC_CAP2_BKOPS))
		return;

	if (card->host->bkops_trigger == ENCRYPT_MAGIC_NUMBER)
		is_storage_encrypting = 1;

	if (!mmc_card_doing_bkops(card) && (mmc_card_check_bkops(card) || is_storage_encrypting)) {
		spin_lock_irqsave(&card->host->lock, flags);
		mmc_card_clr_check_bkops(card);
		spin_unlock_irqrestore(&card->host->lock, flags);
		urgent_bkops = mmc_is_exception_event(card, EXT_CSD_URGENT_BKOPS);
		if (urgent_bkops || card->bkops_check_status || is_storage_encrypting) {
			if (card->ext_csd.raw_bkops_status >= EXT_CSD_BKOPS_LEVEL_2 || is_storage_encrypting) {
				spin_lock_irqsave(&card->host->lock, flags);
				do_bkops = 1;
				spin_unlock_irqrestore(&card->host->lock, flags);
			}
		}
	}

	if (mmc_card_doing_bkops(card) || !do_bkops || card->host->bkops_trigger == ENCRYPT_MAGIC_NUMBER2) {
		spin_lock_irqsave(&card->host->lock, flags);
		mmc_card_clr_check_bkops(card);
		spin_unlock_irqrestore(&card->host->lock, flags);
		return;
	}

	mmc_claim_host(card->host);

	timeout = (card->ext_csd.raw_bkops_status >= EXT_CSD_BKOPS_LEVEL_2) ?
		MMC_BKOPS_MAX_TIMEOUT : 0;

	if (is_storage_encrypting)
		timeout = 50000;

	pr_info("%s: %s, level %d\n", mmc_hostname(card->host), __func__,
		card->ext_csd.raw_bkops_status);
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_BKOPS_START, 1, timeout);
	if (err) {
		pr_warning("%s: error %d starting bkops\n",
			   mmc_hostname(card->host), err);
		goto out;
	}

	spin_lock_irqsave(&card->host->lock, flags);

	mmc_card_set_doing_bkops(card);
	if (card->ext_csd.raw_bkops_status >= EXT_CSD_BKOPS_LEVEL_2) {
		card->host->bkops_trigger = timeout;
		if (card->need_sanitize)
			mmc_card_set_need_sanitize(card);
		mmc_card_set_need_bkops(card);
	}
	spin_unlock_irqrestore(&card->host->lock, flags);
out:
	mmc_release_host(card->host);
}
EXPORT_SYMBOL(mmc_start_bkops);

static void mmc_wait_done(struct mmc_request *mrq)
{
	complete(&mrq->completion);
}

static int __mmc_start_req(struct mmc_host *host, struct mmc_request *mrq)
{
	init_completion(&mrq->completion);
	mrq->done = mmc_wait_done;
	if (mmc_card_removed(host->card)) {
		mrq->cmd->error = -ENOMEDIUM;
		complete(&mrq->completion);
		return -ENOMEDIUM;
	}
	mmc_start_request(host, mrq);
	return 0;
}

static void mmc_wait_for_req_done(struct mmc_host *host,
				  struct mmc_request *mrq)
{
	struct mmc_command *cmd;

	while (1) {
		wait_for_completion_io(&mrq->completion);	

		cmd = mrq->cmd;
		if (is_wifi_mmc_host(host)) {
			if (!cmd->error || !cmd->retries)
			break;
		} else {
			if (!cmd->error || !cmd->retries ||
				mmc_card_removed(host->card))
				break;
		}
		pr_debug("%s: req failed (CMD%u): %d, retrying...\n",
			 mmc_hostname(host), cmd->opcode, cmd->error);
		cmd->retries--;
		cmd->error = 0;
		host->ops->request(host, mrq);
	}
}

static void mmc_pre_req(struct mmc_host *host, struct mmc_request *mrq,
		 bool is_first_req)
{
	if (host->ops->pre_req) {
		mmc_host_clk_hold(host);
		host->ops->pre_req(host, mrq, is_first_req);
		mmc_host_clk_release(host);
	}
}

static void mmc_post_req(struct mmc_host *host, struct mmc_request *mrq,
			 int err)
{
	if (host->ops->post_req) {
		mmc_host_clk_hold(host);
		host->ops->post_req(host, mrq, err);
		mmc_host_clk_release(host);
	}
}

struct mmc_async_req *mmc_start_req(struct mmc_host *host,
				    struct mmc_async_req *areq, int *error)
{
	int err = 0;
	int start_err = 0;
	struct mmc_async_req *data = host->areq;

	
	if (areq)
		mmc_pre_req(host, areq->mrq, !host->areq);

	if (host->areq) {
		ktime_t io_diff = ktime_get(), wait_diff = ktime_get(), wait_ready = ktime_get();
		mmc_wait_for_req_done(host, host->areq->mrq);

		if (mmc_card_sd(host->card)) {
			io_diff = ktime_sub(ktime_get(), host->areq->rq_stime);
			if (ktime_to_us(io_diff) > 400000)
				pr_info("%s (%s), finish(1) cmd(%d) s_sec %d, size %d, time = %lld us\n",
				mmc_hostname(host), current->comm, host->areq->mrq->cmd->opcode,
				host->areq->mrq->cmd->arg , host->areq->mrq->data->blocks , ktime_to_us(io_diff));
			wait_ready = ktime_get();
		}

		err = host->areq->err_check(host->card, host->areq);
		if (mmc_card_sd(host->card)) {
			wait_diff = ktime_sub(ktime_get(), wait_ready);
			if (ktime_to_us(io_diff) > 400000)
				pr_info("%s (%s), finish(2) cmd(%d) s_sec %d, size %d, ready time = %lld us, total time = %lld us\n",
				mmc_hostname(host), current->comm, host->areq->mrq->cmd->opcode, host->areq->mrq->cmd->arg,
				host->areq->mrq->data->blocks, ktime_to_us(wait_diff), ktime_to_us(io_diff));
		}
	}

	if (!err && areq) {
		if (mmc_card_sd(host->card))
			areq->rq_stime = ktime_get();
		start_err = __mmc_start_req(host, areq->mrq);
	}
	if (host->areq)
		mmc_post_req(host, host->areq->mrq, 0);

	
	if ((err || start_err) && areq)
			mmc_post_req(host, areq->mrq, -EINVAL);

	if (err)
		host->areq = NULL;
	else
		host->areq = areq;

	if (error)
		*error = err;
	return data;
}
EXPORT_SYMBOL(mmc_start_req);

void mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
	__mmc_start_req(host, mrq);
	mmc_wait_for_req_done(host, mrq);
}
EXPORT_SYMBOL(mmc_wait_for_req);

int mmc_interrupt_hpi(struct mmc_card *card)
{
	int err;
	u32 status;

	BUG_ON(!card);

	if (!card->ext_csd.hpi_en) {
		pr_info("%s: HPI enable bit unset\n", mmc_hostname(card->host));
		return 1;
	}

	mmc_claim_host(card->host);
	err = mmc_send_status(card, &status);
	if (err) {
		pr_err("%s: Get card status fail\n", mmc_hostname(card->host));
		goto out;
	}

	if (R1_CURRENT_STATE(status) == R1_STATE_PRG) {
		do {
			err = mmc_send_hpi_cmd(card, &status);
			if (err)
				pr_debug("%s: abort HPI (%d error)\n",
					 mmc_hostname(card->host), err);

			err = mmc_send_status(card, &status);
			if (err)
				break;
		} while (!(status & R1_READY_FOR_DATA) || (R1_CURRENT_STATE(status) == R1_STATE_PRG));
	} else
		pr_debug("%s: Left prg-state\n", mmc_hostname(card->host));

out:
	mmc_release_host(card->host);
	return err;
}
EXPORT_SYMBOL(mmc_interrupt_hpi);

int mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd, int retries)
{
	struct mmc_request mrq = {NULL};

	WARN_ON(!host->claimed);

	memset(cmd->resp, 0, sizeof(cmd->resp));
	cmd->retries = retries;

	mrq.cmd = cmd;
	cmd->data = NULL;

	mmc_wait_for_req(host, &mrq);

	return cmd->error;
}

EXPORT_SYMBOL(mmc_wait_for_cmd);

int mmc_interrupt_bkops(struct mmc_card *card)
{
	int err = 0;
	unsigned long flags;

	BUG_ON(!card);

	err = mmc_interrupt_hpi(card);

	spin_lock_irqsave(&card->host->lock, flags);
	mmc_card_clr_doing_bkops(card);
	mmc_card_clr_doing_sanitize(card);
	spin_unlock_irqrestore(&card->host->lock, flags);
	if (err)
		pr_err("%s: send hpi fail : %d\n",
		       mmc_hostname(card->host), err);
	else
		pr_err("%s: send hpi done : %d\n",
		       mmc_hostname(card->host), err);
	return err;
}
EXPORT_SYMBOL(mmc_interrupt_bkops);

int mmc_read_bkops_status(struct mmc_card *card)
{
	int err;
	u8 ext_csd[512];

	mmc_claim_host(card->host);
	err = mmc_send_ext_csd(card, ext_csd);
	mmc_release_host(card->host);
	if (err)
		return err;

	card->ext_csd.raw_bkops_status = ext_csd[EXT_CSD_BKOPS_STATUS];
	card->ext_csd.raw_exception_status = ext_csd[EXT_CSD_EXP_EVENTS_STATUS];

	return 0;
}
EXPORT_SYMBOL(mmc_read_bkops_status);

int mmc_is_exception_event(struct mmc_card *card, unsigned int value)
{
	int err;

	err = mmc_read_bkops_status(card);
	if (err) {
		pr_err("%s: Didn't read bkops status : %d\n",
		       mmc_hostname(card->host), err);
		return 0;
	}

	
	if (card->ext_csd.rev == 5)
		return 1;

	return (card->ext_csd.raw_exception_status & value) ? 1 : 0;
}
EXPORT_SYMBOL(mmc_is_exception_event);

void mmc_set_data_timeout(struct mmc_data *data, const struct mmc_card *card)
{
	unsigned int mult;

	if (mmc_card_sdio(card)) {
		data->timeout_ns = 1000000000;
		data->timeout_clks = 0;
		return;
	}

	mult = mmc_card_sd(card) ? 100 : 10;

	if (data->flags & MMC_DATA_WRITE)
		mult <<= card->csd.r2w_factor;

	data->timeout_ns = card->csd.tacc_ns * mult;
	data->timeout_clks = card->csd.tacc_clks * mult;

	if (mmc_card_sd(card)) {
		unsigned int timeout_us, limit_us;

		timeout_us = data->timeout_ns / 1000;
		if (mmc_host_clk_rate(card->host))
			timeout_us += data->timeout_clks * 1000 /
				(mmc_host_clk_rate(card->host) / 1000);

		if (data->flags & MMC_DATA_WRITE)
			limit_us = 3000000;
		else
			limit_us = 250000;

		if (timeout_us > limit_us || mmc_card_blockaddr(card)) {
			data->timeout_ns = limit_us * 1000;
			data->timeout_clks = 0;
		}
	}

	if (mmc_card_long_read_time(card) && data->flags & MMC_DATA_READ) {
		data->timeout_ns = 300000000;
		data->timeout_clks = 0;
	}

	if (mmc_host_is_spi(card->host)) {
		if (data->flags & MMC_DATA_WRITE) {
			if (data->timeout_ns < 1000000000)
				data->timeout_ns = 1000000000;	
		} else {
			if (data->timeout_ns < 100000000)
				data->timeout_ns =  100000000;	
		}
	}
	
	if (card->quirks & MMC_QUIRK_INAND_DATA_TIMEOUT) {
		data->timeout_ns = 4000000000u; 
		data->timeout_clks = 0;
	}
}
EXPORT_SYMBOL(mmc_set_data_timeout);

unsigned int mmc_align_data_size(struct mmc_card *card, unsigned int sz)
{
	sz = ((sz + 3) / 4) * 4;

	return sz;
}
EXPORT_SYMBOL(mmc_align_data_size);

int __mmc_claim_host(struct mmc_host *host, atomic_t *abort)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long flags;
	int stop;

	might_sleep();

	add_wait_queue(&host->wq, &wait);

	spin_lock_irqsave(&host->lock, flags);
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		stop = abort ? atomic_read(abort) : 0;
		if (stop || !host->claimed || host->claimer == current)
			break;
		spin_unlock_irqrestore(&host->lock, flags);
		schedule();
		spin_lock_irqsave(&host->lock, flags);
	}
	set_current_state(TASK_RUNNING);
	if (!stop) {
		host->claimed = 1;
		host->claimer = current;
		host->claim_cnt += 1;
	} else
		wake_up(&host->wq);
	spin_unlock_irqrestore(&host->lock, flags);
	remove_wait_queue(&host->wq, &wait);
	if (host->ops->enable && !stop && host->claim_cnt == 1)
		host->ops->enable(host);
	return stop;
}

EXPORT_SYMBOL(__mmc_claim_host);

int mmc_try_claim_host(struct mmc_host *host)
{
	int claimed_host = 0;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	if (!host->claimed || host->claimer == current) {
		host->claimed = 1;
		host->claimer = current;
		host->claim_cnt += 1;
		claimed_host = 1;
	}
	spin_unlock_irqrestore(&host->lock, flags);
	if (host->ops->enable && claimed_host && host->claim_cnt == 1)
		host->ops->enable(host);
	return claimed_host;
}
EXPORT_SYMBOL(mmc_try_claim_host);

void mmc_release_host(struct mmc_host *host)
{
	unsigned long flags;

	WARN_ON(!host->claimed);

	if (host->ops->disable && host->claim_cnt == 1)
		host->ops->disable(host);

	spin_lock_irqsave(&host->lock, flags);
	if (--host->claim_cnt) {
		
		spin_unlock_irqrestore(&host->lock, flags);
	} else {
		host->claimed = 0;
		host->claimer = NULL;
		spin_unlock_irqrestore(&host->lock, flags);
		wake_up(&host->wq);
	}
}
EXPORT_SYMBOL(mmc_release_host);

void mmc_set_ios(struct mmc_host *host)
{
	struct mmc_ios *ios = &host->ios;

	pr_debug("%s: clock %uHz busmode %u powermode %u cs %u Vdd %u "
		"width %u timing %u\n",
		 mmc_hostname(host), ios->clock, ios->bus_mode,
		 ios->power_mode, ios->chip_select, ios->vdd,
		 ios->bus_width, ios->timing);
	if (host->card && ((host->card->type == MMC_TYPE_SDIO) || (host->card->type == MMC_TYPE_SDIO_WIFI))) {
		printk(KERN_ERR "%s: clock %uHz busmode %u powermode %u cs %u Vdd %u "
		  "width %u timing %u\n",
		  mmc_hostname(host), ios->clock, ios->bus_mode,
		  ios->power_mode, ios->chip_select, ios->vdd,
		  ios->bus_width, ios->timing);
	}
	if (ios->clock > 0)
		mmc_set_ungated(host);
	host->ops->set_ios(host, ios);
}
EXPORT_SYMBOL(mmc_set_ios);

void mmc_set_chip_select(struct mmc_host *host, int mode)
{
	mmc_host_clk_hold(host);
	host->ios.chip_select = mode;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

static void __mmc_set_clock(struct mmc_host *host, unsigned int hz)
{
	WARN_ON(hz < host->f_min);

	if (hz > host->f_max)
		hz = host->f_max;

	host->ios.clock = hz;
	mmc_set_ios(host);
}

void mmc_set_clock(struct mmc_host *host, unsigned int hz)
{
	mmc_host_clk_hold(host);
	__mmc_set_clock(host, hz);
	mmc_host_clk_release(host);
}
#ifdef CONFIG_WIMAX
EXPORT_SYMBOL(mmc_set_clock);
#endif

#ifdef CONFIG_MMC_CLKGATE
void mmc_gate_clock(struct mmc_host *host)
{
	unsigned long flags;

	WARN_ON(!host->ios.clock);

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clk_old = host->ios.clock;
	host->ios.clock = 0;
	host->clk_gated = true;
	spin_unlock_irqrestore(&host->clk_lock, flags);
	mmc_set_ios(host);
}

void mmc_ungate_clock(struct mmc_host *host)
{
	if (host->clk_old) {
		WARN_ON(host->ios.clock);
		
		__mmc_set_clock(host, host->clk_old);
	}
}

void mmc_set_ungated(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->clk_lock, flags);
	host->clk_gated = false;
	spin_unlock_irqrestore(&host->clk_lock, flags);
}

#else
void mmc_set_ungated(struct mmc_host *host)
{
}
#endif

void mmc_set_bus_mode(struct mmc_host *host, unsigned int mode)
{
	mmc_host_clk_hold(host);
	host->ios.bus_mode = mode;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

void mmc_set_bus_width(struct mmc_host *host, unsigned int width)
{
	mmc_host_clk_hold(host);
	host->ios.bus_width = width;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}

static int mmc_vdd_to_ocrbitnum(int vdd, bool low_bits)
{
	const int max_bit = ilog2(MMC_VDD_35_36);
	int bit;

	if (vdd < 1650 || vdd > 3600)
		return -EINVAL;

	if (vdd >= 1650 && vdd <= 1950)
		return ilog2(MMC_VDD_165_195);

	if (low_bits)
		vdd -= 1;

	
	bit = (vdd - 2000) / 100 + 8;
	if (bit > max_bit)
		return max_bit;
	return bit;
}

u32 mmc_vddrange_to_ocrmask(int vdd_min, int vdd_max)
{
	u32 mask = 0;

	if (vdd_max < vdd_min)
		return 0;

	
	vdd_max = mmc_vdd_to_ocrbitnum(vdd_max, false);
	if (vdd_max < 0)
		return 0;

	
	vdd_min = mmc_vdd_to_ocrbitnum(vdd_min, true);
	if (vdd_min < 0)
		return 0;

	
	while (vdd_max >= vdd_min)
		mask |= 1 << vdd_max--;

	return mask;
}
EXPORT_SYMBOL(mmc_vddrange_to_ocrmask);

#ifdef CONFIG_REGULATOR

int mmc_regulator_get_ocrmask(struct regulator *supply)
{
	int			result = 0;
	int			count;
	int			i;

	count = regulator_count_voltages(supply);
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		int		vdd_uV;
		int		vdd_mV;

		vdd_uV = regulator_list_voltage(supply, i);
		if (vdd_uV <= 0)
			continue;

		vdd_mV = vdd_uV / 1000;
		result |= mmc_vddrange_to_ocrmask(vdd_mV, vdd_mV);
	}

	return result;
}
EXPORT_SYMBOL(mmc_regulator_get_ocrmask);

int mmc_regulator_set_ocr(struct mmc_host *mmc,
			struct regulator *supply,
			unsigned short vdd_bit)
{
	int			result = 0;
	int			min_uV, max_uV;

	if (vdd_bit) {
		int		tmp;
		int		voltage;

		tmp = vdd_bit - ilog2(MMC_VDD_165_195);
		if (tmp == 0) {
			min_uV = 1650 * 1000;
			max_uV = 1950 * 1000;
		} else {
			min_uV = 1900 * 1000 + tmp * 100 * 1000;
			max_uV = min_uV + 100 * 1000;
		}

		voltage = regulator_get_voltage(supply);

		if (mmc->caps2 & MMC_CAP2_BROKEN_VOLTAGE)
			min_uV = max_uV = voltage;

		if (voltage < 0)
			result = voltage;
		else if (voltage < min_uV || voltage > max_uV)
			result = regulator_set_voltage(supply, min_uV, max_uV);
		else
			result = 0;

		if (result == 0 && !mmc->regulator_enabled) {
			result = regulator_enable(supply);
			if (!result)
				mmc->regulator_enabled = true;
		}
	} else if (mmc->regulator_enabled) {
		result = regulator_disable(supply);
		if (result == 0)
			mmc->regulator_enabled = false;
	}

	if (result)
		dev_err(mmc_dev(mmc),
			"could not set regulator OCR (%d)\n", result);
	return result;
}
EXPORT_SYMBOL(mmc_regulator_set_ocr);

#endif 

u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
	int bit;

	ocr &= host->ocr_avail;

	bit = ffs(ocr);
	if (bit) {
		bit -= 1;

		ocr &= 3 << bit;

		mmc_host_clk_hold(host);
		host->ios.vdd = bit;
		mmc_set_ios(host);
		mmc_host_clk_release(host);
	} else {
		pr_warning("%s: host doesn't support card's voltages\n",
				mmc_hostname(host));
		ocr = 0;
	}

	return ocr;
}

int mmc_set_signal_voltage(struct mmc_host *host, int signal_voltage, bool cmd11)
{
	struct mmc_command cmd = {0};
	int err = 0;

	BUG_ON(!host);

	if ((signal_voltage != MMC_SIGNAL_VOLTAGE_330) && cmd11) {
		cmd.opcode = SD_SWITCH_VOLTAGE;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		err = mmc_wait_for_cmd(host, &cmd, 0);
		if (err)
			return err;

		if (!mmc_host_is_spi(host) && (cmd.resp[0] & R1_ERROR))
			return -EIO;
	}

	host->ios.signal_voltage = signal_voltage;

	if (host->ops->start_signal_voltage_switch) {
		mmc_host_clk_hold(host);
		err = host->ops->start_signal_voltage_switch(host, &host->ios);
		mmc_host_clk_release(host);
	}

	return err;
}

void mmc_set_timing(struct mmc_host *host, unsigned int timing)
{
	mmc_host_clk_hold(host);
	host->ios.timing = timing;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}
#ifdef CONFIG_WIMAX
EXPORT_SYMBOL(mmc_set_timing);
#endif
void mmc_set_driver_type(struct mmc_host *host, unsigned int drv_type)
{
	mmc_host_clk_hold(host);
	host->ios.drv_type = drv_type;
	mmc_set_ios(host);
	mmc_host_clk_release(host);
}
void mmc_power_up(struct mmc_host *host)
{
	int bit;

	mmc_host_clk_hold(host);

	
	if (host->ocr)
		bit = ffs(host->ocr) - 1;
	else
		bit = fls(host->ocr_avail) - 1;

	host->ios.vdd = bit;
	if (mmc_host_is_spi(host)) 
		host->ios.chip_select = MMC_CS_HIGH;
	else {
		host->ios.chip_select = MMC_CS_DONTCARE;
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	}
	host->ios.power_mode = MMC_POWER_UP;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_delay(10);

	host->ios.clock = host->f_init;

	host->ios.power_mode = MMC_POWER_ON;
	mmc_set_ios(host);

	mmc_delay(10);

	mmc_host_clk_release(host);
}

void mmc_power_off(struct mmc_host *host)
{
	mmc_host_clk_hold(host);

	host->ios.clock = 0;
	host->ios.vdd = 0;
	

	host->ocr = 1 << (fls(host->ocr_avail) - 1);

	if (!mmc_host_is_spi(host)) {
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
		host->ios.chip_select = MMC_CS_DONTCARE;
	}
	host->ios.power_mode = MMC_POWER_OFF;
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_delay(1);

	mmc_host_clk_release(host);
}

static void __mmc_release_bus(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(host->bus_refs);
	BUG_ON(!host->bus_dead);

	host->bus_ops = NULL;
}

static inline void mmc_bus_get(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	host->bus_refs++;
	spin_unlock_irqrestore(&host->lock, flags);
}

static inline void mmc_bus_put(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	host->bus_refs--;
	if ((host->bus_refs == 0) && host->bus_ops)
		__mmc_release_bus(host);
	spin_unlock_irqrestore(&host->lock, flags);
}

int mmc_resume_bus(struct mmc_host *host)
{
	unsigned long flags;
	int ret = 0;
	if (!mmc_bus_needs_resume(host))
		return -EINVAL;

	printk("%s: Starting deferred resume\n", mmc_hostname(host));
	spin_lock_irqsave(&host->lock, flags);
	host->bus_resume_flags &= ~MMC_BUSRESUME_NEEDS_RESUME;
	host->rescan_disable = 0;
	spin_unlock_irqrestore(&host->lock, flags);

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		mmc_power_up(host);
		if (host->bus_ops->awake && mmc_card_can_sleep(host)) {
			ret = host->bus_ops->awake(host);
			if (ret) {
				BUG_ON(!host->bus_ops->resume);
				ret = host->bus_ops->resume(host);
			}
		} else {
			BUG_ON(!host->bus_ops->resume);
			ret = host->bus_ops->resume(host);
		}
	}

	if (host->bus_ops->detect && !host->bus_dead)
		host->bus_ops->detect(host);

	mmc_bus_put(host);
	pr_info("%s: Deferred resume %s\n", mmc_hostname(host),
		(ret == 0 ? "completed" : "Fail"));
	return ret;
}

EXPORT_SYMBOL(mmc_resume_bus);

void mmc_attach_bus(struct mmc_host *host, const struct mmc_bus_ops *ops)
{
	unsigned long flags;

	BUG_ON(!host);
	BUG_ON(!ops);

	WARN_ON(!host->claimed);

	spin_lock_irqsave(&host->lock, flags);

	BUG_ON(host->bus_ops);
	BUG_ON(host->bus_refs);

	host->bus_ops = ops;
	host->bus_refs = 1;
	host->bus_dead = 0;

	spin_unlock_irqrestore(&host->lock, flags);
}

void mmc_detach_bus(struct mmc_host *host)
{
	unsigned long flags;

	BUG_ON(!host);

	WARN_ON(!host->claimed);
	WARN_ON(!host->bus_ops);

	spin_lock_irqsave(&host->lock, flags);

	host->bus_dead = 1;

	spin_unlock_irqrestore(&host->lock, flags);

	mmc_power_off(host);

	mmc_bus_put(host);
}

void mmc_detect_change(struct mmc_host *host, unsigned long delay)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned long flags;
	spin_lock_irqsave(&host->lock, flags);
	WARN_ON(host->removed);
	spin_unlock_irqrestore(&host->lock, flags);
#endif
#if SD_DEBOUNCE_DEBUG
	extern ktime_t detect_wq;
#endif
	host->detect_change = 1;

	wake_lock(&host->detect_wake_lock);
#if SD_DEBOUNCE_DEBUG
	if (mmc_is_sd_host(host))
		detect_wq = ktime_get();
#endif
	mmc_schedule_delayed_work(&host->detect, delay);
}

EXPORT_SYMBOL(mmc_detect_change);

int mmc_reinit_card(struct mmc_host *host)
{
	int err = 0;
	printk(KERN_INFO "%s: %s\n", mmc_hostname(host),
		__func__);

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead &&
		host->bus_ops->resume) {
		if (host->card && mmc_card_sd(host->card)) {
			mmc_power_off(host);
			msleep(100);
		}
		mmc_power_up(host);
		err = host->bus_ops->resume(host);
	}

	mmc_bus_put(host);
	printk(KERN_INFO "%s: %s return %d\n", mmc_hostname(host),
		__func__, err);
	return err;
}

void mmc_remove_sd_card(struct work_struct *work)
{
	struct mmc_host *host =
		container_of(work, struct mmc_host, remove.work);
	const unsigned int REDETECT_MAX = 5;

	printk(KERN_INFO "%s: %s, redetect_cnt : %d\n", mmc_hostname(host),
		__func__, host->redetect_cnt);

	mod_timer(&sd_remove_tout_timer, (jiffies + msecs_to_jiffies(5000)));
	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);
		mmc_claim_host(host);
		mmc_detach_bus(host);
		mdelay(500);
		mmc_release_host(host);
	}
	mmc_bus_put(host);
	wake_unlock(&mmc_removal_work_wake_lock);

	printk(KERN_INFO "%s: %s exit\n", mmc_hostname(host),
		__func__);

	del_timer_sync(&sd_remove_tout_timer);

	if (host->ops->get_cd(host) && host->redetect_cnt++ < REDETECT_MAX)
		mmc_detect_change(host, 0);
}

void mmc_init_erase(struct mmc_card *card)
{
	unsigned int sz;

	if (is_power_of_2(card->erase_size))
		card->erase_shift = ffs(card->erase_size) - 1;
	else
		card->erase_shift = 0;

	if (mmc_card_sd(card) && card->ssr.au) {
		card->pref_erase = card->ssr.au;
		card->erase_shift = ffs(card->ssr.au) - 1;
	} else if (card->ext_csd.hc_erase_size) {
		card->pref_erase = card->ext_csd.hc_erase_size;
	} else {
		sz = (card->csd.capacity << (card->csd.read_blkbits - 9)) >> 11;
		if (sz < 128)
			card->pref_erase = 512 * 1024 / 512;
		else if (sz < 512)
			card->pref_erase = 1024 * 1024 / 512;
		else if (sz < 1024)
			card->pref_erase = 2 * 1024 * 1024 / 512;
		else
			card->pref_erase = 4 * 1024 * 1024 / 512;
		if (card->pref_erase < card->erase_size)
			card->pref_erase = card->erase_size;
		else {
			sz = card->pref_erase % card->erase_size;
			if (sz)
				card->pref_erase += card->erase_size - sz;
		}
	}
}

static unsigned int mmc_mmc_erase_timeout(struct mmc_card *card,
				          unsigned int arg, unsigned int qty)
{
	unsigned int erase_timeout;

	if (arg == MMC_DISCARD_ARG ||
	    (arg == MMC_TRIM_ARG && card->ext_csd.rev >= 6)) {
		erase_timeout = card->ext_csd.trim_timeout;
	} else if (card->ext_csd.erase_group_def & 1) {
		
		if (arg == MMC_TRIM_ARG)
			erase_timeout = card->ext_csd.trim_timeout;
		else
			erase_timeout = card->ext_csd.hc_erase_timeout;
	} else {
		
		unsigned int mult = (10 << card->csd.r2w_factor);
		unsigned int timeout_clks = card->csd.tacc_clks * mult;
		unsigned int timeout_us;

		
		if (card->csd.tacc_ns < 1000000)
			timeout_us = (card->csd.tacc_ns * mult) / 1000;
		else
			timeout_us = (card->csd.tacc_ns / 1000) * mult;

		timeout_clks <<= 1;
		timeout_us += (timeout_clks * 1000) /
			      (mmc_host_clk_rate(card->host) / 1000);

		erase_timeout = timeout_us / 1000;

		if (!erase_timeout)
			erase_timeout = 1;
	}

	

	erase_timeout *= qty;

	if (mmc_host_is_spi(card->host) && erase_timeout < 1000)
		erase_timeout = 1000;

	return erase_timeout;
}

static unsigned int mmc_sd_erase_timeout(struct mmc_card *card,
					 unsigned int arg,
					 unsigned int qty)
{
	unsigned int erase_timeout;

	if (card->ssr.erase_timeout) {
		
		erase_timeout = card->ssr.erase_timeout * qty +
				card->ssr.erase_offset;
	} else {
		erase_timeout = 250 * qty;
	}

	
	if (erase_timeout < 1000)
		erase_timeout = 1000;

	return erase_timeout;
}

static unsigned int mmc_erase_timeout(struct mmc_card *card,
				      unsigned int arg,
				      unsigned int qty)
{
	if (mmc_card_sd(card))
		return mmc_sd_erase_timeout(card, arg, qty);
	else
		return mmc_mmc_erase_timeout(card, arg, qty);
}

static int
mmc_send_single_read(struct mmc_card *card, struct mmc_host *host, unsigned int from)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	void *data_buf;
	int len = 512;

	data_buf = kmalloc(len, GFP_KERNEL);
	if (data_buf == NULL)
		return -ENOMEM;

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.arg = from;

	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 512;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, data_buf, len);
	mmc_set_data_timeout(&data, card);
	mmc_wait_for_req(host, &mrq);

	kfree(data_buf);

	if (cmd.error)
		return cmd.error;
	if (data.error)
		return data.error;

	return 0;
}

static int mmc_do_erase(struct mmc_card *card, unsigned int from,
			unsigned int to, unsigned int arg)
{
	struct mmc_command cmd = {0};
	unsigned int qty = 0;
	int err;
	ktime_t start, diff;

	if (card->erase_shift)
		qty += ((to >> card->erase_shift) -
			(from >> card->erase_shift)) + 1;
	else if (mmc_card_sd(card))
		qty += to - from + 1;
	else
		qty += ((to / card->erase_size) -
			(from / card->erase_size)) + 1;

	if (!mmc_card_blockaddr(card)) {
		from <<= 9;
		to <<= 9;
	}

	if (card->cid.manfid == HYNIX_MMC && card->ext_csd.rev >= 6)
		if (mmc_send_single_read(card, card->host, from) != 0)
			pr_err("%s, Dummy read failed\n", __func__);

	if (mmc_card_sd(card))
		cmd.opcode = SD_ERASE_WR_BLK_START;
	else
		cmd.opcode = MMC_ERASE_GROUP_START;
	cmd.arg = from;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: group start error %d, "
		       "status %#x\n", err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	memset(&cmd, 0, sizeof(struct mmc_command));
	if (mmc_card_sd(card))
		cmd.opcode = SD_ERASE_WR_BLK_END;
	else
		cmd.opcode = MMC_ERASE_GROUP_END;
	cmd.arg = to;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: group end error %d, status %#x\n",
		       err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	if (mmc_card_mmc(card))
		trace_mmc_req_start(&(card->host->class_dev), MMC_ERASE,
			from, to - from + 1);
	start = ktime_get();

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_ERASE;
	cmd.arg = arg;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	cmd.cmd_timeout_ms = mmc_erase_timeout(card, arg, qty);
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		pr_err("mmc_erase: erase error %d, status %#x\n",
		       err, cmd.resp[0]);
		err = -EIO;
		goto out;
	}

	if (mmc_host_is_spi(card->host))
		goto out;

	do {
		memset(&cmd, 0, sizeof(struct mmc_command));
		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
		
		err = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (err || (cmd.resp[0] & 0xFDF92000)) {
			pr_err("error %d requesting status %#x\n",
				err, cmd.resp[0]);
			err = -EIO;
			goto out;
		}
	} while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
		 R1_CURRENT_STATE(cmd.resp[0]) == R1_STATE_PRG);
out:
	diff = ktime_sub(ktime_get(), start);
	if (ktime_to_ms(diff) >= 3000)
		pr_info("%s: erase(sector %u to %u) takes %lld ms\n",
			mmc_hostname(card->host), from, to, ktime_to_ms(diff));

	if (card->host->tp_enable)
		trace_mmc_request_done(&(card->host->class_dev), MMC_ERASE,
			from, to - from + 1, ktime_to_ms(diff));
	return err;
}

int mmc_erase(struct mmc_card *card, unsigned int from, unsigned int nr,
	      unsigned int arg)
{
	unsigned int rem, to = from + nr;

	if (!(card->host->caps & MMC_CAP_ERASE) ||
	    !(card->csd.cmdclass & CCC_ERASE))
		return -EOPNOTSUPP;

	if (!card->erase_size)
		return -EOPNOTSUPP;

	if (mmc_card_sd(card) && arg != MMC_ERASE_ARG)
		return -EOPNOTSUPP;


	if ((arg & MMC_TRIM_ARGS) &&
	    !(card->ext_csd.sec_feature_support & EXT_CSD_SEC_GB_CL_EN))
		return -EOPNOTSUPP;


	if (arg == MMC_ERASE_ARG) {
		rem = from % card->erase_size;
		if (rem) {
			rem = card->erase_size - rem;
			from += rem;
			if (nr > rem)
				nr -= rem;
			else
				return 0;
		}
		rem = nr % card->erase_size;
		if (rem)
			nr -= rem;
	}

	if (nr == 0)
		return 0;

	to = from + nr;

	if (to <= from)
		return -EINVAL;

	
	to -= 1;
	return mmc_do_erase(card, from, to, arg);
}
EXPORT_SYMBOL(mmc_erase);

int mmc_can_erase(struct mmc_card *card)
{
	if ((card->host->caps & MMC_CAP_ERASE) &&
	    (card->csd.cmdclass & CCC_ERASE) && card->erase_size)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_erase);

int mmc_can_trim(struct mmc_card *card)
{
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_GB_CL_EN)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_trim);

int mmc_can_discard(struct mmc_card *card)
{
	if (card && mmc_card_mmc(card)) {
		
		if (card->ext_csd.rev >= 6)
			return 1;
	}
	if (card->cid.manfid == SAMSUNG_MMC) {
		if (card->ext_csd.sectors == 30535680 ||
			card->ext_csd.sectors == 61071360 ||
			card->ext_csd.sectors == 122142720) {
			if ((card->raw_cid[2] & 0x00FF0000) != 0x00)
				return 1;
		}
	}

	
	if (card->cid.manfid == SANDISK_MMC ) {
		if (card->ext_csd.sectors == 30777344 ||
			card->ext_csd.sectors == 61071360 ||
			card->ext_csd.sectors == 122142720 ) {
			if (!strcmp(card->ext_csd.fwrev, "02f11o"))
				return 1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(mmc_can_discard);

int mmc_can_sanitize(struct mmc_card *card)
{
#if 0
	if (!mmc_can_trim(card) && !mmc_can_erase(card))
		return 0;
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_SANITIZE)
		return 1;
#endif
	return 0;
}
EXPORT_SYMBOL(mmc_can_sanitize);

int mmc_can_secure_erase_trim(struct mmc_card *card)
{
	if (card->ext_csd.sec_feature_support & EXT_CSD_SEC_ER_EN)
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_can_secure_erase_trim);

int mmc_can_poweroff_notify(const struct mmc_card *card)
{
	return card &&
		mmc_card_mmc(card) &&
		card->host->bus_ops->poweroff_notify &&
		(card->poweroff_notify_state == MMC_POWERED_ON);
}
EXPORT_SYMBOL(mmc_can_poweroff_notify);

int mmc_erase_group_aligned(struct mmc_card *card, unsigned int from,
			    unsigned int nr)
{
	if (!card->erase_size)
		return 0;
	if (from % card->erase_size || nr % card->erase_size)
		return 0;
	return 1;
}
EXPORT_SYMBOL(mmc_erase_group_aligned);

static unsigned int mmc_do_calc_max_discard(struct mmc_card *card,
					    unsigned int arg)
{
	struct mmc_host *host = card->host;
	unsigned int max_discard, x, y, qty = 0, max_qty, timeout;
	unsigned int last_timeout = 0;

	if (card->erase_shift)
		max_qty = UINT_MAX >> card->erase_shift;
	else if (mmc_card_sd(card))
		max_qty = UINT_MAX;
	else
		max_qty = UINT_MAX / card->erase_size;

	
	do {
		y = 0;
		for (x = 1; x && x <= max_qty && max_qty - x >= qty; x <<= 1) {
			timeout = mmc_erase_timeout(card, arg, qty + x);
			if (timeout > host->max_discard_to)
				break;
			if (timeout < last_timeout)
				break;
			last_timeout = timeout;
			y = x;
		}
		qty += y;
	} while (y);

	if (!qty)
		return 0;

	if (qty == 1)
		return 1;

	
	if (card->erase_shift)
		max_discard = --qty << card->erase_shift;
	else if (mmc_card_sd(card))
		max_discard = qty;
	else
		max_discard = --qty * card->erase_size;

	return max_discard;
}

unsigned int mmc_calc_max_discard(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	unsigned int max_discard, max_trim;

	if (!host->max_discard_to)
		return UINT_MAX;

	if (mmc_card_mmc(card) && !(card->ext_csd.erase_group_def & 1))
		return card->pref_erase;

	max_discard = mmc_do_calc_max_discard(card, MMC_ERASE_ARG);
	if (mmc_can_trim(card)) {
		max_trim = mmc_do_calc_max_discard(card, MMC_TRIM_ARG);
		if (max_trim < max_discard)
			max_discard = max_trim;
	} else if (max_discard < card->erase_size) {
		max_discard = 0;
	}
	pr_debug("%s: calculated max. discard sectors %u for timeout %u ms\n",
		 mmc_hostname(host), max_discard, host->max_discard_to);
	return max_discard;
}
EXPORT_SYMBOL(mmc_calc_max_discard);

int mmc_set_blocklen(struct mmc_card *card, unsigned int blocklen)
{
	struct mmc_command cmd = {0};

	if (mmc_card_blockaddr(card) || mmc_card_ddr_mode(card))
		return 0;

	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = blocklen;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	return mmc_wait_for_cmd(card->host, &cmd, 5);
}
EXPORT_SYMBOL(mmc_set_blocklen);

static void mmc_hw_reset_for_init(struct mmc_host *host)
{
	if (!(host->caps & MMC_CAP_HW_RESET) || !host->ops->hw_reset)
		return;
	mmc_host_clk_hold(host);
	host->ops->hw_reset(host);
	mmc_host_clk_release(host);
}

int mmc_can_reset(struct mmc_card *card)
{
	u8 rst_n_function;

	if (!mmc_card_mmc(card))
		return 0;
	rst_n_function = card->ext_csd.rst_n_function;
	if ((rst_n_function & EXT_CSD_RST_N_EN_MASK) != EXT_CSD_RST_N_ENABLED)
		return 0;
	return 1;
}
EXPORT_SYMBOL(mmc_can_reset);

static int mmc_do_hw_reset(struct mmc_host *host, int check)
{
	struct mmc_card *card = host->card;

	if (!host->bus_ops->power_restore)
		return -EOPNOTSUPP;

	if (!(host->caps & MMC_CAP_HW_RESET) || !host->ops->hw_reset)
		return -EOPNOTSUPP;

	if (!card)
		return -EINVAL;

	if (!mmc_can_reset(card))
		return -EOPNOTSUPP;

	mmc_host_clk_hold(host);
	mmc_set_clock(host, host->f_init);

	host->ops->hw_reset(host);

	
	if (check) {
		struct mmc_command cmd = {0};
		int err;

		cmd.opcode = MMC_SEND_STATUS;
		if (!mmc_host_is_spi(card->host))
			cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
		err = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (!err) {
			mmc_host_clk_release(host);
			return -ENOSYS;
		}
	}

	host->card->state &= ~(MMC_STATE_HIGHSPEED | MMC_STATE_HIGHSPEED_DDR);
	if (mmc_host_is_spi(host)) {
		host->ios.chip_select = MMC_CS_HIGH;
		host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
	} else {
		host->ios.chip_select = MMC_CS_DONTCARE;
		host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
	}
	host->ios.bus_width = MMC_BUS_WIDTH_1;
	host->ios.timing = MMC_TIMING_LEGACY;
	mmc_set_ios(host);

	mmc_host_clk_release(host);

	return host->bus_ops->power_restore(host);
}

int mmc_hw_reset(struct mmc_host *host)
{
	return mmc_do_hw_reset(host, 0);
}
EXPORT_SYMBOL(mmc_hw_reset);

int mmc_hw_reset_check(struct mmc_host *host)
{
	return mmc_do_hw_reset(host, 1);
}
EXPORT_SYMBOL(mmc_hw_reset_check);

static int mmc_rescan_try_freq(struct mmc_host *host, unsigned freq)
{
	host->f_init = freq;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: trying to init card at %u Hz\n",
		mmc_hostname(host), __func__, host->f_init);
#endif
	mmc_power_up(host);

	mmc_hw_reset_for_init(host);

	
	mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330, 0);

	sdio_reset(host);
	mmc_go_idle(host);

	mmc_send_if_cond(host, host->ocr_avail);

	
	if (!mmc_attach_sdio(host)) {
		pr_info("%s: Find a SDIO card\n", __func__);
		return 0;
	}

	if (!host->ios.vdd)
		mmc_power_up(host);

	if (!mmc_attach_sd(host)) {
		pr_info("%s: Find a SD card\n", __func__);
		return 0;
	}

	if (!host->ios.vdd)
		mmc_power_up(host);

	if (!mmc_attach_mmc(host)) {
		pr_info("%s: Find a MMC/eMMC card\n", __func__);
		return 0;
	}

	pr_info("%s: Can not find a card type. A dummy card ?\n", __func__);

	mmc_power_off(host);
	return -EIO;
}

int _mmc_detect_card_removed(struct mmc_host *host)
{
	int ret;

	if ((host->caps & MMC_CAP_NONREMOVABLE) || !host->bus_ops->alive)
		return 0;

	if (!host->card || mmc_card_removed(host->card))
		return 1;

	ret = host->bus_ops->alive(host);
	if (ret) {
		if(mmc_card_sd(host->card))
			host->card->do_remove = 1;
		else
			mmc_card_set_removed(host->card);
		pr_debug("%s: card remove detected\n", mmc_hostname(host));
	}

	return ret;
}

int mmc_detect_card_removed(struct mmc_host *host)
{
	struct mmc_card *card = host->card;
	int ret;

	WARN_ON(!host->claimed);

	if (!card)
		return 1;

	ret = mmc_card_removed(card);
	if (!host->detect_change && !(host->caps & MMC_CAP_NEEDS_POLL) &&
	    !(host->caps2 & MMC_CAP2_DETECT_ON_ERR))
		return ret;

	host->detect_change = 0;
	if (!ret) {
		ret = _mmc_detect_card_removed(host);
		if (ret && (host->caps2 & MMC_CAP2_DETECT_ON_ERR)) {
			cancel_delayed_work(&host->detect);
			mmc_detect_change(host, 0);
		}
	}

	return ret;
}
EXPORT_SYMBOL(mmc_detect_card_removed);

void mmc_rescan(struct work_struct *work)
{
	struct mmc_host *host =
		container_of(work, struct mmc_host, detect.work);
	bool extend_wakelock = false;
#if SD_DEBOUNCE_DEBUG
	if (mmc_is_sd_host(host)) {
		extern int SD_detect_debounce_time;
		extern ktime_t detect_wq;
		extern ktime_t irq_diff;
		int wq_diff_time;
		int irq_diff_time;
		wq_diff_time = ktime_to_ms(ktime_sub(ktime_get(), detect_wq));
		irq_diff_time = ktime_to_ms(irq_diff);
		
		if (wq_diff_time > SD_detect_debounce_time)
			pr_info("%s: %s wq diff time= %d ms\n", mmc_hostname(host), __func__, wq_diff_time);
		
		if (wq_diff_time > irq_diff_time)
			pr_err("%s: SD may not be rescanned successfully! "
					"wq diff time = %d ms, irq diff time = %d ms\n",
					mmc_hostname(host), wq_diff_time, irq_diff_time);
	}
#endif
	if (host->rescan_disable)
		return;

	mmc_bus_get(host);

	if (host->bus_ops && host->bus_ops->detect && !host->bus_dead
	    && !(host->caps & MMC_CAP_NONREMOVABLE))
		host->bus_ops->detect(host);

	host->detect_change = 0;

	if (host->bus_dead)
		extend_wakelock = 1;

	mmc_bus_put(host);
	mmc_bus_get(host);

	
	if (host->bus_ops != NULL) {
		mmc_bus_put(host);
		goto out;
	}

	mmc_bus_put(host);

	if (host->ops->get_cd && host->ops->get_cd(host) == 0)
		goto out;

	mmc_claim_host(host);
	if (!mmc_rescan_try_freq(host, host->f_min))
		extend_wakelock = true;
	mmc_release_host(host);

 out:
	if (extend_wakelock)
		wake_lock_timeout(&host->detect_wake_lock, HZ / 2);
	else
		wake_unlock(&host->detect_wake_lock);
	if (host->caps & MMC_CAP_NEEDS_POLL) {
		wake_lock(&host->detect_wake_lock);
		mmc_schedule_delayed_work(&host->detect, HZ);
	}
}

void mmc_start_host(struct mmc_host *host)
{
	mmc_power_off(host);
	mmc_detect_change(host, 0);
}

void mmc_stop_host(struct mmc_host *host)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned long flags;
	spin_lock_irqsave(&host->lock, flags);
	host->removed = 1;
	spin_unlock_irqrestore(&host->lock, flags);
#endif

	if (cancel_delayed_work_sync(&host->detect))
		wake_unlock(&host->detect_wake_lock);
	mmc_flush_scheduled_work();

	
	host->pm_flags = 0;

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		mmc_claim_host(host);
		if (mmc_can_poweroff_notify(host->card)) {
			int err = host->bus_ops->poweroff_notify(host,
						MMC_PW_OFF_NOTIFY_LONG);
			if (err)
				pr_info("%s: error [%d] in poweroff notify\n",
					mmc_hostname(host), err);
		}
		mmc_release_host(host);
		
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
		mmc_bus_put(host);
		return;
	}
	mmc_bus_put(host);

	BUG_ON(host->card);

	mmc_power_off(host);
}

int mmc_power_save_host(struct mmc_host *host)
{
	int ret = 0;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering down\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if (!host->bus_ops || host->bus_dead || !host->bus_ops->power_restore) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	if (host->bus_ops->power_save)
		ret = host->bus_ops->power_save(host);
	mmc_claim_host(host);
	if (mmc_can_poweroff_notify(host->card)) {
		int err = host->bus_ops->poweroff_notify(host,
					MMC_PW_OFF_NOTIFY_SHORT);
		if (err)
			pr_info("%s: error [%d] in poweroff notify\n",
				mmc_hostname(host), err);
	}
	mmc_release_host(host);

	mmc_bus_put(host);

	mmc_power_off(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_save_host);

int mmc_power_restore_host(struct mmc_host *host)
{
	int ret;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering up\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if (!host->bus_ops || host->bus_dead || !host->bus_ops->power_restore) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	mmc_power_up(host);
	ret = host->bus_ops->power_restore(host);

	mmc_bus_put(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_restore_host);

int mmc_card_support_bkops(struct mmc_card *card)
{
	if (card && mmc_card_mmc(card) && (card->ext_csd.rev >= 5 && card->ext_csd.bkops) && (card->host->caps & MMC_CAP_NONREMOVABLE)) {
		
		if (card->ext_csd.rev >= 6)
			return 1;
		if (card->cid.manfid == SAMSUNG_MMC) {
			
			if (card->ext_csd.rev >= 6)
				return 1;
			
			
			if (card->ext_csd.sectors == 30535680) {
				if ((card->cid.fwrev == 0x5) || (card->cid.fwrev == 0x15))
					return 1;
				else
					return 0;
			}
			
			if (card->ext_csd.sectors == 61071360) {
				if ((card->cid.fwrev == 0x5) || (card->cid.fwrev == 0x7) || (card->cid.fwrev == 0x15))
					return 1;
				else
					return 0;
			}
			
			if (card->ext_csd.sectors == 122142720) {
				if ((card->cid.fwrev == 0x5) || (card->cid.fwrev == 0x15))
					return 1;
				else
					return 0;
			}
		} else if (card->cid.manfid == SANDISK_MMC) {
			
			if (card->ext_csd.rev >= 6)
				return 1;
			
			if (card->ext_csd.sectors == 30777344 ||
				card->ext_csd.sectors == 61071360 ||
				card->ext_csd.sectors == 122142720 ) {
				if (!strcmp(card->ext_csd.fwrev, "02f11o"))
					return 1;
				else
					return 0;
			}
		} else {
			printk(KERN_DEBUG "%s: manfid = 0x%x prod_name = %s\n", mmc_hostname(card->host), card->cid.manfid, card->cid.prod_name);
			return 0;
		}
	}
	return 0;
}
EXPORT_SYMBOL(mmc_card_support_bkops);

int mmc_card_awake(struct mmc_host *host)
{
	int err = -ENOSYS;

	if (host->caps2 & MMC_CAP2_NO_SLEEP_CMD)
		return 0;

	mmc_bus_get(host);

	if (host->bus_ops && !host->bus_dead && host->bus_ops->awake) {
		err = host->bus_ops->awake(host);
		if (!err)
			mmc_card_clr_sleep(host->card);
	}

	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_card_awake);

int mmc_card_sleep(struct mmc_host *host)
{
	int err = -ENOSYS;

	if (host->caps2 & MMC_CAP2_NO_SLEEP_CMD)
		return 0;

	mmc_bus_get(host);

	if (host->bus_ops && !host->bus_dead && host->bus_ops->sleep) {
		err = host->bus_ops->sleep(host);
		if (!err)
			mmc_card_set_sleep(host->card);
	}

	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_card_sleep);

int mmc_card_can_sleep(struct mmc_host *host)
{
	struct mmc_card *card = host->card;

	if (card && mmc_card_mmc(card) && (card->ext_csd.rev >= 3) && (host->caps & MMC_CAP_NONREMOVABLE))
		return 1;
	return 0;
}
EXPORT_SYMBOL(mmc_card_can_sleep);

int mmc_flush_cache(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int err = 0;

	if (!(host->caps2 & MMC_CAP2_CACHE_CTRL))
		return err;

	if (mmc_card_mmc(card) &&
			(card->ext_csd.cache_size > 0) &&
			(card->ext_csd.cache_ctrl & 1)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_FLUSH_CACHE, 1, 0);
		if (err)
			pr_err("%s: cache flush error %d\n",
					mmc_hostname(card->host), err);
	}

	return err;
}
EXPORT_SYMBOL(mmc_flush_cache);

int mmc_cache_ctrl(struct mmc_host *host, u8 enable)
{
	struct mmc_card *card = host->card;
	unsigned int timeout;
	int err = 0;

	if (!(host->caps2 & MMC_CAP2_CACHE_CTRL) ||
			mmc_card_is_removable(host))
		return err;

	mmc_claim_host(host);
	if (card && mmc_card_mmc(card) &&
			(card->ext_csd.cache_size > 0)) {
		enable = !!enable;

		if (card->ext_csd.cache_ctrl ^ enable) {
			timeout = enable ? card->ext_csd.generic_cmd6_time : 0;
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CACHE_CTRL, enable, timeout);
			if (err)
				pr_err("%s: cache %s error %d\n",
						mmc_hostname(card->host),
						enable ? "on" : "off",
						err);
			else
				card->ext_csd.cache_ctrl = enable;
		}
	}
	mmc_release_host(host);

	return err;
}
EXPORT_SYMBOL(mmc_cache_ctrl);

#ifdef CONFIG_PM

int mmc_suspend_host(struct mmc_host *host)
{
	int err = 0;

	if (mmc_bus_needs_resume(host))
		return 0;

	if (cancel_delayed_work(&host->detect))
		wake_unlock(&host->detect_wake_lock);
	mmc_flush_scheduled_work();
	err = mmc_cache_ctrl(host, 0);
	if (err)
		goto out;

	mmc_bus_get(host);
	if (host->bus_ops && !host->bus_dead) {
		if (!(host->card && mmc_card_sdio(host->card)))
			if (!mmc_try_claim_host(host))
				err = -EBUSY;

		if (!err) {
			if (host->bus_ops->suspend) {
#if 0
				if (mmc_card_doing_bkops(host->card))
					mmc_interrupt_bkops(host->card);
#endif
				err = host->bus_ops->suspend(host);
			}
			if (!(host->card && mmc_card_sdio(host->card)))
				mmc_release_host(host);

			if (err == -ENOSYS || !host->bus_ops->resume) {
				if (host->bus_ops->remove)
					host->bus_ops->remove(host);
				mmc_claim_host(host);
				mmc_detach_bus(host);
				mmc_power_off(host);
				mmc_release_host(host);
				host->pm_flags = 0;
				err = 0;
			}
		}
	}
#ifdef CONFIG_PM_RUNTIME
	if (mmc_bus_manual_resume(host) &&
		(host->card && !mmc_card_doing_bkops(host->card)
		&& !mmc_card_doing_sanitize(host->card)))
			host->bus_resume_flags |= MMC_BUSRESUME_NEEDS_RESUME;
#endif

	mmc_bus_put(host);

	if (!err && !mmc_card_keep_power(host) &&
		(host->card && !mmc_card_doing_bkops(host->card)
		&& !mmc_card_doing_sanitize(host->card)))
			mmc_power_off(host);

out:
	return err;
}

EXPORT_SYMBOL(mmc_suspend_host);

int mmc_resume_host(struct mmc_host *host)
{
	int err = 0;

	mmc_bus_get(host);
	if (mmc_bus_manual_resume(host)) {
		if (host->card && !mmc_card_doing_bkops(host->card)
			&& !mmc_card_doing_sanitize(host->card))
			host->bus_resume_flags |= MMC_BUSRESUME_NEEDS_RESUME;
		mmc_bus_put(host);
		return 0;
	}

	if (host->bus_ops && !host->bus_dead) {
		if (!mmc_card_keep_power(host)) {
			mmc_power_up(host);
			mmc_select_voltage(host, host->ocr);
			if (mmc_card_sdio(host->card) &&
			    (host->caps & MMC_CAP_POWER_OFF_CARD)) {
				pm_runtime_disable(&host->card->dev);
				pm_runtime_set_active(&host->card->dev);
				pm_runtime_enable(&host->card->dev);
			}
		}
		BUG_ON(!host->bus_ops->resume);
		err = host->bus_ops->resume(host);
		if (err) {
			pr_warning("%s: error %d during resume "
					    "(card was removed?)\n",
					    mmc_hostname(host), err);
			err = 0;
		}
	}
	host->pm_flags &= ~MMC_PM_KEEP_POWER;
	mmc_bus_put(host);

	return err;
}
EXPORT_SYMBOL(mmc_resume_host);

#ifdef CONFIG_MMC_CPRM_SUPPORT
int mmc_read_sd_status(struct mmc_card *card)
{
	return mmc_sd_read_sd_status(card);
}

EXPORT_SYMBOL(mmc_read_sd_status);
#endif

int mmc_pm_notify(struct notifier_block *notify_block,
					unsigned long mode, void *unused)
{
	struct mmc_host *host = container_of(
		notify_block, struct mmc_host, pm_notify);
	unsigned long flags;


	switch (mode) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:

		spin_lock_irqsave(&host->lock, flags);
		if (mmc_bus_needs_resume(host)) {
			spin_unlock_irqrestore(&host->lock, flags);
			break;
		}
		host->rescan_disable = 1;
		spin_unlock_irqrestore(&host->lock, flags);
		if (cancel_delayed_work_sync(&host->detect))
			wake_unlock(&host->detect_wake_lock);

		if (!host->bus_ops || host->bus_ops->suspend)
			break;
		mmc_claim_host(host);
		if (mmc_can_poweroff_notify(host->card)) {
			int err = host->bus_ops->poweroff_notify(host,
						MMC_PW_OFF_NOTIFY_SHORT);
			if (err)
				pr_info("%s: error [%d] in poweroff notify\n",
					mmc_hostname(host), err);
		}
		mmc_release_host(host);

		
		if (host->bus_ops->remove)
			host->bus_ops->remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
		host->pm_flags = 0;
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:

		spin_lock_irqsave(&host->lock, flags);
		if (mmc_bus_manual_resume(host)) {
			spin_unlock_irqrestore(&host->lock, flags);
			break;
		}
		host->rescan_disable = 0;
		spin_unlock_irqrestore(&host->lock, flags);
		mmc_detect_change(host, 0);

	}

	return 0;
}
#endif

#ifdef CONFIG_MMC_EMBEDDED_SDIO
void mmc_set_embedded_sdio_data(struct mmc_host *host,
				struct sdio_cis *cis,
				struct sdio_cccr *cccr,
				struct sdio_embedded_func *funcs,
				int num_funcs)
{
	host->embedded_sdio_data.cis = cis;
	host->embedded_sdio_data.cccr = cccr;
	host->embedded_sdio_data.funcs = funcs;
	host->embedded_sdio_data.num_funcs = num_funcs;
}

EXPORT_SYMBOL(mmc_set_embedded_sdio_data);
#endif

static void mmc_req_tout_timer_hdlr(unsigned long data)
{
	printk("[mmc-SD] : Remove time out dump start \n");
	show_state_filter(TASK_UNINTERRUPTIBLE);
	printk("[mmc-SD] : Remove time out dump finish \n");
	del_timer(&sd_remove_tout_timer);
}

extern unsigned int get_tamper_sf(void);
static int __init mmc_init(void)
{
	int ret;

	workqueue = alloc_ordered_workqueue("kmmcd", 0);
	if (!workqueue)
		return -ENOMEM;
	stats_workqueue = create_singlethread_workqueue("mmc_stats");
	if (!stats_workqueue)
		return -ENOMEM;
	if (get_tamper_sf() == 1)
		stats_interval = MMC_STATS_LOG_INTERVAL;

	wake_lock_init(&mmc_removal_work_wake_lock, WAKE_LOCK_SUSPEND,
		       "mmc_removal_work");

	setup_timer(&sd_remove_tout_timer, mmc_req_tout_timer_hdlr,
		(unsigned long)NULL);

	ret = mmc_register_bus();
	if (ret)
		goto destroy_workqueue;

	ret = mmc_register_host_class();
	if (ret)
		goto unregister_bus;

	ret = sdio_register_bus();
	if (ret)
		goto unregister_host_class;

	return 0;

unregister_host_class:
	mmc_unregister_host_class();
unregister_bus:
	mmc_unregister_bus();
destroy_workqueue:
	destroy_workqueue(workqueue);
	if (stats_workqueue)
		destroy_workqueue(stats_workqueue);
	wake_lock_destroy(&mmc_removal_work_wake_lock);

	return ret;
}

static void __exit mmc_exit(void)
{
	sdio_unregister_bus();
	mmc_unregister_host_class();
	mmc_unregister_bus();
	destroy_workqueue(workqueue);
	if (stats_workqueue)
		destroy_workqueue(stats_workqueue);
	wake_lock_destroy(&mmc_removal_work_wake_lock);
	del_timer_sync(&sd_remove_tout_timer);
}

subsys_initcall(mmc_init);
module_exit(mmc_exit);

MODULE_LICENSE("GPL");
