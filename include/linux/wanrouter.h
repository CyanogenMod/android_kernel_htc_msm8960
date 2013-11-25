/*****************************************************************************
* wanrouter.h	Definitions for the WAN Multiprotocol Router Module.
*		This module provides API and common services for WAN Link
*		Drivers and is completely hardware-independent.
*
* Author: 	Nenad Corbic <ncorbic@sangoma.com>
*		Gideon Hack 	
* Additions:	Arnaldo Melo
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jul 21, 2000  Nenad Corbic	Added WAN_FT1_READY State
* Feb 24, 2000  Nenad Corbic    Added support for socket based x25api
* Jan 28, 2000  Nenad Corbic    Added support for the ASYNC protocol.
* Oct 04, 1999  Nenad Corbic 	Updated for 2.1.0 release
* Jun 02, 1999  Gideon Hack	Added support for the S514 adapter.
* May 23, 1999	Arnaldo Melo	Added local_addr to wanif_conf_t
*				WAN_DISCONNECTING state added
* Jul 20, 1998	David Fong	Added Inverse ARP options to 'wanif_conf_t'
* Jun 12, 1998	David Fong	Added Cisco HDLC support.
* Dec 16, 1997	Jaspreet Singh	Moved 'enable_IPX' and 'network_number' to
*				'wanif_conf_t'
* Dec 05, 1997	Jaspreet Singh	Added 'pap', 'chap' to 'wanif_conf_t'
*				Added 'authenticator' to 'wan_ppp_conf_t'
* Nov 06, 1997	Jaspreet Singh	Changed Router Driver version to 1.1 from 1.0
* Oct 20, 1997	Jaspreet Singh	Added 'cir','bc','be' and 'mc' to 'wanif_conf_t'
*				Added 'enable_IPX' and 'network_number' to 
*				'wan_device_t'.  Also added defines for
*				UDP PACKET TYPE, Interrupt test, critical values
*				for RACE conditions.
* Oct 05, 1997	Jaspreet Singh	Added 'dlci_num' and 'dlci[100]' to 
*				'wan_fr_conf_t' to configure a list of dlci(s)
*				for a NODE 
* Jul 07, 1997	Jaspreet Singh	Added 'ttl' to 'wandev_conf_t' & 'wan_device_t'
* May 29, 1997 	Jaspreet Singh	Added 'tx_int_enabled' to 'wan_device_t'
* May 21, 1997	Jaspreet Singh	Added 'udp_port' to 'wan_device_t'
* Apr 25, 1997  Farhan Thawar   Added 'udp_port' to 'wandev_conf_t'
* Jan 16, 1997	Gene Kozin	router_devlist made public
* Jan 02, 1997	Gene Kozin	Initial version (based on wanpipe.h).
*****************************************************************************/

#ifndef	_ROUTER_H
#define	_ROUTER_H

#define	ROUTER_NAME	"wanrouter"	
#define	ROUTER_VERSION	1		
#define	ROUTER_RELEASE	1		
#define	ROUTER_IOCTL	'W'		
#define	ROUTER_MAGIC	0x524D4157L	

enum router_ioctls
{
	ROUTER_SETUP	= ROUTER_IOCTL<<8,	
	ROUTER_DOWN,				
	ROUTER_STAT,				
	ROUTER_IFNEW,				
	ROUTER_IFDEL,				
	ROUTER_IFSTAT,				
	ROUTER_USER	= (ROUTER_IOCTL<<8)+16,	
	ROUTER_USER_MAX	= (ROUTER_IOCTL<<8)+31
};

#define PROC_DATA_PORT_0 0x8000	
#define PROC_DATA_PORT_1 0x8001	

#define	NLPID_IP	0xCC	
#define	NLPID_SNAP	0x80	
#define	NLPID_CLNP	0x81	
#define	NLPID_ESIS	0x82	
#define	NLPID_ISIS	0x83	
#define	NLPID_Q933	0x08	

#define	WAN_IFNAME_SZ	15	
#define	WAN_DRVNAME_SZ	15	
#define	WAN_ADDRESS_SZ	31	
#define USED_BY_FIELD	8	

#define UDP_PTPIPE_TYPE 	0x01
#define UDP_FPIPE_TYPE		0x02
#define UDP_CPIPE_TYPE		0x03
#define UDP_DRVSTATS_TYPE 	0x04
#define UDP_INVALID_TYPE  	0x05

#define CMD_OK		0		
#define CMD_TIMEOUT	0xFF		

#define UDP_PKT_FRM_STACK	0x00
#define UDP_PKT_FRM_NETWORK	0x01

#define MAX_INTR_TEST_COUNTER	100

#define CRITICAL_IN_ISR		0xA1
#define CRITICAL_INTR_HANDLED	0xB1


typedef struct wan_x25_conf
{
	unsigned lo_pvc;	
	unsigned hi_pvc;	
	unsigned lo_svc;	
	unsigned hi_svc;	
	unsigned hdlc_window;	
	unsigned pkt_window;	
	unsigned t1;		
	unsigned t2;		
	unsigned t4;		
	unsigned n2;		
	unsigned t10_t20;	
	unsigned t11_t21;	
	unsigned t12_t22;	
	unsigned t13_t23;	
	unsigned t16_t26;	
	unsigned t28;		
	unsigned r10_r20;	
	unsigned r12_r22;	
	unsigned r13_r23;	
	unsigned ccitt_compat;	
	unsigned x25_conf_opt;   
	unsigned char LAPB_hdlc_only; 
	unsigned char logging;     
	unsigned char oob_on_modem; 
} wan_x25_conf_t;

typedef struct wan_fr_conf
{
	unsigned signalling;	
	unsigned t391;		
	unsigned t392;		
	unsigned n391;		
	unsigned n392;		
	unsigned n393;		
	unsigned dlci_num;	
	unsigned  dlci[100];    
} wan_fr_conf_t;

typedef struct wan_ppp_conf
{
	unsigned restart_tmr;	
	unsigned auth_rsrt_tmr;	
	unsigned auth_wait_tmr;	
	unsigned mdm_fail_tmr;	
	unsigned dtr_drop_tmr;	
	unsigned connect_tmout;	
	unsigned conf_retry;	
	unsigned term_retry;	
	unsigned fail_retry;	
	unsigned auth_retry;	
	unsigned auth_options;	
	unsigned ip_options;	
	char	authenticator;	
	char	ip_mode;	
} wan_ppp_conf_t;

typedef struct wan_chdlc_conf
{
	unsigned char ignore_dcd;	
	unsigned char ignore_cts;	
	unsigned char ignore_keepalive;	
	unsigned char hdlc_streaming;	
	unsigned char receive_only;	
	unsigned keepalive_tx_tmr;	
	unsigned keepalive_rx_tmr;	
	unsigned keepalive_err_margin;	
	unsigned slarp_timer;		
} wan_chdlc_conf_t;


typedef struct wandev_conf
{
	unsigned magic;		
	unsigned config_id;	
				
	unsigned ioport;	
	unsigned long maddr;	
	unsigned msize;		
	int irq;		
	int dma;		
        char S514_CPU_no[1];	
        unsigned PCI_slot_no;	
	char auto_pci_cfg;	
	char comm_port;		 
	unsigned bps;		
	unsigned mtu;		
        unsigned udp_port;      
	unsigned char ttl;	
	unsigned char ft1;	
        char interface;		
	char clocking;		
	char line_coding;	
	char station;		
	char connection;	
	char read_mode;		
	char receive_only;	
	char tty;		
	unsigned tty_major;	
	unsigned tty_minor; 	
	unsigned tty_mode;	
	char backup;		
	unsigned hw_opt[4];	
	unsigned reserved[4];
				
	unsigned data_size;	
	void* data;		
	union			
	{
		wan_x25_conf_t x25;	
		wan_ppp_conf_t ppp;	
		wan_fr_conf_t fr;	
		wan_chdlc_conf_t chdlc;	
	} u;
} wandev_conf_t;

#define	WANCONFIG_X25	101	
#define	WANCONFIG_FR	102	
#define	WANCONFIG_PPP	103	
#define WANCONFIG_CHDLC	104	
#define WANCONFIG_BSC	105	
#define WANCONFIG_HDLC	106	
#define WANCONFIG_MPPP  107	

#define	WANOPT_OFF	0
#define	WANOPT_ON	1
#define	WANOPT_NO	0
#define	WANOPT_YES	1

#define	WANOPT_RS232	0
#define	WANOPT_V35	1

#define	WANOPT_NRZ	0
#define	WANOPT_NRZI	1
#define	WANOPT_FM0	2
#define	WANOPT_FM1	3

#define	WANOPT_POINTTOPOINT	0	
#define	WANOPT_MULTIDROP	1	

#define	WANOPT_EXTERNAL	0
#define	WANOPT_INTERNAL	1

#define	WANOPT_DTE		0
#define	WANOPT_DCE		1
#define	WANOPT_CPE		0
#define	WANOPT_NODE		1
#define	WANOPT_SECONDARY	0
#define	WANOPT_PRIMARY		1

#define	WANOPT_PERMANENT	0	
#define	WANOPT_SWITCHED		1	
#define	WANOPT_ONDEMAND		2	

#define	WANOPT_FR_ANSI		1	
#define	WANOPT_FR_Q933		2	
#define	WANOPT_FR_LMI		3	

#define	WANOPT_PPP_STATIC	0
#define	WANOPT_PPP_HOST		1
#define	WANOPT_PPP_PEER		2

#define WANOPT_ONE 		1
#define WANOPT_TWO		2
#define WANOPT_ONE_AND_HALF	3

#define WANOPT_NONE	0
#define WANOPT_ODD      1
#define WANOPT_EVEN	2


#define WANOPT_PRI 0
#define WANOPT_SEC 1
#define	WANOPT_INTR	0
#define WANOPT_POLL	1


#define WANOPT_TTY_SYNC  0
#define WANOPT_TTY_ASYNC 1
typedef struct wandev_stat
{
	unsigned state;		
	unsigned ndev;		

	
	unsigned connection;	
	unsigned media_type;	
	unsigned mtu;		

	
	unsigned modem_status;	
	unsigned rx_frames;	
	unsigned rx_overruns;	
	unsigned rx_crc_err;	
	unsigned rx_aborts;	
	unsigned rx_bad_length;	
	unsigned rx_dropped;	
	unsigned tx_frames;	
	unsigned tx_underruns;	
	unsigned tx_timeouts;	
	unsigned tx_rejects;	

	
	unsigned rx_bad_format;	
	unsigned rx_bad_addr;	
	unsigned tx_retries;	
	unsigned reserved[16];	
} wandev_stat_t;

enum wan_states
{
	WAN_UNCONFIGURED,	
	WAN_DISCONNECTED,	
	WAN_CONNECTING,		
	WAN_CONNECTED,		
	WAN_LIMIT,		
	WAN_DUALPORT,		
	WAN_DISCONNECTING,
	WAN_FT1_READY		
};

enum {
	WAN_LOCAL_IP,
	WAN_POINTOPOINT_IP,
	WAN_NETMASK_IP,
	WAN_BROADCAST_IP
};

#define	WAN_MODEM_CTS	0x0001	
#define	WAN_MODEM_DCD	0x0002	
#define	WAN_MODEM_DTR	0x0010	
#define	WAN_MODEM_RTS	0x0020	

typedef struct wanif_conf
{
	unsigned magic;			
	unsigned config_id;		
	char name[WAN_IFNAME_SZ+1];	
	char addr[WAN_ADDRESS_SZ+1];	
	char usedby[USED_BY_FIELD];	
	unsigned idle_timeout;		
	unsigned hold_timeout;		
	unsigned cir;			
	unsigned bc;			
	unsigned be;			 
	unsigned char enable_IPX;	
	unsigned char inarp;		
	unsigned inarp_interval;	
	unsigned long network_number;	
	char mc;			
	char local_addr[WAN_ADDRESS_SZ+1];
	unsigned char port;		
	unsigned char protocol;		
	char pap;			
	char chap;			
	unsigned char userid[511];	
	unsigned char passwd[511];	
	unsigned char sysname[31];	
	unsigned char ignore_dcd;	
	unsigned char ignore_cts;	
	unsigned char ignore_keepalive;	
	unsigned char hdlc_streaming;	
	unsigned keepalive_tx_tmr;	
	unsigned keepalive_rx_tmr;	
	unsigned keepalive_err_margin;	
	unsigned slarp_timer;		
	unsigned char ttl;		
	char interface;			
	char clocking;			
	unsigned bps;			
	unsigned mtu;			
	unsigned char if_down;		
	unsigned char gateway;		
	unsigned char true_if_encoding;	

	unsigned char asy_data_trans;     
        unsigned char rts_hs_for_receive; 
        unsigned char xon_xoff_hs_for_receive;
	unsigned char xon_xoff_hs_for_transmit;
	unsigned char dcd_hs_for_transmit;
	unsigned char cts_hs_for_transmit;
	unsigned char async_mode;
	unsigned tx_bits_per_char;
	unsigned rx_bits_per_char;
	unsigned stop_bits;  
	unsigned char parity;
 	unsigned break_timer;
        unsigned inter_char_timer;
	unsigned rx_complete_length;
	unsigned xon_char;
	unsigned xoff_char;
	unsigned char receive_only;	
} wanif_conf_t;

#ifdef	__KERNEL__

#include <linux/fs.h>		
#include <linux/proc_fs.h>	
#include <linux/netdevice.h>	
#include <linux/spinlock.h>     

struct wan_device {
	unsigned magic;			
	char* name;			
	void* private;			
	unsigned config_id;		
					
	unsigned ioport;		
	char S514_cpu_no[1];		
	unsigned char S514_slot_no;	
	unsigned long maddr;		
	unsigned msize;			
	int irq;			
	int dma;			
	unsigned bps;			
	unsigned mtu;			
	unsigned udp_port;              
        unsigned char ttl;		
	unsigned enable_tx_int; 	
	char interface;			
	char clocking;			
	char line_coding;		
	char station;			
	char connection;		
	char signalling;		
	char read_mode;			
	char new_if_cnt;                 
	char del_if_cnt;		
	unsigned char piggyback;        
	unsigned hw_opt[4];		
					
	char state;			
	char api_status;		
	struct net_device_stats stats; 	
	unsigned reserved[16];		
	unsigned long critical;		
	spinlock_t lock;                

					
	int (*setup) (struct wan_device *wandev, wandev_conf_t *conf);
	int (*shutdown) (struct wan_device *wandev);
	int (*update) (struct wan_device *wandev);
	int (*ioctl) (struct wan_device *wandev, unsigned cmd,
		unsigned long arg);
	int (*new_if)(struct wan_device *wandev, struct net_device *dev,
		      wanif_conf_t *conf);
	int (*del_if)(struct wan_device *wandev, struct net_device *dev);
					
	struct wan_device* next;	
	struct net_device* dev;		
	unsigned ndev;			
	struct proc_dir_entry *dent;	
};

extern int register_wan_device(struct wan_device *wandev);
extern int unregister_wan_device(char *name);

extern int wanrouter_proc_init(void);
extern void wanrouter_proc_cleanup(void);
extern int wanrouter_proc_add(struct wan_device *wandev);
extern int wanrouter_proc_delete(struct wan_device *wandev);
extern long wanrouter_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern struct wan_device *wanrouter_router_devlist;

#endif	
#endif	
