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

#ifndef __USB_CORE_HCD_H
#define __USB_CORE_HCD_H

#ifdef __KERNEL__

#include <linux/rwsem.h>

#define MAX_TOPO_LEVEL		6


#define USB_PID_EXT			0xf0	
#define USB_PID_OUT			0xe1
#define USB_PID_ACK			0xd2
#define USB_PID_DATA0			0xc3
#define USB_PID_PING			0xb4	
#define USB_PID_SOF			0xa5
#define USB_PID_NYET			0x96	
#define USB_PID_DATA2			0x87	
#define USB_PID_SPLIT			0x78	
#define USB_PID_IN			0x69
#define USB_PID_NAK			0x5a
#define USB_PID_DATA1			0x4b
#define USB_PID_PREAMBLE		0x3c	
#define USB_PID_ERR			0x3c	
#define USB_PID_SETUP			0x2d
#define USB_PID_STALL			0x1e
#define USB_PID_MDATA			0x0f	




struct usb_hcd {

	struct usb_bus		self;		
	struct kref		kref;		

	const char		*product_desc;	
	int			speed;		
	char			irq_descr[24];	

	struct timer_list	rh_timer;	
	struct urb		*status_urb;	
#ifdef CONFIG_USB_SUSPEND
	struct work_struct	wakeup_work;	
#endif

	const struct hc_driver	*driver;	

	unsigned long		flags;
#define HCD_FLAG_HW_ACCESSIBLE		0	
#define HCD_FLAG_POLL_RH		2	
#define HCD_FLAG_POLL_PENDING		3	
#define HCD_FLAG_WAKEUP_PENDING		4	
#define HCD_FLAG_RH_RUNNING		5	
#define HCD_FLAG_DEAD			6	

#define HCD_HW_ACCESSIBLE(hcd)	((hcd)->flags & (1U << HCD_FLAG_HW_ACCESSIBLE))
#define HCD_POLL_RH(hcd)	((hcd)->flags & (1U << HCD_FLAG_POLL_RH))
#define HCD_POLL_PENDING(hcd)	((hcd)->flags & (1U << HCD_FLAG_POLL_PENDING))
#define HCD_WAKEUP_PENDING(hcd)	((hcd)->flags & (1U << HCD_FLAG_WAKEUP_PENDING))
#define HCD_RH_RUNNING(hcd)	((hcd)->flags & (1U << HCD_FLAG_RH_RUNNING))
#define HCD_DEAD(hcd)		((hcd)->flags & (1U << HCD_FLAG_DEAD))

	
	unsigned		rh_registered:1;
	unsigned		rh_pollable:1;	
	unsigned		msix_enabled:1;	

	unsigned		uses_new_polling:1;
	unsigned		wireless:1;	
	unsigned		authorized_default:1;
	unsigned		has_tt:1;	

	unsigned int		irq;		
	void __iomem		*regs;		
	u64			rsrc_start;	
	u64			rsrc_len;	
	unsigned		power_budget;	

	struct mutex		*bandwidth_mutex;
	struct usb_hcd		*shared_hcd;
	struct usb_hcd		*primary_hcd;

	struct work_struct	ssr_work;
#define HCD_BUFFER_POOLS	4
	struct dma_pool		*pool[HCD_BUFFER_POOLS];

	int			state;
#	define	__ACTIVE		0x01
#	define	__SUSPEND		0x04
#	define	__TRANSIENT		0x80

#	define	HC_STATE_HALT		0
#	define	HC_STATE_RUNNING	(__ACTIVE)
#	define	HC_STATE_QUIESCING	(__SUSPEND|__TRANSIENT|__ACTIVE)
#	define	HC_STATE_RESUMING	(__SUSPEND|__TRANSIENT)
#	define	HC_STATE_SUSPENDED	(__SUSPEND)

#define	HC_IS_RUNNING(state) ((state) & __ACTIVE)
#define	HC_IS_SUSPENDED(state) ((state) & __SUSPEND)


	unsigned long hcd_priv[0]
			__attribute__ ((aligned(sizeof(s64))));
};

static inline struct usb_bus *hcd_to_bus(struct usb_hcd *hcd)
{
	return &hcd->self;
}

static inline struct usb_hcd *bus_to_hcd(struct usb_bus *bus)
{
	return container_of(bus, struct usb_hcd, self);
}

struct hcd_timeout {	
	struct list_head	timeout_list;
	struct timer_list	timer;
};



struct hc_driver {
	const char	*description;	
	const char	*product_desc;	
	size_t		hcd_priv_size;	

	
	irqreturn_t	(*irq) (struct usb_hcd *hcd);

	int	flags;
#define	HCD_MEMORY	0x0001		
#define	HCD_LOCAL_MEM	0x0002		
#define	HCD_SHARED	0x0004		
#define	HCD_OLD_ENUM	0x0008		
#define	HCD_USB11	0x0010		
#define	HCD_USB2	0x0020		
#define	HCD_USB3	0x0040		
#define	HCD_MASK	0x0070

	
	int	(*reset) (struct usb_hcd *hcd);
	int	(*start) (struct usb_hcd *hcd);

	
	int	(*pci_suspend)(struct usb_hcd *hcd, bool do_wakeup);

	
	int	(*pci_resume)(struct usb_hcd *hcd, bool hibernated);

	
	void	(*stop) (struct usb_hcd *hcd);

	
	void	(*shutdown) (struct usb_hcd *hcd);

	
	int	(*get_frame_number) (struct usb_hcd *hcd);

	
	int	(*urb_enqueue)(struct usb_hcd *hcd,
				struct urb *urb, gfp_t mem_flags);
	int	(*urb_dequeue)(struct usb_hcd *hcd,
				struct urb *urb, int status);

	int	(*map_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb,
				   gfp_t mem_flags);
	void    (*unmap_urb_for_dma)(struct usb_hcd *hcd, struct urb *urb);

	
	void	(*endpoint_disable)(struct usb_hcd *hcd,
			struct usb_host_endpoint *ep);

	void	(*endpoint_reset)(struct usb_hcd *hcd,
			struct usb_host_endpoint *ep);

	
	int	(*hub_status_data) (struct usb_hcd *hcd, char *buf);
	int	(*hub_control) (struct usb_hcd *hcd,
				u16 typeReq, u16 wValue, u16 wIndex,
				char *buf, u16 wLength);
	int	(*bus_suspend)(struct usb_hcd *);
	int	(*bus_resume)(struct usb_hcd *);
	int	(*start_port_reset)(struct usb_hcd *, unsigned port_num);

		
	void	(*relinquish_port)(struct usb_hcd *, int);
		
	int	(*port_handed_over)(struct usb_hcd *, int);

		
	void	(*clear_tt_buffer_complete)(struct usb_hcd *,
				struct usb_host_endpoint *);

	
		
	int	(*alloc_dev)(struct usb_hcd *, struct usb_device *);
		
	void	(*free_dev)(struct usb_hcd *, struct usb_device *);
	
	int	(*alloc_streams)(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags);
	int	(*free_streams)(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags);

	
		
	int	(*add_endpoint)(struct usb_hcd *, struct usb_device *,
				struct usb_host_endpoint *);
		
	int	(*drop_endpoint)(struct usb_hcd *, struct usb_device *,
				 struct usb_host_endpoint *);
	int	(*check_bandwidth)(struct usb_hcd *, struct usb_device *);
	void	(*reset_bandwidth)(struct usb_hcd *, struct usb_device *);
		
	int	(*address_device)(struct usb_hcd *, struct usb_device *udev);
	int	(*update_hub_device)(struct usb_hcd *, struct usb_device *hdev,
			struct usb_tt *tt, gfp_t mem_flags);
	int	(*reset_device)(struct usb_hcd *, struct usb_device *);
	int	(*update_device)(struct usb_hcd *, struct usb_device *);
	int	(*set_usb2_hw_lpm)(struct usb_hcd *, struct usb_device *, int);

	
	void	(*log_urb_complete)(struct urb *urb, char * event,
			unsigned extra);
	void	(*dump_regs)(struct usb_hcd *);
	void	(*dump_qh_qtd)(struct usb_hcd *);
	void    (*reset_sof_bug_handler)(struct usb_hcd *hcd, u32 val);
	
	int	(*set_int_latency)(struct usb_hcd *hcd, int latency);
};

extern int usb_hcd_link_urb_to_ep(struct usb_hcd *hcd, struct urb *urb);
extern int usb_hcd_check_unlink_urb(struct usb_hcd *hcd, struct urb *urb,
		int status);
extern void usb_hcd_unlink_urb_from_ep(struct usb_hcd *hcd, struct urb *urb);

extern int usb_hcd_submit_urb(struct urb *urb, gfp_t mem_flags);
extern int usb_hcd_unlink_urb(struct urb *urb, int status);
extern void usb_hcd_giveback_urb(struct usb_hcd *hcd, struct urb *urb,
		int status);
extern int usb_hcd_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
		gfp_t mem_flags);
extern void usb_hcd_unmap_urb_setup_for_dma(struct usb_hcd *, struct urb *);
extern void usb_hcd_unmap_urb_for_dma(struct usb_hcd *, struct urb *);
extern void usb_hcd_flush_endpoint(struct usb_device *udev,
		struct usb_host_endpoint *ep);
extern void usb_hcd_disable_endpoint(struct usb_device *udev,
		struct usb_host_endpoint *ep);
extern void usb_hcd_reset_endpoint(struct usb_device *udev,
		struct usb_host_endpoint *ep);
extern void usb_hcd_synchronize_unlinks(struct usb_device *udev);
extern int usb_hcd_alloc_bandwidth(struct usb_device *udev,
		struct usb_host_config *new_config,
		struct usb_host_interface *old_alt,
		struct usb_host_interface *new_alt);
extern int usb_hcd_get_frame_number(struct usb_device *udev);

extern struct usb_hcd *usb_create_hcd(const struct hc_driver *driver,
		struct device *dev, const char *bus_name);
extern struct usb_hcd *usb_create_shared_hcd(const struct hc_driver *driver,
		struct device *dev, const char *bus_name,
		struct usb_hcd *shared_hcd);
extern struct usb_hcd *usb_get_hcd(struct usb_hcd *hcd);
extern void usb_put_hcd(struct usb_hcd *hcd);
extern int usb_hcd_is_primary_hcd(struct usb_hcd *hcd);
#ifdef CONFIG_USB
extern int usb_add_hcd(struct usb_hcd *hcd,
		unsigned int irqnum, unsigned long irqflags);
extern void usb_remove_hcd(struct usb_hcd *hcd);
#else
static inline int
usb_add_hcd(struct usb_hcd *hcd, unsigned int irqnum, unsigned long irqflags)
{
	return 0;
}
static inline void usb_remove_hcd(struct usb_hcd *hcd) {}
#endif

struct platform_device;
extern void usb_hcd_platform_shutdown(struct platform_device *dev);

#ifdef CONFIG_PCI
struct pci_dev;
struct pci_device_id;
extern int usb_hcd_pci_probe(struct pci_dev *dev,
				const struct pci_device_id *id);
extern void usb_hcd_pci_remove(struct pci_dev *dev);
extern void usb_hcd_pci_shutdown(struct pci_dev *dev);

#ifdef CONFIG_PM_SLEEP
extern const struct dev_pm_ops usb_hcd_pci_pm_ops;
#endif
#endif 

int hcd_buffer_create(struct usb_hcd *hcd);
void hcd_buffer_destroy(struct usb_hcd *hcd);

void *hcd_buffer_alloc(struct usb_bus *bus, size_t size,
	gfp_t mem_flags, dma_addr_t *dma);
void hcd_buffer_free(struct usb_bus *bus, size_t size,
	void *addr, dma_addr_t dma);

extern irqreturn_t usb_hcd_irq(int irq, void *__hcd);

extern void usb_hc_died(struct usb_hcd *hcd);
extern void usb_hcd_poll_rh_status(struct usb_hcd *hcd);
extern void usb_wakeup_notification(struct usb_device *hdev,
		unsigned int portnum);

#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> (ep)) & 1)
#define	usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << (ep)))
#define usb_settoggle(dev, ep, out, bit) \
		((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << (ep))) | \
		 ((bit) << (ep)))


extern struct usb_device *usb_alloc_dev(struct usb_device *parent,
					struct usb_bus *, unsigned port);
extern int usb_new_device(struct usb_device *dev);
extern void usb_disconnect(struct usb_device **);

extern int usb_get_configuration(struct usb_device *dev);
extern void usb_destroy_configuration(struct usb_device *dev);



#include <linux/usb/ch11.h>


struct usb_device;

struct usb_tt {
	struct usb_device	*hub;	
	int			multi;	
	unsigned		think_time;	

	
	spinlock_t		lock;
	struct list_head	clear_list;	
	struct work_struct	clear_work;
};

struct usb_tt_clear {
	struct list_head	clear_list;
	unsigned		tt;
	u16			devinfo;
	struct usb_hcd		*hcd;
	struct usb_host_endpoint	*ep;
};

extern int usb_hub_clear_tt_buffer(struct urb *urb);
extern void usb_ep0_reinit(struct usb_device *);

#define DeviceRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define DeviceOutRequest \
	((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)

#define InterfaceRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

#define EndpointRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define EndpointOutRequest \
	((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

#define ClearHubFeature		(0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature	(0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor	(0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus		(0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus		(0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature		(0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature		(0x2300 | USB_REQ_SET_FEATURE)



#define SetHubDepth		(0x3000 | HUB_SET_DEPTH)
#define GetPortErrorCount	(0x8000 | HUB_GET_PORT_ERR_COUNT)

#define FRAME_TIME_USECS	1000L
#define BitTime(bytecount) (7 * 8 * bytecount / 6) 
		

#define NS_TO_US(ns)	((ns + 500L) / 1000L)
			


#define BW_HOST_DELAY	1000L		
#define BW_HUB_LS_SETUP	333L		
			

#define FRAME_TIME_BITS			12000L	
#define FRAME_TIME_MAX_BITS_ALLOC	(90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_MAX_USECS_ALLOC	(90L * FRAME_TIME_USECS / 100L)

#define USB2_HOST_DELAY	5	
#define HS_NSECS(bytes) (((55 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_NSECS_ISO(bytes) (((38 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_USECS(bytes)		NS_TO_US(HS_NSECS(bytes))
#define HS_USECS_ISO(bytes)	NS_TO_US(HS_NSECS_ISO(bytes))

extern long usb_calc_bus_time(int speed, int is_input,
			int isoc, int bytecount);


extern void usb_set_device_state(struct usb_device *udev,
		enum usb_device_state new_state);



extern struct list_head usb_bus_list;
extern struct mutex usb_bus_list_lock;
extern wait_queue_head_t usb_kill_urb_queue;

extern int usb_find_interface_driver(struct usb_device *dev,
	struct usb_interface *interface);

#define usb_endpoint_out(ep_dir)	(!((ep_dir) & USB_DIR_IN))

#ifdef CONFIG_PM
extern void usb_root_hub_lost_power(struct usb_device *rhdev);
extern int hcd_bus_suspend(struct usb_device *rhdev, pm_message_t msg);
extern int hcd_bus_resume(struct usb_device *rhdev, pm_message_t msg);
#endif 

#ifdef CONFIG_USB_SUSPEND
extern void usb_hcd_resume_root_hub(struct usb_hcd *hcd);
#else
static inline void usb_hcd_resume_root_hub(struct usb_hcd *hcd)
{
	return;
}
#endif 



#ifdef CONFIG_USB_DEVICEFS

extern void usbfs_update_special(void);
extern int usbfs_init(void);
extern void usbfs_cleanup(void);

#else 

static inline void usbfs_update_special(void) {}
static inline int usbfs_init(void) { return 0; }
static inline void usbfs_cleanup(void) { }

#endif 


#if defined(CONFIG_USB_MON) || defined(CONFIG_USB_MON_MODULE)

struct usb_mon_operations {
	void (*urb_submit)(struct usb_bus *bus, struct urb *urb);
	void (*urb_submit_error)(struct usb_bus *bus, struct urb *urb, int err);
	void (*urb_complete)(struct usb_bus *bus, struct urb *urb, int status);
	
};

extern struct usb_mon_operations *mon_ops;

static inline void usbmon_urb_submit(struct usb_bus *bus, struct urb *urb)
{
	if (bus->monitored)
		(*mon_ops->urb_submit)(bus, urb);
}

static inline void usbmon_urb_submit_error(struct usb_bus *bus, struct urb *urb,
    int error)
{
	if (bus->monitored)
		(*mon_ops->urb_submit_error)(bus, urb, error);
}

static inline void usbmon_urb_complete(struct usb_bus *bus, struct urb *urb,
		int status)
{
	if (bus->monitored)
		(*mon_ops->urb_complete)(bus, urb, status);
}

int usb_mon_register(struct usb_mon_operations *ops);
void usb_mon_deregister(void);

#else

static inline void usbmon_urb_submit(struct usb_bus *bus, struct urb *urb) {}
static inline void usbmon_urb_submit_error(struct usb_bus *bus, struct urb *urb,
    int error) {}
static inline void usbmon_urb_complete(struct usb_bus *bus, struct urb *urb,
		int status) {}

#endif 



#define	RUN_CONTEXT (in_irq() ? "in_irq" \
		: (in_interrupt() ? "in_interrupt" : "can sleep"))


extern struct rw_semaphore ehci_cf_port_reset_rwsem;

#define USB_UHCI_LOADED		0
#define USB_OHCI_LOADED		1
#define USB_EHCI_LOADED		2
extern unsigned long usb_hcds_loaded;

#endif 

#endif 
