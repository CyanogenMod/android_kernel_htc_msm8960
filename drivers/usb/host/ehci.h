/*
 * Copyright (c) 2001-2002 by David Brownell
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

#ifndef __LINUX_EHCI_HCD_H
#define __LINUX_EHCI_HCD_H


#ifdef CONFIG_USB_EHCI_BIG_ENDIAN_DESC
typedef __u32 __bitwise __hc32;
typedef __u16 __bitwise __hc16;
#else
#define __hc32	__le32
#define __hc16	__le16
#endif

struct ehci_stats {
	
	unsigned long		normal;
	unsigned long		error;
	unsigned long		reclaim;
	unsigned long		lost_iaa;

	
	unsigned long		complete;
	unsigned long		unlink;
};


#define	EHCI_MAX_ROOT_PORTS	15		

enum ehci_rh_state {
	EHCI_RH_HALTED,
	EHCI_RH_SUSPENDED,
	EHCI_RH_RUNNING
};

struct ehci_hcd {			
	
	struct ehci_caps __iomem *caps;
	struct ehci_regs __iomem *regs;
	struct ehci_dbg_port __iomem *debug;

	__u32			hcs_params;	
	spinlock_t		lock;
	enum ehci_rh_state	rh_state;

	
	struct ehci_qh		*async;
	struct ehci_qh		*dummy;		
	struct ehci_qh		*reclaim;
	struct ehci_qh		*qh_scan_next;
	unsigned		scanning : 1;

	
#define	DEFAULT_I_TDPS		1024		
	unsigned		periodic_size;
	__hc32			*periodic;	
	dma_addr_t		periodic_dma;
	unsigned		i_thresh;	

	union ehci_shadow	*pshadow;	
	int			next_uframe;	
	unsigned		periodic_sched;	
	unsigned		uframe_periodic_max; 


	
	struct list_head	cached_itd_list;
	struct list_head	cached_sitd_list;
	unsigned		clock_frame;

	
	unsigned long		reset_done [EHCI_MAX_ROOT_PORTS];

	
	unsigned long		bus_suspended;		
	unsigned long		companion_ports;	
	unsigned long		owned_ports;		
	unsigned long		port_c_suspend;		
	unsigned long		suspended_ports;	
	unsigned long		resuming_ports;		

	
	struct dma_pool		*qh_pool;	
	struct dma_pool		*qtd_pool;	
	struct dma_pool		*itd_pool;	
	struct dma_pool		*sitd_pool;	

	struct timer_list	iaa_watchdog;
	struct timer_list	watchdog;
	unsigned long		actions;
	unsigned		periodic_stamp;
	unsigned		random_frame;
	unsigned long		next_statechange;
	ktime_t			last_periodic_enable;
	ktime_t			last_susp_resume;
	u32			command;

	unsigned		max_log2_irq_thresh;

	
	unsigned		no_selective_suspend:1;
	unsigned		has_fsl_port_bug:1; 
	unsigned		big_endian_mmio:1;
	unsigned		big_endian_desc:1;
	unsigned		big_endian_capbase:1;
	unsigned		has_amcc_usb23:1;
	unsigned		need_io_watchdog:1;
	unsigned		broken_periodic:1;
	unsigned		amd_pll_fix:1;
	unsigned		fs_i_thresh:1;	
	unsigned		use_dummy_qh:1;	
	unsigned		has_synopsys_hc_bug:1; 
	unsigned		frame_index_bug:1; 
	unsigned		susp_sof_bug:1; 
	unsigned		resume_sof_bug:1;
	unsigned		reset_sof_bug:1; 

	
	#define OHCI_CTRL_HCFS          (3 << 6)
	#define OHCI_USB_OPER           (2 << 6)
	#define OHCI_USB_SUSPEND        (3 << 6)

	#define OHCI_HCCTRL_OFFSET      0x4
	#define OHCI_HCCTRL_LEN         0x4
	__hc32			*ohci_hcctrl_reg;
	unsigned		has_hostpc:1;
	unsigned		has_lpm:1;  
	unsigned		has_ppcd:1; 
	u8			sbrn;		

	
#ifdef EHCI_STATS
	struct ehci_stats	stats;
#	define COUNT(x) do { (x)++; } while (0)
#else
#	define COUNT(x) do {} while (0)
#endif

	
#ifdef DEBUG
	struct dentry		*debug_dir;
#endif
	struct usb_phy	*transceiver;
};

static inline struct ehci_hcd *hcd_to_ehci (struct usb_hcd *hcd)
{
	return (struct ehci_hcd *) (hcd->hcd_priv);
}
static inline struct usb_hcd *ehci_to_hcd (struct ehci_hcd *ehci)
{
	return container_of ((void *) ehci, struct usb_hcd, hcd_priv);
}


static inline void
iaa_watchdog_start(struct ehci_hcd *ehci)
{
	WARN_ON(timer_pending(&ehci->iaa_watchdog));
	mod_timer(&ehci->iaa_watchdog,
			jiffies + msecs_to_jiffies(EHCI_IAA_MSECS));
}

static inline void iaa_watchdog_done(struct ehci_hcd *ehci)
{
	del_timer(&ehci->iaa_watchdog);
}

enum ehci_timer_action {
	TIMER_IO_WATCHDOG,
	TIMER_ASYNC_SHRINK,
	TIMER_ASYNC_OFF,
};

static inline void
timer_action_done (struct ehci_hcd *ehci, enum ehci_timer_action action)
{
	clear_bit (action, &ehci->actions);
}

static void free_cached_lists(struct ehci_hcd *ehci);


#include <linux/usb/ehci_def.h>


#define	QTD_NEXT(ehci, dma)	cpu_to_hc32(ehci, (u32)dma)

struct ehci_qtd {
	
	__hc32			hw_next;	
	__hc32			hw_alt_next;    
	__hc32			hw_token;       
#define	QTD_TOGGLE	(1 << 31)	
#define	QTD_LENGTH(tok)	(((tok)>>16) & 0x7fff)
#define	QTD_IOC		(1 << 15)	
#define	QTD_CERR(tok)	(((tok)>>10) & 0x3)
#define	QTD_PID(tok)	(((tok)>>8) & 0x3)
#define	QTD_STS_ACTIVE	(1 << 7)	
#define	QTD_STS_HALT	(1 << 6)	
#define	QTD_STS_DBE	(1 << 5)	
#define	QTD_STS_BABBLE	(1 << 4)	
#define	QTD_STS_XACT	(1 << 3)	
#define	QTD_STS_MMF	(1 << 2)	
#define	QTD_STS_STS	(1 << 1)	
#define	QTD_STS_PING	(1 << 0)	

#define ACTIVE_BIT(ehci)	cpu_to_hc32(ehci, QTD_STS_ACTIVE)
#define HALT_BIT(ehci)		cpu_to_hc32(ehci, QTD_STS_HALT)
#define STATUS_BIT(ehci)	cpu_to_hc32(ehci, QTD_STS_STS)

	__hc32			hw_buf [5];        
	__hc32			hw_buf_hi [5];        

	
	dma_addr_t		qtd_dma;		
	struct list_head	qtd_list;		
	struct urb		*urb;			
	size_t			length;			
} __attribute__ ((aligned (32)));

#define QTD_MASK(ehci)	cpu_to_hc32 (ehci, ~0x1f)

#define IS_SHORT_READ(token) (QTD_LENGTH (token) != 0 && QTD_PID (token) == 1)


#define Q_NEXT_TYPE(ehci,dma)	((dma) & cpu_to_hc32(ehci, 3 << 1))

#define Q_TYPE_ITD	(0 << 1)
#define Q_TYPE_QH	(1 << 1)
#define Q_TYPE_SITD	(2 << 1)
#define Q_TYPE_FSTN	(3 << 1)

#define QH_NEXT(ehci,dma)	(cpu_to_hc32(ehci, (((u32)dma)&~0x01f)|Q_TYPE_QH))

#define EHCI_LIST_END(ehci)	cpu_to_hc32(ehci, 1) 

union ehci_shadow {
	struct ehci_qh		*qh;		
	struct ehci_itd		*itd;		
	struct ehci_sitd	*sitd;		
	struct ehci_fstn	*fstn;		
	__hc32			*hw_next;	
	void			*ptr;
};



struct ehci_qh_hw {
	__hc32			hw_next;	
	__hc32			hw_info1;       
#define	QH_HEAD		0x00008000
	__hc32			hw_info2;        
#define	QH_SMASK	0x000000ff
#define	QH_CMASK	0x0000ff00
#define	QH_HUBADDR	0x007f0000
#define	QH_HUBPORT	0x3f800000
#define	QH_MULT		0xc0000000
	__hc32			hw_current;	

	
	__hc32			hw_qtd_next;
	__hc32			hw_alt_next;
	__hc32			hw_token;
	__hc32			hw_buf [5];
	__hc32			hw_buf_hi [5];
} __attribute__ ((aligned(32)));

struct ehci_qh {
	struct ehci_qh_hw	*hw;
	
	dma_addr_t		qh_dma;		
	union ehci_shadow	qh_next;	
	struct list_head	qtd_list;	
	struct ehci_qtd		*dummy;
	struct ehci_qh		*reclaim;	

	struct ehci_hcd		*ehci;
	unsigned long		unlink_time;

	u32			refcount;
	unsigned		stamp;

	u8			needs_rescan;	
	u8			qh_state;
#define	QH_STATE_LINKED		1		
#define	QH_STATE_UNLINK		2		
#define	QH_STATE_IDLE		3		
#define	QH_STATE_UNLINK_WAIT	4		
#define	QH_STATE_COMPLETING	5		

	u8			xacterrs;	
#define	QH_XACTERR_MAX		32		

	
	u8			usecs;		
	u8			gap_uf;		
	u8			c_usecs;	
	u16			tt_usecs;	
	unsigned short		period;		
	unsigned short		start;		
#define NO_FRAME ((unsigned short)~0)			

	struct usb_device	*dev;		
	unsigned		is_out:1;	
	unsigned		clearing_tt:1;	
};


struct ehci_iso_packet {
	
	u64			bufp;		
	__hc32			transaction;	
	u8			cross;		
	
	u32			buf1;
};

struct ehci_iso_sched {
	struct list_head	td_list;
	unsigned		span;
	struct ehci_iso_packet	packet [0];
};

struct ehci_iso_stream {
	
	struct ehci_qh_hw	*hw;

	u32			refcount;
	u8			bEndpointAddress;
	u8			highspeed;
	struct list_head	td_list;	
	struct list_head	free_list;	
	struct usb_device	*udev;
	struct usb_host_endpoint *ep;

	
	int			next_uframe;
	__hc32			splits;

	u8			usecs, c_usecs;
	u16			interval;
	u16			tt_usecs;
	u16			maxp;
	u16			raw_mask;
	unsigned		bandwidth;

	
	__hc32			buf0;
	__hc32			buf1;
	__hc32			buf2;

	
	__hc32			address;
};


struct ehci_itd {
	
	__hc32			hw_next;           
	__hc32			hw_transaction [8]; 
#define EHCI_ISOC_ACTIVE        (1<<31)        
#define EHCI_ISOC_BUF_ERR       (1<<30)        
#define EHCI_ISOC_BABBLE        (1<<29)        
#define EHCI_ISOC_XACTERR       (1<<28)        
#define	EHCI_ITD_LENGTH(tok)	(((tok)>>16) & 0x0fff)
#define	EHCI_ITD_IOC		(1 << 15)	

#define ITD_ACTIVE(ehci)	cpu_to_hc32(ehci, EHCI_ISOC_ACTIVE)

	__hc32			hw_bufp [7];	
	__hc32			hw_bufp_hi [7];	

	
	dma_addr_t		itd_dma;	
	union ehci_shadow	itd_next;	

	struct urb		*urb;
	struct ehci_iso_stream	*stream;	
	struct list_head	itd_list;	

	
	unsigned		frame;		
	unsigned		pg;
	unsigned		index[8];	
} __attribute__ ((aligned (32)));


struct ehci_sitd {
	
	__hc32			hw_next;
	__hc32			hw_fullspeed_ep;	
	__hc32			hw_uframe;		
	__hc32			hw_results;		
#define	SITD_IOC	(1 << 31)	
#define	SITD_PAGE	(1 << 30)	
#define	SITD_LENGTH(x)	(0x3ff & ((x)>>16))
#define	SITD_STS_ACTIVE	(1 << 7)	
#define	SITD_STS_ERR	(1 << 6)	
#define	SITD_STS_DBE	(1 << 5)	
#define	SITD_STS_BABBLE	(1 << 4)	
#define	SITD_STS_XACT	(1 << 3)	
#define	SITD_STS_MMF	(1 << 2)	
#define	SITD_STS_STS	(1 << 1)	

#define SITD_ACTIVE(ehci)	cpu_to_hc32(ehci, SITD_STS_ACTIVE)

	__hc32			hw_buf [2];		
	__hc32			hw_backpointer;		
	__hc32			hw_buf_hi [2];		

	
	dma_addr_t		sitd_dma;
	union ehci_shadow	sitd_next;	

	struct urb		*urb;
	struct ehci_iso_stream	*stream;	
	struct list_head	sitd_list;	
	unsigned		frame;
	unsigned		index;
} __attribute__ ((aligned (32)));


struct ehci_fstn {
	__hc32			hw_next;	
	__hc32			hw_prev;	

	
	dma_addr_t		fstn_dma;
	union ehci_shadow	fstn_next;	
} __attribute__ ((aligned (32)));



#define ehci_prepare_ports_for_controller_suspend(ehci, do_wakeup)	\
		ehci_adjust_port_wakeup_flags(ehci, true, do_wakeup);

#define ehci_prepare_ports_for_controller_resume(ehci)			\
		ehci_adjust_port_wakeup_flags(ehci, false, false);


#ifdef CONFIG_USB_EHCI_ROOT_HUB_TT


#define	ehci_is_TDI(e)			(ehci_to_hcd(e)->has_tt)

static inline unsigned int
ehci_port_speed(struct ehci_hcd *ehci, unsigned int portsc)
{
	if (ehci_is_TDI(ehci)) {
		switch ((portsc >> (ehci->has_hostpc ? 25 : 26)) & 3) {
		case 0:
			return 0;
		case 1:
			return USB_PORT_STAT_LOW_SPEED;
		case 2:
		default:
			return USB_PORT_STAT_HIGH_SPEED;
		}
	}
	return USB_PORT_STAT_HIGH_SPEED;
}

#else

#define	ehci_is_TDI(e)			(0)

#define	ehci_port_speed(ehci, portsc)	USB_PORT_STAT_HIGH_SPEED
#endif


#ifdef CONFIG_PPC_83xx
#define	ehci_has_fsl_portno_bug(e)		((e)->has_fsl_port_bug)
#else
#define	ehci_has_fsl_portno_bug(e)		(0)
#endif


#ifdef CONFIG_USB_EHCI_BIG_ENDIAN_MMIO
#define ehci_big_endian_mmio(e)		((e)->big_endian_mmio)
#define ehci_big_endian_capbase(e)	((e)->big_endian_capbase)
#else
#define ehci_big_endian_mmio(e)		0
#define ehci_big_endian_capbase(e)	0
#endif

#if defined(CONFIG_ARM) && defined(CONFIG_ARCH_IXP4XX)
#define readl_be(addr)		__raw_readl((__force unsigned *)addr)
#define writel_be(val, addr)	__raw_writel(val, (__force unsigned *)addr)
#endif

static inline unsigned int ehci_readl(const struct ehci_hcd *ehci,
		__u32 __iomem * regs)
{
#ifdef CONFIG_USB_EHCI_BIG_ENDIAN_MMIO
	return ehci_big_endian_mmio(ehci) ?
		readl_be(regs) :
		readl(regs);
#else
	return readl(regs);
#endif
}

static inline void ehci_writel(const struct ehci_hcd *ehci,
		const unsigned int val, __u32 __iomem *regs)
{
#ifdef CONFIG_USB_EHCI_BIG_ENDIAN_MMIO
	ehci_big_endian_mmio(ehci) ?
		writel_be(val, regs) :
		writel(val, regs);
#else
	writel(val, regs);
#endif
}

#ifdef CONFIG_44x
static inline void set_ohci_hcfs(struct ehci_hcd *ehci, int operational)
{
	u32 hc_control;

	hc_control = (readl_be(ehci->ohci_hcctrl_reg) & ~OHCI_CTRL_HCFS);
	if (operational)
		hc_control |= OHCI_USB_OPER;
	else
		hc_control |= OHCI_USB_SUSPEND;

	writel_be(hc_control, ehci->ohci_hcctrl_reg);
	(void) readl_be(ehci->ohci_hcctrl_reg);
}
#else
static inline void set_ohci_hcfs(struct ehci_hcd *ehci, int operational)
{ }
#endif


#ifdef CONFIG_USB_EHCI_BIG_ENDIAN_DESC
#define ehci_big_endian_desc(e)		((e)->big_endian_desc)

static inline __hc32 cpu_to_hc32 (const struct ehci_hcd *ehci, const u32 x)
{
	return ehci_big_endian_desc(ehci)
		? (__force __hc32)cpu_to_be32(x)
		: (__force __hc32)cpu_to_le32(x);
}

static inline u32 hc32_to_cpu (const struct ehci_hcd *ehci, const __hc32 x)
{
	return ehci_big_endian_desc(ehci)
		? be32_to_cpu((__force __be32)x)
		: le32_to_cpu((__force __le32)x);
}

static inline u32 hc32_to_cpup (const struct ehci_hcd *ehci, const __hc32 *x)
{
	return ehci_big_endian_desc(ehci)
		? be32_to_cpup((__force __be32 *)x)
		: le32_to_cpup((__force __le32 *)x);
}

#else

static inline __hc32 cpu_to_hc32 (const struct ehci_hcd *ehci, const u32 x)
{
	return cpu_to_le32(x);
}

static inline u32 hc32_to_cpu (const struct ehci_hcd *ehci, const __hc32 x)
{
	return le32_to_cpu(x);
}

static inline u32 hc32_to_cpup (const struct ehci_hcd *ehci, const __hc32 *x)
{
	return le32_to_cpup(x);
}

#endif

#ifdef	CONFIG_ARM_DMA_MEM_BUFFERABLE
static inline void ehci_sync_mem(void)
{
	mb();
}
#else
static inline void ehci_sync_mem(void)
{
}
#endif


#ifdef CONFIG_PCI

static unsigned ehci_read_frame_index(struct ehci_hcd *ehci);

#else

static inline unsigned ehci_read_frame_index(struct ehci_hcd *ehci)
{
	return ehci_readl(ehci, &ehci->regs->frame_index);
}

#endif


#ifndef DEBUG
#define STUB_DEBUG_FILES
#endif	


#endif 
