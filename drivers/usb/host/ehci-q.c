/*
 * Copyright (C) 2001-2004 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */





#ifdef CONFIG_ARCH_APQ8064
void dbg_log_event(struct urb *urb, char * event, unsigned extra);
#endif

static int
qtd_fill(struct ehci_hcd *ehci, struct ehci_qtd *qtd, dma_addr_t buf,
		  size_t len, int token, int maxpacket)
{
	int	i, count;
	u64	addr = buf;

	
	qtd->hw_buf[0] = cpu_to_hc32(ehci, (u32)addr);
	qtd->hw_buf_hi[0] = cpu_to_hc32(ehci, (u32)(addr >> 32));
	count = 0x1000 - (buf & 0x0fff);	
	if (likely (len < count))		
		count = len;
	else {
		buf +=  0x1000;
		buf &= ~0x0fff;

		
		for (i = 1; count < len && i < 5; i++) {
			addr = buf;
			qtd->hw_buf[i] = cpu_to_hc32(ehci, (u32)addr);
			qtd->hw_buf_hi[i] = cpu_to_hc32(ehci,
					(u32)(addr >> 32));
			buf += 0x1000;
			if ((count + 0x1000) < len)
				count += 0x1000;
			else
				count = len;
		}

		
		if (count != len)
			count -= (count % maxpacket);
	}
	qtd->hw_token = cpu_to_hc32(ehci, (count << 16) | token);
	qtd->length = count;

	return count;
}


static inline void
qh_update (struct ehci_hcd *ehci, struct ehci_qh *qh, struct ehci_qtd *qtd)
{
	struct ehci_qh_hw *hw = qh->hw;

	
	BUG_ON(qh->qh_state != QH_STATE_IDLE);

	hw->hw_qtd_next = QTD_NEXT(ehci, qtd->qtd_dma);
	hw->hw_alt_next = EHCI_LIST_END(ehci);

	if (!(hw->hw_info1 & cpu_to_hc32(ehci, 1 << 14))) {
		unsigned	is_out, epnum;

		is_out = qh->is_out;
		epnum = (hc32_to_cpup(ehci, &hw->hw_info1) >> 8) & 0x0f;
		if (unlikely (!usb_gettoggle (qh->dev, epnum, is_out))) {
			hw->hw_token &= ~cpu_to_hc32(ehci, QTD_TOGGLE);
			usb_settoggle (qh->dev, epnum, is_out, 1);
		}
	}

	hw->hw_token &= cpu_to_hc32(ehci, QTD_TOGGLE | QTD_STS_PING);
}

static void
qh_refresh (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd *qtd;

	if (list_empty (&qh->qtd_list))
		qtd = qh->dummy;
	else {
		qtd = list_entry (qh->qtd_list.next,
				struct ehci_qtd, qtd_list);
		if (cpu_to_hc32(ehci, qtd->qtd_dma) == qh->hw->hw_current) {
			qh->hw->hw_qtd_next = qtd->hw_next;
			qtd = NULL;
		}
	}

	if (qtd)
		qh_update (ehci, qh, qtd);
}


static void qh_link_async(struct ehci_hcd *ehci, struct ehci_qh *qh);

static void ehci_clear_tt_buffer_complete(struct usb_hcd *hcd,
		struct usb_host_endpoint *ep)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	struct ehci_qh		*qh = ep->hcpriv;
	unsigned long		flags;

	spin_lock_irqsave(&ehci->lock, flags);
	qh->clearing_tt = 0;
	if (qh->qh_state == QH_STATE_IDLE && !list_empty(&qh->qtd_list)
			&& ehci->rh_state == EHCI_RH_RUNNING)
		qh_link_async(ehci, qh);
	spin_unlock_irqrestore(&ehci->lock, flags);
}

static void ehci_clear_tt_buffer(struct ehci_hcd *ehci, struct ehci_qh *qh,
		struct urb *urb, u32 token)
{

	if (urb->dev->tt && !usb_pipeint(urb->pipe) && !qh->clearing_tt) {
#ifdef DEBUG
		struct usb_device *tt = urb->dev->tt->hub;
		dev_dbg(&tt->dev,
			"clear tt buffer port %d, a%d ep%d t%08x\n",
			urb->dev->ttport, urb->dev->devnum,
			usb_pipeendpoint(urb->pipe), token);
#endif 
		if (!ehci_is_TDI(ehci)
				|| urb->dev->tt->hub !=
				   ehci_to_hcd(ehci)->self.root_hub) {
			if (usb_hub_clear_tt_buffer(urb) == 0)
				qh->clearing_tt = 1;
		} else {

		}
	}
}

static int qtd_copy_status (
	struct ehci_hcd *ehci,
	struct urb *urb,
	size_t length,
	u32 token
)
{
	int	status = -EINPROGRESS;

	
	if (likely (QTD_PID (token) != 2))
		urb->actual_length += length - QTD_LENGTH (token);

	
	if (unlikely(urb->unlinked))
		return status;

	
	if (unlikely (IS_SHORT_READ (token)))
		status = -EREMOTEIO;

	
	if (token & QTD_STS_HALT) {
		if (token & QTD_STS_BABBLE) {
			
			status = -EOVERFLOW;
		
		} else if (QTD_CERR(token)) {
			status = -EPIPE;

		} else if (token & QTD_STS_MMF) {
			
			status = -EPROTO;
		} else if (token & QTD_STS_DBE) {
			status = (QTD_PID (token) == 1) 
				? -ENOSR  
				: -ECOMM; 
		} else if (token & QTD_STS_XACT) {
			
			ehci_dbg(ehci, "devpath %s ep%d%s 3strikes\n",
				urb->dev->devpath,
				usb_pipeendpoint(urb->pipe),
				usb_pipein(urb->pipe) ? "in" : "out");
			status = -EPROTO;
		} else {	
			status = -EPROTO;
		}

		ehci_vdbg (ehci,
			"dev%d ep%d%s qtd token %08x --> status %d\n",
			usb_pipedevice (urb->pipe),
			usb_pipeendpoint (urb->pipe),
			usb_pipein (urb->pipe) ? "in" : "out",
			token, status);
	}

	return status;
}

static void
ehci_urb_done(struct ehci_hcd *ehci, struct urb *urb, int status)
__releases(ehci->lock)
__acquires(ehci->lock)
{
	if (likely (urb->hcpriv != NULL)) {
		struct ehci_qh	*qh = (struct ehci_qh *) urb->hcpriv;

		
		if ((qh->hw->hw_info2 & cpu_to_hc32(ehci, QH_SMASK)) != 0) {

			
			ehci_to_hcd(ehci)->self.bandwidth_int_reqs--;
		}
		qh_put (qh);
	}

	if (unlikely(urb->unlinked)) {
		COUNT(ehci->stats.unlink);
	} else {
		
		if (status == -EINPROGRESS || status == -EREMOTEIO)
			status = 0;
		COUNT(ehci->stats.complete);
	}

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s status %d len %d/%d\n",
		__func__, urb->dev->devpath, urb,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		status,
		urb->actual_length, urb->transfer_buffer_length);
#endif

	
	usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
	spin_unlock (&ehci->lock);
	usb_hcd_giveback_urb(ehci_to_hcd(ehci), urb, status);
	spin_lock (&ehci->lock);
}

static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);
static void unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);

static int qh_schedule (struct ehci_hcd *ehci, struct ehci_qh *qh);

static unsigned
qh_completions (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd		*last, *end = qh->dummy;
	struct list_head	*entry, *tmp;
	int			last_status;
	int			stopped;
	unsigned		count = 0;
	u8			state;
	struct ehci_qh_hw	*hw = qh->hw;

	if (unlikely (list_empty (&qh->qtd_list)))
		return count;

	state = qh->qh_state;
	qh->qh_state = QH_STATE_COMPLETING;
	stopped = (state == QH_STATE_IDLE);

 rescan:
	last = NULL;
	last_status = -EINPROGRESS;
	qh->needs_rescan = 0;

	list_for_each_safe (entry, tmp, &qh->qtd_list) {
		struct ehci_qtd	*qtd;
		struct urb	*urb;
		u32		token = 0;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		urb = qtd->urb;

		
		if (last) {
			if (likely (last->urb != urb)) {
				ehci_urb_done(ehci, last->urb, last_status);
				count++;
				last_status = -EINPROGRESS;
			}
			ehci_qtd_free (ehci, last);
			last = NULL;
		}

		
		if (qtd == end)
			break;

		
		rmb ();
		token = hc32_to_cpu(ehci, qtd->hw_token);

		
 retry_xacterr:
		if ((token & QTD_STS_ACTIVE) == 0) {

			
			if (token & QTD_STS_DBE)
				ehci_dbg(ehci,
					"detected DataBufferErr for urb %p ep%d%s len %d, qtd %p [qh %p]\n",
					urb,
					usb_endpoint_num(&urb->ep->desc),
					usb_endpoint_dir_in(&urb->ep->desc) ? "in" : "out",
					urb->transfer_buffer_length,
					qtd,
					qh);

			if ((token & QTD_STS_HALT) != 0) {

				if ((token & QTD_STS_XACT) &&
						QTD_CERR(token) == 0 &&
						++qh->xacterrs < QH_XACTERR_MAX &&
						!urb->unlinked) {
					ehci_dbg(ehci,
	"detected XactErr len %zu/%zu retry %d\n",
	qtd->length - QTD_LENGTH(token), qtd->length, qh->xacterrs);

					token &= ~QTD_STS_HALT;
					token |= QTD_STS_ACTIVE |
							(EHCI_TUNE_CERR << 10);
					qtd->hw_token = cpu_to_hc32(ehci,
							token);
					wmb();
					hw->hw_token = cpu_to_hc32(ehci,
							token);
					goto retry_xacterr;
				}
				stopped = 1;

			} else if (IS_SHORT_READ (token)
					&& !(qtd->hw_alt_next
						& EHCI_LIST_END(ehci))) {
				stopped = 1;
			}

		
		} else if (likely (!stopped
				&& ehci->rh_state == EHCI_RH_RUNNING)) {
			break;

		
		} else {
			stopped = 1;

			
			if (ehci->rh_state != EHCI_RH_RUNNING)
				last_status = -ESHUTDOWN;

			else if (last_status == -EINPROGRESS && !urb->unlinked)
				continue;

			
			if (state == QH_STATE_IDLE
					&& cpu_to_hc32(ehci, qtd->qtd_dma)
						== hw->hw_current) {
				token = hc32_to_cpu(ehci, hw->hw_token);

				ehci_clear_tt_buffer(ehci, qh, urb, token);
			}
		}

		if (last_status == -EINPROGRESS) {
			last_status = qtd_copy_status(ehci, urb,
					qtd->length, token);
			if (last_status == -EREMOTEIO
					&& (qtd->hw_alt_next
						& EHCI_LIST_END(ehci)))
				last_status = -EINPROGRESS;

			if (unlikely(last_status != -EINPROGRESS &&
					last_status != -EREMOTEIO)) {
				if (last_status != -EPIPE)
					ehci_clear_tt_buffer(ehci, qh, urb,
							token);
			}
		}

		if (stopped && qtd->qtd_list.prev != &qh->qtd_list) {
			last = list_entry (qtd->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			last->hw_next = qtd->hw_next;
		}

		
		list_del (&qtd->qtd_list);
		last = qtd;

		
		qh->xacterrs = 0;
	}

	
	if (likely (last != NULL)) {
		ehci_urb_done(ehci, last->urb, last_status);
		count++;
		ehci_qtd_free (ehci, last);
	}

	
	if (unlikely(qh->needs_rescan)) {
		
		if (state == QH_STATE_IDLE)
			goto rescan;

		if (state != QH_STATE_LINKED)
			qh->needs_rescan = 0;
	}

	
	qh->qh_state = state;

	if (stopped != 0 || hw->hw_qtd_next == EHCI_LIST_END(ehci)) {
		switch (state) {
		case QH_STATE_IDLE:
			qh_refresh(ehci, qh);
			break;
		case QH_STATE_LINKED:

			
			qh->needs_rescan = 1;
			break;
		
		}
	}

	return count;
}


#define hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))
#define max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

static void qtd_list_free (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list
) {
	struct list_head	*entry, *temp;

	list_for_each_safe (entry, temp, qtd_list) {
		struct ehci_qtd	*qtd;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		list_del (&qtd->qtd_list);
		ehci_qtd_free (ehci, qtd);
	}
}

static struct list_head *
qh_urb_transaction (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*head,
	gfp_t			flags
) {
	struct ehci_qtd		*qtd, *qtd_prev;
	dma_addr_t		buf;
	int			len, this_sg_len, maxpacket;
	int			is_input;
	u32			token;
	int			i;
	struct scatterlist	*sg;

	qtd = ehci_qtd_alloc (ehci, flags);
	if (unlikely (!qtd))
		return NULL;
	list_add_tail (&qtd->qtd_list, head);
	qtd->urb = urb;
#ifdef CONFIG_ARCH_APQ8064
	dbg_log_event(NULL,"Alloc QTD: ",(int) qtd);
#endif
	token = QTD_STS_ACTIVE;
	token |= (EHCI_TUNE_CERR << 10);
	

	len = urb->transfer_buffer_length;
	is_input = usb_pipein (urb->pipe);
	if (usb_pipecontrol (urb->pipe)) {
		
		qtd_fill(ehci, qtd, urb->setup_dma,
				sizeof (struct usb_ctrlrequest),
				token | (2  << 8), 8);

		
		token ^= QTD_TOGGLE;
		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
#ifdef CONFIG_ARCH_APQ8064
		dbg_log_event(NULL,"alloc SETUP QTD ",(int) qtd);
#endif
		qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);

		
		if (len == 0)
			token |= (1  << 8);
	}

	i = urb->num_mapped_sgs;
	if (len > 0 && i > 0) {
		sg = urb->sg;
		buf = sg_dma_address(sg);

		this_sg_len = min_t(int, sg_dma_len(sg), len);
	} else {
		sg = NULL;
		buf = urb->transfer_dma;
		this_sg_len = len;
	}

	if (is_input)
		token |= (1  << 8);
	

	maxpacket = max_packet(usb_maxpacket(urb->dev, urb->pipe, !is_input));

	for (;;) {
		int this_qtd_len;

		this_qtd_len = qtd_fill(ehci, qtd, buf, this_sg_len, token,
				maxpacket);
		this_sg_len -= this_qtd_len;
		len -= this_qtd_len;
		buf += this_qtd_len;

		if (is_input)
			qtd->hw_alt_next = ehci->async->hw->hw_alt_next;

		
		if ((maxpacket & (this_qtd_len + (maxpacket - 1))) == 0)
			token ^= QTD_TOGGLE;

		if (likely(this_sg_len <= 0)) {
			if (--i <= 0 || len <= 0)
				break;
			sg = sg_next(sg);
			buf = sg_dma_address(sg);
			this_sg_len = min_t(int, sg_dma_len(sg), len);
		}

		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
#ifdef CONFIG_ARCH_APQ8064
		dbg_log_event(NULL, "Alloc another QTD: ",(int) qtd);
#endif
		qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);
	}

	if (likely ((urb->transfer_flags & URB_SHORT_NOT_OK) == 0
				|| usb_pipecontrol (urb->pipe)))
		qtd->hw_alt_next = EHCI_LIST_END(ehci);

	if (likely (urb->transfer_buffer_length != 0)) {
		int	one_more = 0;

		if (usb_pipecontrol (urb->pipe)) {
			one_more = 1;
			token ^= 0x0100;	
			token |= QTD_TOGGLE;	
		} else if (usb_pipeout(urb->pipe)
				&& (urb->transfer_flags & URB_ZERO_PACKET)
				&& !(urb->transfer_buffer_length % maxpacket)) {
			one_more = 1;
		}
		if (one_more) {
			qtd_prev = qtd;
			qtd = ehci_qtd_alloc (ehci, flags);
			if (unlikely (!qtd))
				goto cleanup;
			qtd->urb = urb;
			qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
			list_add_tail (&qtd->qtd_list, head);

			
			qtd_fill(ehci, qtd, 0, 0, token, 0);
		}
	}

	
	if (likely (!(urb->transfer_flags & URB_NO_INTERRUPT)))
		qtd->hw_token |= cpu_to_hc32(ehci, QTD_IOC);
	return head;

cleanup:
	qtd_list_free (ehci, urb, head);
	return NULL;
}




static struct ehci_qh *
qh_make (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	gfp_t			flags
) {
	struct ehci_qh		*qh = ehci_qh_alloc (ehci, flags);
	u32			info1 = 0, info2 = 0;
	int			is_input, type;
	int			maxp = 0;
	struct usb_tt		*tt = urb->dev->tt;
	struct ehci_qh_hw	*hw;

	if (!qh)
		return qh;

	info1 |= usb_pipeendpoint (urb->pipe) << 8;
	info1 |= usb_pipedevice (urb->pipe) << 0;

	is_input = usb_pipein (urb->pipe);
	type = usb_pipetype (urb->pipe);
	maxp = usb_maxpacket (urb->dev, urb->pipe, !is_input);

	if (max_packet(maxp) > 1024) {
		ehci_dbg(ehci, "bogus qh maxpacket %d\n", max_packet(maxp));
		goto done;
	}

	if (type == PIPE_INTERRUPT) {
		qh->usecs = NS_TO_US(usb_calc_bus_time(USB_SPEED_HIGH,
				is_input, 0,
				hb_mult(maxp) * max_packet(maxp)));
		qh->start = NO_FRAME;
		qh->stamp = ehci->periodic_stamp;

		if (urb->dev->speed == USB_SPEED_HIGH) {
			qh->c_usecs = 0;
			qh->gap_uf = 0;

			qh->period = urb->interval >> 3;
			if (qh->period == 0 && urb->interval != 1) {
				urb->interval = 1;
			} else if (qh->period > ehci->periodic_size) {
				qh->period = ehci->periodic_size;
				urb->interval = qh->period << 3;
			}
		} else {
			int		think_time;

			
			qh->gap_uf = 1 + usb_calc_bus_time (urb->dev->speed,
					is_input, 0, maxp) / (125 * 1000);

			
			if (is_input) {		
				qh->c_usecs = qh->usecs + HS_USECS (0);
				qh->usecs = HS_USECS (1);
			} else {		
				qh->usecs += HS_USECS (1);
				qh->c_usecs = HS_USECS (0);
			}

			think_time = tt ? tt->think_time : 0;
			qh->tt_usecs = NS_TO_US (think_time +
					usb_calc_bus_time (urb->dev->speed,
					is_input, 0, max_packet (maxp)));
			qh->period = urb->interval;
			if (qh->period > ehci->periodic_size) {
				qh->period = ehci->periodic_size;
				urb->interval = qh->period;
			}
		}
	}

	
	qh->dev = urb->dev;

	
	switch (urb->dev->speed) {
	case USB_SPEED_LOW:
		info1 |= (1 << 12);	
		

	case USB_SPEED_FULL:
		
		if (type != PIPE_INTERRUPT)
			info1 |= (EHCI_TUNE_RL_TT << 28);
		if (type == PIPE_CONTROL) {
			info1 |= (1 << 27);	
			info1 |= 1 << 14;	
		}
		info1 |= maxp << 16;

		info2 |= (EHCI_TUNE_MULT_TT << 30);

		if (ehci_has_fsl_portno_bug(ehci))
			info2 |= (urb->dev->ttport-1) << 23;
		else
			info2 |= urb->dev->ttport << 23;

		if (tt && tt->hub != ehci_to_hcd(ehci)->self.root_hub)
			info2 |= tt->hub->devnum << 16;

		

		break;

	case USB_SPEED_HIGH:		
		info1 |= (2 << 12);	
		if (type == PIPE_CONTROL) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			info1 |= 64 << 16;	
			info1 |= 1 << 14;	
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else if (type == PIPE_BULK) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			info1 |= max_packet(maxp) << 16;
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else {		
			info1 |= max_packet (maxp) << 16;
			info2 |= hb_mult (maxp) << 30;
		}
		break;
	default:
		dbg ("bogus dev %p speed %d", urb->dev, urb->dev->speed);
done:
		qh_put (qh);
		return NULL;
	}

	

	
	qh->qh_state = QH_STATE_IDLE;
	hw = qh->hw;
	hw->hw_info1 = cpu_to_hc32(ehci, info1);
	hw->hw_info2 = cpu_to_hc32(ehci, info2);
	qh->is_out = !is_input;
	usb_settoggle (urb->dev, usb_pipeendpoint (urb->pipe), !is_input, 1);
	qh_refresh (ehci, qh);
	return qh;
}



static void qh_link_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	__hc32		dma = QH_NEXT(ehci, qh->qh_dma);
	struct ehci_qh	*head;

	
	if (unlikely(qh->clearing_tt))
		return;

	WARN_ON(qh->qh_state != QH_STATE_IDLE);

	
	head = ehci->async;
	timer_action_done (ehci, TIMER_ASYNC_OFF);
	if (!head->qh_next.qh) {
		u32	cmd = ehci_readl(ehci, &ehci->regs->command);

		if (!(cmd & CMD_ASE)) {
			
			(void)handshake(ehci, &ehci->regs->status,
					STS_ASS, 0, 150);
			cmd |= CMD_ASE;
			ehci_writel(ehci, cmd, &ehci->regs->command);
			
		}
	}

	
	qh_refresh(ehci, qh);

	
	qh->qh_next = head->qh_next;
	qh->hw->hw_next = head->hw->hw_next;
	wmb ();

	head->qh_next.qh = qh;
	head->hw->hw_next = dma;
#ifdef CONFIG_ARCH_APQ8064
	dbg_log_event(NULL, "linking QH to current QH, cur QH =  ",(int) head);
#endif
	qh_get(qh);
	qh->xacterrs = 0;
	qh->qh_state = QH_STATE_LINKED;
	
}


static struct ehci_qh *qh_append_tds (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	int			epnum,
	void			**ptr
)
{
	struct ehci_qh		*qh = NULL;
	__hc32			qh_addr_mask = cpu_to_hc32(ehci, 0x7f);

	qh = (struct ehci_qh *) *ptr;
	if (unlikely (qh == NULL)) {
		
#ifdef CONFIG_ARCH_APQ8064
		dbg_log_event(NULL, "QH does not exisit", 0);
#endif
		qh = qh_make (ehci, urb, GFP_ATOMIC);
		*ptr = qh;
	}
	if (likely (qh != NULL)) {
		struct ehci_qtd	*qtd;

		if (unlikely (list_empty (qtd_list)))
			qtd = NULL;
		else
			qtd = list_entry (qtd_list->next, struct ehci_qtd,
					qtd_list);

		
		if (unlikely (epnum == 0)) {

                        
                        if (usb_pipedevice (urb->pipe) == 0)
				qh->hw->hw_info1 &= ~qh_addr_mask;
		}

		if (likely (qtd != NULL)) {
			struct ehci_qtd		*dummy;
			dma_addr_t		dma;
			__hc32			token;

#ifdef CONFIG_ARCH_APQ8064
			dbg_log_event(NULL,"adding QTD to QH " ,(int) qh);
#endif
			token = qtd->hw_token;
			qtd->hw_token = HALT_BIT(ehci);

			dummy = qh->dummy;

			dma = dummy->qtd_dma;
			*dummy = *qtd;
			dummy->qtd_dma = dma;

			list_del (&qtd->qtd_list);
			list_add (&dummy->qtd_list, qtd_list);
			list_splice_tail(qtd_list, &qh->qtd_list);

			ehci_qtd_init(ehci, qtd, qtd->qtd_dma);
			qh->dummy = qtd;

			
			dma = qtd->qtd_dma;
			qtd = list_entry (qh->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			qtd->hw_next = QTD_NEXT(ehci, dma);

			
			wmb ();
			dummy->hw_token = token;

			urb->hcpriv = qh_get (qh);
		}
	}
	return qh;
}


static int
submit_async (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	gfp_t			mem_flags
) {
	int			epnum;
	unsigned long		flags;
	struct ehci_qh		*qh = NULL;
	int			rc;

	epnum = urb->ep->desc.bEndpointAddress;

#ifdef EHCI_URB_TRACE
	{
		struct ehci_qtd *qtd;
		qtd = list_entry(qtd_list->next, struct ehci_qtd, qtd_list);
		ehci_dbg(ehci,
			 "%s %s urb %p ep%d%s len %d, qtd %p [qh %p]\n",
			 __func__, urb->dev->devpath, urb,
			 epnum & 0x0f, (epnum & USB_DIR_IN) ? "in" : "out",
			 urb->transfer_buffer_length,
			 qtd, urb->ep->hcpriv);
	}
#endif

	spin_lock_irqsave (&ehci->lock, flags);
	if (unlikely(!HCD_HW_ACCESSIBLE(ehci_to_hcd(ehci)))) {
		rc = -ESHUTDOWN;
		goto done;
	}
	rc = usb_hcd_link_urb_to_ep(ehci_to_hcd(ehci), urb);
	if (unlikely(rc))
		goto done;

	qh = qh_append_tds(ehci, urb, qtd_list, epnum, &urb->ep->hcpriv);
	if (unlikely(qh == NULL)) {
		usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
		rc = -ENOMEM;
		goto done;
	}

	if (likely (qh->qh_state == QH_STATE_IDLE))
		qh_link_async(ehci, qh);
 done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	if (unlikely (qh == NULL))
		qtd_list_free (ehci, urb, qtd_list);
	return rc;
}

#ifdef CONFIG_USB_EHCI_EHSET
static int
submit_single_step_set_feature(
	struct usb_hcd  *hcd,
	struct urb      *urb,
	int 		is_setup
) {
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	struct list_head	qtd_list;
	struct list_head	*head ;

	struct ehci_qtd		*qtd, *qtd_prev;
	dma_addr_t		buf;
	int			len, maxpacket;
	u32			token;

	INIT_LIST_HEAD(&qtd_list);
	head = &qtd_list;

	qtd = ehci_qtd_alloc(ehci, GFP_KERNEL);
	if (unlikely(!qtd))
		return -1;
	list_add_tail(&qtd->qtd_list, head);
	qtd->urb = urb;

	token = QTD_STS_ACTIVE;
	token |= (EHCI_TUNE_CERR << 10);

	len = urb->transfer_buffer_length;
	if (is_setup) {
		
		qtd_fill(ehci, qtd, urb->setup_dma,
				sizeof(struct usb_ctrlrequest),
				token | (2  << 8), 8);

		submit_async(ehci, urb, &qtd_list, GFP_ATOMIC);
		return 0; 
	}

	token ^= QTD_TOGGLE;   
	buf = urb->transfer_dma;

	token |= (1  << 8);  

	maxpacket = max_packet(usb_maxpacket(urb->dev, urb->pipe, 0));

	qtd_fill(ehci, qtd, buf, len, token, maxpacket);


	qtd->hw_alt_next = EHCI_LIST_END(ehci);

	token ^= 0x0100;	
	token |= QTD_TOGGLE;	

	qtd_prev = qtd;
	qtd = ehci_qtd_alloc(ehci, GFP_ATOMIC);
	if (unlikely(!qtd))
		goto cleanup;
	qtd->urb = urb;
	qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
	list_add_tail(&qtd->qtd_list, head);

	
	qtd_fill(ehci, qtd, 0, 0, token, 0);

	
	if (likely(!(urb->transfer_flags & URB_NO_INTERRUPT)))
		qtd->hw_token |= cpu_to_hc32(ehci, QTD_IOC);

	submit_async(ehci, urb, &qtd_list, GFP_KERNEL);

	return 0;

cleanup:
	qtd_list_free(ehci, urb, head);
	return -1;
}
#endif



static void end_unlink_async (struct ehci_hcd *ehci)
{
	struct ehci_qh		*qh = ehci->reclaim;
	struct ehci_qh		*next;

	iaa_watchdog_done(ehci);

	
	qh->qh_state = QH_STATE_IDLE;
	qh->qh_next.qh = NULL;
	qh_put (qh);			

	
	next = qh->reclaim;
	ehci->reclaim = next;
	qh->reclaim = NULL;

	qh_completions (ehci, qh);

	if (!list_empty(&qh->qtd_list) && ehci->rh_state == EHCI_RH_RUNNING) {
		qh_link_async (ehci, qh);
	} else {
		if (ehci->rh_state == EHCI_RH_RUNNING
				&& ehci->async->qh_next.qh == NULL)
			timer_action (ehci, TIMER_ASYNC_OFF);
	}
	qh_put(qh);			

	if (next) {
		ehci->reclaim = NULL;
		start_unlink_async (ehci, next);
	}

	if (ehci->has_synopsys_hc_bug)
		ehci_writel(ehci, (u32) ehci->async->qh_dma,
			    &ehci->regs->async_next);
}


static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	int		cmd = ehci_readl(ehci, &ehci->regs->command);
	struct ehci_qh	*prev;

#ifdef DEBUG
	assert_spin_locked(&ehci->lock);
	if (ehci->reclaim
			|| (qh->qh_state != QH_STATE_LINKED
				&& qh->qh_state != QH_STATE_UNLINK_WAIT)
			)
		BUG ();
#endif

	
	if (unlikely (qh == ehci->async)) {
		
		if (ehci->rh_state != EHCI_RH_HALTED
				&& !ehci->reclaim) {
			
			ehci_writel(ehci, cmd & ~CMD_ASE,
				    &ehci->regs->command);
			wmb ();
			
			timer_action_done (ehci, TIMER_ASYNC_OFF);
		}
		return;
	}

	qh->qh_state = QH_STATE_UNLINK;
	ehci->reclaim = qh = qh_get (qh);

#ifdef CONFIG_ARCH_APQ8064
	dbg_log_event(NULL, "unlink QH ",(int) qh);
#endif
	prev = ehci->async;
	while (prev->qh_next.qh != qh)
		prev = prev->qh_next.qh;

	prev->hw->hw_next = qh->hw->hw_next;
#ifdef CONFIG_ARCH_APQ8064
	dbg_log_event(NULL, "unlink QH prev->hw->hw_next",(int) prev->hw->hw_next);
#endif
	prev->qh_next = qh->qh_next;
	if (ehci->qh_scan_next == qh)
		ehci->qh_scan_next = qh->qh_next.qh;
	wmb ();

	
	if (unlikely(ehci->rh_state != EHCI_RH_RUNNING)) {
		end_unlink_async (ehci);
		return;
	}

	cmd |= CMD_IAAD;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	(void)ehci_readl(ehci, &ehci->regs->command);
	iaa_watchdog_start(ehci);
}


static void scan_async (struct ehci_hcd *ehci)
{
	bool			stopped;
	struct ehci_qh		*qh;
	enum ehci_timer_action	action = TIMER_IO_WATCHDOG;

	timer_action_done (ehci, TIMER_ASYNC_SHRINK);
	stopped = (ehci->rh_state != EHCI_RH_RUNNING);

	ehci->qh_scan_next = ehci->async->qh_next.qh;
	while (ehci->qh_scan_next) {
		qh = ehci->qh_scan_next;
		ehci->qh_scan_next = qh->qh_next.qh;
 rescan:
		
		if (!list_empty(&qh->qtd_list)) {
			int temp;

			qh = qh_get(qh);
			temp = qh_completions(ehci, qh);
			if (qh->needs_rescan)
				unlink_async(ehci, qh);
			qh->unlink_time = jiffies + EHCI_SHRINK_JIFFIES;
			qh_put(qh);
			if (temp != 0)
				goto rescan;
		}

		if (list_empty(&qh->qtd_list)
				&& qh->qh_state == QH_STATE_LINKED) {
			if (!ehci->reclaim && (stopped ||
					time_after_eq(jiffies, qh->unlink_time)))
				start_unlink_async(ehci, qh);
			else
				action = TIMER_ASYNC_SHRINK;
		}
	}
	if (action == TIMER_ASYNC_SHRINK)
		timer_action (ehci, TIMER_ASYNC_SHRINK);
}
