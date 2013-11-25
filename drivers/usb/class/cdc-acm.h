

#ifndef CMSPAR
#define CMSPAR			0
#endif


#define ACM_TTY_MAJOR		166
#define ACM_TTY_MINORS		32


#define USB_RT_ACM		(USB_TYPE_CLASS | USB_RECIP_INTERFACE)


#define ACM_CTRL_DTR		0x01
#define ACM_CTRL_RTS		0x02


#define ACM_CTRL_DCD		0x01
#define ACM_CTRL_DSR		0x02
#define ACM_CTRL_BRK		0x04
#define ACM_CTRL_RI		0x08

#define ACM_CTRL_FRAMING	0x10
#define ACM_CTRL_PARITY		0x20
#define ACM_CTRL_OVERRUN	0x40


#define ACM_NW  16
#define ACM_NR  16

struct acm_wb {
	unsigned char *buf;
	dma_addr_t dmah;
	int len;
	int use;
	struct urb		*urb;
	struct acm		*instance;
};

struct acm_rb {
	int			size;
	unsigned char		*base;
	dma_addr_t		dma;
	int			index;
	struct acm		*instance;
};

struct acm {
	struct usb_device *dev;				
	struct usb_interface *control;			
	struct usb_interface *data;			
	struct tty_port port;			 	
	struct urb *ctrlurb;				
	u8 *ctrl_buffer;				
	dma_addr_t ctrl_dma;				
	u8 *country_codes;				
	unsigned int country_code_size;			
	unsigned int country_rel_date;			
	struct acm_wb wb[ACM_NW];
	unsigned long read_urbs_free;
	struct urb *read_urbs[ACM_NR];
	struct acm_rb read_buffers[ACM_NR];
	int rx_buflimit;
	int rx_endpoint;
	spinlock_t read_lock;
	int write_used;					
	int transmitting;
	spinlock_t write_lock;
	struct mutex mutex;
	bool disconnected;
	struct usb_cdc_line_coding line;		
	struct work_struct work;			
	unsigned int ctrlin;				
	unsigned int ctrlout;				
	unsigned int writesize;				
	unsigned int readsize,ctrlsize;			
	unsigned int minor;				
	unsigned char clocal;				
	unsigned int ctrl_caps;				
	unsigned int susp_count;			
	unsigned int combined_interfaces:1;		
	unsigned int is_int_ep:1;			
	unsigned int throttled:1;			
	unsigned int throttle_req:1;			
	u8 bInterval;
	struct acm_wb *delayed_wb;			
};

#define CDC_DATA_INTERFACE_TYPE	0x0a

#define NO_UNION_NORMAL			1
#define SINGLE_RX_URB			2
#define NO_CAP_LINE			4
#define NOT_A_MODEM			8
#define NO_DATA_INTERFACE		16
