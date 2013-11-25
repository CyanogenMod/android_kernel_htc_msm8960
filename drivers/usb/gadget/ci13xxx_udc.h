/*
 * ci13xxx_udc.h - structures, registers, and macros MIPS USB IP core
 *
 * Copyright (C) 2008 Chipidea - MIPS Technologies, Inc. All rights reserved.
 *
 * Author: David Lopo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Description: MIPS USB IP core family device controller
 *              Structures, registers and logging macros
 */

#ifndef _CI13XXX_h_
#define _CI13XXX_h_

#define CI13XXX_PAGE_SIZE  4096ul 
#define ENDPT_MAX          (32)
#define CTRL_PAYLOAD_MAX   (64)
#define RX        (0)  
#define TX        (1)  

#define CI13XX_REQ_VENDOR_ID(id)  (id & 0xFFFF0000UL)

#define MSM_ETD_TYPE			BIT(1)
#define MSM_EP_PIPE_ID_RESET_VAL	0x1F001F

struct ci13xxx_td {
	
	u32 next;
#define TD_TERMINATE          BIT(0)
#define TD_ADDR_MASK          (0xFFFFFFEUL << 5)
	
	u32 token;
#define TD_STATUS             (0x00FFUL <<  0)
#define TD_STATUS_TR_ERR      BIT(3)
#define TD_STATUS_DT_ERR      BIT(5)
#define TD_STATUS_HALTED      BIT(6)
#define TD_STATUS_ACTIVE      BIT(7)
#define TD_MULTO              (0x0003UL << 10)
#define TD_IOC                BIT(15)
#define TD_TOTAL_BYTES        (0x7FFFUL << 16)
	
	u32 page[5];
#define TD_CURR_OFFSET        (0x0FFFUL <<  0)
#define TD_FRAME_NUM          (0x07FFUL <<  0)
#define TD_RESERVED_MASK      (0x0FFFUL <<  0)
};

struct ci13xxx_qh {
	
	u32 cap;
#define QH_IOS                BIT(15)
#define QH_MAX_PKT            (0x07FFUL << 16)
#define QH_ZLT                BIT(29)
#define QH_MULT               (0x0003UL << 30)
#define QH_MULT_SHIFT         11
	
	u32 curr;
	
	struct ci13xxx_td        td;
	
	u32 RESERVED;
	struct usb_ctrlrequest   setup;
} __attribute__ ((packed));

struct ci13xxx_req {
	struct usb_request   req;
	unsigned             map;
	struct list_head     queue;
	struct ci13xxx_td   *ptr;
	dma_addr_t           dma;
	struct ci13xxx_td   *zptr;
	dma_addr_t           zdma;
};

struct ci13xxx_ep {
	struct usb_ep                          ep;
	const struct usb_endpoint_descriptor  *desc;
	u8                                     dir;
	u8                                     num;
	u8                                     type;
	char                                   name[16];
	struct {
		struct list_head   queue;
		struct ci13xxx_qh *ptr;
		dma_addr_t         dma;
	}                                      qh;
	int                                    wedge;

	
	spinlock_t                            *lock;
	struct device                         *device;
	struct dma_pool                       *td_pool;
	unsigned long dTD_update_fail_count;
	unsigned long			      prime_fail_count;
	int				      prime_timer_count;
	struct timer_list		      prime_timer;
};

struct ci13xxx;
struct ci13xxx_udc_driver {
	const char	*name;
	unsigned long	 flags;
#define CI13XXX_REGS_SHARED		BIT(0)
#define CI13XXX_REQUIRE_TRANSCEIVER	BIT(1)
#define CI13XXX_PULLUP_ON_VBUS		BIT(2)
#define CI13XXX_DISABLE_STREAMING	BIT(3)
#define CI13XXX_ZERO_ITC		BIT(4)
#define CI13XXX_IS_OTG			BIT(5)

#define CI13XXX_CONTROLLER_RESET_EVENT			0
#define CI13XXX_CONTROLLER_CONNECT_EVENT		1
#define CI13XXX_CONTROLLER_SUSPEND_EVENT		2
#define CI13XXX_CONTROLLER_REMOTE_WAKEUP_EVENT	3
#define CI13XXX_CONTROLLER_RESUME_EVENT	        4
#define CI13XXX_CONTROLLER_DISCONNECT_EVENT	    5
	void	(*notify_event) (struct ci13xxx *udc, unsigned event);
};

struct ci13xxx {
	spinlock_t		  *lock;      
	void __iomem              *regs;      

	struct dma_pool           *qh_pool;   
	struct dma_pool           *td_pool;   
	struct usb_request        *status;    

	struct usb_gadget          gadget;     
	struct ci13xxx_ep          ci13xxx_ep[ENDPT_MAX]; 
	u32                        ep0_dir;    
#define ep0out ci13xxx_ep[0]
#define ep0in  ci13xxx_ep[hw_ep_max / 2]
	u8                         remote_wakeup; 
	u8                         suspended;  
	u8                         configured;  
	u8                         test_mode;  

	struct delayed_work        rw_work;    
	struct usb_gadget_driver  *driver;     
	struct ci13xxx_udc_driver *udc_driver; 
	int                        vbus_active; 
	int                        softconnect; 
	unsigned long dTD_update_fail_count;
	struct usb_phy            *transceiver; 
};

struct ci13xxx_platform_data {
	u8 usb_core_id;
	void *prv_data;
};

#define REG_BITS   (32)

#define HCCPARAMS_LEN         BIT(17)

#define DCCPARAMS_DEN         (0x1F << 0)
#define DCCPARAMS_DC          BIT(7)

#define TESTMODE_FORCE        BIT(0)

#define USBCMD_RS             BIT(0)
#define USBCMD_RST            BIT(1)
#define USBCMD_SUTW           BIT(13)
#define USBCMD_ATDTW          BIT(14)

#define USBi_UI               BIT(0)
#define USBi_UEI              BIT(1)
#define USBi_PCI              BIT(2)
#define USBi_URI              BIT(6)
#define USBi_SLI              BIT(8)

#define DEVICEADDR_USBADRA    BIT(24)
#define DEVICEADDR_USBADR     (0x7FUL << 25)

#define PORTSC_FPR            BIT(6)
#define PORTSC_SUSP           BIT(7)
#define PORTSC_HSP            BIT(9)
#define PORTSC_PTC            (0x0FUL << 16)

#define DEVLC_PSPD            (0x03UL << 25)
#define    DEVLC_PSPD_HS      (0x02UL << 25)

#define USBMODE_CM            (0x03UL <<  0)
#define    USBMODE_CM_IDLE    (0x00UL <<  0)
#define    USBMODE_CM_DEVICE  (0x02UL <<  0)
#define    USBMODE_CM_HOST    (0x03UL <<  0)
#define USBMODE_SLOM          BIT(3)
#define USBMODE_SDIS          BIT(4)
#define USBCMD_ITC(n)         (n << 16) 
#define USBCMD_ITC_MASK       (0xFF << 16)

#define ENDPTCTRL_RXS         BIT(0)
#define ENDPTCTRL_RXT         (0x03UL <<  2)
#define ENDPTCTRL_RXR         BIT(6)         
#define ENDPTCTRL_RXE         BIT(7)
#define ENDPTCTRL_TXS         BIT(16)
#define ENDPTCTRL_TXT         (0x03UL << 18)
#define ENDPTCTRL_TXR         BIT(22)        
#define ENDPTCTRL_TXE         BIT(23)

#define ci13xxx_printk(level, format, args...) \
do { \
	if (_udc == NULL) \
		printk(level "[%s] " format "\n", __func__, ## args); \
	else \
		dev_printk(level, _udc->gadget.dev.parent, \
			   "[%s] " format "\n", __func__, ## args); \
} while (0)

#ifndef err
#define err(format, args...)    ci13xxx_printk(KERN_ERR, format, ## args)
#endif

#define warn(format, args...)   ci13xxx_printk(KERN_WARNING, format, ## args)
#define info(format, args...)   ci13xxx_printk(KERN_INFO, format, ## args)

#ifdef TRACE
#define trace(format, args...)      ci13xxx_printk(KERN_DEBUG, format, ## args)
#define dbg_trace(format, args...)  dev_dbg(dev, format, ##args)
#else
#define trace(format, args...)      do {} while (0)
#define dbg_trace(format, args...)  do {} while (0)
#endif

#endif	
