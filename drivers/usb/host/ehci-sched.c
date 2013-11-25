/*
 * Copyright (c) 2001-2004 by David Brownell
 * Copyright (c) 2003 Michal Sojka, for high-speed iso transfers
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




static int ehci_get_frame (struct usb_hcd *hcd);

#ifdef CONFIG_PCI

static unsigned ehci_read_frame_index(struct ehci_hcd *ehci)
{
	unsigned uf;

	uf = ehci_readl(ehci, &ehci->regs->frame_index);
	if (unlikely(ehci->frame_index_bug && ((uf & 7) == 0)))
		uf = ehci_readl(ehci, &ehci->regs->frame_index);
	return uf;
}

#endif


static union ehci_shadow *
periodic_next_shadow(struct ehci_hcd *ehci, union ehci_shadow *periodic,
		__hc32 tag)
{
	switch (hc32_to_cpu(ehci, tag)) {
	case Q_TYPE_QH:
		return &periodic->qh->qh_next;
	case Q_TYPE_FSTN:
		return &periodic->fstn->fstn_next;
	case Q_TYPE_ITD:
		return &periodic->itd->itd_next;
	
	default:
		return &periodic->sitd->sitd_next;
	}
}

static __hc32 *
shadow_next_periodic(struct ehci_hcd *ehci, union ehci_shadow *periodic,
		__hc32 tag)
{
	switch (hc32_to_cpu(ehci, tag)) {
	
	case Q_TYPE_QH:
		return &periodic->qh->hw->hw_next;
	
	default:
		return periodic->hw_next;
	}
}

static void periodic_unlink (struct ehci_hcd *ehci, unsigned frame, void *ptr)
{
	union ehci_shadow	*prev_p = &ehci->pshadow[frame];
	__hc32			*hw_p = &ehci->periodic[frame];
	union ehci_shadow	here = *prev_p;

	
	while (here.ptr && here.ptr != ptr) {
		prev_p = periodic_next_shadow(ehci, prev_p,
				Q_NEXT_TYPE(ehci, *hw_p));
		hw_p = shadow_next_periodic(ehci, &here,
				Q_NEXT_TYPE(ehci, *hw_p));
		here = *prev_p;
	}
	
	if (!here.ptr)
		return;

	*prev_p = *periodic_next_shadow(ehci, &here,
			Q_NEXT_TYPE(ehci, *hw_p));

	if (!ehci->use_dummy_qh ||
	    *shadow_next_periodic(ehci, &here, Q_NEXT_TYPE(ehci, *hw_p))
			!= EHCI_LIST_END(ehci))
		*hw_p = *shadow_next_periodic(ehci, &here,
				Q_NEXT_TYPE(ehci, *hw_p));
	else
		*hw_p = ehci->dummy->qh_dma;
}

static unsigned short
periodic_usecs (struct ehci_hcd *ehci, unsigned frame, unsigned uframe)
{
	__hc32			*hw_p = &ehci->periodic [frame];
	union ehci_shadow	*q = &ehci->pshadow [frame];
	unsigned		usecs = 0;
	struct ehci_qh_hw	*hw;

	while (q->ptr) {
		switch (hc32_to_cpu(ehci, Q_NEXT_TYPE(ehci, *hw_p))) {
		case Q_TYPE_QH:
			hw = q->qh->hw;
			
			if (hw->hw_info2 & cpu_to_hc32(ehci, 1 << uframe))
				usecs += q->qh->usecs;
			
			if (hw->hw_info2 & cpu_to_hc32(ehci,
					1 << (8 + uframe)))
				usecs += q->qh->c_usecs;
			hw_p = &hw->hw_next;
			q = &q->qh->qh_next;
			break;
		
		default:
			if (q->fstn->hw_prev != EHCI_LIST_END(ehci)) {
				ehci_dbg (ehci, "ignoring FSTN cost ...\n");
			}
			hw_p = &q->fstn->hw_next;
			q = &q->fstn->fstn_next;
			break;
		case Q_TYPE_ITD:
			if (q->itd->hw_transaction[uframe])
				usecs += q->itd->stream->usecs;
			hw_p = &q->itd->hw_next;
			q = &q->itd->itd_next;
			break;
		case Q_TYPE_SITD:
			
			if (q->sitd->hw_uframe & cpu_to_hc32(ehci,
					1 << uframe)) {
				if (q->sitd->hw_fullspeed_ep &
						cpu_to_hc32(ehci, 1<<31))
					usecs += q->sitd->stream->usecs;
				else	
					usecs += HS_USECS_ISO (188);
			}

			
			if (q->sitd->hw_uframe &
					cpu_to_hc32(ehci, 1 << (8 + uframe))) {
				
				usecs += q->sitd->stream->c_usecs;
			}

			hw_p = &q->sitd->hw_next;
			q = &q->sitd->sitd_next;
			break;
		}
	}
#ifdef	DEBUG
	if (usecs > ehci->uframe_periodic_max)
		ehci_err (ehci, "uframe %d sched overrun: %d usecs\n",
			frame * 8 + uframe, usecs);
#endif
	return usecs;
}


static int same_tt (struct usb_device *dev1, struct usb_device *dev2)
{
	if (!dev1->tt || !dev2->tt)
		return 0;
	if (dev1->tt != dev2->tt)
		return 0;
	if (dev1->tt->multi)
		return dev1->ttport == dev2->ttport;
	else
		return 1;
}

#ifdef CONFIG_USB_EHCI_TT_NEWSCHED

static inline unsigned char tt_start_uframe(struct ehci_hcd *ehci, __hc32 mask)
{
	unsigned char smask = QH_SMASK & hc32_to_cpu(ehci, mask);
	if (!smask) {
		ehci_err(ehci, "invalid empty smask!\n");
		
		return 7;
	}
	return ffs(smask) - 1;
}

static const unsigned char
max_tt_usecs[] = { 125, 125, 125, 125, 125, 125, 30, 0 };

static inline void carryover_tt_bandwidth(unsigned short tt_usecs[8])
{
	int i;
	for (i=0; i<7; i++) {
		if (max_tt_usecs[i] < tt_usecs[i]) {
			tt_usecs[i+1] += tt_usecs[i] - max_tt_usecs[i];
			tt_usecs[i] = max_tt_usecs[i];
		}
	}
}

static void
periodic_tt_usecs (
	struct ehci_hcd *ehci,
	struct usb_device *dev,
	unsigned frame,
	unsigned short tt_usecs[8]
)
{
	__hc32			*hw_p = &ehci->periodic [frame];
	union ehci_shadow	*q = &ehci->pshadow [frame];
	unsigned char		uf;

	memset(tt_usecs, 0, 16);

	while (q->ptr) {
		switch (hc32_to_cpu(ehci, Q_NEXT_TYPE(ehci, *hw_p))) {
		case Q_TYPE_ITD:
			hw_p = &q->itd->hw_next;
			q = &q->itd->itd_next;
			continue;
		case Q_TYPE_QH:
			if (same_tt(dev, q->qh->dev)) {
				uf = tt_start_uframe(ehci, q->qh->hw->hw_info2);
				tt_usecs[uf] += q->qh->tt_usecs;
			}
			hw_p = &q->qh->hw->hw_next;
			q = &q->qh->qh_next;
			continue;
		case Q_TYPE_SITD:
			if (same_tt(dev, q->sitd->urb->dev)) {
				uf = tt_start_uframe(ehci, q->sitd->hw_uframe);
				tt_usecs[uf] += q->sitd->stream->tt_usecs;
			}
			hw_p = &q->sitd->hw_next;
			q = &q->sitd->sitd_next;
			continue;
		
		default:
			ehci_dbg(ehci, "ignoring periodic frame %d FSTN\n",
					frame);
			hw_p = &q->fstn->hw_next;
			q = &q->fstn->fstn_next;
		}
	}

	carryover_tt_bandwidth(tt_usecs);

	if (max_tt_usecs[7] < tt_usecs[7])
		ehci_err(ehci, "frame %d tt sched overrun: %d usecs\n",
			frame, tt_usecs[7] - max_tt_usecs[7]);
}

static int tt_available (
	struct ehci_hcd		*ehci,
	unsigned		period,
	struct usb_device	*dev,
	unsigned		frame,
	unsigned		uframe,
	u16			usecs
)
{
	if ((period == 0) || (uframe >= 7))	
		return 0;

	for (; frame < ehci->periodic_size; frame += period) {
		unsigned short tt_usecs[8];

		periodic_tt_usecs (ehci, dev, frame, tt_usecs);

		ehci_vdbg(ehci, "tt frame %d check %d usecs start uframe %d in"
			" schedule %d/%d/%d/%d/%d/%d/%d/%d\n",
			frame, usecs, uframe,
			tt_usecs[0], tt_usecs[1], tt_usecs[2], tt_usecs[3],
			tt_usecs[4], tt_usecs[5], tt_usecs[6], tt_usecs[7]);

		if (max_tt_usecs[uframe] <= tt_usecs[uframe]) {
			ehci_vdbg(ehci, "frame %d uframe %d fully scheduled\n",
				frame, uframe);
			return 0;
		}

		if (125 < usecs) {
			int ufs = (usecs / 125);
			int i;
			for (i = uframe; i < (uframe + ufs) && i < 8; i++)
				if (0 < tt_usecs[i]) {
					ehci_vdbg(ehci,
						"multi-uframe xfer can't fit "
						"in frame %d uframe %d\n",
						frame, i);
					return 0;
				}
		}

		tt_usecs[uframe] += usecs;

		carryover_tt_bandwidth(tt_usecs);

		
		if (max_tt_usecs[7] < tt_usecs[7]) {
			ehci_vdbg(ehci,
				"tt unavailable usecs %d frame %d uframe %d\n",
				usecs, frame, uframe);
			return 0;
		}
	}

	return 1;
}

#else

static int tt_no_collision (
	struct ehci_hcd		*ehci,
	unsigned		period,
	struct usb_device	*dev,
	unsigned		frame,
	u32			uf_mask
)
{
	if (period == 0)	
		return 0;

	for (; frame < ehci->periodic_size; frame += period) {
		union ehci_shadow	here;
		__hc32			type;
		struct ehci_qh_hw	*hw;

		here = ehci->pshadow [frame];
		type = Q_NEXT_TYPE(ehci, ehci->periodic [frame]);
		while (here.ptr) {
			switch (hc32_to_cpu(ehci, type)) {
			case Q_TYPE_ITD:
				type = Q_NEXT_TYPE(ehci, here.itd->hw_next);
				here = here.itd->itd_next;
				continue;
			case Q_TYPE_QH:
				hw = here.qh->hw;
				if (same_tt (dev, here.qh->dev)) {
					u32		mask;

					mask = hc32_to_cpu(ehci,
							hw->hw_info2);
					
					mask |= mask >> 8;
					if (mask & uf_mask)
						break;
				}
				type = Q_NEXT_TYPE(ehci, hw->hw_next);
				here = here.qh->qh_next;
				continue;
			case Q_TYPE_SITD:
				if (same_tt (dev, here.sitd->urb->dev)) {
					u16		mask;

					mask = hc32_to_cpu(ehci, here.sitd
								->hw_uframe);
					
					mask |= mask >> 8;
					if (mask & uf_mask)
						break;
				}
				type = Q_NEXT_TYPE(ehci, here.sitd->hw_next);
				here = here.sitd->sitd_next;
				continue;
			
			default:
				ehci_dbg (ehci,
					"periodic frame %d bogus type %d\n",
					frame, type);
			}

			
			return 0;
		}
	}

	
	return 1;
}

#endif 


static int enable_periodic (struct ehci_hcd *ehci)
{
	u32	cmd;
	int	status;

	if (ehci->periodic_sched++)
		return 0;

	status = handshake_on_error_set_halt(ehci, &ehci->regs->status,
					     STS_PSS, 0, 9 * 125);
	if (status) {
		usb_hc_died(ehci_to_hcd(ehci));
		return status;
	}

	cmd = ehci_readl(ehci, &ehci->regs->command) | CMD_PSE;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	

	
	ehci->next_uframe = ehci_read_frame_index(ehci)
		% (ehci->periodic_size << 3);
	if (unlikely(ehci->broken_periodic))
		ehci->last_periodic_enable = ktime_get_real();
	return 0;
}

static int disable_periodic (struct ehci_hcd *ehci)
{
	u32	cmd;
	int	status;

	if (--ehci->periodic_sched)
		return 0;

	if (unlikely(ehci->broken_periodic)) {
		
		ktime_t safe = ktime_add_us(ehci->last_periodic_enable, 1000);
		ktime_t now = ktime_get_real();
		s64 delay = ktime_us_delta(safe, now);

		if (unlikely(delay > 0))
			udelay(delay);
	}

	status = handshake_on_error_set_halt(ehci, &ehci->regs->status,
					     STS_PSS, STS_PSS, 9 * 125);
	if (status) {
		usb_hc_died(ehci_to_hcd(ehci));
		return status;
	}

	cmd = ehci_readl(ehci, &ehci->regs->command) & ~CMD_PSE;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	

	free_cached_lists(ehci);

	ehci->next_uframe = -1;
	return 0;
}


static int qh_link_periodic (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	unsigned	i;
	unsigned	period = qh->period;

	dev_dbg (&qh->dev->dev,
		"link qh%d-%04x/%p start %d [%d/%d us]\n",
		period, hc32_to_cpup(ehci, &qh->hw->hw_info2)
			& (QH_CMASK | QH_SMASK),
		qh, qh->start, qh->usecs, qh->c_usecs);

	
	if (period == 0)
		period = 1;

	for (i = qh->start; i < ehci->periodic_size; i += period) {
		union ehci_shadow	*prev = &ehci->pshadow[i];
		__hc32			*hw_p = &ehci->periodic[i];
		union ehci_shadow	here = *prev;
		__hc32			type = 0;

		
		while (here.ptr) {
			type = Q_NEXT_TYPE(ehci, *hw_p);
			if (type == cpu_to_hc32(ehci, Q_TYPE_QH))
				break;
			prev = periodic_next_shadow(ehci, prev, type);
			hw_p = shadow_next_periodic(ehci, &here, type);
			here = *prev;
		}

		while (here.ptr && qh != here.qh) {
			if (qh->period > here.qh->period)
				break;
			prev = &here.qh->qh_next;
			hw_p = &here.qh->hw->hw_next;
			here = *prev;
		}
		
		if (qh != here.qh) {
			qh->qh_next = here;
			if (here.qh)
				qh->hw->hw_next = *hw_p;
			wmb ();
			prev->qh = qh;
			*hw_p = QH_NEXT (ehci, qh->qh_dma);
		}
	}
	qh->qh_state = QH_STATE_LINKED;
	qh->xacterrs = 0;
	qh_get (qh);

	
	ehci_to_hcd(ehci)->self.bandwidth_allocated += qh->period
		? ((qh->usecs + qh->c_usecs) / qh->period)
		: (qh->usecs * 8);

	
	return enable_periodic(ehci);
}

static int qh_unlink_periodic(struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	unsigned	i;
	unsigned	period;

	
	
	
	
	
	

	
	if ((period = qh->period) == 0)
		period = 1;

	for (i = qh->start; i < ehci->periodic_size; i += period)
		periodic_unlink (ehci, i, qh);

	
	ehci_to_hcd(ehci)->self.bandwidth_allocated -= qh->period
		? ((qh->usecs + qh->c_usecs) / qh->period)
		: (qh->usecs * 8);

	dev_dbg (&qh->dev->dev,
		"unlink qh%d-%04x/%p start %d [%d/%d us]\n",
		qh->period,
		hc32_to_cpup(ehci, &qh->hw->hw_info2) & (QH_CMASK | QH_SMASK),
		qh, qh->start, qh->usecs, qh->c_usecs);

	
	qh->qh_state = QH_STATE_UNLINK;
	qh->qh_next.ptr = NULL;
	qh_put (qh);

	
	return disable_periodic(ehci);
}

static void intr_deschedule (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	unsigned		wait;
	struct ehci_qh_hw	*hw = qh->hw;
	int			rc;

	if (qh->qh_state != QH_STATE_LINKED) {
		if (qh->qh_state == QH_STATE_COMPLETING)
			qh->needs_rescan = 1;
		return;
	}

	qh_unlink_periodic (ehci, qh);

	if (list_empty (&qh->qtd_list)
			|| (cpu_to_hc32(ehci, QH_CMASK)
					& hw->hw_info2) != 0)
		wait = 2;
	else
		wait = 55;	

	udelay (wait);
	qh->qh_state = QH_STATE_IDLE;
	hw->hw_next = EHCI_LIST_END(ehci);
	wmb ();

	qh_completions(ehci, qh);

	
	if (!list_empty(&qh->qtd_list) &&
			ehci->rh_state == EHCI_RH_RUNNING) {
		rc = qh_schedule(ehci, qh);

		if (rc != 0)
			ehci_err(ehci, "can't reschedule qh %p, err %d\n",
					qh, rc);
	}
}


static int check_period (
	struct ehci_hcd *ehci,
	unsigned	frame,
	unsigned	uframe,
	unsigned	period,
	unsigned	usecs
) {
	int		claimed;

	if (uframe >= 8)
		return 0;

	
	usecs = ehci->uframe_periodic_max - usecs;

	if (unlikely (period == 0)) {
		do {
			for (uframe = 0; uframe < 7; uframe++) {
				claimed = periodic_usecs (ehci, frame, uframe);
				if (claimed > usecs)
					return 0;
			}
		} while ((frame += 1) < ehci->periodic_size);

	
	} else {
		do {
			claimed = periodic_usecs (ehci, frame, uframe);
			if (claimed > usecs)
				return 0;
		} while ((frame += period) < ehci->periodic_size);
	}

	
	return 1;
}

static int check_intr_schedule (
	struct ehci_hcd		*ehci,
	unsigned		frame,
	unsigned		uframe,
	const struct ehci_qh	*qh,
	__hc32			*c_maskp
)
{
	int		retval = -ENOSPC;
	u8		mask = 0;

	if (qh->c_usecs && uframe >= 6)		
		goto done;

	if (!check_period (ehci, frame, uframe, qh->period, qh->usecs))
		goto done;
	if (!qh->c_usecs) {
		retval = 0;
		*c_maskp = 0;
		goto done;
	}

#ifdef CONFIG_USB_EHCI_TT_NEWSCHED
	if (tt_available (ehci, qh->period, qh->dev, frame, uframe,
				qh->tt_usecs)) {
		unsigned i;

		
		for (i=uframe+1; i<8 && i<uframe+4; i++)
			if (!check_period (ehci, frame, i,
						qh->period, qh->c_usecs))
				goto done;
			else
				mask |= 1 << i;

		retval = 0;

		*c_maskp = cpu_to_hc32(ehci, mask << 8);
	}
#else
	mask = 0x03 << (uframe + qh->gap_uf);
	*c_maskp = cpu_to_hc32(ehci, mask << 8);

	mask |= 1 << uframe;
	if (tt_no_collision (ehci, qh->period, qh->dev, frame, mask)) {
		if (!check_period (ehci, frame, uframe + qh->gap_uf + 1,
					qh->period, qh->c_usecs))
			goto done;
		if (!check_period (ehci, frame, uframe + qh->gap_uf,
					qh->period, qh->c_usecs))
			goto done;
		retval = 0;
	}
#endif
done:
	return retval;
}

static int qh_schedule(struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	int		status;
	unsigned	uframe;
	__hc32		c_mask;
	unsigned	frame;		
	struct ehci_qh_hw	*hw = qh->hw;

	qh_refresh(ehci, qh);
	hw->hw_next = EHCI_LIST_END(ehci);
	frame = qh->start;

	
	if (frame < qh->period) {
		uframe = ffs(hc32_to_cpup(ehci, &hw->hw_info2) & QH_SMASK);
		status = check_intr_schedule (ehci, frame, --uframe,
				qh, &c_mask);
	} else {
		uframe = 0;
		c_mask = 0;
		status = -ENOSPC;
	}

	if (status) {
		
		if (qh->period) {
			int		i;

			for (i = qh->period; status && i > 0; --i) {
				frame = ++ehci->random_frame % qh->period;
				for (uframe = 0; uframe < 8; uframe++) {
					status = check_intr_schedule (ehci,
							frame, uframe, qh,
							&c_mask);
					if (status == 0)
						break;
				}
			}

		
		} else {
			frame = 0;
			status = check_intr_schedule (ehci, 0, 0, qh, &c_mask);
		}
		if (status)
			goto done;
		qh->start = frame;

		
		hw->hw_info2 &= cpu_to_hc32(ehci, ~(QH_CMASK | QH_SMASK));
		hw->hw_info2 |= qh->period
			? cpu_to_hc32(ehci, 1 << uframe)
			: cpu_to_hc32(ehci, QH_SMASK);
		hw->hw_info2 |= c_mask;
	} else
		ehci_dbg (ehci, "reused qh %p schedule\n", qh);

	
	status = qh_link_periodic (ehci, qh);
done:
	return status;
}

static int intr_submit (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	gfp_t			mem_flags
) {
	unsigned		epnum;
	unsigned long		flags;
	struct ehci_qh		*qh;
	int			status;
	struct list_head	empty;

	
	epnum = urb->ep->desc.bEndpointAddress;

	spin_lock_irqsave (&ehci->lock, flags);

	if (unlikely(!HCD_HW_ACCESSIBLE(ehci_to_hcd(ehci)))) {
		status = -ESHUTDOWN;
		goto done_not_linked;
	}
	status = usb_hcd_link_urb_to_ep(ehci_to_hcd(ehci), urb);
	if (unlikely(status))
		goto done_not_linked;

	
	INIT_LIST_HEAD (&empty);
	qh = qh_append_tds(ehci, urb, &empty, epnum, &urb->ep->hcpriv);
	if (qh == NULL) {
		status = -ENOMEM;
		goto done;
	}
	if (qh->qh_state == QH_STATE_IDLE) {
		if ((status = qh_schedule (ehci, qh)) != 0)
			goto done;
	}

	
	qh = qh_append_tds(ehci, urb, qtd_list, epnum, &urb->ep->hcpriv);
	BUG_ON (qh == NULL);

	
	ehci_to_hcd(ehci)->self.bandwidth_int_reqs++;

done:
	if (unlikely(status))
		usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
done_not_linked:
	spin_unlock_irqrestore (&ehci->lock, flags);
	if (status)
		qtd_list_free (ehci, urb, qtd_list);

	return status;
}



static struct ehci_iso_stream *
iso_stream_alloc (gfp_t mem_flags)
{
	struct ehci_iso_stream *stream;

	stream = kzalloc(sizeof *stream, mem_flags);
	if (likely (stream != NULL)) {
		INIT_LIST_HEAD(&stream->td_list);
		INIT_LIST_HEAD(&stream->free_list);
		stream->next_uframe = -1;
		stream->refcount = 1;
	}
	return stream;
}

static void
iso_stream_init (
	struct ehci_hcd		*ehci,
	struct ehci_iso_stream	*stream,
	struct usb_device	*dev,
	int			pipe,
	unsigned		interval
)
{
	static const u8 smask_out [] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f };

	u32			buf1;
	unsigned		epnum, maxp;
	int			is_input;
	long			bandwidth;

	epnum = usb_pipeendpoint (pipe);
	is_input = usb_pipein (pipe) ? USB_DIR_IN : 0;
	maxp = usb_maxpacket(dev, pipe, !is_input);
	if (is_input) {
		buf1 = (1 << 11);
	} else {
		buf1 = 0;
	}

	
	if (dev->speed == USB_SPEED_HIGH) {
		unsigned multi = hb_mult(maxp);

		stream->highspeed = 1;

		maxp = max_packet(maxp);
		buf1 |= maxp;
		maxp *= multi;

		stream->buf0 = cpu_to_hc32(ehci, (epnum << 8) | dev->devnum);
		stream->buf1 = cpu_to_hc32(ehci, buf1);
		stream->buf2 = cpu_to_hc32(ehci, multi);

		stream->usecs = HS_USECS_ISO (maxp);
		bandwidth = stream->usecs * 8;
		bandwidth /= interval;

	} else {
		u32		addr;
		int		think_time;
		int		hs_transfers;

		addr = dev->ttport << 24;
		if (!ehci_is_TDI(ehci)
				|| (dev->tt->hub !=
					ehci_to_hcd(ehci)->self.root_hub))
			addr |= dev->tt->hub->devnum << 16;
		addr |= epnum << 8;
		addr |= dev->devnum;
		stream->usecs = HS_USECS_ISO (maxp);
		think_time = dev->tt ? dev->tt->think_time : 0;
		stream->tt_usecs = NS_TO_US (think_time + usb_calc_bus_time (
				dev->speed, is_input, 1, maxp));
		hs_transfers = max (1u, (maxp + 187) / 188);
		if (is_input) {
			u32	tmp;

			addr |= 1 << 31;
			stream->c_usecs = stream->usecs;
			stream->usecs = HS_USECS_ISO (1);
			stream->raw_mask = 1;

			
			tmp = (1 << (hs_transfers + 2)) - 1;
			stream->raw_mask |= tmp << (8 + 2);
		} else
			stream->raw_mask = smask_out [hs_transfers - 1];
		bandwidth = stream->usecs + stream->c_usecs;
		bandwidth /= interval << 3;

		
		stream->address = cpu_to_hc32(ehci, addr);
	}
	stream->bandwidth = bandwidth;

	stream->udev = dev;

	stream->bEndpointAddress = is_input | epnum;
	stream->interval = interval;
	stream->maxp = maxp;
}

static void
iso_stream_put(struct ehci_hcd *ehci, struct ehci_iso_stream *stream)
{
	stream->refcount--;

	if (stream->refcount == 1) {
		

		while (!list_empty (&stream->free_list)) {
			struct list_head	*entry;

			entry = stream->free_list.next;
			list_del (entry);

			
			if (stream->highspeed) {
				struct ehci_itd		*itd;

				itd = list_entry (entry, struct ehci_itd,
						itd_list);
				dma_pool_free (ehci->itd_pool, itd,
						itd->itd_dma);
			} else {
				struct ehci_sitd	*sitd;

				sitd = list_entry (entry, struct ehci_sitd,
						sitd_list);
				dma_pool_free (ehci->sitd_pool, sitd,
						sitd->sitd_dma);
			}
		}

		stream->bEndpointAddress &= 0x0f;
		if (stream->ep)
			stream->ep->hcpriv = NULL;

		kfree(stream);
	}
}

static inline struct ehci_iso_stream *
iso_stream_get (struct ehci_iso_stream *stream)
{
	if (likely (stream != NULL))
		stream->refcount++;
	return stream;
}

static struct ehci_iso_stream *
iso_stream_find (struct ehci_hcd *ehci, struct urb *urb)
{
	unsigned		epnum;
	struct ehci_iso_stream	*stream;
	struct usb_host_endpoint *ep;
	unsigned long		flags;

	epnum = usb_pipeendpoint (urb->pipe);
	if (usb_pipein(urb->pipe))
		ep = urb->dev->ep_in[epnum];
	else
		ep = urb->dev->ep_out[epnum];

	spin_lock_irqsave (&ehci->lock, flags);
	stream = ep->hcpriv;

	if (unlikely (stream == NULL)) {
		stream = iso_stream_alloc(GFP_ATOMIC);
		if (likely (stream != NULL)) {
			
			ep->hcpriv = stream;
			stream->ep = ep;
			iso_stream_init(ehci, stream, urb->dev, urb->pipe,
					urb->interval);
		}

	
	} else if (unlikely (stream->hw != NULL)) {
		ehci_dbg (ehci, "dev %s ep%d%s, not iso??\n",
			urb->dev->devpath, epnum,
			usb_pipein(urb->pipe) ? "in" : "out");
		stream = NULL;
	}

	
	stream = iso_stream_get (stream);

	spin_unlock_irqrestore (&ehci->lock, flags);
	return stream;
}



static struct ehci_iso_sched *
iso_sched_alloc (unsigned packets, gfp_t mem_flags)
{
	struct ehci_iso_sched	*iso_sched;
	int			size = sizeof *iso_sched;

	size += packets * sizeof (struct ehci_iso_packet);
	iso_sched = kzalloc(size, mem_flags);
	if (likely (iso_sched != NULL)) {
		INIT_LIST_HEAD (&iso_sched->td_list);
	}
	return iso_sched;
}

static inline void
itd_sched_init(
	struct ehci_hcd		*ehci,
	struct ehci_iso_sched	*iso_sched,
	struct ehci_iso_stream	*stream,
	struct urb		*urb
)
{
	unsigned	i;
	dma_addr_t	dma = urb->transfer_dma;

	
	iso_sched->span = urb->number_of_packets * stream->interval;

	for (i = 0; i < urb->number_of_packets; i++) {
		struct ehci_iso_packet	*uframe = &iso_sched->packet [i];
		unsigned		length;
		dma_addr_t		buf;
		u32			trans;

		length = urb->iso_frame_desc [i].length;
		buf = dma + urb->iso_frame_desc [i].offset;

		trans = EHCI_ISOC_ACTIVE;
		trans |= buf & 0x0fff;
		if (unlikely (((i + 1) == urb->number_of_packets))
				&& !(urb->transfer_flags & URB_NO_INTERRUPT))
			trans |= EHCI_ITD_IOC;
		trans |= length << 16;
		uframe->transaction = cpu_to_hc32(ehci, trans);

		
		uframe->bufp = (buf & ~(u64)0x0fff);
		buf += length;
		if (unlikely ((uframe->bufp != (buf & ~(u64)0x0fff))))
			uframe->cross = 1;
	}
}

static void
iso_sched_free (
	struct ehci_iso_stream	*stream,
	struct ehci_iso_sched	*iso_sched
)
{
	if (!iso_sched)
		return;
	
	list_splice (&iso_sched->td_list, &stream->free_list);
	kfree (iso_sched);
}

static int
itd_urb_transaction (
	struct ehci_iso_stream	*stream,
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	gfp_t			mem_flags
)
{
	struct ehci_itd		*itd;
	dma_addr_t		itd_dma;
	int			i;
	unsigned		num_itds;
	struct ehci_iso_sched	*sched;
	unsigned long		flags;

	sched = iso_sched_alloc (urb->number_of_packets, mem_flags);
	if (unlikely (sched == NULL))
		return -ENOMEM;

	itd_sched_init(ehci, sched, stream, urb);

	if (urb->interval < 8)
		num_itds = 1 + (sched->span + 7) / 8;
	else
		num_itds = urb->number_of_packets;

	
	spin_lock_irqsave (&ehci->lock, flags);
	for (i = 0; i < num_itds; i++) {


		
		if (likely (!list_empty(&stream->free_list))) {
			itd = list_entry (stream->free_list.prev,
					struct ehci_itd, itd_list);
			list_del (&itd->itd_list);
			itd_dma = itd->itd_dma;
		} else {
			spin_unlock_irqrestore (&ehci->lock, flags);
			itd = dma_pool_alloc (ehci->itd_pool, mem_flags,
					&itd_dma);
			spin_lock_irqsave (&ehci->lock, flags);
			if (!itd) {
				iso_sched_free(stream, sched);
				spin_unlock_irqrestore(&ehci->lock, flags);
				return -ENOMEM;
			}
		}

		memset (itd, 0, sizeof *itd);
		itd->itd_dma = itd_dma;
		list_add (&itd->itd_list, &sched->td_list);
	}
	spin_unlock_irqrestore (&ehci->lock, flags);

	
	urb->hcpriv = sched;
	urb->error_count = 0;
	return 0;
}


static inline int
itd_slot_ok (
	struct ehci_hcd		*ehci,
	u32			mod,
	u32			uframe,
	u8			usecs,
	u32			period
)
{
	uframe %= period;
	do {
		
		if (periodic_usecs (ehci, uframe >> 3, uframe & 0x7)
				> (ehci->uframe_periodic_max - usecs))
			return 0;

		
		uframe += period;
	} while (uframe < mod);
	return 1;
}

static inline int
sitd_slot_ok (
	struct ehci_hcd		*ehci,
	u32			mod,
	struct ehci_iso_stream	*stream,
	u32			uframe,
	struct ehci_iso_sched	*sched,
	u32			period_uframes
)
{
	u32			mask, tmp;
	u32			frame, uf;

	mask = stream->raw_mask << (uframe & 7);

	
	if (mask & ~0xffff)
		return 0;


	
	uframe %= period_uframes;
	do {
		u32		max_used;

		frame = uframe >> 3;
		uf = uframe & 7;

#ifdef CONFIG_USB_EHCI_TT_NEWSCHED
		if (!tt_available (ehci, period_uframes << 3,
				stream->udev, frame, uf, stream->tt_usecs))
			return 0;
#else
		if (!tt_no_collision (ehci, period_uframes << 3,
				stream->udev, frame, mask))
			return 0;
#endif

		
		max_used = ehci->uframe_periodic_max - stream->usecs;
		for (tmp = stream->raw_mask & 0xff; tmp; tmp >>= 1, uf++) {
			if (periodic_usecs (ehci, frame, uf) > max_used)
				return 0;
		}

		
		if (stream->c_usecs) {
			uf = uframe & 7;
			max_used = ehci->uframe_periodic_max - stream->c_usecs;
			do {
				tmp = 1 << uf;
				tmp <<= 8;
				if ((stream->raw_mask & tmp) == 0)
					continue;
				if (periodic_usecs (ehci, frame, uf)
						> max_used)
					return 0;
			} while (++uf < 8);
		}

		
		uframe += period_uframes;
	} while (uframe < mod);

	stream->splits = cpu_to_hc32(ehci, stream->raw_mask << (uframe & 7));
	return 1;
}


#define SCHEDULE_SLOP	80	

static int
iso_stream_schedule (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct ehci_iso_stream	*stream
)
{
	u32			now, next, start, period, span;
	int			status;
	unsigned		mod = ehci->periodic_size << 3;
	struct ehci_iso_sched	*sched = urb->hcpriv;

	period = urb->interval;
	span = sched->span;
	if (!stream->highspeed) {
		period <<= 3;
		span <<= 3;
	}

	if (span > mod - SCHEDULE_SLOP) {
		ehci_dbg (ehci, "iso request %p too long\n", urb);
		status = -EFBIG;
		goto fail;
	}

	now = ehci_read_frame_index(ehci) & (mod - 1);

	if (likely (!list_empty (&stream->td_list))) {
		u32	excess;

		if (!stream->highspeed && ehci->fs_i_thresh)
			next = now + ehci->i_thresh;
		else
			next = now;

		excess = (stream->next_uframe - period - next) & (mod - 1);
		if (excess >= mod - 2 * SCHEDULE_SLOP)
			start = next + excess - mod + period *
					DIV_ROUND_UP(mod - excess, period);
		else
			start = next + excess + period;
		if (start - now >= mod) {
			ehci_dbg(ehci, "request %p would overflow (%d+%d >= %d)\n",
					urb, start - now - period, period,
					mod);
			status = -EFBIG;
			goto fail;
		}
	}

	else {
		int done = 0;
		start = SCHEDULE_SLOP + (now & ~0x07);

		

		next = start;
		start += period;
		do {
			start--;
			
			if (stream->highspeed) {
				if (itd_slot_ok(ehci, mod, start,
						stream->usecs, period))
					done = 1;
			} else {
				if ((start % 8) >= 6)
					continue;
				if (sitd_slot_ok(ehci, mod, stream,
						start, sched, period))
					done = 1;
			}
		} while (start > next && !done);

		
		if (!done) {
			ehci_dbg(ehci, "iso resched full %p (now %d max %d)\n",
				urb, now, now + mod);
			status = -ENOSPC;
			goto fail;
		}
	}

	
	if (unlikely(start - now + span - period
				>= mod - 2 * SCHEDULE_SLOP)) {
		ehci_dbg(ehci, "request %p would overflow (%d+%d >= %d)\n",
				urb, start - now, span - period,
				mod - 2 * SCHEDULE_SLOP);
		status = -EFBIG;
		goto fail;
	}

	stream->next_uframe = start & (mod - 1);

	
	urb->start_frame = stream->next_uframe;
	if (!stream->highspeed)
		urb->start_frame >>= 3;
	return 0;

 fail:
	iso_sched_free(stream, sched);
	urb->hcpriv = NULL;
	return status;
}


static inline void
itd_init(struct ehci_hcd *ehci, struct ehci_iso_stream *stream,
		struct ehci_itd *itd)
{
	int i;

	
	itd->hw_next = EHCI_LIST_END(ehci);
	itd->hw_bufp [0] = stream->buf0;
	itd->hw_bufp [1] = stream->buf1;
	itd->hw_bufp [2] = stream->buf2;

	for (i = 0; i < 8; i++)
		itd->index[i] = -1;

	
}

static inline void
itd_patch(
	struct ehci_hcd		*ehci,
	struct ehci_itd		*itd,
	struct ehci_iso_sched	*iso_sched,
	unsigned		index,
	u16			uframe
)
{
	struct ehci_iso_packet	*uf = &iso_sched->packet [index];
	unsigned		pg = itd->pg;

	

	uframe &= 0x07;
	itd->index [uframe] = index;

	itd->hw_transaction[uframe] = uf->transaction;
	itd->hw_transaction[uframe] |= cpu_to_hc32(ehci, pg << 12);
	itd->hw_bufp[pg] |= cpu_to_hc32(ehci, uf->bufp & ~(u32)0);
	itd->hw_bufp_hi[pg] |= cpu_to_hc32(ehci, (u32)(uf->bufp >> 32));

	
	if (unlikely (uf->cross)) {
		u64	bufp = uf->bufp + 4096;

		itd->pg = ++pg;
		itd->hw_bufp[pg] |= cpu_to_hc32(ehci, bufp & ~(u32)0);
		itd->hw_bufp_hi[pg] |= cpu_to_hc32(ehci, (u32)(bufp >> 32));
	}
}

static inline void
itd_link (struct ehci_hcd *ehci, unsigned frame, struct ehci_itd *itd)
{
	union ehci_shadow	*prev = &ehci->pshadow[frame];
	__hc32			*hw_p = &ehci->periodic[frame];
	union ehci_shadow	here = *prev;
	__hc32			type = 0;

	
	while (here.ptr) {
		type = Q_NEXT_TYPE(ehci, *hw_p);
		if (type == cpu_to_hc32(ehci, Q_TYPE_QH))
			break;
		prev = periodic_next_shadow(ehci, prev, type);
		hw_p = shadow_next_periodic(ehci, &here, type);
		here = *prev;
	}

	itd->itd_next = here;
	itd->hw_next = *hw_p;
	prev->itd = itd;
	itd->frame = frame;
	wmb ();
	*hw_p = cpu_to_hc32(ehci, itd->itd_dma | Q_TYPE_ITD);
}

static int
itd_link_urb (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	unsigned		mod,
	struct ehci_iso_stream	*stream
)
{
	int			packet;
	unsigned		next_uframe, uframe, frame;
	struct ehci_iso_sched	*iso_sched = urb->hcpriv;
	struct ehci_itd		*itd;

	next_uframe = stream->next_uframe & (mod - 1);

	if (unlikely (list_empty(&stream->td_list))) {
		ehci_to_hcd(ehci)->self.bandwidth_allocated
				+= stream->bandwidth;
		ehci_vdbg (ehci,
			"schedule devp %s ep%d%s-iso period %d start %d.%d\n",
			urb->dev->devpath, stream->bEndpointAddress & 0x0f,
			(stream->bEndpointAddress & USB_DIR_IN) ? "in" : "out",
			urb->interval,
			next_uframe >> 3, next_uframe & 0x7);
	}

	if (ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs == 0) {
		if (ehci->amd_pll_fix == 1)
			usb_amd_quirk_pll_disable();
	}

	ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs++;

	
	for (packet = 0, itd = NULL; packet < urb->number_of_packets; ) {
		if (itd == NULL) {
			
			

			

			itd = list_entry (iso_sched->td_list.next,
					struct ehci_itd, itd_list);
			list_move_tail (&itd->itd_list, &stream->td_list);
			itd->stream = iso_stream_get (stream);
			itd->urb = urb;
			itd_init (ehci, stream, itd);
		}

		uframe = next_uframe & 0x07;
		frame = next_uframe >> 3;

		itd_patch(ehci, itd, iso_sched, packet, uframe);

		next_uframe += stream->interval;
		next_uframe &= mod - 1;
		packet++;

		
		if (((next_uframe >> 3) != frame)
				|| packet == urb->number_of_packets) {
			itd_link(ehci, frame & (ehci->periodic_size - 1), itd);
			itd = NULL;
		}
	}
	stream->next_uframe = next_uframe;

	
	iso_sched_free (stream, iso_sched);
	urb->hcpriv = NULL;

	timer_action (ehci, TIMER_IO_WATCHDOG);
	return enable_periodic(ehci);
}

#define	ISO_ERRS (EHCI_ISOC_BUF_ERR | EHCI_ISOC_BABBLE | EHCI_ISOC_XACTERR)

static unsigned
itd_complete (
	struct ehci_hcd	*ehci,
	struct ehci_itd	*itd
) {
	struct urb				*urb = itd->urb;
	struct usb_iso_packet_descriptor	*desc;
	u32					t;
	unsigned				uframe;
	int					urb_index = -1;
	struct ehci_iso_stream			*stream = itd->stream;
	struct usb_device			*dev;
	unsigned				retval = false;

	
	for (uframe = 0; uframe < 8; uframe++) {
		if (likely (itd->index[uframe] == -1))
			continue;
		urb_index = itd->index[uframe];
		desc = &urb->iso_frame_desc [urb_index];

		t = hc32_to_cpup(ehci, &itd->hw_transaction [uframe]);
		itd->hw_transaction [uframe] = 0;

		
		if (unlikely (t & ISO_ERRS)) {
			urb->error_count++;
			if (t & EHCI_ISOC_BUF_ERR)
				desc->status = usb_pipein (urb->pipe)
					? -ENOSR  
					: -ECOMM; 
			else if (t & EHCI_ISOC_BABBLE)
				desc->status = -EOVERFLOW;
			else 
				desc->status = -EPROTO;

			
			if (!(t & EHCI_ISOC_BABBLE)) {
				desc->actual_length = EHCI_ITD_LENGTH(t);
				urb->actual_length += desc->actual_length;
			}
		} else if (likely ((t & EHCI_ISOC_ACTIVE) == 0)) {
			desc->status = 0;
			desc->actual_length = EHCI_ITD_LENGTH(t);
			urb->actual_length += desc->actual_length;
		} else {
			
			desc->status = -EXDEV;
		}
	}

	
	if (likely ((urb_index + 1) != urb->number_of_packets))
		goto done;


	
	dev = urb->dev;
	ehci_urb_done(ehci, urb, 0);
	retval = true;
	urb = NULL;
	(void) disable_periodic(ehci);
	ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs--;

	if (ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs == 0) {
		if (ehci->amd_pll_fix == 1)
			usb_amd_quirk_pll_enable();
	}

	if (unlikely(list_is_singular(&stream->td_list))) {
		ehci_to_hcd(ehci)->self.bandwidth_allocated
				-= stream->bandwidth;
		ehci_vdbg (ehci,
			"deschedule devp %s ep%d%s-iso\n",
			dev->devpath, stream->bEndpointAddress & 0x0f,
			(stream->bEndpointAddress & USB_DIR_IN) ? "in" : "out");
	}
	iso_stream_put (ehci, stream);

done:
	itd->urb = NULL;
	if (ehci->clock_frame != itd->frame || itd->index[7] != -1) {
		
		itd->stream = NULL;
		list_move(&itd->itd_list, &stream->free_list);
		iso_stream_put(ehci, stream);
	} else {
		list_move(&itd->itd_list, &ehci->cached_itd_list);
		if (stream->refcount == 2) {
			stream->ep->hcpriv = NULL;
			stream->ep = NULL;
		}
	}
	return retval;
}


static int itd_submit (struct ehci_hcd *ehci, struct urb *urb,
	gfp_t mem_flags)
{
	int			status = -EINVAL;
	unsigned long		flags;
	struct ehci_iso_stream	*stream;

	
	stream = iso_stream_find (ehci, urb);
	if (unlikely (stream == NULL)) {
		ehci_dbg (ehci, "can't get iso stream\n");
		return -ENOMEM;
	}
	if (unlikely (urb->interval != stream->interval)) {
		ehci_dbg (ehci, "can't change iso interval %d --> %d\n",
			stream->interval, urb->interval);
		goto done;
	}

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s len %d, %d pkts %d uframes [%p]\n",
		__func__, urb->dev->devpath, urb,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		urb->transfer_buffer_length,
		urb->number_of_packets, urb->interval,
		stream);
#endif

	
	status = itd_urb_transaction (stream, ehci, urb, mem_flags);
	if (unlikely (status < 0)) {
		ehci_dbg (ehci, "can't init itds\n");
		goto done;
	}

	
	spin_lock_irqsave (&ehci->lock, flags);
	if (unlikely(!HCD_HW_ACCESSIBLE(ehci_to_hcd(ehci)))) {
		status = -ESHUTDOWN;
		goto done_not_linked;
	}
	status = usb_hcd_link_urb_to_ep(ehci_to_hcd(ehci), urb);
	if (unlikely(status))
		goto done_not_linked;
	status = iso_stream_schedule(ehci, urb, stream);
	if (likely (status == 0))
		itd_link_urb (ehci, urb, ehci->periodic_size << 3, stream);
	else
		usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
done_not_linked:
	spin_unlock_irqrestore (&ehci->lock, flags);

done:
	if (unlikely (status < 0))
		iso_stream_put (ehci, stream);
	return status;
}



static inline void
sitd_sched_init(
	struct ehci_hcd		*ehci,
	struct ehci_iso_sched	*iso_sched,
	struct ehci_iso_stream	*stream,
	struct urb		*urb
)
{
	unsigned	i;
	dma_addr_t	dma = urb->transfer_dma;

	
	iso_sched->span = urb->number_of_packets * stream->interval;

	for (i = 0; i < urb->number_of_packets; i++) {
		struct ehci_iso_packet	*packet = &iso_sched->packet [i];
		unsigned		length;
		dma_addr_t		buf;
		u32			trans;

		length = urb->iso_frame_desc [i].length & 0x03ff;
		buf = dma + urb->iso_frame_desc [i].offset;

		trans = SITD_STS_ACTIVE;
		if (((i + 1) == urb->number_of_packets)
				&& !(urb->transfer_flags & URB_NO_INTERRUPT))
			trans |= SITD_IOC;
		trans |= length << 16;
		packet->transaction = cpu_to_hc32(ehci, trans);

		
		packet->bufp = buf;
		packet->buf1 = (buf + length) & ~0x0fff;
		if (packet->buf1 != (buf & ~(u64)0x0fff))
			packet->cross = 1;

		
		if (stream->bEndpointAddress & USB_DIR_IN)
			continue;
		length = (length + 187) / 188;
		if (length > 1) 
			length |= 1 << 3;
		packet->buf1 |= length;
	}
}

static int
sitd_urb_transaction (
	struct ehci_iso_stream	*stream,
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	gfp_t			mem_flags
)
{
	struct ehci_sitd	*sitd;
	dma_addr_t		sitd_dma;
	int			i;
	struct ehci_iso_sched	*iso_sched;
	unsigned long		flags;

	iso_sched = iso_sched_alloc (urb->number_of_packets, mem_flags);
	if (iso_sched == NULL)
		return -ENOMEM;

	sitd_sched_init(ehci, iso_sched, stream, urb);

	
	spin_lock_irqsave (&ehci->lock, flags);
	for (i = 0; i < urb->number_of_packets; i++) {



		
		if (!list_empty(&stream->free_list)) {
			sitd = list_entry (stream->free_list.prev,
					 struct ehci_sitd, sitd_list);
			list_del (&sitd->sitd_list);
			sitd_dma = sitd->sitd_dma;
		} else {
			spin_unlock_irqrestore (&ehci->lock, flags);
			sitd = dma_pool_alloc (ehci->sitd_pool, mem_flags,
					&sitd_dma);
			spin_lock_irqsave (&ehci->lock, flags);
			if (!sitd) {
				iso_sched_free(stream, iso_sched);
				spin_unlock_irqrestore(&ehci->lock, flags);
				return -ENOMEM;
			}
		}

		memset (sitd, 0, sizeof *sitd);
		sitd->sitd_dma = sitd_dma;
		list_add (&sitd->sitd_list, &iso_sched->td_list);
	}

	
	urb->hcpriv = iso_sched;
	urb->error_count = 0;

	spin_unlock_irqrestore (&ehci->lock, flags);
	return 0;
}


static inline void
sitd_patch(
	struct ehci_hcd		*ehci,
	struct ehci_iso_stream	*stream,
	struct ehci_sitd	*sitd,
	struct ehci_iso_sched	*iso_sched,
	unsigned		index
)
{
	struct ehci_iso_packet	*uf = &iso_sched->packet [index];
	u64			bufp = uf->bufp;

	sitd->hw_next = EHCI_LIST_END(ehci);
	sitd->hw_fullspeed_ep = stream->address;
	sitd->hw_uframe = stream->splits;
	sitd->hw_results = uf->transaction;
	sitd->hw_backpointer = EHCI_LIST_END(ehci);

	bufp = uf->bufp;
	sitd->hw_buf[0] = cpu_to_hc32(ehci, bufp);
	sitd->hw_buf_hi[0] = cpu_to_hc32(ehci, bufp >> 32);

	sitd->hw_buf[1] = cpu_to_hc32(ehci, uf->buf1);
	if (uf->cross)
		bufp += 4096;
	sitd->hw_buf_hi[1] = cpu_to_hc32(ehci, bufp >> 32);
	sitd->index = index;
}

static inline void
sitd_link (struct ehci_hcd *ehci, unsigned frame, struct ehci_sitd *sitd)
{
	
	sitd->sitd_next = ehci->pshadow [frame];
	sitd->hw_next = ehci->periodic [frame];
	ehci->pshadow [frame].sitd = sitd;
	sitd->frame = frame;
	wmb ();
	ehci->periodic[frame] = cpu_to_hc32(ehci, sitd->sitd_dma | Q_TYPE_SITD);
}

static int
sitd_link_urb (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	unsigned		mod,
	struct ehci_iso_stream	*stream
)
{
	int			packet;
	unsigned		next_uframe;
	struct ehci_iso_sched	*sched = urb->hcpriv;
	struct ehci_sitd	*sitd;

	next_uframe = stream->next_uframe;

	if (list_empty(&stream->td_list)) {
		
		ehci_to_hcd(ehci)->self.bandwidth_allocated
				+= stream->bandwidth;
		ehci_vdbg (ehci,
			"sched devp %s ep%d%s-iso [%d] %dms/%04x\n",
			urb->dev->devpath, stream->bEndpointAddress & 0x0f,
			(stream->bEndpointAddress & USB_DIR_IN) ? "in" : "out",
			(next_uframe >> 3) & (ehci->periodic_size - 1),
			stream->interval, hc32_to_cpu(ehci, stream->splits));
	}

	if (ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs == 0) {
		if (ehci->amd_pll_fix == 1)
			usb_amd_quirk_pll_disable();
	}

	ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs++;

	
	for (packet = 0, sitd = NULL;
			packet < urb->number_of_packets;
			packet++) {

		
		BUG_ON (list_empty (&sched->td_list));

		

		sitd = list_entry (sched->td_list.next,
				struct ehci_sitd, sitd_list);
		list_move_tail (&sitd->sitd_list, &stream->td_list);
		sitd->stream = iso_stream_get (stream);
		sitd->urb = urb;

		sitd_patch(ehci, stream, sitd, sched, packet);
		sitd_link(ehci, (next_uframe >> 3) & (ehci->periodic_size - 1),
				sitd);

		next_uframe += stream->interval << 3;
	}
	stream->next_uframe = next_uframe & (mod - 1);

	
	iso_sched_free (stream, sched);
	urb->hcpriv = NULL;

	timer_action (ehci, TIMER_IO_WATCHDOG);
	return enable_periodic(ehci);
}


#define	SITD_ERRS (SITD_STS_ERR | SITD_STS_DBE | SITD_STS_BABBLE \
				| SITD_STS_XACT | SITD_STS_MMF)

static unsigned
sitd_complete (
	struct ehci_hcd		*ehci,
	struct ehci_sitd	*sitd
) {
	struct urb				*urb = sitd->urb;
	struct usb_iso_packet_descriptor	*desc;
	u32					t;
	int					urb_index = -1;
	struct ehci_iso_stream			*stream = sitd->stream;
	struct usb_device			*dev;
	unsigned				retval = false;

	urb_index = sitd->index;
	desc = &urb->iso_frame_desc [urb_index];
	t = hc32_to_cpup(ehci, &sitd->hw_results);

	
	if (t & SITD_ERRS) {
		urb->error_count++;
		if (t & SITD_STS_DBE)
			desc->status = usb_pipein (urb->pipe)
				? -ENOSR  
				: -ECOMM; 
		else if (t & SITD_STS_BABBLE)
			desc->status = -EOVERFLOW;
		else 
			desc->status = -EPROTO;
	} else {
		desc->status = 0;
		desc->actual_length = desc->length - SITD_LENGTH(t);
		urb->actual_length += desc->actual_length;
	}

	
	if ((urb_index + 1) != urb->number_of_packets)
		goto done;


	
	dev = urb->dev;
	ehci_urb_done(ehci, urb, 0);
	retval = true;
	urb = NULL;
	(void) disable_periodic(ehci);
	ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs--;

	if (ehci_to_hcd(ehci)->self.bandwidth_isoc_reqs == 0) {
		if (ehci->amd_pll_fix == 1)
			usb_amd_quirk_pll_enable();
	}

	if (list_is_singular(&stream->td_list)) {
		ehci_to_hcd(ehci)->self.bandwidth_allocated
				-= stream->bandwidth;
		ehci_vdbg (ehci,
			"deschedule devp %s ep%d%s-iso\n",
			dev->devpath, stream->bEndpointAddress & 0x0f,
			(stream->bEndpointAddress & USB_DIR_IN) ? "in" : "out");
	}
	iso_stream_put (ehci, stream);

done:
	sitd->urb = NULL;
	if (ehci->clock_frame != sitd->frame) {
		
		sitd->stream = NULL;
		list_move(&sitd->sitd_list, &stream->free_list);
		iso_stream_put(ehci, stream);
	} else {
		list_move(&sitd->sitd_list, &ehci->cached_sitd_list);
		if (stream->refcount == 2) {
			stream->ep->hcpriv = NULL;
			stream->ep = NULL;
		}
	}
	return retval;
}


static int sitd_submit (struct ehci_hcd *ehci, struct urb *urb,
	gfp_t mem_flags)
{
	int			status = -EINVAL;
	unsigned long		flags;
	struct ehci_iso_stream	*stream;

	
	stream = iso_stream_find (ehci, urb);
	if (stream == NULL) {
		ehci_dbg (ehci, "can't get iso stream\n");
		return -ENOMEM;
	}
	if (urb->interval != stream->interval) {
		ehci_dbg (ehci, "can't change iso interval %d --> %d\n",
			stream->interval, urb->interval);
		goto done;
	}

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"submit %p dev%s ep%d%s-iso len %d\n",
		urb, urb->dev->devpath,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		urb->transfer_buffer_length);
#endif

	
	status = sitd_urb_transaction (stream, ehci, urb, mem_flags);
	if (status < 0) {
		ehci_dbg (ehci, "can't init sitds\n");
		goto done;
	}

	
	spin_lock_irqsave (&ehci->lock, flags);
	if (unlikely(!HCD_HW_ACCESSIBLE(ehci_to_hcd(ehci)))) {
		status = -ESHUTDOWN;
		goto done_not_linked;
	}
	status = usb_hcd_link_urb_to_ep(ehci_to_hcd(ehci), urb);
	if (unlikely(status))
		goto done_not_linked;
	status = iso_stream_schedule(ehci, urb, stream);
	if (status == 0)
		sitd_link_urb (ehci, urb, ehci->periodic_size << 3, stream);
	else
		usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
done_not_linked:
	spin_unlock_irqrestore (&ehci->lock, flags);

done:
	if (status < 0)
		iso_stream_put (ehci, stream);
	return status;
}


static void free_cached_lists(struct ehci_hcd *ehci)
{
	struct ehci_itd *itd, *n;
	struct ehci_sitd *sitd, *sn;

	list_for_each_entry_safe(itd, n, &ehci->cached_itd_list, itd_list) {
		struct ehci_iso_stream	*stream = itd->stream;
		itd->stream = NULL;
		list_move(&itd->itd_list, &stream->free_list);
		iso_stream_put(ehci, stream);
	}

	list_for_each_entry_safe(sitd, sn, &ehci->cached_sitd_list, sitd_list) {
		struct ehci_iso_stream	*stream = sitd->stream;
		sitd->stream = NULL;
		list_move(&sitd->sitd_list, &stream->free_list);
		iso_stream_put(ehci, stream);
	}
}


static void
scan_periodic (struct ehci_hcd *ehci)
{
	unsigned	now_uframe, frame, clock, clock_frame, mod;
	unsigned	modified;

	mod = ehci->periodic_size << 3;

	now_uframe = ehci->next_uframe;
	if (ehci->rh_state == EHCI_RH_RUNNING) {
		clock = ehci_read_frame_index(ehci);
		clock_frame = (clock >> 3) & (ehci->periodic_size - 1);
	} else  {
		clock = now_uframe + mod - 1;
		clock_frame = -1;
	}
	if (ehci->clock_frame != clock_frame) {
		free_cached_lists(ehci);
		ehci->clock_frame = clock_frame;
	}
	clock &= mod - 1;
	clock_frame = clock >> 3;
	++ehci->periodic_stamp;

	for (;;) {
		union ehci_shadow	q, *q_p;
		__hc32			type, *hw_p;
		unsigned		incomplete = false;

		frame = now_uframe >> 3;

restart:
		
		q_p = &ehci->pshadow [frame];
		hw_p = &ehci->periodic [frame];
		q.ptr = q_p->ptr;
		type = Q_NEXT_TYPE(ehci, *hw_p);
		modified = 0;

		while (q.ptr != NULL) {
			unsigned		uf;
			union ehci_shadow	temp;
			int			live;

			live = (ehci->rh_state == EHCI_RH_RUNNING);
			switch (hc32_to_cpu(ehci, type)) {
			case Q_TYPE_QH:
				
				temp.qh = qh_get (q.qh);
				type = Q_NEXT_TYPE(ehci, q.qh->hw->hw_next);
				q = q.qh->qh_next;
				if (temp.qh->stamp != ehci->periodic_stamp) {
					modified = qh_completions(ehci, temp.qh);
					if (!modified)
						temp.qh->stamp = ehci->periodic_stamp;
					if (unlikely(list_empty(&temp.qh->qtd_list) ||
							temp.qh->needs_rescan))
						intr_deschedule(ehci, temp.qh);
				}
				qh_put (temp.qh);
				break;
			case Q_TYPE_FSTN:
				if (q.fstn->hw_prev != EHCI_LIST_END(ehci)) {
					dbg ("ignoring completions from FSTNs");
				}
				type = Q_NEXT_TYPE(ehci, q.fstn->hw_next);
				q = q.fstn->fstn_next;
				break;
			case Q_TYPE_ITD:
				if (frame == clock_frame && live) {
					rmb();
					for (uf = 0; uf < 8; uf++) {
						if (q.itd->hw_transaction[uf] &
							    ITD_ACTIVE(ehci))
							break;
					}
					if (uf < 8) {
						incomplete = true;
						q_p = &q.itd->itd_next;
						hw_p = &q.itd->hw_next;
						type = Q_NEXT_TYPE(ehci,
							q.itd->hw_next);
						q = *q_p;
						break;
					}
				}

				*q_p = q.itd->itd_next;
				if (!ehci->use_dummy_qh ||
				    q.itd->hw_next != EHCI_LIST_END(ehci))
					*hw_p = q.itd->hw_next;
				else
					*hw_p = ehci->dummy->qh_dma;
				type = Q_NEXT_TYPE(ehci, q.itd->hw_next);
				wmb();
				modified = itd_complete (ehci, q.itd);
				q = *q_p;
				break;
			case Q_TYPE_SITD:
				if (((frame == clock_frame) ||
				     (((frame + 1) & (ehci->periodic_size - 1))
				      == clock_frame))
				    && live
				    && (q.sitd->hw_results &
					SITD_ACTIVE(ehci))) {

					incomplete = true;
					q_p = &q.sitd->sitd_next;
					hw_p = &q.sitd->hw_next;
					type = Q_NEXT_TYPE(ehci,
							q.sitd->hw_next);
					q = *q_p;
					break;
				}

				*q_p = q.sitd->sitd_next;
				if (!ehci->use_dummy_qh ||
				    q.sitd->hw_next != EHCI_LIST_END(ehci))
					*hw_p = q.sitd->hw_next;
				else
					*hw_p = ehci->dummy->qh_dma;
				type = Q_NEXT_TYPE(ehci, q.sitd->hw_next);
				wmb();
				modified = sitd_complete (ehci, q.sitd);
				q = *q_p;
				break;
			default:
				dbg ("corrupt type %d frame %d shadow %p",
					type, frame, q.ptr);
				
				q.ptr = NULL;
			}

			
			if (unlikely (modified)) {
				if (likely(ehci->periodic_sched > 0))
					goto restart;
				
				now_uframe = clock;
				break;
			}
		}

		if (incomplete && ehci->rh_state == EHCI_RH_RUNNING) {
			ehci->next_uframe = now_uframe;
			break;
		}

		
		
		
		
		

		

		if (now_uframe == clock) {
			unsigned	now;

			if (ehci->rh_state != EHCI_RH_RUNNING
					|| ehci->periodic_sched == 0)
				break;
			ehci->next_uframe = now_uframe;
			now = ehci_read_frame_index(ehci) & (mod - 1);
			if (now_uframe == now)
				break;

			
			clock = now;
			clock_frame = clock >> 3;
			if (ehci->clock_frame != clock_frame) {
				free_cached_lists(ehci);
				ehci->clock_frame = clock_frame;
				++ehci->periodic_stamp;
			}
		} else {
			now_uframe++;
			now_uframe &= mod - 1;
		}
	}
}
