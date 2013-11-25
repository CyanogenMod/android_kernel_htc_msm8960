/*
 * Header file describing the internal (inter-module) DHD interfaces.
 *
 * Provides type definitions and function prototypes used to link the
 * DHD OS, bus, and protocol modules.
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd.h 365285 2012-10-29 02:16:05Z $
 */


#ifndef _dhd_h_
#define _dhd_h_

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/ethtool.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(CONFIG_HAS_WAKELOCK)
#include <linux/wakelock.h>
#endif 
struct task_struct;
struct sched_param;
int setScheduler(struct task_struct *p, int policy, struct sched_param *param);

#define ALL_INTERFACES	0xff

#include <wlioctl.h>
#include <wlfc_proto.h>


struct dhd_bus;
struct dhd_prot;
struct dhd_info;
struct dhd_ioctl;

enum dhd_bus_state {
	DHD_BUS_DOWN,		
	DHD_BUS_LOAD,		
	DHD_BUS_DATA		
};

enum dhd_op_flags {
	DHD_FLAG_STA_MODE				= (1 << (0)), 
	DHD_FLAG_HOSTAP_MODE				= (1 << (1)), 
	DHD_FLAG_P2P_MODE				= (1 << (2)), 
	
	DHD_FLAG_CONCURR_SINGLE_CHAN_MODE = (DHD_FLAG_STA_MODE | DHD_FLAG_P2P_MODE),
	DHD_FLAG_CONCURR_MULTI_CHAN_MODE		= (1 << (4)), 
	
	DHD_FLAG_P2P_GC_MODE				= (1 << (5)),
	DHD_FLAG_P2P_GO_MODE				= (1 << (6)),
	DHD_FLAG_MBSS_MODE				= (1 << (7)) 
};

#define MANUFACTRING_FW 	"WLTEST"

#ifndef MAX_CNTL_TIMEOUT
#define MAX_CNTL_TIMEOUT  2
#endif

#ifndef MAX_CONCUR_MODE_CNTL_TIMEOUT
#define MAX_CONCUR_MODE_CNTL_TIMEOUT  3
#endif

#ifndef RXGLOM_FAIL_COUNT
#define RXGLOM_FAIL_COUNT 10
#endif

#ifndef RXGLOM_CONCUR_MODE_FAIL_COUNT
#define RXGLOM_CONCUR_MODE_FAIL_COUNT 300
#endif

#define DHD_SCAN_ASSOC_ACTIVE_TIME	40 
#define DHD_SCAN_UNASSOC_ACTIVE_TIME 80 
#define DHD_SCAN_PASSIVE_TIME		130 

#ifndef POWERUP_MAX_RETRY
#define POWERUP_MAX_RETRY	(10) 
#endif
#ifndef POWERUP_WAIT_MS
#define POWERUP_WAIT_MS		2000 
#endif

enum dhd_bus_wake_state {
	WAKE_LOCK_OFF,
	WAKE_LOCK_PRIV,
	WAKE_LOCK_DPC,
	WAKE_LOCK_IOCTL,
	WAKE_LOCK_DOWNLOAD,
	WAKE_LOCK_TMOUT,
	WAKE_LOCK_WATCHDOG,
	WAKE_LOCK_LINK_DOWN_TMOUT,
	WAKE_LOCK_PNO_FIND_TMOUT,
	WAKE_LOCK_SOFTAP_SET,
	WAKE_LOCK_SOFTAP_STOP,
	WAKE_LOCK_SOFTAP_START,
	WAKE_LOCK_SOFTAP_THREAD,
	WAKE_LOCK_MAX
};

enum dhd_prealloc_index {
	DHD_PREALLOC_PROT = 0,
	DHD_PREALLOC_RXBUF,
	DHD_PREALLOC_DATABUF,
	DHD_PREALLOC_OSL_BUF,
	DHD_PREALLOC_SKB_BUF
#if defined(STATIC_WL_PRIV_STRUCT)
	,DHD_PREALLOC_WIPHY_ESCAN0 = 5,
#if defined(DUAL_ESCAN_RESULT_BUFFER)
	DHD_PREALLOC_WIPHY_ESCAN1
#endif 
#endif 
};

typedef enum  {
	DHD_IF_NONE = 0,
	DHD_IF_ADD,
	DHD_IF_DEL,
	DHD_IF_CHANGE,
	DHD_IF_DELETING
} dhd_if_state_t;


#if defined(DHD_USE_STATIC_BUF)

uint8* dhd_os_prealloc(void *osh, int section, uint size);
void dhd_os_prefree(void *osh, void *addr, uint size);
#define DHD_OS_PREALLOC(osh, section, size) dhd_os_prealloc(osh, section, size)
#define DHD_OS_PREFREE(osh, addr, size) dhd_os_prefree(osh, addr, size)

#else

#define DHD_OS_PREALLOC(osh, section, size) MALLOC(osh, size)
#define DHD_OS_PREFREE(osh, addr, size) MFREE(osh, addr, size)

#endif 

#ifndef DHD_SDALIGN
#define DHD_SDALIGN	32
#endif

typedef struct reorder_info {
	void **p;
	uint8 flow_id;
	uint8 cur_idx;
	uint8 exp_idx;
	uint8 max_idx;
	uint8 pend_pkts;
} reorder_info_t;

#ifdef DHDTCPACK_SUPPRESS
#define MAXTCPSTREAMS 4	
typedef struct tcp_ack_info {
	void *p_tcpackinqueue;
	uint32 tcpack_number;
	uint ip_tcp_ttllen;
	uint8 ipaddrs[8];		
	uint8 tcpports[4];		
} tcp_ack_info_t;

void dhd_onoff_tcpack_sup(void *pub, bool on);
#endif 

typedef struct dhd_pub {
	
	osl_t *osh;		
	struct dhd_bus *bus;	
	struct dhd_prot *prot;	
	struct dhd_info  *info; 

	
	bool up;		
	bool txoff;		
	bool dongle_reset;  
	enum dhd_bus_state busstate;
	uint hdrlen;		
	uint maxctl;		
	uint rxsz;		
	uint8 wme_dp;	

	
	bool iswl;		
	ulong drv_version;	
	struct ether_addr mac;	
	dngl_stats_t dstats;	

	
	ulong tx_packets;	
	ulong tx_multicast;	
	ulong tx_errors;	
	ulong tx_ctlpkts;	
	ulong tx_ctlerrs;	
	ulong rx_packets;	
	ulong rx_multicast;	
	ulong rx_errors;	
	ulong rx_ctlpkts;	
	ulong rx_ctlerrs;	
	ulong rx_dropped;	
	ulong rx_flushed;  
	ulong wd_dpc_sched;   

	ulong rx_readahead_cnt;	
	ulong tx_realloc;	
	ulong fc_packets;       

	
	int bcmerror;
	uint tickcnt;

	
	int dongle_error;

	uint8 country_code[WLC_CNTRY_BUF_SZ];

	
	int suspend_disable_flag; 
	int in_suspend;			
#ifdef PNO_SUPPORT
	int pno_enable;                 
	int pno_suspend;		
#endif 
	int suspend_bcn_li_dtim;         
#ifdef PKT_FILTER_SUPPORT
	int early_suspended;	
	int dhcp_in_progress;	
#endif

	
	char * pktfilter[100];
	int pktfilter_count;

	wl_country_t dhd_cspec;		
	char eventmask[WL_EVENTING_MASK_LEN];
	int	op_mode;				

/* Set this to 1 to use a seperate interface (p2p0) for p2p operations.
 *  For ICS MR1 releases it should be disable to be compatable with ICS MR1 Framework
 *  see target dhd-cdc-sdmmc-panda-cfg80211-icsmr1-gpl-debug in Makefile
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(CONFIG_HAS_WAKELOCK)
	struct wake_lock wakelock[WAKE_LOCK_MAX];
#endif 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
	struct mutex 	wl_start_stop_lock; 
	struct mutex 	wl_softap_lock;		 
#endif 

#ifdef WLBTAMP
	uint16	maxdatablks;
#endif 
#ifdef PROP_TXSTATUS
	int   wlfc_enabled;
	void* wlfc_state;
#endif
#ifdef ROAM_AP_ENV_DETECTION
	bool	roam_env_detection;
#endif
	bool	dongle_isolation;
	bool	dongle_trap_occured;	
	int   hang_was_sent;
	int   rxcnt_timeout;		
	int   txcnt_timeout;		
#ifdef WLMEDIA_HTSF
	uint8 htsfdlystat_sz; 
#endif
	struct reorder_info *reorder_bufs[WLHOST_REORDERDATA_MAXFLOWS];
#if defined(ARP_OFFLOAD_SUPPORT)
	uint32 arp_version;
#endif

#ifdef RXFRAME_THREAD
#ifdef RXF_CHAIN
#define MAXSKBPEND 0x10000
#else
#define MAXSKBPEND 0x1000000
#endif
        void *skbbuf[MAXSKBPEND];
        uint32 store_idx;
        uint32 sent_idx;
#endif
#ifdef DHDTCPACK_SUPPRESS
	int tcp_ack_info_cnt;
	tcp_ack_info_t tcp_ack_info_tbl[MAXTCPSTREAMS];
#endif 
} dhd_pub_t;


	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(CONFIG_PM_SLEEP)

	#define DHD_PM_RESUME_WAIT_INIT(a) DECLARE_WAIT_QUEUE_HEAD(a);
	#define _DHD_PM_RESUME_WAIT(a, b) do {\
			int retry = 0; \
			SMP_RD_BARRIER_DEPENDS(); \
			while (dhd_mmc_suspend && retry++ != b) { \
				SMP_RD_BARRIER_DEPENDS(); \
				wait_event_interruptible_timeout(a, !dhd_mmc_suspend, 1); \
			} \
		} 	while (0)
#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)
	#define DHD_PM_RESUME_WAIT(a) 		_DHD_PM_RESUME_WAIT(a, 500)
#else
	#define DHD_PM_RESUME_WAIT(a) 		_DHD_PM_RESUME_WAIT(a, 200)
#endif 
	#define DHD_PM_RESUME_WAIT_FOREVER(a) 	_DHD_PM_RESUME_WAIT(a, ~0)
#ifdef CUSTOMER_HW4
	#define DHD_PM_RESUME_RETURN_ERROR(a)   do { \
			if (dhd_mmc_suspend) { \
				printf("%s[%d]: mmc is still in suspend state!!!\n", \
						__FUNCTION__, __LINE__); \
				return a; \
			} \
		} while (0)
#else
	#define DHD_PM_RESUME_RETURN_ERROR(a)	do { if (dhd_mmc_suspend) return a; } while (0)
#endif 
	#define DHD_PM_RESUME_RETURN		do { if (dhd_mmc_suspend) return; } while (0)

	#define DHD_SPINWAIT_SLEEP_INIT(a) DECLARE_WAIT_QUEUE_HEAD(a);
	#define SPINWAIT_SLEEP(a, exp, us) do { \
		uint countdown = (us) + 9999; \
		while ((exp) && (countdown >= 10000)) { \
			wait_event_interruptible_timeout(a, FALSE, 1); \
			countdown -= 10000; \
		} \
	} while (0)

	#else

	#define DHD_PM_RESUME_WAIT_INIT(a)
	#define DHD_PM_RESUME_WAIT(a)
	#define DHD_PM_RESUME_WAIT_FOREVER(a)
	#define DHD_PM_RESUME_RETURN_ERROR(a)
	#define DHD_PM_RESUME_RETURN

	#define DHD_SPINWAIT_SLEEP_INIT(a)
	#define SPINWAIT_SLEEP(a, exp, us)  do { \
		uint countdown = (us) + 9; \
		while ((exp) && (countdown >= 10)) { \
			OSL_DELAY(10);  \
			countdown -= 10;  \
		} \
	} while (0)

	#endif 
#ifndef DHDTHREAD
#undef	SPINWAIT_SLEEP
#define SPINWAIT_SLEEP(a, exp, us) SPINWAIT(exp, us)
#endif 
#define DHD_IF_VIF	0x01	

unsigned long dhd_os_spin_lock(dhd_pub_t *pub);
void dhd_os_spin_unlock(dhd_pub_t *pub, unsigned long flags);

extern int dhd_os_wake_lock(dhd_pub_t *pub);
extern int dhd_os_wake_unlock(dhd_pub_t *pub);
extern int dhd_os_wake_lock_timeout(dhd_pub_t *pub);
extern int dhd_os_wake_lock_rx_timeout_enable(dhd_pub_t *pub, int val);
extern int dhd_os_wake_lock_ctrl_timeout_enable(dhd_pub_t *pub, int val);
extern void dhd_htc_wake_lock_timeout(dhd_pub_t *pub, int sec);

inline static void MUTEX_LOCK_SOFTAP_SET_INIT(dhd_pub_t * dhdp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
	mutex_init(&dhdp->wl_softap_lock);
#endif 
}

inline static void MUTEX_LOCK_SOFTAP_SET(dhd_pub_t * dhdp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
	mutex_lock(&dhdp->wl_softap_lock);
#endif 
}

inline static void MUTEX_UNLOCK_SOFTAP_SET(dhd_pub_t * dhdp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
	mutex_unlock(&dhdp->wl_softap_lock);
#endif 
}

#ifdef DHD_DEBUG_WAKE_LOCK
#define DHD_OS_WAKE_LOCK(pub) \
	do { \
		printf("call wake_lock: %s %d\n", \
			__FUNCTION__, __LINE__); \
		dhd_os_wake_lock(pub); \
	} while (0)
#define DHD_OS_WAKE_UNLOCK(pub) \
	do { \
		printf("call wake_unlock: %s %d\n", \
			__FUNCTION__, __LINE__); \
		dhd_os_wake_unlock(pub); \
	} while (0)
#define DHD_OS_WAKE_LOCK_TIMEOUT(pub) \
	do { \
		printf("call wake_lock_timeout: %s %d\n", \
			__FUNCTION__, __LINE__); \
		dhd_os_wake_lock_timeout(pub); \
	} while (0)
#define DHD_OS_WAKE_LOCK_RX_TIMEOUT_ENABLE(pub, val) \
	do { \
		printf("call wake_lock_rx_timeout_enable[%d]: %s %d\n", \
			val, __FUNCTION__, __LINE__); \
		dhd_os_wake_lock_rx_timeout_enable(pub, val); \
	} while (0)
#define DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE(pub, val) \
	do { \
		printf("call wake_lock_ctrl_timeout_enable[%d]: %s %d\n", \
			val, __FUNCTION__, __LINE__); \
		dhd_os_wake_lock_ctrl_timeout_enable(pub, val); \
	} while (0)
#else
#define DHD_OS_WAKE_LOCK(pub) 			dhd_os_wake_lock(pub)
#define DHD_OS_WAKE_UNLOCK(pub) 		dhd_os_wake_unlock(pub)
#define DHD_OS_WAKE_LOCK_TIMEOUT(pub)		dhd_os_wake_lock_timeout(pub)
#define DHD_OS_WAKE_LOCK_RX_TIMEOUT_ENABLE(pub, val) \
	dhd_os_wake_lock_rx_timeout_enable(pub, val)
#define DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE(pub, val) \
	dhd_os_wake_lock_ctrl_timeout_enable(pub, val)
#endif 

#define WAKE_LOCK_TIMEOUT(pub, sec)		dhd_htc_wake_lock_timeout(pub, sec)

#define DHD_PACKET_TIMEOUT_MS	1000
#define DHD_EVENT_TIMEOUT_MS	1500

#if defined(CUSTOMER_HW4) && defined(PNO_SUPPORT)
#define DHD_PNO_TIMEOUT_MS	10000
#endif

void dhd_net_if_lock(struct net_device *dev);
void dhd_net_if_unlock(struct net_device *dev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1 && 1
extern struct mutex _dhd_sdio_mutex_lock_;
#endif

typedef struct dhd_if_event {
	uint8 ifidx;
	uint8 action;
	uint8 flags;
	uint8 bssidx;
	uint8 is_AP;
} dhd_if_event_t;

typedef enum dhd_attach_states
{
	DHD_ATTACH_STATE_INIT = 0x0,
	DHD_ATTACH_STATE_NET_ALLOC = 0x1,
	DHD_ATTACH_STATE_DHD_ALLOC = 0x2,
	DHD_ATTACH_STATE_ADD_IF = 0x4,
	DHD_ATTACH_STATE_PROT_ATTACH = 0x8,
	DHD_ATTACH_STATE_WL_ATTACH = 0x10,
	DHD_ATTACH_STATE_THREADS_CREATED = 0x20,
	DHD_ATTACH_STATE_WAKELOCKS_INIT = 0x40,
	DHD_ATTACH_STATE_CFG80211 = 0x80,
	DHD_ATTACH_STATE_EARLYSUSPEND_DONE = 0x100,
	DHD_ATTACH_STATE_DONE = 0x200,
	DHD_ATTACH_STATE_SOFTAP = 0x400   
} dhd_attach_states_t;

#define DHD_PID_KT_INVALID 	-1
#define DHD_PID_KT_TL_INVALID	-2


osl_t *dhd_osl_attach(void *pdev, uint bustype);
void dhd_osl_detach(osl_t *osh);

extern dhd_pub_t *dhd_attach(osl_t *osh, struct dhd_bus *bus, uint bus_hdrlen);
#if defined(WLP2P) && defined(WL_CFG80211)
extern int dhd_attach_p2p(dhd_pub_t *);
extern int dhd_detach_p2p(dhd_pub_t *);
#endif 
extern int dhd_net_attach(dhd_pub_t *dhdp, int idx);

extern void dhd_detach(dhd_pub_t *dhdp);
extern void dhd_free(dhd_pub_t *dhdp);

extern void dhd_txflowcontrol(dhd_pub_t *dhdp, int ifidx, bool on);

extern bool dhd_prec_enq(dhd_pub_t *dhdp, struct pktq *q, void *pkt, int prec);

extern void dhd_rx_frame(dhd_pub_t *dhdp, int ifidx, void *rxp, int numpkt, uint8 chan);

extern char *dhd_ifname(dhd_pub_t *dhdp, int idx);

extern void dhd_sched_dpc(dhd_pub_t *dhdp);

extern void dhd_txcomplete(dhd_pub_t *dhdp, void *txp, bool success);

extern int dhd_os_proto_block(dhd_pub_t * pub);
extern int dhd_os_proto_unblock(dhd_pub_t * pub);
extern int dhd_os_ioctl_resp_wait(dhd_pub_t * pub, uint * condition, bool * pending);
extern int dhd_os_ioctl_resp_wake(dhd_pub_t * pub);
extern unsigned int dhd_os_get_ioctl_resp_timeout(void);
extern void dhd_os_set_ioctl_resp_timeout(unsigned int timeout_msec);
extern void * dhd_os_open_image(char * filename);
extern int dhd_os_get_image_block(char * buf, int len, void * image);
extern void dhd_os_close_image(void * image);
extern void dhd_os_wd_timer(void *bus, uint wdtick);
extern void dhd_os_sdlock(dhd_pub_t * pub);
extern void dhd_os_sdunlock(dhd_pub_t * pub);
extern void dhd_os_sdlock_txq(dhd_pub_t * pub);
extern void dhd_os_sdunlock_txq(dhd_pub_t * pub);
extern void dhd_os_sdlock_rxq(dhd_pub_t * pub);
extern void dhd_os_sdunlock_rxq(dhd_pub_t * pub);
extern void dhd_os_sdlock_sndup_rxq(dhd_pub_t * pub);
#ifdef DHDTCPACK_SUPPRESS
extern void dhd_os_tcpacklock(dhd_pub_t *pub);
extern void dhd_os_tcpackunlock(dhd_pub_t *pub);
#endif 

extern void dhd_customer_gpio_wlan_ctrl(int onoff);
extern int dhd_custom_get_mac_address(unsigned char *buf);
extern void dhd_os_sdunlock_sndup_rxq(dhd_pub_t * pub);
extern void dhd_os_sdlock_eventq(dhd_pub_t * pub);
extern void dhd_os_sdunlock_eventq(dhd_pub_t * pub);
extern bool dhd_os_check_hang(dhd_pub_t *dhdp, int ifidx, int ret);
extern int dhd_os_send_hang_message(dhd_pub_t *dhdp);
extern int net_os_send_hang_message(struct net_device *dev);
extern void dhd_set_version_info(dhd_pub_t *pub, char *fw);


#ifdef PNO_SUPPORT
extern int dhd_pno_enable(dhd_pub_t *dhd, int pfn_enabled);
extern int dhd_pno_clean(dhd_pub_t *dhd);
extern int dhd_pno_set(dhd_pub_t *dhd, wlc_ssid_t* ssids_local, int nssid,
                       ushort  scan_fr, int pno_repeat, int pno_freq_expo_max);
extern int dhd_pno_get_status(dhd_pub_t *dhd);
extern int dhd_dev_pno_reset(struct net_device *dev);
extern int dhd_dev_pno_set(struct net_device *dev, wlc_ssid_t* ssids_local,
                           int nssid, ushort  scan_fr, int pno_repeat, int pno_freq_expo_max);
extern int dhd_dev_pno_enable(struct net_device *dev,  int pfn_enabled);
extern int dhd_dev_get_pno_status(struct net_device *dev);
#endif 

extern bool dhd_check_ap_mode_set(dhd_pub_t *dhd);
extern int wl_android_black_list_match(char *ea);


#ifdef PKT_FILTER_SUPPORT
#define DHD_UNICAST_FILTER_NUM		0
#define DHD_BROADCAST_FILTER_NUM	1
#define DHD_MULTICAST4_FILTER_NUM	2
#define DHD_MULTICAST6_FILTER_NUM	3
#define DHD_MDNS_FILTER_NUM		4
extern int 	dhd_os_enable_packet_filter(dhd_pub_t *dhdp, int val);
extern void dhd_enable_packet_filter(int value, dhd_pub_t *dhd);
extern int net_os_enable_packet_filter(struct net_device *dev, int val);
extern int dhd_os_set_packet_filter(dhd_pub_t *dhdp, int val);
extern int net_os_rxfilter_add_remove(struct net_device *dev, int val, int num);
#endif 

extern int dhd_get_suspend_bcn_li_dtim(dhd_pub_t *dhd);
extern bool dhd_support_sta_mode(dhd_pub_t *dhd);
struct dd_pkt_filter_s{
	int add;
	int id;
	int offset;
	char mask[256];
	char pattern[256];
};

extern int dhd_set_pktfilter(dhd_pub_t *dhd, int add, int id, int offset, char *mask, char *pattern);
extern int wl_android_set_pktfilter(struct net_device *dev, struct dd_pkt_filter_s *data);


#ifdef DHD_DEBUG
extern int write_to_file(dhd_pub_t *dhd, uint8 *buf, int size);
#endif 
#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)
extern int dhd_customer_oob_irq_map(unsigned long *irq_flags_ptr);
#endif 
extern void dhd_os_sdtxlock(dhd_pub_t * pub);
extern void dhd_os_sdtxunlock(dhd_pub_t * pub);

typedef struct {
	uint32 limit;		
	uint32 increment;	
	uint32 elapsed;		
	uint32 tick;		
} dhd_timeout_t;

extern void dhd_timeout_start(dhd_timeout_t *tmo, uint usec);
extern int dhd_timeout_expired(dhd_timeout_t *tmo);

extern int dhd_ifname2idx(struct dhd_info *dhd, char *name);
extern int dhd_net2idx(struct dhd_info *dhd, struct net_device *net);
extern struct net_device * dhd_idx2net(void *pub, int ifidx);
extern int net_os_send_hang_message(struct net_device *dev);
extern void dhd_state_set_flags(struct dhd_pub *dhd_pub, dhd_attach_states_t flags, int add);
extern int wl_host_event(dhd_pub_t *dhd_pub, int *idx, void *pktdata,
                         wl_event_msg_t *, void **data_ptr);
extern void wl_event_to_host_order(wl_event_msg_t * evt);

extern int dhd_wl_ioctl(dhd_pub_t *dhd_pub, int ifindex, wl_ioctl_t *ioc, void *buf, int len);
extern int dhd_wl_ioctl_cmd(dhd_pub_t *dhd_pub, int cmd, void *arg, int len, uint8 set,
                            int ifindex);

extern void dhd_common_init(osl_t *osh);

extern int dhd_do_driver_init(struct net_device *net);
extern int dhd_add_if(struct dhd_info *dhd, int ifidx, void *handle,
	char *name, uint8 *mac_addr, uint32 flags, uint8 bssidx);
extern void dhd_del_if(struct dhd_info *dhd, int ifidx);

extern void dhd_vif_add(struct dhd_info *dhd, int ifidx, char * name);
extern void dhd_vif_del(struct dhd_info *dhd, int ifidx);

extern void dhd_event(struct dhd_info *dhd, char *evpkt, int evlen, int ifidx);
extern void dhd_vif_sendup(struct dhd_info *dhd, int ifidx, uchar *cp, int len);


extern int dhd_sendpkt(dhd_pub_t *dhdp, int ifidx, void *pkt);

extern void dhd_sendup_event_common(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data);
extern void dhd_sendup_event(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data);
extern int dhd_bus_devreset(dhd_pub_t *dhdp, uint8 flag);
extern uint dhd_bus_status(dhd_pub_t *dhdp);
extern int  dhd_bus_start(dhd_pub_t *dhdp);
extern void dhd_info_send_hang_message(dhd_pub_t *dhdp);
extern int dhd_bus_membytes(dhd_pub_t *dhdp, bool set, uint32 address, uint8 *data, uint size);
extern void dhd_print_buf(void *pbuf, int len, int bytes_per_line);
extern bool dhd_is_associated(dhd_pub_t *dhd, void *bss_buf, int *retval);
extern uint dhd_bus_chip_id(dhd_pub_t *dhdp);

#if defined(KEEP_ALIVE)
extern int dhd_keep_alive_onoff(dhd_pub_t *dhd);
#endif 

extern bool dhd_is_concurrent_mode(dhd_pub_t *dhd);

typedef enum cust_gpio_modes {
	WLAN_RESET_ON,
	WLAN_RESET_OFF,
	WLAN_POWER_ON,
	WLAN_POWER_OFF
} cust_gpio_modes_t;

extern int wl_iw_iscan_set_scan_broadcast_prep(struct net_device *dev, uint flag);
extern int wl_iw_send_priv_event(struct net_device *dev, char *flag);
extern int net_os_send_rssilow_message(struct net_device *dev);

extern uint dhd_watchdog_ms;

#if defined(DHD_DEBUG)
extern uint dhd_console_ms;
extern uint wl_msg_level;
#endif 

extern uint dhd_slpauto;

extern uint dhd_intr;

extern uint dhd_poll;

extern uint dhd_arp_mode;

extern uint dhd_arp_enable;

extern uint dhd_pkt_filter_enable;

extern uint dhd_pkt_filter_init;

extern uint dhd_master_mode;

extern uint dhd_roam_disable;

extern uint dhd_radio_up;

extern int dhd_idletime;
#ifdef DHD_USE_IDLECOUNT
#define DHD_IDLETIME_TICKS 5
#else
#define DHD_IDLETIME_TICKS 1
#endif 

extern uint dhd_sdiod_drive_strength;

#ifdef SOFTAP
extern uint dhd_apsta;
#endif
extern uint dhd_force_tx_queueing;
#define DEFAULT_KEEP_ALIVE_PERIOD	55000
#ifndef CUSTOM_KEEP_ALIVE_PERIOD
#define CUSTOM_KEEP_ALIVE_PERIOD	DEFAULT_KEEP_ALIVE_PERIOD
#endif

#define NULL_PKT_STR	"null_pkt"

#define DEFAULT_GLOM_VALUE 	-1
#ifndef CUSTOM_GLOM_SETTING
#define CUSTOM_GLOM_SETTING 	DEFAULT_GLOM_VALUE
#endif

#ifndef CUSTOM_DPC_PRIO_SETTING
#define CUSTOM_DPC_PRIO_SETTING         DEFAULT_DHP_DPC_PRIO
#endif

#ifdef RXFRAME_THREAD
#ifndef CUSTOM_RXF_PRIO_SETTING
#define CUSTOM_RXF_PRIO_SETTING         DEFAULT_DHP_DPC_PRIO + 1
#endif
#endif

#define WL_AUTO_ROAM_TRIGGER -75
#define DEFAULT_ROAM_TRIGGER_VALUE -75 
#define DEFAULT_ROAM_TRIGGER_SETTING 	-1
#ifndef CUSTOM_ROAM_TRIGGER_SETTING
#define CUSTOM_ROAM_TRIGGER_SETTING 	DEFAULT_ROAM_TRIGGER_VALUE
#endif

#define DEFAULT_ROAM_DELTA_VALUE  10 
#define DEFAULT_ROAM_DELTA_SETTING 	-1
#ifndef CUSTOM_ROAM_DELTA_SETTING
#define CUSTOM_ROAM_DELTA_SETTING 	DEFAULT_ROAM_DELTA_VALUE
#endif

#define DEFAULT_DHP_DPC_PRIO  103
#ifndef CUSTOM_DPC_PRIO_SETTING
#define CUSTOM_DPC_PRIO_SETTING 	DEFAULT_DHP_DPC_PRIO
#endif

#define DEFAULT_SUSPEND_BCN_LI_DTIM		3
#ifndef CUSTOM_SUSPEND_BCN_LI_DTIM
#define CUSTOM_SUSPEND_BCN_LI_DTIM		DEFAULT_SUSPEND_BCN_LI_DTIM
#endif

#ifdef SDTEST
extern uint dhd_pktgen;

extern uint dhd_pktgen_len;
#define MAX_PKTGEN_LEN 1800
#endif


#define MOD_PARAM_PATHLEN	2048
#define MOD_PARAM_INFOLEN	512
extern char fw_path[MOD_PARAM_PATHLEN];
extern char nv_path[MOD_PARAM_PATHLEN];

#ifdef SOFTAP
extern char fw_path2[MOD_PARAM_PATHLEN];
#endif

extern uint dhd_download_fw_on_driverload;

#if defined(WL_CFG80211) && defined(SUPPORT_DEEP_SLEEP)
extern int trigger_deep_sleep;
int dhd_deepsleep(struct net_device *dev, int flag);
#endif 

#define DHD_MAX_IFS	16
#define DHD_DEL_IF	-0xe
#define DHD_BAD_IF	-0xf
#ifdef PNO_SUPPORT
#define MAX_PFN_NUMBER	2
#define PFN_SCAN_FREQ	300 
#define PFN_WAKE_TIME	20000	
int dhd_set_pfn_ssid(char * ssid, int ssid_len);
int dhd_del_pfn_ssid(char * ssid, int ssid_len);
void dhd_clear_pfn(void);
int dhd_set_pfn(dhd_pub_t *dhd, int enabled);
#endif

enum pkt_filter_id {
	ALLOW_UNICAST = 100,
	ALLOW_ARP,
	ALLOW_DHCP,
	ALLOW_IPV4_MULTICAST,
	ALLOW_IPV6_MULTICAST,
};
int dhd_set_pktfilter(dhd_pub_t * dhd, int add, int id, int offset, char *mask, char *pattern);

#ifdef PROP_TXSTATUS
typedef struct dhd_pkttag {
	uint16	if_flags;
	uint8	dstn_ether[ETHER_ADDR_LEN];
	uint32	htod_tag;
	
	union {
		struct {
			void* stuff;
			uint32 thing1;
			uint32 thing2;
		} sd;
		struct {
			void* bus;
			void* urb;
		} usb;
	} bus_specific;
} dhd_pkttag_t;

#define DHD_PKTTAG_SET_H2DTAG(tag, h2dvalue)	((dhd_pkttag_t*)(tag))->htod_tag = (h2dvalue)
#define DHD_PKTTAG_H2DTAG(tag)					(((dhd_pkttag_t*)(tag))->htod_tag)

#define DHD_PKTTAG_IFMASK		0xf
#define DHD_PKTTAG_IFTYPE_MASK	0x1
#define DHD_PKTTAG_IFTYPE_SHIFT	7
#define DHD_PKTTAG_FIFO_MASK	0x7
#define DHD_PKTTAG_FIFO_SHIFT	4

#define DHD_PKTTAG_SIGNALONLY_MASK			0x1
#define DHD_PKTTAG_SIGNALONLY_SHIFT			10

#define DHD_PKTTAG_ONETIMEPKTRQST_MASK		0x1
#define DHD_PKTTAG_ONETIMEPKTRQST_SHIFT		11

#define DHD_PKTTAG_PKTDIR_MASK			0x1
#define DHD_PKTTAG_PKTDIR_SHIFT			9

#define DHD_PKTTAG_CREDITCHECK_MASK		0x1
#define DHD_PKTTAG_CREDITCHECK_SHIFT	8

#define DHD_PKTTAG_INVALID_FIFOID 0x7

#define DHD_PKTTAG_SETFIFO(tag, fifo)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & ~(DHD_PKTTAG_FIFO_MASK << DHD_PKTTAG_FIFO_SHIFT)) | \
	(((fifo) & DHD_PKTTAG_FIFO_MASK) << DHD_PKTTAG_FIFO_SHIFT)
#define DHD_PKTTAG_FIFO(tag)			((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_FIFO_SHIFT) & DHD_PKTTAG_FIFO_MASK)

#define DHD_PKTTAG_SETIF(tag, if)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & ~DHD_PKTTAG_IFMASK) | ((if) & DHD_PKTTAG_IFMASK)
#define DHD_PKTTAG_IF(tag)	(((dhd_pkttag_t*)(tag))->if_flags & DHD_PKTTAG_IFMASK)

#define DHD_PKTTAG_SETIFTYPE(tag, isAP)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & \
	~(DHD_PKTTAG_IFTYPE_MASK << DHD_PKTTAG_IFTYPE_SHIFT)) | \
	(((isAP) & DHD_PKTTAG_IFTYPE_MASK) << DHD_PKTTAG_IFTYPE_SHIFT)
#define DHD_PKTTAG_IFTYPE(tag)	((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_IFTYPE_SHIFT) & DHD_PKTTAG_IFTYPE_MASK)

#define DHD_PKTTAG_SETCREDITCHECK(tag, check)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & \
	~(DHD_PKTTAG_CREDITCHECK_MASK << DHD_PKTTAG_CREDITCHECK_SHIFT)) | \
	(((check) & DHD_PKTTAG_CREDITCHECK_MASK) << DHD_PKTTAG_CREDITCHECK_SHIFT)
#define DHD_PKTTAG_CREDITCHECK(tag)	((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_CREDITCHECK_SHIFT) & DHD_PKTTAG_CREDITCHECK_MASK)

#define DHD_PKTTAG_SETPKTDIR(tag, dir)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & \
	~(DHD_PKTTAG_PKTDIR_MASK << DHD_PKTTAG_PKTDIR_SHIFT)) | \
	(((dir) & DHD_PKTTAG_PKTDIR_MASK) << DHD_PKTTAG_PKTDIR_SHIFT)
#define DHD_PKTTAG_PKTDIR(tag)	((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_PKTDIR_SHIFT) & DHD_PKTTAG_PKTDIR_MASK)

#define DHD_PKTTAG_SETSIGNALONLY(tag, signalonly)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & \
	~(DHD_PKTTAG_SIGNALONLY_MASK << DHD_PKTTAG_SIGNALONLY_SHIFT)) | \
	(((signalonly) & DHD_PKTTAG_SIGNALONLY_MASK) << DHD_PKTTAG_SIGNALONLY_SHIFT)
#define DHD_PKTTAG_SIGNALONLY(tag)	((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_SIGNALONLY_SHIFT) & DHD_PKTTAG_SIGNALONLY_MASK)

#define DHD_PKTTAG_SETONETIMEPKTRQST(tag)	((dhd_pkttag_t*)(tag))->if_flags = \
	(((dhd_pkttag_t*)(tag))->if_flags & \
	~(DHD_PKTTAG_ONETIMEPKTRQST_MASK << DHD_PKTTAG_ONETIMEPKTRQST_SHIFT)) | \
	(1 << DHD_PKTTAG_ONETIMEPKTRQST_SHIFT)
#define DHD_PKTTAG_ONETIMEPKTRQST(tag)	((((dhd_pkttag_t*)(tag))->if_flags >> \
	DHD_PKTTAG_ONETIMEPKTRQST_SHIFT) & DHD_PKTTAG_ONETIMEPKTRQST_MASK)

#define DHD_PKTTAG_SETDSTN(tag, dstn_MAC_ea)	memcpy(((dhd_pkttag_t*)((tag)))->dstn_ether, \
	(dstn_MAC_ea), ETHER_ADDR_LEN)
#define DHD_PKTTAG_DSTN(tag)	((dhd_pkttag_t*)(tag))->dstn_ether

typedef int (*f_commitpkt_t)(void* ctx, void* p);

#ifdef PROP_TXSTATUS_DEBUG
#define DHD_WLFC_CTRINC_MAC_CLOSE(entry)	do { (entry)->closed_ct++; } while (0)
#define DHD_WLFC_CTRINC_MAC_OPEN(entry)		do { (entry)->opened_ct++; } while (0)
#else
#define DHD_WLFC_CTRINC_MAC_CLOSE(entry)	do {} while (0)
#define DHD_WLFC_CTRINC_MAC_OPEN(entry)		do {} while (0)
#endif

#endif 

enum dhdhtc_pwr_ctrl{
	DHDHTC_POWER_CTRL_ANDROID_NORMAL = 0,
	DHDHTC_POWER_CTRL_BROWSER_LOAD_PAGE,
	DHDHTC_POWER_CTRL_USER_CONFIG,
	DHDHTC_POWER_CTRL_WIFI_PHONE,
	DHDHTC_POWER_CTRL_FOTA_DOWNLOADING,
	DHDHTC_POWER_CTRL_KDDI_APK,
	DHDHTC_POWER_CTRL_MAX_NUM,
};
extern int dhdhtc_update_wifi_power_mode(int is_screen_off);
extern int dhdhtc_set_power_control(int power_mode, unsigned int reason);
extern unsigned int dhdhtc_get_cur_pwr_ctrl(void);
extern int dhdhtc_update_dtim_listen_interval(int is_screen_off);
#ifdef BCMSDIOH_STD
extern int sd_uhsimode;
#endif
extern char firmware_path[MOD_PARAM_PATHLEN];
extern void dhd_wait_for_event(dhd_pub_t *dhd, bool *lockvar);
extern void dhd_wait_event_wakeup(dhd_pub_t*dhd);

#if defined(SOFTAP)
extern bool ap_fw_loaded;
#endif

#define IFLOCK_INIT(lock)       *lock = 0
#define IFLOCK(lock)    while (InterlockedCompareExchange((lock), 1, 0))	\
	NdisStallExecution(1);
#define IFUNLOCK(lock)  InterlockedExchange((lock), 0)
#define IFLOCK_FREE(lock)

#ifdef PNO_SUPPORT
extern int dhd_pno_enable(dhd_pub_t *dhd, int pfn_enabled);
extern int dhd_pnoenable(dhd_pub_t *dhd, int pfn_enabled);
extern int dhd_pno_clean(dhd_pub_t *dhd);
extern int dhd_pno_set(dhd_pub_t *dhd, wlc_ssid_t* ssids_local, int nssid,
                       ushort  scan_fr, int pno_repeat, int pno_freq_expo_max);
extern int dhd_pno_get_status(dhd_pub_t *dhd);
extern int dhd_pno_set_add(dhd_pub_t *dhd, wl_pfn_t *netinfo, int nssid, ushort scan_fr,
	ushort slowscan_fr, uint8 pno_repeat, uint8 pno_freq_expo_max, int16 flags);
extern int dhd_pno_cfg(dhd_pub_t *dhd, wl_pfn_cfg_t *pcfg);
extern int dhd_pno_suspend(dhd_pub_t *dhd, int pfn_suspend);
#endif 
#ifdef ARP_OFFLOAD_SUPPORT
#define MAX_IPV4_ENTRIES	8
void dhd_arp_offload_set(dhd_pub_t * dhd, int arp_mode);
void dhd_arp_offload_enable(dhd_pub_t * dhd, int arp_enable);

void dhd_aoe_hostip_clr(dhd_pub_t *dhd, int idx);
void dhd_aoe_arp_clr(dhd_pub_t *dhd, int idx);
int dhd_arp_get_arp_hostip_table(dhd_pub_t *dhd, void *buf, int buflen, int idx);
void dhd_arp_offload_add_ip(dhd_pub_t *dhd, uint32 ipaddr, int idx);
#endif 

int dhd_ioctl_process(dhd_pub_t *pub, int ifidx, struct dhd_ioctl *ioc);

#if defined(CUSTOMER_HW4) && defined(SUPPORT_MULTIPLE_REVISION)
extern int concate_revision(struct dhd_bus *bus, char *path, int path_len);
#endif 
extern int dhd_get_txrx_stats(struct net_device *net, unsigned long *rx_packets, unsigned long *tx_packets);
extern bool dhd_APUP;

#define MAX_TXQ_FULL_EVENT 300
extern int block_ap_event;
extern int dhdcdc_power_active_while_plugin;
extern char wl_abdroid_gatewaybuf[8+1]; 
#ifdef DHD_BCM_WIFI_HDMI
extern bool dhd_bcm_whdmi_enable;

#define DHD_WHDMI_SOFTAP_IF_NAME	"wl0.2"
#define DHD_WHDMI_SOFTAP_IF_NAME_LEN	5
#define DHD_WHDMI_SOFTAP_IF_NUM		2

#endif 
#endif 
