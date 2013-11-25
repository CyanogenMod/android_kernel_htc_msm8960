/*
 * Linux cfg80211 driver - Android related functions
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
 * $Id: wl_android.c 366419 2012-11-02 04:51:33Z $
 */

#include <linux/module.h>
#include <linux/netdevice.h>

#include <wl_android.h>
#include <wldev_common.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include <linux_osl.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <bcmsdbus.h>
#ifdef WL_CFG80211
#include <wl_cfg80211.h>
#endif
#if defined(CONFIG_WIFI_CONTROL_FUNC)
#include <linux/platform_device.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
#include <linux/wlan_plat.h>
#else
#include <linux/wifi_tiwlan.h>
#endif
#include <wl_iw.h>
#endif 

#include <sdio.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/clk.h>
#include "bcmsdh_sdmmc.h"
#include "../../../mmc/host/msm_sdcc.h"
extern PBCMSDH_SDMMC_INSTANCE gInstance;
extern int module_insert;


#define CMD_START		"START"
#define CMD_STOP		"STOP"
#define	CMD_SCAN_ACTIVE		"SCAN-ACTIVE"
#define	CMD_SCAN_PASSIVE	"SCAN-PASSIVE"
#define CMD_RSSI		"RSSI"
#define CMD_LINKSPEED		"LINKSPEED"
#define CMD_RXFILTER_START	"RXFILTER-START"
#define CMD_RXFILTER_STOP	"RXFILTER-STOP"
#define CMD_RXFILTER_ADD	"RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE	"RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP	"BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE		"BTCOEXMODE"
#define CMD_SETSUSPENDOPT	"SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE      "SETSUSPENDMODE"
#define CMD_MAC_ADDR		"MACADDR"
#define CMD_P2P_DEV_ADDR	"P2P_DEV_ADDR"
#define CMD_SETFWPATH		"SETFWPATH"
#define CMD_SETBAND		"SETBAND"
#define CMD_GETBAND		"GETBAND"
#define CMD_COUNTRY		"COUNTRY"
#define CMD_P2P_SET_NOA		"P2P_SET_NOA"
#if !defined WL_ENABLE_P2P_IF
#define CMD_P2P_GET_NOA			"P2P_GET_NOA"
#endif
#define CMD_P2P_SET_PS		"P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE 		"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE 	"SETROAMMODE"

#ifdef SUPPORT_AUTO_CHANNEL
#define CMD_SET_HAPD_AUTO_CHANNEL	"HAPD_AUTO_CHANNEL"
#endif 

#define CMD_P2P_SET_MPC 		"P2P_MPC_SET"
#define CMD_DEAUTH_STA 		"DEAUTH_STA"

#define CMD_DTIM_SKIP_SET	"DTIMSKIPSET"
#define CMD_DTIM_SKIP_GET	"DTIMSKIPGET"
#define CMD_TXPOWER_SET		"TXPOWER"
#define CMD_POWER_MODE_SET	"POWERMODE"
#define CMD_POWER_MODE_GET	"GETPOWER"
#define CMD_AP_TXPOWER_SET	"AP_TXPOWER_SET"
#define CMD_AP_ASSOC_LIST_GET	"AP_ASSOC_LIST_GET"
#define CMD_AP_MAC_LIST_SET	"AP_MAC_LIST_SET"
#define CMD_SCAN_MINRSSI_SET	"SCAN_MINRSSI"
#define CMD_LOW_RSSI_SET	"LOW_RSSI_IND_SET"
#define CMD_GET_TX_FAIL        "GET_TX_FAIL"
#if defined(HTC_TX_TRACKING)
#define CMD_TX_TRACKING		"SET_TX_TRACKING"
#endif
#ifdef SUPPORT_HIDDEN_AP
#define CMD_SET_HAPD_MAX_NUM_STA	"HAPD_MAX_NUM_STA"
#define CMD_SET_HAPD_SSID		"HAPD_SSID"
#define CMD_SET_HAPD_HIDE_SSID		"HAPD_HIDE_SSID"
#endif 
#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
#define CMD_HAPD_STA_DISASSOC		"HAPD_STA_DISASSOC"
#endif 

#define CMD_GETWIFILOCK		"GETWIFILOCK"
#define CMD_SETWIFICALL		"WIFICALL"
#define CMD_SETPROJECT		"SET_PROJECT"
#define CMD_SETLOWPOWERMODE		"SET_LOWPOWERMODE"
#define CMD_GATEWAYADD		"GATEWAY-ADD"
#ifdef APSTA_CONCURRENT
#define CMD_SET_AP_CFG		"SET_AP_CFG"
#define CMD_GET_AP_STATUS	"GET_AP_STATUS"
#define CMD_SET_APSTA		"SET_APSTA"
#define CMD_SET_BCN_TIMEOUT "SET_BCN_TIMEOUT"
#define CMD_SET_VENDR_IE    "SET_VENDR_IE"
#define CMD_ADD_VENDR_IE    "ADD_VENDR_IE"
#define CMD_DEL_VENDR_IE    "DEL_VENDR_IE"
#define CMD_SCAN_SUPPRESS   "SCAN_SUPPRESS"
#define CMD_SCAN_ABORT		"SCAN_ABORT"
#define CMD_GET_CONAP_CHANNEL "GET_CONAP_CHANNEL"
#define CMD_OLD_DNGL 		  "OLD_DNGL"
#endif

#ifdef BRCM_WPSAP
#define CMD_SET_WSEC		"SET_WSEC"
#define CMD_WPS_RESULT		"WPS_RESULT"
#endif 

#ifdef BCMCCX				
#define CMD_GETCCKM_RN		"get cckm_rn"
#define CMD_SETCCKM_KRK		"set cckm_krk"
#define CMD_GET_ASSOC_RES_IES	"get assoc_res_ies"
#endif


#ifdef PNO_SUPPORT
#define CMD_PNOSSIDCLR_SET	"PNOSSIDCLR"
#define CMD_PNOSETUP_SET	"PNOSETUP "
#define CMD_PNOENABLE_SET	"PNOFORCE"
#define CMD_PNODEBUG_SET	"PNODEBUG"
#define CMD_PFN_REMOVE		"PFN_REMOVE"

#define PNO_TLV_PREFIX			'S'
#define PNO_TLV_VERSION			'1'
#define PNO_TLV_SUBVERSION 		'2'
#define PNO_TLV_RESERVED		'0'
#define PNO_TLV_TYPE_SSID_IE		'S'
#define PNO_TLV_TYPE_TIME		'T'
#define PNO_TLV_FREQ_REPEAT		'R'
#define PNO_TLV_FREQ_EXPO_MAX		'M'

#define CMD_GET_AUTO_CHANNEL	"AUTOCHANNELGET"

typedef struct cmd_tlv {
	char prefix;
	char version;
	char subver;
	char reserved;
} cmd_tlv_t;
#endif 

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"


#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)

static uint32 last_txframes = 0xffffffff;
static uint32 last_txretrans = 0xffffffff;
static uint32 last_txerror = 0xffffffff;

#define TX_FAIL_CHECK_COUNT		100
static int wl_android_get_tx_fail(struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	wl_cnt_t cnt;
	int error = 0;
	uint32 curr_txframes = 0;
	uint32 curr_txretrans = 0;
	uint32 curr_txerror = 0;
	uint32 txframes_diff = 0;
	uint32 txretrans_diff = 0;
	uint32 txerror_diff = 0;
	uint32 diff_ratio = 0;
	uint32 total_cnt = 0;

	memset(&cnt, 0, sizeof(wl_cnt_t));
	strcpy((char *)&cnt, "counters");

	if ((error = wldev_ioctl(dev, WLC_GET_VAR, &cnt, sizeof(wl_cnt_t), 0)) < 0) {
		DHD_ERROR(("%s: get tx fail fail\n", __func__));
		last_txframes = 0xffffffff;
		last_txretrans = 0xffffffff;
		last_txerror = 0xffffffff;
		goto exit;
	}

	curr_txframes = cnt.txframe;
	curr_txretrans = cnt.txretrans;
    curr_txerror = cnt.txerror;

	if (last_txframes != 0xffffffff) {
		if ((curr_txframes >= last_txframes) && (curr_txretrans >= last_txretrans) && (curr_txerror >= last_txerror)) {

			txframes_diff = curr_txframes - last_txframes;
			txretrans_diff = curr_txretrans - last_txretrans;
			txerror_diff = curr_txerror - last_txerror;
			total_cnt = txframes_diff + txretrans_diff + txerror_diff;

			if (total_cnt > TX_FAIL_CHECK_COUNT) {
				diff_ratio = ((txretrans_diff + txerror_diff)  * 100) / total_cnt;
			}
		}
	}
	last_txframes = curr_txframes;
	last_txretrans = curr_txretrans;
	last_txerror = curr_txerror;

exit:
	printf("TXPER:%d, txframes: %d ,txretrans: %d, txerror: %d, total: %d\n", diff_ratio, txframes_diff, txretrans_diff, txerror_diff, total_cnt);
	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_GET_TX_FAIL, diff_ratio);

	return bytes_written;
}

#ifdef ROAM_API
#define CMD_ROAMTRIGGER_SET "SETROAMTRIGGER"
#define CMD_ROAMTRIGGER_GET "GETROAMTRIGGER"
#define CMD_ROAMDELTA_SET "SETROAMDELTA"
#define CMD_ROAMDELTA_GET "GETROAMDELTA"
#define CMD_ROAMSCANPERIOD_SET "SETROAMSCANPERIOD"
#define CMD_ROAMSCANPERIOD_GET "GETROAMSCANPERIOD"
#define CMD_FULLROAMSCANPERIOD_SET "SETFULLROAMSCANPERIOD"
#define CMD_FULLROAMSCANPERIOD_GET "GETFULLROAMSCANPERIOD"
#define CMD_COUNTRYREV_SET "SETCOUNTRYREV"
#define CMD_COUNTRYREV_GET "GETCOUNTRYREV"
#endif 

#ifdef WES_SUPPORT
#define CMD_GETROAMSCANCONTROL "GETROAMSCANCONTROL"
#define CMD_SETROAMSCANCONTROL "SETROAMSCANCONTROL"
#define CMD_GETROAMSCANCHANNELS "GETROAMSCANCHANNELS"
#define CMD_SETROAMSCANCHANNELS "SETROAMSCANCHANNELS"

#define CMD_GETSCANCHANNELTIME "GETSCANCHANNELTIME"
#define CMD_SETSCANCHANNELTIME "SETSCANCHANNELTIME"
#define CMD_GETSCANHOMETIME "GETSCANHOMETIME"
#define CMD_SETSCANHOMETIME "SETSCANHOMETIME"
#define CMD_GETSCANHOMEAWAYTIME "GETSCANHOMEAWAYTIME"
#define CMD_SETSCANHOMEAWAYTIME "SETSCANHOMEAWAYTIME"
#define CMD_GETSCANNPROBES "GETSCANNPROBES"
#define CMD_SETSCANNPROBES "SETSCANNPROBES"

#define CMD_SENDACTIONFRAME "SENDACTIONFRAME"
#define CMD_REASSOC "REASSOC"

#define CMD_GETWESMODE "GETWESMODE"
#define CMD_SETWESMODE "SETWESMODE"

#define CMD_GETOKCMODE "GETOKCMODE"
#define CMD_SETOKCMODE "SETOKCMODE"

#define ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS 100

typedef struct android_wifi_reassoc_params {
	unsigned char bssid[18];
	int channel;
} android_wifi_reassoc_params_t;

#define ANDROID_WIFI_REASSOC_PARAMS_SIZE sizeof(struct android_wifi_reassoc_params)

#define ANDROID_WIFI_ACTION_FRAME_SIZE 1040

typedef struct android_wifi_af_params {
	unsigned char bssid[18];
	int channel;
	int dwell_time;
	int len;
	unsigned char data[ANDROID_WIFI_ACTION_FRAME_SIZE];
} android_wifi_af_params_t;

#define ANDROID_WIFI_AF_PARAMS_SIZE sizeof(struct android_wifi_af_params)
#endif 
#ifdef SUPPORT_AMPDU_MPDU_CMD
#define CMD_AMPDU_MPDU		"AMPDU_MPDU"
#endif 

#define CMD_CHANGE_RL 	"CHANGE_RL"
#define CMD_RESTORE_RL  "RESTORE_RL"
#endif 
typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

#ifdef WL_GENL
static s32 wl_genl_handle_msg(struct sk_buff *skb, struct genl_info *info);
static int wl_genl_init(void);
static int wl_genl_deinit(void);

extern struct net init_net;
static struct nla_policy wl_genl_policy[BCM_GENL_ATTR_MAX + 1] = {
	[BCM_GENL_ATTR_MSG] = { .type = NLA_NUL_STRING },
};

#define WL_GENL_VER 1
static struct genl_family wl_genl_family = {
	.id = GENL_ID_GENERATE,    
	.hdrsize = 0,
	.name = "bcm-genl",        
	.version = WL_GENL_VER,     
	.maxattr = BCM_GENL_ATTR_MAX,
};

struct genl_ops wl_genl_ops = {
	.cmd = BCM_GENL_CMD_MSG,
	.flags = 0,
	.policy = wl_genl_policy,
	.doit = wl_genl_handle_msg,
	.dumpit = NULL,
};

static struct genl_multicast_group wl_genl_mcast = {
	.id = GENL_ID_GENERATE,    
	.name = "bcm-genl-mcast",
};

#endif 

void dhd_customer_gpio_wlan_ctrl(int onoff);
int dhd_dev_reset(struct net_device *dev, uint8 flag);
int dhd_dev_init_ioctl(struct net_device *dev);
#ifdef WL_CFG80211
int wl_cfg80211_get_mac_addr(struct net_device *net, struct ether_addr *mac_addr);
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr);
int wl_cfg80211_set_btcoex_dhcp(struct net_device *dev, char *command);
void wl_cfg80211_set_btcoex_done(struct net_device *dev);
#if defined(CUSTOMER_HW4) && defined(WES_SUPPORT)
int wl_cfg80211_set_wes_mode(int mode);
int wl_cfg80211_get_wes_mode(void);
int wl_cfg80211_get_ioctl_version(void);
#endif
int wl_cfg80211_set_apsta_concurrent(struct net_device *dev, bool enable);

#else
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr)
{ return 0; }
int wl_cfg80211_set_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_get_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_p2p_ps(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_mpc(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_deauth_sta(struct net_device *net, char* buf, int len)
{ return 0; }
#endif 
extern int dhd_os_check_if_up(void *dhdp);
extern void *bcmsdh_get_drvdata(void);
#if defined(PROP_TXSTATUS) && !defined(PROP_TXSTATUS_VSDB)
extern int dhd_wlfc_init(dhd_pub_t *dhd);
extern void dhd_wlfc_deinit(dhd_pub_t *dhd);
#endif

#if defined(CUSTOMER_HW4) && defined(WES_SUPPORT)
extern int get_roamscan_mode(struct net_device *dev, int *mode);
extern int set_roamscan_mode(struct net_device *dev, int mode);
extern int get_roamscan_channel_list(struct net_device *dev, unsigned char channels[]);
extern int set_roamscan_channel_list(struct net_device *dev, unsigned char n,
	unsigned char channels[], int ioctl_ver);
#endif

extern bool check_hang_already(struct net_device *dev); 


extern bool ap_fw_loaded;
#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
extern char iface_name[IFNAMSIZ];
#endif 

#ifndef WIFI_TURNOFF_DELAY
#define WIFI_TURNOFF_DELAY	100
#endif

static int g_wifi_on = TRUE;
int block_ap_event = 0;

#ifdef APSTA_CONCURRENT
static int wl_android_get_ap_status(struct net_device *net, char *command, int total_len)
{
	printf("%s: enter, command=%s\n", __FUNCTION__, command);
	
	return wldev_get_ap_status(net);
}

extern int net_os_send_hang_message(struct net_device *dev);

static int wl_android_set_ap_cfg(struct net_device *net, char *command, int total_len)
{
	int res;
	printf("%s: enter, command=%s\n", __FUNCTION__, command);

	wl_cfg80211_set_apsta_concurrent(net, TRUE);

	

    if((res = wldev_set_apsta_cfg(net, (command + strlen(CMD_SET_AP_CFG) + 1))) < 0){
        printf("%s fail to set wldev_set_apsta_cfg res[%d] \n",__FUNCTION__,res);
        net_os_send_hang_message(net);
    }else{
        printf("%s Successful set wldev_set_apsta_cfg res[%d] \n",__FUNCTION__,res);
    }

	
	return res;
}

static int wl_android_set_apsta(struct net_device *net, char *command, int total_len)
{
	int enable = bcm_atoi(command + strlen(CMD_SET_APSTA) + 1);

	printf("%s: enter, command=%s, enable=%d\n", __FUNCTION__, command, enable);

	if (enable) {
		wldev_set_pktfilter_enable(net, FALSE);
		wldev_set_apsta(net, TRUE);
	} else {
		wldev_set_apsta(net, FALSE);
		wldev_set_pktfilter_enable(net, TRUE);
	}		

	return 0;
}
	
static int wl_android_set_bcn_timeout(struct net_device *dev, char *command, int total_len)
{
	int err = -1;
    	char tran_buf[32] = {0};
    	char *ptr;
	int bcn_timeout;
	ptr = command + strlen(CMD_SET_BCN_TIMEOUT) + 1;
    	memcpy(tran_buf, ptr, 2);
    	bcn_timeout = bcm_strtoul(tran_buf, NULL, 10);
    	printf("%s bcn_timeout[%d]\n",__FUNCTION__,bcn_timeout);
	if (bcn_timeout > 0 || bcn_timeout < 20) {
		if ((err = wldev_iovar_setint(dev, "bcn_timeout", bcn_timeout))) {
			printf("%s: bcn_timeout setting error\n", __func__);		
		}
	}
	else {
		DHD_ERROR(("%s Incorrect bcn_timeout setting %d, ignored\n", __FUNCTION__, bcn_timeout));
	}
	return 0;

}
static int wl_android_scansuppress(struct net_device *net, char *command, int total_len)
{
	int enable = bcm_atoi(command + strlen(CMD_SCAN_SUPPRESS) + 1);
    printf("[Hugh] %s enter cmd=%s\n",__FUNCTION__,command +strlen(CMD_SCAN_SUPPRESS) + 1);

    wldev_set_scansuppress(net,enable);
    return 0;
}
extern s32 wl_cfg80211_scan_abort(struct net_device *ndev);
static int wl_android_scanabort(struct net_device *net, char *command, int total_len)
{
	wldev_set_scanabort(net);
    wl_cfg80211_scan_abort(net);
    return 0;
}
extern struct net_device *ap_net_dev;
extern int wldev_get_conap_ctrl_channel(struct net_device *dev,uint8 *ctrl_channel);

static int wl_android_get_conap_channel(struct net_device *net, char *buf, int len)
{

    int error = 0;
    uint8 conap_ctrl_channel = 0;
    char tmp[256];
    char *ptmp;

    memset(tmp, 0, sizeof(tmp));
    ptmp = tmp;

    if (!ap_net_dev ){
        printf("%s NULL ConAP netdev[%p]",__FUNCTION__,ap_net_dev);
        error = -1;
        ptmp += snprintf(ptmp, 80, "%d|", error);
        ptmp += snprintf(ptmp, 80, " ConAp interface not enable");
        goto exit;
    }
    
    if((error = wldev_get_conap_ctrl_channel(net,&conap_ctrl_channel))){
        printf("%s get Chanspec failed\n",__func__);
        ptmp += snprintf(ptmp, 80, "%d|", error);
        ptmp += snprintf(ptmp, 80, " conap got chanspec fail"); 
        goto exit;
        
    }else{
        printf("%s get hostap successful ctrl_channel[%d]\n",__func__,conap_ctrl_channel);

        ptmp += snprintf(ptmp, 80, "%d|", conap_ctrl_channel);
        ptmp += snprintf(ptmp, 80, " conap got conap_channel");
    }
exit:
	memset(buf, 0x0, len);
	memcpy(buf, tmp, len);
	return len;
}

#endif

static int wl_android_get_link_speed(struct net_device *net, char *command, int total_len)
{
	int link_speed;
	int bytes_written;
	int error;

	error = wldev_get_link_speed(net, &link_speed);
	if (error)
		return -1;

	
	link_speed = link_speed / 1000;
	bytes_written = snprintf(command, total_len, "LinkSpeed %d", link_speed);
	DHD_INFO(("%s: command result is %s\n", __FUNCTION__, command));
	return bytes_written;
}

#define TRAFFIC_SUPER_HIGH_WATER_MARK	2600*(3000/1000)
#define TRAFFIC_HIGH_WATER_MARK			2300*(3000/1000)
#define TRAFFIC_LOW_WATER_MARK			256*(3000/1000)
typedef enum traffic_ind {
	TRAFFIC_STATS_NORMAL = 0,
	TRAFFIC_STATS_HIGH,
	TRAFFIC_STATS_SUPER_HIGH,
} traffic_ind_t;


static int traffic_stats_flag = TRAFFIC_STATS_NORMAL;
static unsigned long current_traffic_count = 0;
static unsigned long last_traffic_count = 0;

#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif

#ifdef CONFIG_PERFLOCK
struct perf_lock *wlan_perf_lock;
#endif

struct msm_bus_scale_pdata *bus_scale_table = NULL;
uint32_t bus_perf_client = 0;

void wlan_lock_perf(void)
{
	unsigned ret;
#ifdef CONFIG_PERFLOCK
	if (!is_perf_lock_active(wlan_perf_lock))
		perf_lock(wlan_perf_lock);
#endif
	
	if (bus_perf_client) {
		ret = msm_bus_scale_client_update_request(
				bus_perf_client, 1);
		if (ret)
			printf("%s: Failed to vote for "
					"bus bandwidth %d\n", __func__, ret);
	}
}

void wlan_unlock_perf(void)
{
	unsigned ret;
#ifdef CONFIG_PERFLOCK
	if (is_perf_lock_active(wlan_perf_lock))
		perf_unlock(wlan_perf_lock);
#endif
	
	if (bus_perf_client) {
		ret = msm_bus_scale_client_update_request(
				bus_perf_client, 0);
		if (ret)
			printf("%s: Failed to devote "
					"for bus bw %d\n", __func__, ret);
	}
}

void wlan_init_perf(void)
{
#ifdef CONFIG_PERFLOCK
	wlan_perf_lock = perflock_acquire("bcmdhd");
	perf_lock_init(wlan_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "bcmdhd");
#endif
}

void wlan_deinit_perf(void)
{
#ifdef CONFIG_PERFLOCK
	if (is_perf_lock_active(wlan_perf_lock))
		perf_unlock(wlan_perf_lock);
	perflock_release("bcmdhd");
#endif
}

extern void wl_cfg80211_send_priv_event(struct net_device *dev, char *flag);

int multi_core_locked = 0;

void wlan_lock_multi_core(struct net_device *dev)
{
	dhd_pub_t *dhdp = bcmsdh_get_drvdata();

	wl_cfg80211_send_priv_event(dev, "PERF_LOCK");
	multi_core_locked = 1;
	if (dhdp) {
		dhd_sched_dpc(dhdp);
	} else {
		printf("%s: dhdp is null", __func__);
	}
}

void wlan_unlock_multi_core(struct net_device *dev)
{
	dhd_pub_t *dhdp = bcmsdh_get_drvdata();

	multi_core_locked = 0;
	if (dhdp) {
		dhd_sched_dpc(dhdp);
	} else {
		printf("%s: dhdp is null", __func__);
	}
	wl_cfg80211_send_priv_event(dev, "PERF_UNLOCK");
}

void wl_android_traffic_monitor(struct net_device *dev)
{
	unsigned long rx_packets_count = 0;
	unsigned long tx_packets_count = 0;
	unsigned long traffic_diff = 0;

	
	dhd_get_txrx_stats(dev, &rx_packets_count, &tx_packets_count);
	current_traffic_count = rx_packets_count + tx_packets_count;

	if (current_traffic_count >= last_traffic_count) {
		traffic_diff = current_traffic_count - last_traffic_count;
        switch (traffic_stats_flag) {
        case TRAFFIC_STATS_NORMAL:
			if (traffic_diff > TRAFFIC_HIGH_WATER_MARK) {
				traffic_stats_flag = TRAFFIC_STATS_HIGH;
				wlan_lock_perf();
				printf("lock cpu here, traffic-count=%ld\n", traffic_diff / 3);
                if (traffic_diff > TRAFFIC_SUPER_HIGH_WATER_MARK) {
                    traffic_stats_flag = TRAFFIC_STATS_SUPER_HIGH;
                    wlan_lock_multi_core(dev);
                    printf("lock 2nd cpu here, traffic-count=%ld\n", traffic_diff / 3);
                }
			}
            break;
        case TRAFFIC_STATS_HIGH:
            if (traffic_diff > TRAFFIC_SUPER_HIGH_WATER_MARK) {
                traffic_stats_flag = TRAFFIC_STATS_SUPER_HIGH;
                wlan_lock_multi_core(dev);
				printf("lock 2nd cpu here, traffic-count=%ld\n", traffic_diff / 3);
            }
            else if (traffic_diff < TRAFFIC_LOW_WATER_MARK) {
				traffic_stats_flag = TRAFFIC_STATS_NORMAL;
				wlan_unlock_perf();
				printf("unlock cpu here, traffic-count=%ld\n", traffic_diff / 3);
			}
            break;
        case TRAFFIC_STATS_SUPER_HIGH:
			if (traffic_diff < TRAFFIC_SUPER_HIGH_WATER_MARK) {
                traffic_stats_flag = TRAFFIC_STATS_HIGH;
                wlan_unlock_multi_core(dev);
				printf("unlock 2nd cpu here, traffic-count=%ld\n", traffic_diff / 3);
                if (traffic_diff < TRAFFIC_LOW_WATER_MARK) {
                    traffic_stats_flag = TRAFFIC_STATS_NORMAL;
                    wlan_unlock_perf();
                    printf("unlock cpu here, traffic-count=%ld\n", traffic_diff / 3);
                }
            }
            break;
        default:
            break;
        }
	}
	last_traffic_count = current_traffic_count;
	
}

static int wl_android_get_rssi(struct net_device *net, char *command, int total_len)
{
	wlc_ssid_t ssid = {0};
	int rssi;
	int bytes_written = 0;
	int error;

	error = wldev_get_rssi(net, &rssi);
	if (error)
		return -1;

	error = wldev_get_ssid(net, &ssid);
	if (error)
		return -1;
	if ((ssid.SSID_len == 0) || (ssid.SSID_len > DOT11_MAX_SSID_LEN)) {
		DHD_TRACE(("%s: wldev_get_ssid failed\n", __FUNCTION__));
	} else {
		memcpy(command, ssid.SSID, ssid.SSID_len);
		bytes_written = ssid.SSID_len;
	}
	bytes_written += snprintf(&command[bytes_written], total_len, " rssi %d", rssi);
	DHD_INFO(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));
	wl_android_traffic_monitor(net);
	return bytes_written;
}

static int wl_android_set_suspendopt(struct net_device *dev, char *command, int total_len)
{
	int suspend_flag;
	int ret_now;
	int ret = 0;

#ifdef CUSTOMER_HW4
	if (!dhd_download_fw_on_driverload) {
#endif 
		suspend_flag = *(command + strlen(CMD_SETSUSPENDOPT) + 1) - '0';

		if (suspend_flag != 0)
			suspend_flag = 1;
		ret_now = net_os_set_suspend_disable(dev, suspend_flag);

		if (ret_now != suspend_flag) {
			if (!(ret = net_os_set_suspend(dev, ret_now, 1)))
				DHD_INFO(("%s: Suspend Flag %d -> %d\n",
					__FUNCTION__, ret_now, suspend_flag));
			else
				DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
		}
#ifdef CUSTOMER_HW4
	}
#endif 
	return ret;
}

extern int dhdcdc_wifiLock;

static int wl_android_set_suspendmode(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;

#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(DHD_USE_EARLYSUSPEND)
	int suspend_flag;

	suspend_flag = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';
	if (suspend_flag != 0)
		suspend_flag = 1;

    
    if (suspend_flag == 1)
        dhdcdc_wifiLock = 0;
    else if (suspend_flag == 0)
        dhdcdc_wifiLock = 1;
    

	if (!(ret = net_os_set_suspend(dev, suspend_flag, 0)))
		DHD_INFO(("%s: Suspend Mode %d\n", __FUNCTION__, suspend_flag));
	else
		DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
#endif

	return ret;
}

static int wl_android_get_band(struct net_device *dev, char *command, int total_len)
{
	uint band;
	int bytes_written;
	int error;

	error = wldev_get_band(dev, &band);
	if (error)
		return -1;
	bytes_written = snprintf(command, total_len, "Band %d", band);
	return bytes_written;
}

#if defined(HTC_TX_TRACKING)
static uint old_tx_stat_chk;
static uint old_tx_stat_chk_prd;
static uint old_tx_stat_chk_ratio;
static uint old_tx_stat_chk_num;
static int wl_android_set_tx_tracking(struct net_device *dev, char *command, int total_len)
{
        int bytes_written = 0;
        char iovbuf[32];
        
        uint tx_stat_chk = 0; 
        uint tx_stat_chk_prd = 5; 
        uint tx_stat_chk_ratio = 8; 
        uint tx_stat_chk_num = 5; 

        sscanf(command + strlen(CMD_TX_TRACKING) + 1, "%u %u %u %u", &tx_stat_chk, &tx_stat_chk_prd, &tx_stat_chk_ratio, &tx_stat_chk_num);
                printf("wl_android_set_tx_tracking command=%s", command);

	if (tx_stat_chk_num != old_tx_stat_chk_num) {
       	bcm_mkiovar("tx_stat_chk_num", (char *)&tx_stat_chk_num, 4, iovbuf, sizeof(iovbuf));
       	wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1);
		old_tx_stat_chk_num = tx_stat_chk_num;
	} else 
		DHD_INFO(("tx_stat_chk_num duplicate, ignore!\n"));

	if (tx_stat_chk != old_tx_stat_chk) {
		bcm_mkiovar("tx_stat_chk", (char *)&tx_stat_chk, 4, iovbuf, sizeof(iovbuf));
		wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1);
		old_tx_stat_chk = tx_stat_chk;
	} else 
		DHD_INFO(("tx_stat_chk duplicate, ignore!\n"));

	if (tx_stat_chk_ratio != old_tx_stat_chk_ratio) {
		bcm_mkiovar("tx_stat_chk_ratio", (char *)&tx_stat_chk_ratio, 4, iovbuf, sizeof(iovbuf));
		wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1);
		old_tx_stat_chk_ratio = tx_stat_chk_ratio;
	} else 
		DHD_INFO(("tx_stat_chk_ratio duplicate, ignore!\n"));

	if (tx_stat_chk_prd != old_tx_stat_chk_prd) {
		bcm_mkiovar("tx_stat_chk_prd", (char *)&tx_stat_chk_prd, 4, iovbuf, sizeof(iovbuf));
		wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1);
		old_tx_stat_chk_prd = tx_stat_chk_prd;
	} else 
		DHD_INFO(("tx_stat_chk_prd duplicate, ignore!\n"));


        return bytes_written;
}
#endif

#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)
#ifdef ROAM_API
int wl_android_set_roam_trigger(
	struct net_device *dev, char* command, int total_len)
{
	int roam_trigger[2];

	sscanf(command, "%*s %10d", &roam_trigger[0]);
	roam_trigger[1] = WLC_BAND_ALL;

	return wldev_ioctl(dev, WLC_SET_ROAM_TRIGGER, roam_trigger,
		sizeof(roam_trigger), 1);
}

static int wl_android_get_roam_trigger(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_trigger[2] = {0, 0};

	roam_trigger[1] = WLC_BAND_2G;
	if (wldev_ioctl(dev, WLC_GET_ROAM_TRIGGER, roam_trigger,
		sizeof(roam_trigger), 0)) {
		roam_trigger[1] = WLC_BAND_5G;
		if (wldev_ioctl(dev, WLC_GET_ROAM_TRIGGER, roam_trigger,
			sizeof(roam_trigger), 0))
			return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMTRIGGER_GET, roam_trigger[0]);

	return bytes_written;
}

int wl_android_set_roam_delta(
	struct net_device *dev, char* command, int total_len)
{
	int roam_delta[2];

	sscanf(command, "%*s %10d", &roam_delta[0]);
	roam_delta[1] = WLC_BAND_ALL;

	return wldev_ioctl(dev, WLC_SET_ROAM_DELTA, roam_delta,
		sizeof(roam_delta), 1);
}

static int wl_android_get_roam_delta(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_delta[2] = {0, 0};

	roam_delta[1] = WLC_BAND_2G;
	if (wldev_ioctl(dev, WLC_GET_ROAM_DELTA, roam_delta,
		sizeof(roam_delta), 0)) {
		roam_delta[1] = WLC_BAND_5G;
		if (wldev_ioctl(dev, WLC_GET_ROAM_DELTA, roam_delta,
			sizeof(roam_delta), 0))
			return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMDELTA_GET, roam_delta[0]);

	return bytes_written;
}

int wl_android_set_roam_scan_period(
	struct net_device *dev, char* command, int total_len)
{
	int roam_scan_period = 0;

	sscanf(command, "%*s %10d", &roam_scan_period);
	return wldev_ioctl(dev, WLC_SET_ROAM_SCAN_PERIOD, &roam_scan_period,
		sizeof(roam_scan_period), 1);
}

static int wl_android_get_roam_scan_period(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_scan_period = 0;

	if (wldev_ioctl(dev, WLC_GET_ROAM_SCAN_PERIOD, &roam_scan_period,
		sizeof(roam_scan_period), 0))
		return -1;

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMSCANPERIOD_GET, roam_scan_period);

	return bytes_written;
}

int wl_android_set_full_roam_scan_period(
	struct net_device *dev, char* command, int total_len)
{
	int error = 0;
	int full_roam_scan_period = 0;
	char smbuf[WLC_IOCTL_SMLEN];

	sscanf(command+sizeof("SETFULLROAMSCANPERIOD"), "%d", &full_roam_scan_period);
	WL_TRACE(("%s: fullroamperiod = %d\n", __func__, full_roam_scan_period));

	error = wldev_iovar_setbuf(dev, "fullroamperiod", &full_roam_scan_period,
		sizeof(full_roam_scan_period), smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set full roam scan period, error = %d\n", error));
	}

	return error;
}

static int wl_android_get_full_roam_scan_period(
	struct net_device *dev, char *command, int total_len)
{
	int error;
	int bytes_written;
	int full_roam_scan_period = 0;

	error = wldev_iovar_getint(dev, "fullroamperiod", &full_roam_scan_period);

	if (error) {
		DHD_ERROR(("%s: get full roam scan period failed code %d\n",
			__func__, error));
		return -1;
	} else {
		DHD_INFO(("%s: get full roam scan period %d\n", __func__, full_roam_scan_period));
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_FULLROAMSCANPERIOD_GET, full_roam_scan_period);

	return bytes_written;
}

int wl_android_set_country_rev(
	struct net_device *dev, char* command, int total_len)
{
	int error = 0;
	wl_country_t cspec = {{0}, 0, {0} };
	char country_code[WLC_CNTRY_BUF_SZ];
	char smbuf[WLC_IOCTL_SMLEN];
	int rev = 0;

	memset(country_code, 0, sizeof(country_code));
	sscanf(command+sizeof("SETCOUNTRYREV"), "%10s %10d", country_code, &rev);
	WL_TRACE(("%s: country_code = %s, rev = %d\n", __FUNCTION__,
		country_code, rev));

	memcpy(cspec.country_abbrev, country_code, sizeof(country_code));
	memcpy(cspec.ccode, country_code, sizeof(country_code));
	cspec.rev = rev;

	error = wldev_iovar_setbuf(dev, "country", (char *)&cspec,
		sizeof(cspec), smbuf, sizeof(smbuf), NULL);

	if (error) {
		DHD_ERROR(("%s: set country '%s/%d' failed code %d\n",
			__FUNCTION__, cspec.ccode, cspec.rev, error));
	} else {
		dhd_bus_country_set(dev, &cspec);
		DHD_INFO(("%s: set country '%s/%d'\n",
			__FUNCTION__, cspec.ccode, cspec.rev));
	}

	return error;
}

static int wl_android_get_country_rev(
	struct net_device *dev, char *command, int total_len)
{
	int error;
	int bytes_written;
	char smbuf[WLC_IOCTL_SMLEN];
	wl_country_t cspec;

	error = wldev_iovar_getbuf(dev, "country", NULL, 0, smbuf,
		sizeof(smbuf), NULL);

	if (error) {
		DHD_ERROR(("%s: get country failed code %d\n",
			__FUNCTION__, error));
		return -1;
	} else {
		memcpy(&cspec, smbuf, sizeof(cspec));
		DHD_INFO(("%s: get country '%c%c %d'\n",
			__FUNCTION__, cspec.ccode[0], cspec.ccode[1], cspec.rev));
	}

	bytes_written = snprintf(command, total_len, "%s %c%c %d",
		CMD_COUNTRYREV_GET, cspec.ccode[0], cspec.ccode[1], cspec.rev);

	return bytes_written;
}
#endif 

static int wl_android_get_dtim_skip(struct net_device *dev, char *command, int total_len)
{
	char iovbuf[32];
	int bytes_written;
	int error = 0;

	memset(iovbuf, 0, sizeof(iovbuf));
	strcpy(iovbuf, "bcn_li_dtim");
	
	if ((error = wldev_ioctl(dev, WLC_GET_VAR, &iovbuf, sizeof(iovbuf), 0)) >= 0) {
		bytes_written = snprintf(command, total_len, "Dtim_skip %d", iovbuf[0]);
		DHD_INFO(("%s: get dtim_skip = %d\n", __FUNCTION__, iovbuf[0]));
		return bytes_written;
	}
	else  {
		DHD_ERROR(("%s: get dtim_skip failed code %d\n", __FUNCTION__, error));
		return -1;
	}
}

static int wl_android_set_dtim_skip(struct net_device *dev, char *command, int total_len)
{
	char iovbuf[32];
	int bytes_written;
	int error = -1;
	int bcn_li_dtim;

	bcn_li_dtim = htod32((uint)*(command + strlen(CMD_DTIM_SKIP_SET) + 1) - '0');
	
	if ((bcn_li_dtim >= 0) || ((bcn_li_dtim <= 5))) {
		memset(iovbuf, 0, sizeof(iovbuf));
		bcm_mkiovar("bcn_li_dtim", (char *)&bcn_li_dtim, 4, iovbuf, sizeof(iovbuf));
	
		if ((error = wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1)) >= 0) {
			bytes_written = snprintf(command, total_len, "Dtim_skip %d", iovbuf[0]);
			DHD_INFO(("%s: set dtim_skip = %d\n", __FUNCTION__, iovbuf[0]));
			return bytes_written;
		}
		else {
			DHD_ERROR(("%s: set dtim_skip failed code %d\n", __FUNCTION__, error));
		}
	}
	else {
		DHD_ERROR(("%s Incorrect dtim_skip setting %d, ignored\n", __FUNCTION__, bcn_li_dtim));
	}
	return -1;
}

static int wl_android_set_txpower(struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int error = -1;
	int txpower = -1;

	txpower = bcm_atoi(command + strlen(CMD_TXPOWER_SET) + 1);
	
	if ((txpower >= 0) || ((txpower <= 127))) {
		txpower |= WL_TXPWR_OVERRIDE;
		txpower = htod32(txpower);
		if ((error = wldev_iovar_setint(dev, "qtxpower", txpower)) >= 0)  {
			bytes_written = snprintf(command, total_len, "OK");
        	        DHD_INFO(("%s: set TXpower 0x%X is OK\n", __FUNCTION__, txpower));
			return bytes_written;
		}
		else {
                	DHD_ERROR(("%s: set tx power failed, txpower=%d\n", __FUNCTION__, txpower));
		}
        } else {
                DHD_ERROR(("%s: Unsupported tx power value, txpower=%d\n", __FUNCTION__, txpower));
        }

	bytes_written = snprintf(command, total_len, "FAIL");
	return -1;
}

static int wl_android_set_ap_txpower(struct net_device *dev, char *command, int total_len)
{
	int ap_txpower = 0, ap_txpower_orig = 0;
	char iovbuf[32];

	ap_txpower = bcm_atoi(command + strlen(CMD_AP_TXPOWER_SET) + 1);
	if (ap_txpower == 0) {
		
		memset(iovbuf, 0, sizeof(iovbuf));
		bcm_mkiovar("qtxpower", (char *)&ap_txpower_orig, 4, iovbuf, sizeof(iovbuf));
		wldev_ioctl(dev, WLC_GET_VAR, &iovbuf, sizeof(iovbuf), 0);
		DHD_ERROR(("default tx power is %d\n", ap_txpower_orig));
		ap_txpower_orig |= WL_TXPWR_OVERRIDE;
	}

	if (ap_txpower == 99) {
		
		ap_txpower = ap_txpower_orig;
	} else {
		ap_txpower |= WL_TXPWR_OVERRIDE;
	}

	DHD_ERROR(("set tx power to 0x%x\n", ap_txpower));
	bcm_mkiovar("qtxpower", (char *)&ap_txpower, 4, iovbuf, sizeof(iovbuf));
	wldev_ioctl(dev, WLC_SET_VAR, &iovbuf, sizeof(iovbuf), 1);

	return 0;
}

static int wl_android_set_scan_minrssi(struct net_device *dev, char *command, int total_len)
{
	int minrssi = 0;
	int err = 0;
	int bytes_written;

	DHD_TRACE(("%s\n", __FUNCTION__));

	minrssi = bcm_strtoul((char *)(command + strlen("SCAN_MINRSSI") + 1), NULL, 10);

	err = wldev_iovar_setint(dev, "scanresults_minrssi", minrssi);

	if (err) {
		DHD_ERROR(("set scan_minrssi fail!\n"));
		bytes_written = snprintf(command, total_len, "FAIL");
	} else
		bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;
}

#ifdef WLC_E_RSSI_LOW 
static int wl_android_low_rssi_set(struct net_device *dev, char *command, int total_len)
{
	int low_rssi_trigger;
	int low_rssi_duration;
	int trigger_offset;
	int duration_offset;
	char tran_buf[16] = {0};
	char *pp;
	int err = 0;
	int bytes_written;
	
	DHD_TRACE(("%s\n", __FUNCTION__));

	trigger_offset = strcspn(command, " ");
	pp = command + trigger_offset + 1;
	duration_offset = strcspn(pp, " ");

	memcpy(tran_buf, pp, duration_offset);
	low_rssi_trigger = bcm_strtoul(tran_buf, NULL, 10);
	err |= wldev_iovar_setint(dev, "low_rssi_trigger", low_rssi_trigger);

	memset(tran_buf, 0, 16);
	pp = command + trigger_offset + duration_offset + 2;
	memcpy(tran_buf, pp, strlen(command) - (trigger_offset+duration_offset+1));
	low_rssi_duration = bcm_strtoul(tran_buf, NULL, 10);
	err |= wldev_iovar_setint(dev, "low_rssi_duration", low_rssi_duration);

	DHD_TRACE(("set low rssi trigger %d, duration %d\n", low_rssi_trigger, low_rssi_duration));

	if (err) {
		DHD_ERROR(("set low rssi ind fail!\n"));
		bytes_written = snprintf(command, total_len, "FAIL");
	} else
		bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;
}
#endif  

static struct mac_list_set android_mac_list_buf;
static struct mflist android_ap_black_list;
static int android_ap_macmode = MACLIST_MODE_DISABLED;
int wl_android_black_list_match(char *mac)
{
	int i;

	if (android_ap_macmode) {
		for (i = 0; i < android_ap_black_list.count; i++) {
			if (!bcmp(mac, &android_ap_black_list.ea[i],
				sizeof(struct ether_addr))) {
				DHD_ERROR(("mac in black list, ignore it\n"));
				break;
			}
		}

		if (i == android_ap_black_list.count)
			return 1;
	}

	return 0;
}

void wl_cfg80211_send_priv_event(struct net_device *dev, char *flag);
static int assoc_count_buff = 0;
extern int sta_event_sent;

static int
wl_android_get_assoc_sta_list(struct net_device *dev, char *buf, int len)
{
	struct maclist *maclist = (struct maclist *) buf;
	int ret,i;
	char mac_lst[256];
	char *p_mac_str;

	bcm_mdelay(500);
	maclist->count = 10;
	ret = wldev_ioctl(dev, WLC_GET_ASSOCLIST, buf, len, 0);

	memset(mac_lst, 0, sizeof(mac_lst));
	p_mac_str = mac_lst;


	p_mac_str += snprintf(p_mac_str, 80, "%d|", maclist->count);

	for (i = 0; i < maclist->count; i++) {
		struct ether_addr *id = &maclist->ea[i];


		p_mac_str += snprintf(p_mac_str, 80, "%02X:%02X:%02X:%02X:%02X:%02X,",
			id->octet[0], id->octet[1], id->octet[2],
			id->octet[3], id->octet[4], id->octet[5]);
	}

	if (ret != 0) {
		DHD_ERROR(("get assoc count fail\n"));
		maclist->count = 0;
	} else
		printf("get assoc count %d, len %d\n", maclist->count, len);

    
    if (!sta_event_sent && assoc_count_buff && (assoc_count_buff != maclist->count)) {
        wl_cfg80211_send_priv_event(dev, "STA_LEAVE");
    }

    assoc_count_buff = maclist->count;
    sta_event_sent = 0;
    


	memset(buf, 0x0, len);
	memcpy(buf, mac_lst, len);
	return len;
}

#ifdef APSTA_CONCURRENT
extern struct wl_priv *wlcfg_drv_priv;
int old_dongle = 0;
#endif

static int wl_android_set_ap_mac_list(struct net_device *dev, void *buf)
{
	struct mac_list_set *mac_list_set = (struct mac_list_set *)buf;
	struct maclist *white_maclist = (struct maclist *)&mac_list_set->white_list;
	struct maclist *black_maclist = (struct maclist *)&mac_list_set->black_list;
	int mac_mode = mac_list_set->mode;
	int length;
	int i;

#ifdef APSTA_CONCURRENT
    struct wl_priv *wl;
    wl = wlcfg_drv_priv;
    printf("%s in\n", __func__);

    if(wl && wl->apsta_concurrent){
        printf("APSTA CONCURRENT SET SKIP set %s \n",__FUNCTION__);
        return 0;
    }
#else
    printf("%s in\n", __func__);
#endif

	if (white_maclist->count > 16 || black_maclist->count > 16) {
		DHD_TRACE(("invalid count white: %d black: %d\n", white_maclist->count, black_maclist->count));
		return 0;
	}

	if (buf != (char *)&android_mac_list_buf) {
		DHD_TRACE(("Backup the mac list\n"));
		memcpy((char *)&android_mac_list_buf, buf, sizeof(struct mac_list_set));
	} else
		DHD_TRACE(("recover, don't back up mac list\n"));

	android_ap_macmode = mac_mode;
	if (mac_mode == MACLIST_MODE_DISABLED) {

		bzero(&android_ap_black_list, sizeof(struct mflist));

		wldev_ioctl(dev, WLC_SET_MACMODE, &mac_mode, sizeof(mac_mode), 1);
	} else {
		scb_val_t scbval;
		char mac_buf[256] = {0};
		struct maclist *assoc_maclist = (struct maclist *) mac_buf;

		mac_mode = MACLIST_MODE_ALLOW;

		wldev_ioctl(dev, WLC_SET_MACMODE, &mac_mode, sizeof(mac_mode), 1);

		length = sizeof(white_maclist->count)+white_maclist->count*ETHER_ADDR_LEN;
		wldev_ioctl(dev, WLC_SET_MACLIST, white_maclist, length, 1);
		printf("White List, length %d:\n", length);
		for (i = 0; i < white_maclist->count; i++)
			printf("mac %d: %02X:%02X:%02X:%02X:%02X:%02X\n",
				i, white_maclist->ea[i].octet[0], white_maclist->ea[i].octet[1], white_maclist->ea[i].octet[2],
				white_maclist->ea[i].octet[3], white_maclist->ea[i].octet[4], white_maclist->ea[i].octet[5]);

		
		bcopy(black_maclist, &android_ap_black_list, sizeof(android_ap_black_list));

		printf("Black List, size %d:\n", sizeof(android_ap_black_list));
		for (i = 0; i < android_ap_black_list.count; i++)
			printf("mac %d: %02X:%02X:%02X:%02X:%02X:%02X\n",
				i, android_ap_black_list.ea[i].octet[0], android_ap_black_list.ea[i].octet[1], android_ap_black_list.ea[i].octet[2],
				android_ap_black_list.ea[i].octet[3], android_ap_black_list.ea[i].octet[4], android_ap_black_list.ea[i].octet[5]);

		
		assoc_maclist->count = 8;
		wldev_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, 256, 0);
		if (assoc_maclist->count) {
			int j;
			for (i = 0; i < assoc_maclist->count; i++) {
				for (j = 0; j < white_maclist->count; j++) {
					if (!bcmp(&assoc_maclist->ea[i], &white_maclist->ea[j], ETHER_ADDR_LEN)) {
						DHD_TRACE(("match allow, let it be\n"));
						break;
					}
				}
				if (j == white_maclist->count) {
						DHD_TRACE(("match black, deauth it\n"));
						scbval.val = htod32(1);
						bcopy(&assoc_maclist->ea[i], &scbval.ea, ETHER_ADDR_LEN);
						wldev_ioctl(dev, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scbval,
							sizeof(scb_val_t), 1);
				}
			}
		}
	}
	return 0;
}
static int wl_android_del_pfn(struct net_device *dev, char *command, int total_len)
{
	char ssid[33];
	int ssid_offset;
	int ssid_size;
	int bytes_written;

	DHD_TRACE(("%s\n", __FUNCTION__));

	memset(ssid, 0, sizeof(ssid));

	ssid_offset = strcspn(command, " ");
	ssid_size = strlen(command) - ssid_offset;

	if (ssid_offset == 0) {
		DHD_ERROR(("%s, no ssid specified\n", __FUNCTION__));
		return 0;
	}

	strncpy(ssid, command + ssid_offset+1,
			MIN(ssid_size, sizeof(ssid)));

	DHD_ERROR(("%s: remove ssid: %s\n", __FUNCTION__, ssid));
	dhd_del_pfn_ssid(ssid, ssid_size);

	bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;

}
#endif 
#ifdef WL_CFG80211
static int wl_android_get_mac_addr(struct net_device *ndev, char *command, int total_len)
{
	int ret;
	int bytes_written = 0;
	struct ether_addr id;

	ret = wl_cfg80211_get_mac_addr(ndev, &id);
	if (ret)
		return 0;
	bytes_written = snprintf(command, total_len, "Macaddr = %02X:%02X:%02X:%02X:%02X:%02X\n",
		id.octet[0], id.octet[1], id.octet[2],
		id.octet[3], id.octet[4], id.octet[5]);
	return bytes_written;
}
#endif

#ifdef BCMCCX
static int wl_android_get_cckm_rn(struct net_device *dev, char *command)
{
	int error, rn;

	WL_TRACE(("%s:wl_android_get_cckm_rn\n", dev->name));
	
	error = wldev_iovar_getint(dev, "cckm_rn", &rn);
	if (unlikely(error)) {
		WL_ERR(("wl_android_get_cckm_rn error (%d)\n", error));
		return -1;
	}
	
	memcpy(command, &rn, sizeof(int));

	return sizeof(int);
}

static int wl_android_set_cckm_krk(struct net_device *dev, char *command)
{
	int error;
	unsigned char key[16];

	static char iovar_buf[WLC_IOCTL_MEDLEN];

	WL_TRACE(("%s: wl_iw_set_cckm_krk\n", dev->name));

	memset(iovar_buf, 0, sizeof(iovar_buf));
	memcpy(key, command+strlen("set cckm_krk")+1, 16);

	error = wldev_iovar_setbuf(dev, "cckm_krk",key, sizeof(key), iovar_buf, WLC_IOCTL_MEDLEN,NULL);
	if (unlikely(error))
	{
		WL_ERR((" cckm_krk set error (%d)\n", error));
		return -1;
	}
	return 0;
}

static int wl_android_get_assoc_res_ies(struct net_device *dev, char *command)
{
	int error;
	u8 buf[WL_ASSOC_INFO_MAX];
	wl_assoc_info_t assoc_info;
	u32 resp_ies_len = 0;
	int bytes_written = 0;

	WL_TRACE(("%s: wl_iw_get_assoc_res_ies\n", dev->name));

	error = wldev_iovar_getbuf(dev, "assoc_info", NULL, 0, buf,WL_ASSOC_INFO_MAX, NULL);
	if (unlikely(error)) {
		WL_ERR(("could not get assoc info (%d)\n", error));
		return -1;
	}
	
	memcpy(&assoc_info, buf, sizeof(wl_assoc_info_t));
	assoc_info.req_len = htod32(assoc_info.req_len);
	assoc_info.resp_len = htod32(assoc_info.resp_len);
	assoc_info.flags = htod32(assoc_info.flags);

	if (assoc_info.resp_len) {
		resp_ies_len = assoc_info.resp_len - sizeof(struct dot11_assoc_resp);
	}

	
	memcpy(command, &resp_ies_len, sizeof(u32));
	bytes_written= sizeof(u32);

	
	if (resp_ies_len) {
		error = wldev_iovar_getbuf(dev, "assoc_resp_ies", NULL, 0, buf,WL_ASSOC_INFO_MAX, NULL);
		if (unlikely(error)) {
			WL_ERR(("could not get assoc resp_ies (%d)\n", error));
			return -1;
		}

		memcpy(command+sizeof(u32), buf, resp_ies_len);
		bytes_written += resp_ies_len;
	}
	return bytes_written;
}

#endif 

extern char project_type[33];
static int wl_android_wifi_call = 0;
static struct mutex wl_wificall_mutex;
static struct mutex wl_wifionoff_mutex;
char wl_abdroid_gatewaybuf[8+1]; 
#ifdef BCM4329_LOW_POWER
extern int LowPowerMode;

extern bool hasDLNA;
extern bool allowMulticast;
extern int dhd_set_keepalive(int value);
#endif

static int active_level = -80;
static int active_period = 20000; 
static int wl_android_active_expired = 0;
struct timer_list *wl_android_active_timer = NULL;
static int screen_off = 0;

static void wl_android_act_time_expire(void)
{
	struct timer_list **timer;
	timer = &wl_android_active_timer;

	if (*timer) {
		WL_TRACE(("ac timer expired\n"));
		del_timer_sync(*timer);
		kfree(*timer);
		*timer = NULL;
		if (screen_off)
			return;
		wl_android_active_expired = 1;
	}
	return;
}

static void wl_android_deactive(void)
{
	struct timer_list **timer;
	timer = &wl_android_active_timer;

	if (*timer) {
		WL_TRACE(("previous ac exist\n"));
		del_timer_sync(*timer);
		kfree(*timer);
		*timer = NULL;
	}
	wl_android_active_expired = 0;
	WL_TRACE(("wl_android_deactive\n"));
	return;
}

void wl_android_set_active_level(int level)
{
	active_level = level;
	printf("set active level to %d\n", active_level);
	return;
}

void wl_android_set_active_period(int period)
{
	active_period = period;
	printf("set active_period to %d\n", active_period);
	return;
}

int wl_android_get_active_level(void)
{
	return active_level;
}

int wl_android_get_active_period(void)
{
	return active_period;
}

void wl_android_set_screen_off(int off)
{
	screen_off = off;
	printf("wl_android_set_screen_off %d\n", screen_off);
	if (screen_off)
		wl_android_deactive();

	return;
}

static int wl_android_set_power_mode(struct net_device *dev, char *command, int total_len)
{

#if 1
	int bytes_written;
	int pm_val;
    int force_pm = 3;
    int mpc = 0;

	pm_val = bcm_atoi(command + strlen(CMD_POWER_MODE_SET) + 1);

		switch (pm_val) {
		
		case 0:
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_ANDROID_NORMAL);
			dhdhtc_update_wifi_power_mode(screen_off);
            mpc = 1;
            wldev_iovar_setint(dev, "mpc", mpc);
			wl_cfg80211_set_btcoex_done(dev);
			break;
		case 1:
			dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_ANDROID_NORMAL);
			dhdhtc_update_wifi_power_mode(screen_off);
            mpc = 0;
            wldev_iovar_setint(dev, "mpc", mpc);
            wldev_ioctl(dev, WLC_SET_PM, &force_pm, sizeof(force_pm), 1);
			break;
		 
		case 10:
			wl_android_deactive();
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_BROWSER_LOAD_PAGE);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		case 11:
			if (!screen_off) {
				struct timer_list **timer;
				timer = &wl_android_active_timer;

				if (*timer) {
					mod_timer(*timer, jiffies + active_period * HZ / 1000);
					
				} else {
					*timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
					if (*timer) {
						(*timer)->function = (void *)wl_android_act_time_expire;
						init_timer(*timer);
						(*timer)->expires = jiffies + active_period * HZ / 1000;
						add_timer(*timer);
						
					}
				}
				dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_BROWSER_LOAD_PAGE);
				dhdhtc_update_wifi_power_mode(screen_off);
			}
			break;
		 
		case 20:
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_USER_CONFIG);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		case 21:
			dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_USER_CONFIG);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		 
		case 30:
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_FOTA_DOWNLOADING);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		case 31:
			dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_FOTA_DOWNLOADING);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;

		case 87: 
			printf("wifilock release\n");
			dhdcdc_wifiLock = 0;
			dhdhtc_update_wifi_power_mode(screen_off);
			dhdhtc_update_dtim_listen_interval(screen_off);
			break;

		case 88: 
			printf("wifilock accquire\n");
			dhdcdc_wifiLock = 1;
			dhdhtc_update_wifi_power_mode(screen_off);
			dhdhtc_update_dtim_listen_interval(screen_off);
			break;

		case 99: 
			dhdcdc_power_active_while_plugin = !dhdcdc_power_active_while_plugin;
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
#if 0
		
		case 30:
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_WIFI_PHONE);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		case 31:
			dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_WIFI_PHONE);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
#endif
		default:
			DHD_ERROR(("%s: not support mode: %d\n", __func__, pm_val));
			break;

	}

#else

	static int  pm = PM_FAST;
	int  pm_local = PM_OFF;
	char powermode_val = 0;
	int bytes_written;

	DHD_INFO(("%s: DHCP session cmd:%s\n", __func__, command));

	strncpy((char *)&powermode_val, command + strlen("POWERMODE") + 1, 1);

	if (strnicmp((char *)&powermode_val, "1", strlen("1")) == 0) {

		WL_TRACE(("%s: DHCP session starts\n", __func__));
		wldev_ioctl(dev, WLC_GET_PM, &pm, sizeof(pm), 0);
		wldev_ioctl(dev, WLC_SET_PM, &pm_local, sizeof(pm_local), 1);

		net_os_set_packet_filter(dev, 0);

#ifdef COEX_DHCP
#if 0
		g_bt->ts_dhcp_start = JF2MS;
		g_bt->dhcp_done = FALSE;
		WL_TRACE_COEX(("%s: DHCP start, pm:%d changed to pm:%d\n",
			__func__, pm, pm_local));
#endif
#endif
	} else if (strnicmp((char *)&powermode_val, "0", strlen("0")) == 0) {

		wldev_ioctl(dev, WLC_SET_PM, &pm, sizeof(pm), 1);

		net_os_set_packet_filter(dev, 1);

#ifdef COEX_DHCP
#if 0
		g_bt->dhcp_done = TRUE;
		g_bt->ts_dhcp_ok = JF2MS;
		WL_TRACE_COEX(("%s: DHCP done for:%d ms, restored pm:%d\n",
			__func__, (g_bt->ts_dhcp_ok - g_bt->ts_dhcp_start), pm));
#endif
#endif
	} else {
		DHD_ERROR(("%s Unkwown yet power setting, ignored\n",
			__func__));
	}
#endif
	bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;

}

static int wl_android_get_power_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int pm_local = PM_FAST;
	int bytes_written;

	error = wldev_ioctl(dev, WLC_GET_PM, &pm_local, sizeof(pm_local), 0);
	if (!error) {
		DHD_TRACE(("%s: Powermode = %d\n", __func__, pm_local));
		if (pm_local == PM_OFF)
			pm_local = 1;
		else
			pm_local = 0;
		bytes_written = snprintf(command, total_len, "powermode = %d\n", pm_local);

	} else {
		DHD_TRACE(("%s: Error = %d\n", __func__, error));
		bytes_written = snprintf(command, total_len, "FAIL\n");
	}
	return bytes_written;
}
static int wl_android_get_wifilock(struct net_device *ndev, char *command, int total_len)
{
	int bytes_written = 0;

	bytes_written = snprintf(command, total_len, "%d", dhdcdc_wifiLock);
	printf("dhdcdc_wifiLock: %s\n", command);

	return bytes_written;
}

int wl_android_is_during_wifi_call(void)
{
	return wl_android_wifi_call;
}

static int wl_android_set_wificall(struct net_device *ndev, char *command, int total_len)
{
	int bytes_written = 0;
	char *s;
	int set_val;

	mutex_lock(&wl_wificall_mutex);
	s =  command + strlen("WIFICALL") + 1;
	set_val = bcm_atoi(s);



	switch (set_val) {
	case 0:
		if (0 == wl_android_is_during_wifi_call()) {
			printf("wifi call is in disconnected state!\n");
			break;
		}

		printf("wifi call ends: %d\n", set_val);
		wl_android_wifi_call = 0;

		dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_WIFI_PHONE);
		dhdhtc_update_wifi_power_mode(screen_off);

		dhdhtc_update_dtim_listen_interval(screen_off);

		break;
	case 1:
		if (1 == wl_android_is_during_wifi_call()) {
			printf("wifi call is already in running state!\n");
			break;
		}

		printf("wifi call comes: %d\n", set_val);
		wl_android_wifi_call = 1;

		dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_WIFI_PHONE);
		dhdhtc_update_wifi_power_mode(screen_off);

		dhdhtc_update_dtim_listen_interval(screen_off);

		break;
	default:
		DHD_ERROR(("%s: not support mode: %d\n", __func__, set_val));
		break;

	}

	bytes_written = snprintf(command, total_len, "OK");
	mutex_unlock(&wl_wificall_mutex);

	return bytes_written;
}

int dhd_set_project(char * project, int project_len)
{

        if ((project_len < 1) || (project_len > 32)) {
                printf("Invaild project name length!\n");
                return -1;
        }

        strncpy(project_type, project, project_len);
        DHD_DEFAULT(("%s: set project type: %s\n", __FUNCTION__, project_type));

        return 0;
}

static int wl_android_set_project(struct net_device *ndev, char *command, int total_len)
{
	int bytes_written = 0;
	char project[33];
	int project_offset;
	int project_size;

	DHD_TRACE(("%s\n", __FUNCTION__));
	memset(project, 0, sizeof(project));

	project_offset = strcspn(command, " ");
	project_size = strlen(command) - project_offset;

	if (project_offset == 0) {
		DHD_ERROR(("%s, no project specified\n", __FUNCTION__));
		return 0;
	}

	if (project_size > 32) {
		DHD_ERROR(("%s: project name is too long: %s\n", __FUNCTION__,
				(char *)command + project_offset + 1));
		return 0;
	}

	strncpy(project, command + project_offset + 1, MIN(project_size, sizeof(project)));

	DHD_DEFAULT(("%s: set project: %s\n", __FUNCTION__, project));
	dhd_set_project(project, project_size);

	bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;
}

#ifdef BCM4329_LOW_POWER
static int wl_android_set_lowpowermode(struct net_device *ndev, char *command, int total_len)
{
	int bytes_written = 0;
	char *s;
	int set_val;

	DHD_TRACE(("%s\n", __func__));
	s =  command + strlen("SET_LOWPOWERMODE") + 1;
	set_val = bcm_atoi(s);

	switch (set_val) {
	case 0:
		printf("LowPowerMode: %d\n", set_val);
		LowPowerMode = 0;
		break;
	case 1:
		printf("LowPowerMode: %d\n", set_val);
		LowPowerMode = 1;
		break;
	default:
		DHD_ERROR(("%s: not support mode: %d\n", __func__, set_val));
		break;
	}

	bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;
}
#endif

#ifdef WES_SUPPORT
int wl_android_get_roam_scan_control(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;

	error = get_roamscan_mode(dev, &mode);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Control, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETROAMSCANCONTROL, mode);

	return bytes_written;
}

int wl_android_set_roam_scan_control(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = set_roamscan_mode(dev, mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Control %d, error = %d\n",
		 __FUNCTION__, mode, error));
		return -1;
	}

	return 0;
}

int wl_android_get_roam_scan_channels(struct net_device *dev, char *command, int total_len)
{
	int bytes_written = 0;
	unsigned char channels[ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS] = {0};
	int channel_cnt = 0;
	char channel_info[10 + (ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS * 3)] = {0};
	int channel_info_len = 0;
	int i = 0;

	channel_cnt = get_roamscan_channel_list(dev, channels);

	channel_info_len += sprintf(&channel_info[channel_info_len], "%d ", channel_cnt);
	for (i = 0; i < channel_cnt; i++) {
		channel_info_len += sprintf(&channel_info[channel_info_len], "%d ", channels[i]);

		if (channel_info_len > (sizeof(channel_info) - 10))
			break;
	}
	channel_info_len += sprintf(&channel_info[channel_info_len], "%s", "\0");

	bytes_written = snprintf(command, total_len, "%s %s",
		CMD_GETROAMSCANCHANNELS, channel_info);
	return bytes_written;
}

int wl_android_set_roam_scan_channels(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	unsigned char *p = (unsigned char *)(command + strlen(CMD_SETROAMSCANCHANNELS) + 1);
	int ioctl_version = wl_cfg80211_get_ioctl_version();
	error = set_roamscan_channel_list(dev, p[0], &p[1], ioctl_version);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Channels %d, error = %d\n",
		 __FUNCTION__, p[0], error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_channel_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_CHANNEL_TIME, &time, sizeof(time), 0);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Channel Time, error = %d\n",
		__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANCHANNELTIME, time);

	return bytes_written;
}

int wl_android_set_scan_channel_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_CHANNEL_TIME, &time, sizeof(time), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Channel Time %d, error = %d\n",
		__FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_home_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_HOME_TIME, &time, sizeof(time), 0);
	if (error) {
		DHD_ERROR(("Failed to get Scan Home Time, error = %d\n", error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANHOMETIME, time);

	return bytes_written;
}

int wl_android_set_scan_home_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_HOME_TIME, &time, sizeof(time), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Home Time %d, error = %d\n",
		__FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_home_away_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_iovar_getint(dev, "scan_home_away_time", &time);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Home Away Time, error = %d\n",
		__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANHOMEAWAYTIME, time);

	return bytes_written;
}

int wl_android_set_scan_home_away_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "scan_home_away_time", time);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Home Away Time %d, error = %d\n",
		 __FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_nprobes(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int num = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_NPROBES, &num, sizeof(num), 0);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan NProbes, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANNPROBES, num);

	return bytes_written;
}

int wl_android_set_scan_nprobes(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int num = 0;

	if (sscanf(command, "%*s %d", &num) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_NPROBES, &num, sizeof(num), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan NProbes %d, error = %d\n",
		__FUNCTION__, num, error));
		return -1;
	}

	return 0;
}

int wl_android_send_action_frame(struct net_device *dev, char *command, int total_len)
{
	int error = -1;
	android_wifi_af_params_t *params = NULL;
	wl_action_frame_t *action_frame = NULL;
	wl_af_params_t *af_params = NULL;
	char *smbuf = NULL;
	struct ether_addr tmp_bssid;
	int tmp_channel = 0;

	params = (android_wifi_af_params_t *)(command + strlen(CMD_SENDACTIONFRAME) + 1);
	if (params == NULL) {
		DHD_ERROR(("%s: Invalid params \n", __FUNCTION__));
		goto send_action_frame_out;
	}

	smbuf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL);
	if (smbuf == NULL) {
		DHD_ERROR(("%s: failed to allocated memory %d bytes\n",
		__FUNCTION__, WLC_IOCTL_MAXLEN));
		goto send_action_frame_out;
	}

	af_params = (wl_af_params_t *) kzalloc(WL_WIFI_AF_PARAMS_SIZE, GFP_KERNEL);
	if (af_params == NULL)
	{
		DHD_ERROR(("%s: unable to allocate frame\n", __FUNCTION__));
		goto send_action_frame_out;
	}

	memset(&tmp_bssid, 0, ETHER_ADDR_LEN);
	if (bcm_ether_atoe((const char *)params->bssid, (struct ether_addr *)&tmp_bssid) == 0) {
		memset(&tmp_bssid, 0, ETHER_ADDR_LEN);

		error = wldev_ioctl(dev, WLC_GET_BSSID, &tmp_bssid, ETHER_ADDR_LEN, false);
		if (error) {
			memset(&tmp_bssid, 0, ETHER_ADDR_LEN);
			DHD_ERROR(("%s: failed to get bssid, error=%d\n", __FUNCTION__, error));
			goto send_action_frame_out;
		}
	}

	if (params->channel < 0) {
		struct channel_info ci;
		error = wldev_ioctl(dev, WLC_GET_CHANNEL, &ci, sizeof(ci), false);
		if (error) {
			DHD_ERROR(("%s: failed to get channel, error=%d\n", __FUNCTION__, error));
			goto send_action_frame_out;
		}

		tmp_channel = ci.hw_channel;
	}
	else {
		tmp_channel = params->channel;
	}

	af_params->channel = tmp_channel;
	af_params->dwell_time = params->dwell_time;
	memcpy(&af_params->BSSID, &tmp_bssid, ETHER_ADDR_LEN);
	action_frame = &af_params->action_frame;

	action_frame->packetId = 0;
	memcpy(&action_frame->da, &tmp_bssid, ETHER_ADDR_LEN);
	action_frame->len = params->len;
	memcpy(action_frame->data, params->data, action_frame->len);

	error = wldev_iovar_setbuf(dev, "actframe", af_params,
		sizeof(wl_af_params_t), smbuf, WLC_IOCTL_MAXLEN, NULL);
	if (error) {
		DHD_ERROR(("%s: failed to set action frame, error=%d\n", __FUNCTION__, error));
	}

send_action_frame_out:
	if (af_params)
		kfree(af_params);

	if (smbuf)
		kfree(smbuf);

	if (error)
		return -1;
	else
		return 0;
}

int wl_android_reassoc(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	android_wifi_reassoc_params_t *params = NULL;
	uint band;
	chanspec_t channel;
	u32 params_size;
	wl_reassoc_params_t reassoc_params;

	params = (android_wifi_reassoc_params_t *)(command + strlen(CMD_REASSOC) + 1);
	if (params == NULL) {
		DHD_ERROR(("%s: Invalid params \n", __FUNCTION__));
		return -1;
	}

	memset(&reassoc_params, 0, WL_REASSOC_PARAMS_FIXED_SIZE);

	if (bcm_ether_atoe((const char *)params->bssid,
	(struct ether_addr *)&reassoc_params.bssid) == 0) {
		DHD_ERROR(("%s: Invalid bssid \n", __FUNCTION__));
		return -1;
	}

	if (params->channel < 0) {
		DHD_ERROR(("%s: Invalid Channel \n", __FUNCTION__));
		return -1;
	}

	reassoc_params.chanspec_num = 1;

	channel = params->channel;
#ifdef D11AC_IOTYPES
	if (wl_cfg80211_get_ioctl_version() == 1) {
		band = ((channel <= CH_MAX_2G_CHANNEL) ?
		WL_LCHANSPEC_BAND_2G : WL_LCHANSPEC_BAND_5G);
		reassoc_params.chanspec_list[0] = channel |
		band | WL_LCHANSPEC_BW_20 | WL_LCHANSPEC_CTL_SB_NONE;
	}
	else {
		band = ((channel <= CH_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G);
		reassoc_params.chanspec_list[0] = channel | band | WL_LCHANSPEC_BW_20;
	}
#else
	band = ((channel <= CH_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G);
	reassoc_params.chanspec_list[0] = channel |
	band | WL_CHANSPEC_BW_20 | WL_CHANSPEC_CTL_SB_NONE;
#endif 
	params_size = WL_REASSOC_PARAMS_FIXED_SIZE + sizeof(chanspec_t);

	error = wldev_ioctl(dev, WLC_REASSOC, &reassoc_params, params_size, true);
	if (error) {
		DHD_ERROR(("%s: failed to reassoc, error=%d\n", __FUNCTION__, error));
	}

	if (error)
		return -1;
	else
		return 0;
}

int wl_android_get_wes_mode(struct net_device *dev, char *command, int total_len)
{
	int bytes_written = 0;
	int mode = 0;

	mode = wl_cfg80211_get_wes_mode();

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETWESMODE, mode);

	return bytes_written;
}

int wl_android_set_wes_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wl_cfg80211_set_wes_mode(mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set WES Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}

	return 0;
}

int wl_android_get_okc_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;

	error = wldev_iovar_getint(dev, "okc_enable", &mode);
	if (error) {
		DHD_ERROR(("%s: Failed to get OKC Mode, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETOKCMODE, mode);

	return bytes_written;
}

int wl_android_set_okc_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "okc_enable", mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set OKC Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}

	return error;
}
#endif 

#if 0
static int wl_android_set_pno_setup(struct net_device *dev, char *command, int total_len)
{
	wlc_ssid_t ssids_local[MAX_PFN_LIST_COUNT];
	int res = -1;
	int nssid = 0;
	cmd_tlv_t *cmd_tlv_temp;
	char *str_ptr;
	int tlv_size_left;
	int pno_time = 0;
	int pno_repeat = 0;
	int pno_freq_expo_max = 0;

#ifdef PNO_SET_DEBUG
	int i;
	char pno_in_example[] = {
		'P', 'N', 'O', 'S', 'E', 'T', 'U', 'P', ' ',
		'S', '1', '2', '0',
		'S',
		0x05,
		'd', 'l', 'i', 'n', 'k',
		'S',
		0x04,
		'G', 'O', 'O', 'G',
		'T',
		'0', 'B',
		'R',
		'2',
		'M',
		'2',
		0x00
		};
#endif 

	DHD_INFO(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));

	if (total_len < (strlen(CMD_PNOSETUP_SET) + sizeof(cmd_tlv_t))) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		goto exit_proc;
	}


#ifdef PNO_SET_DEBUG
	memcpy(command, pno_in_example, sizeof(pno_in_example));
	for (i = 0; i < sizeof(pno_in_example); i++)
		printf("%02X ", command[i]);
	printf("\n");
	total_len = sizeof(pno_in_example);
#endif

	str_ptr = command + strlen(CMD_PNOSETUP_SET);
	tlv_size_left = total_len - strlen(CMD_PNOSETUP_SET);

	cmd_tlv_temp = (cmd_tlv_t *)str_ptr;
	memset(ssids_local, 0, sizeof(ssids_local));

	if ((cmd_tlv_temp->prefix == PNO_TLV_PREFIX) &&
		(cmd_tlv_temp->version == PNO_TLV_VERSION) &&
		(cmd_tlv_temp->subver == PNO_TLV_SUBVERSION)) {

		str_ptr += sizeof(cmd_tlv_t);
		tlv_size_left -= sizeof(cmd_tlv_t);

		if ((nssid = wl_iw_parse_ssid_list_tlv(&str_ptr, ssids_local,
			MAX_PFN_LIST_COUNT, &tlv_size_left)) <= 0) {
			DHD_ERROR(("SSID is not presented or corrupted ret=%d\n", nssid));
			goto exit_proc;
		} else {
			if ((str_ptr[0] != PNO_TLV_TYPE_TIME) || (tlv_size_left <= 1)) {
				DHD_ERROR(("%s scan duration corrupted field size %d\n",
					__FUNCTION__, tlv_size_left));
				goto exit_proc;
			}
			str_ptr++;
			pno_time = simple_strtoul(str_ptr, &str_ptr, 16);
			DHD_INFO(("%s: pno_time=%d\n", __FUNCTION__, pno_time));

			if (str_ptr[0] != 0) {
				if ((str_ptr[0] != PNO_TLV_FREQ_REPEAT)) {
					DHD_ERROR(("%s pno repeat : corrupted field\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_repeat = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_INFO(("%s :got pno_repeat=%d\n", __FUNCTION__, pno_repeat));
				if (str_ptr[0] != PNO_TLV_FREQ_EXPO_MAX) {
					DHD_ERROR(("%s FREQ_EXPO_MAX corrupted field size\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_freq_expo_max = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_INFO(("%s: pno_freq_expo_max=%d\n",
					__FUNCTION__, pno_freq_expo_max));
			}
		}
	} else {
		DHD_ERROR(("%s get wrong TLV command\n", __FUNCTION__));
		goto exit_proc;
	}

	res = dhd_dev_pno_set(dev, ssids_local, nssid, pno_time, pno_repeat, pno_freq_expo_max);

exit_proc:
	return res;
}
#endif 

static int wl_android_get_p2p_dev_addr(struct net_device *ndev, char *command, int total_len)
{
	int ret;
	int bytes_written = 0;

	ret = wl_cfg80211_get_p2p_dev_addr(ndev, (struct ether_addr*)command);
	if (ret)
		return 0;
	bytes_written = sizeof(struct ether_addr);
	return bytes_written;
}

static int wl_android_gateway_add(struct net_device *ndev, char *command, int total_len)
{
	int bytes_written = 0;

	int i;
	DHD_TRACE(("Driver GET GATEWAY-ADD CMD!!!\n"));
	sscanf(command+12,"%d",&i);
	sprintf( wl_abdroid_gatewaybuf, "%02x%02x%02x%02x",
	i & 0xff, ((i >> 8) & 0xff), ((i >> 16) & 0xff), ((i >> 24) & 0xff)
	);

	if (strcmp(wl_abdroid_gatewaybuf, "00000000") == 0)
		sprintf( wl_abdroid_gatewaybuf, "FFFFFFFF");

	DHD_TRACE(("gatewaybuf: %s",wl_abdroid_gatewaybuf));
#ifdef BCM4329_LOW_POWER
	if (LowPowerMode == 1) {
		if (screen_off && !hasDLNA && !allowMulticast)
			dhd_set_keepalive(1);
	}
#endif
	bytes_written = snprintf(command, total_len, "OK");

	return bytes_written;
}

static int last_auto_channel = 6;
static int wl_android_auto_channel(struct net_device *dev, char *command, int total_len)
{
	int chosen = 0;
	char req_buf[64] = {0};
	wl_uint32_list_t *request = (wl_uint32_list_t *)req_buf;
	int rescan = 0;
	int retry = 0;
	int updown = 0;
	wlc_ssid_t null_ssid;
	int res = 0;
	int spec = 0;
	int start_channel = 1, end_channel = 14;
	int i = 0;
	int channel = 0;
	int isup = 0;
	int bytes_written = 0;
	int apsta_var = 0;
	
	int band = WLC_BAND_2G;
	

	DHD_TRACE(("Enter %s\n", __func__));

	channel = bcm_atoi(command);

	wldev_ioctl(dev, WLC_GET_UP, &isup, sizeof(isup), 0);

	res = wldev_ioctl(dev, WLC_DOWN, &updown, sizeof(updown), 1);
	if (res) {
		DHD_ERROR(("%s fail to set updown\n", __func__));
		goto fail;
	}

	apsta_var = 0;
	res = wldev_ioctl(dev, WLC_SET_AP, &apsta_var, sizeof(apsta_var), 1);
	if (res) {
		DHD_ERROR(("%s fail to set apsta_var 0\n", __func__));
		goto fail;
	}
	apsta_var = 1;
	res = wldev_ioctl(dev, WLC_SET_AP, &apsta_var, sizeof(apsta_var), 1);
	if (res) {
		DHD_ERROR(("%s fail to set apsta_var 1\n", __func__));
		goto fail;
	}
	res = wldev_ioctl(dev, WLC_GET_AP, &apsta_var, sizeof(apsta_var), 0);

	updown = 1;
	res = wldev_ioctl(dev, WLC_UP, &updown, sizeof(updown), 1);
	if (res < 0) {
		DHD_ERROR(("%s fail to set apsta \n", __func__));
		goto fail;
	}

auto_channel_retry:
	memset(&null_ssid, 0, sizeof(wlc_ssid_t));
	
	#if 1 
	null_ssid.SSID_len = strlen("");
	strncpy(null_ssid.SSID, "", null_ssid.SSID_len);
	#else
	null_ssid.SSID_len = strlen("test");
	strncpy(null_ssid.SSID, "test", null_ssid.SSID_len);
	#endif
	

	res |= wldev_ioctl(dev, WLC_SET_SPECT_MANAGMENT, &spec, sizeof(spec), 1);
	res |= wldev_ioctl(dev, WLC_SET_SSID, &null_ssid, sizeof(null_ssid), 1);
	
	res |= wldev_ioctl(dev, WLC_SET_BAND, &band, sizeof(band), 1);
	
	res |= wldev_ioctl(dev, WLC_UP, &updown, sizeof(updown), 1);

	memset(&null_ssid, 0, sizeof(wlc_ssid_t));
	res |= wldev_ioctl(dev, WLC_SET_SSID, &null_ssid, sizeof(null_ssid), 1);

	request->count = htod32(0);
	if (channel >> 8) {
		start_channel = (channel >> 8) & 0xff;
		end_channel = channel & 0xff;
		request->count = end_channel - start_channel + 1;
		DHD_ERROR(("request channel: %d to %d ,request->count =%d\n", start_channel, end_channel, request->count));
		for (i = 0; i < request->count; i++) {
			request->element[i] = CH20MHZ_CHSPEC((start_channel + i));
			
			printf("request.element[%d]=0x%x\n", i, request->element[i]);
		}
	}

	res = wldev_ioctl(dev, WLC_START_CHANNEL_SEL, request, sizeof(req_buf), 1);
	if (res < 0) {
		DHD_ERROR(("can't start auto channel\n"));
		chosen = 6;
		goto fail;
	}

get_channel_retry:
		bcm_mdelay(500);

	res = wldev_ioctl(dev, WLC_GET_CHANNEL_SEL, &chosen, sizeof(chosen), 0);

	if (res < 0 || dtoh32(chosen) == 0) {
		if (retry++ < 6)
			goto get_channel_retry;
		else {
			DHD_ERROR(("can't get auto channel sel, err = %d, "
						"chosen = %d\n", res, chosen));
			chosen = 6; 
		}
	}

	if ((chosen == start_channel) && (!rescan++)) {
		retry = 0;
		goto auto_channel_retry;
	}

	if (channel == 0) {
		channel = chosen;
		last_auto_channel = chosen;
	} else {
		DHD_ERROR(("channel range from %d to %d ,chosen = %d\n", start_channel, end_channel, chosen));

		if (chosen > end_channel) {
			if (chosen <= 6)
				chosen = end_channel;
			else
				chosen = start_channel;
		} else if (chosen < start_channel)
			chosen = start_channel;

		channel = chosen;
	}

	res = wldev_ioctl(dev, WLC_DOWN, &updown, sizeof(updown), 1);
	if (res < 0) {
		DHD_ERROR(("%s fail to set up err =%d\n", __func__, res));
		goto fail;
	}
	
	band = WLC_BAND_AUTO;
	res |= wldev_ioctl(dev, WLC_SET_BAND, &band, sizeof(band), 1);
	

fail :

	bytes_written = snprintf(command, total_len, "%d", channel);
	return bytes_written;

}
extern int msm_otg_setclk( int on);

extern int android_wifi_off;


int wl_android_wifi_on(struct net_device *dev)
{
	int ret = 0;
	int retry = POWERUP_MAX_RETRY;

	printf("%s in (smp_mb)\n", __FUNCTION__);
	if (!dev) {
		DHD_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

	android_wifi_off = 0;
	old_tx_stat_chk = 0xff;
	old_tx_stat_chk_prd= 0xff;
	old_tx_stat_chk_ratio = 0xff;
	old_tx_stat_chk_num = 0xff;
	smp_mb();
	
	mutex_lock(&wl_wifionoff_mutex);
	if (!g_wifi_on) {
		do {
			dhd_customer_gpio_wlan_ctrl(WLAN_RESET_ON);
			ret = sdioh_start(NULL, 0);
			if (ret == 0)
				break;
			DHD_ERROR(("\nfailed to power up wifi chip, retry again (%d left) **\n\n",
				retry+1));
			dhd_customer_gpio_wlan_ctrl(WLAN_RESET_OFF);
		} while (retry-- >= 0);
		if (ret != 0) {
			DHD_ERROR(("\nfailed to power up wifi chip, max retry reached **\n\n"));
			goto exit;
		}
		ret = dhd_dev_reset(dev, FALSE);
		sdioh_start(NULL, 1);
		if (!ret) {
			if (dhd_dev_init_ioctl(dev) < 0)
				ret = -EFAULT;
		}
#if defined(PROP_TXSTATUS) && !defined(PROP_TXSTATUS_VSDB)
		dhd_wlfc_init(bcmsdh_get_drvdata());
#endif
		last_txframes = 0xffffffff;
		last_txretrans = 0xffffffff;
		last_txerror = 0xffffffff;
		g_wifi_on = TRUE;
	}

   
   if(bus_scale_table) {
		bus_perf_client =
			msm_bus_scale_register_client(bus_scale_table);
		if (!bus_perf_client)
			printf("%s: Failed to register BUS "
					"scaling client!!\n", __func__);
	}
	
	

exit:
	mutex_unlock(&wl_wifionoff_mutex);

	return ret;
}

int wl_android_wifi_off(struct net_device *dev)
{
	int ret = 0;

	printf("%s in (smp_mb)\n", __FUNCTION__);
	if (!dev) {
		DHD_TRACE(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}
	android_wifi_off = 1;
	smp_mb();

#if defined(HW_OOB)
	bcmsdh_oob_intr_set(FALSE);
#endif

	
	wl_cfg80211_set_btcoex_done(dev);

	bcm_mdelay(100);

	mutex_lock(&wl_wifionoff_mutex);
	
	if (dhd_APUP) {
		printf("apmode off - AP_DOWN\n");
		dhd_APUP = false;
		if (check_hang_already(dev)) {
			printf("Don't send AP_DOWN due to alreayd hang\n");
		}
		else {
			wlan_unlock_multi_core(dev);
			wl_iw_send_priv_event(dev, "AP_DOWN");
		}
	}
	
	if (g_wifi_on) {
#if defined(PROP_TXSTATUS) && !defined(PROP_TXSTATUS_VSDB)
		dhd_wlfc_deinit(bcmsdh_get_drvdata());
#endif
		ret = dhd_dev_reset(dev, TRUE);
		sdioh_stop(NULL);
		dhd_customer_gpio_wlan_ctrl(WLAN_RESET_OFF);
		g_wifi_on = FALSE;
	}
    wlan_unlock_multi_core(dev);
	wlan_unlock_perf();
	
	if (bus_perf_client)
		msm_bus_scale_unregister_client(bus_perf_client);

	
	
	mutex_unlock(&wl_wifionoff_mutex);
	bcm_mdelay(500);
	return ret;
}

static int wl_android_set_fwpath(struct net_device *net, char *command, int total_len)
{
	if ((strlen(command) - strlen(CMD_SETFWPATH)) > MOD_PARAM_PATHLEN)
		return -1;
	bcm_strncpy_s(fw_path, sizeof(fw_path),
		command + strlen(CMD_SETFWPATH) + 1, MOD_PARAM_PATHLEN - 1);
	if (strstr(fw_path, "apsta") != NULL) {
		DHD_INFO(("GOT APSTA FIRMWARE\n"));
		ap_fw_loaded = TRUE;
	} else {
		DHD_INFO(("GOT STA FIRMWARE\n"));
		ap_fw_loaded = FALSE;
	}
	return 0;
}


static int
wl_android_set_pmk(struct net_device *dev, char *command, int total_len)
{
	uchar pmk[33];
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
#ifdef OKC_DEBUG
	int i = 0;
#endif

	bzero(pmk, sizeof(pmk));
	memcpy((char *)pmk, command + strlen("SET_PMK "), 32);
	error = wldev_iovar_setbuf(dev, "okc_info_pmk", pmk, 32, smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set PMK for OKC, error = %d\n", error));
	}
#ifdef OKC_DEBUG
	DHD_ERROR(("PMK is "));
	for (i = 0; i < 32; i++)
		DHD_ERROR(("%02X ", pmk[i]));

	DHD_ERROR(("\n"));
#endif
	return error;
}

static int
wl_android_okc_enable(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char okc_enable = 0;

	okc_enable = command[strlen(CMD_OKC_ENABLE) + 1] - '0';
	error = wldev_iovar_setint(dev, "okc_enable", okc_enable);
	if (error) {
		DHD_ERROR(("Failed to %s OKC, error = %d\n",
			okc_enable ? "enable" : "disable", error));
	}

	return error;
}


#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)
#ifdef SUPPORT_AMPDU_MPDU_CMD
static int
wl_android_set_ampdu_mpdu(struct net_device *dev, const char* string_num)
{
	int err = 0;
	int ampdu_mpdu;

	ampdu_mpdu = bcm_atoi(string_num);

	if (ampdu_mpdu > 32) {
		DHD_ERROR(("%s : ampdu_mpdu MAX value is 32.\n", __FUNCTION__));
		return -1;
	}

	DHD_ERROR(("%s : ampdu_mpdu = %d\n", __FUNCTION__, ampdu_mpdu));
	err = wldev_iovar_setint(dev, "ampdu_mpdu", ampdu_mpdu);
	if (err < 0) {
		DHD_ERROR(("%s : ampdu_mpdu set error. %d\n", __FUNCTION__, err));
		return -1;
	}

	return 0;
}
#endif 

#ifdef SUPPORT_AUTO_CHANNEL
static int
wl_android_set_auto_channel(struct net_device *dev, const char* string_num,
	char* command, int total_len)
{
	int channel;
	int chosen = 0;
	int retry = 0;
	int ret = 0;

	
	u32 req_buf[8] = {7, 0x2B01, 0x2B02, 0x2B03, 0x2B04, 0x2B05, 0x2B06,
		0x2B07};

	
	wl_uint32_list_t request;

	channel = bcm_atoi(string_num);
	DHD_INFO(("%s : HAPD_AUTO_CHANNEL = %d\n", __FUNCTION__, channel));

	if (channel == 20)
		ret = wldev_ioctl(dev, WLC_START_CHANNEL_SEL, (void *)&req_buf,
			sizeof(req_buf), true);
	else { 
		request.count = htod32(0);
		ret = wldev_ioctl(dev, WLC_START_CHANNEL_SEL, (void *)&request,
			sizeof(request), true);
	}

	if (ret < 0) {
		DHD_ERROR(("%s: can't start auto channel scan, err = %d\n",
			__FUNCTION__, ret));
		channel = 0;
		goto done;
	}

	
	bcm_mdelay(500);

	retry = 10;
	while (retry--) {
		ret = wldev_ioctl(dev, WLC_GET_CHANNEL_SEL, &chosen, sizeof(chosen),
			false);
		if (ret < 0 || dtoh32(chosen) == 0) {
			DHD_INFO(("%s: %d tried, ret = %d, chosen = %d\n",
				__FUNCTION__, (10 - retry), ret, chosen));
			bcm_mdelay(200);
		}
		else {
			channel = (u16)chosen & 0x00FF;
			DHD_ERROR(("%s: selected channel = %d\n", __FUNCTION__, channel));
			break;
		}
	}

	if (retry == 0) {
		DHD_ERROR(("%s: auto channel timed out, failed\n", __FUNCTION__));
		channel = 0;
	}

done:
	snprintf(command, 4, "%d", channel);
	DHD_INFO(("%s: command result is %s\n", __FUNCTION__, command));

	return 4;
}
#endif 

#ifdef SUPPORT_HIDDEN_AP
static int
wl_android_set_max_num_sta(struct net_device *dev, const char* string_num)
{
	int max_assoc;

	max_assoc = bcm_atoi(string_num);
	DHD_INFO(("%s : HAPD_MAX_NUM_STA = %d\n", __FUNCTION__, max_assoc));
	wldev_iovar_setint(dev, "maxassoc", max_assoc);
	return 1;
}

static int
wl_android_set_ssid(struct net_device *dev, const char* hapd_ssid)
{
	wlc_ssid_t ssid;
	s32 ret;

	ssid.SSID_len = strlen(hapd_ssid);
	bcm_strncpy_s(ssid.SSID, sizeof(ssid.SSID), hapd_ssid, ssid.SSID_len);
	DHD_INFO(("%s: HAPD_SSID = %s\n", __FUNCTION__, ssid.SSID));
	ret = wldev_ioctl(dev, WLC_SET_SSID, &ssid, sizeof(wlc_ssid_t), true);
	if (ret < 0) {
		DHD_ERROR(("%s : WLC_SET_SSID Error:%d\n", __FUNCTION__, ret));
	}
	return 1;

}

static int
wl_android_set_hide_ssid(struct net_device *dev, const char* string_num)
{
	int hide_ssid;
	int enable = 0;

	hide_ssid = bcm_atoi(string_num);
	DHD_INFO(("%s: HAPD_HIDE_SSID = %d\n", __FUNCTION__, hide_ssid));
	if (hide_ssid)
		enable = 1;
	wldev_iovar_setint(dev, "closednet", enable);
	return 1;
}
#endif 

#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
static int
wl_android_sta_diassoc(struct net_device *dev, const char* straddr)
{
	scb_val_t scbval;

	DHD_INFO(("%s: deauth STA %s\n", __FUNCTION__, straddr));

	
	scbval.val = htod32(1);
	bcm_ether_atoe(straddr, &scbval.ea);

	DHD_INFO(("%s: deauth STA: "MACDBG "\n", __FUNCTION__,
		MAC2STRDBG(scbval.ea.octet)));

	wldev_ioctl(dev, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scbval,
		sizeof(scb_val_t), true);

	return 1;
}
#endif 

static int
wl_android_ch_res_rl(struct net_device *dev, bool change)
{
	int error = 0;
	s32 srl = 7;
	s32 lrl = 4;
	printk("%s enter\n", __FUNCTION__);
	if (change) {
		srl = 4;
		lrl = 2;
	}
	error = wldev_ioctl(dev, WLC_SET_SRL, &srl, sizeof(s32), true);
	if (error) {
		DHD_ERROR(("Failed to set SRL, error = %d\n", error));
	}
	error = wldev_ioctl(dev, WLC_SET_LRL, &lrl, sizeof(s32), true);
	if (error) {
		DHD_ERROR(("Failed to set LRL, error = %d\n", error));
	}
	return error;
}
#endif 

int wl_android_set_roam_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "roam_off", mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}
	else
		DHD_ERROR(("%s: succeeded to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
	return 0;
}

int wl_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd)
{
#define PRIVATE_COMMAND_MAX_LEN 8192
	int ret = 0;
	char *command = NULL;
	int bytes_written = 0;
	android_wifi_priv_cmd priv_cmd;
#ifdef BCM4329_LOW_POWER
	struct dd_pkt_filter_s *data;
#endif
	net_os_wake_lock(net);

	if (!ifr->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}
	if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(android_wifi_priv_cmd))) {
		ret = -EFAULT;
		goto exit;
	}
	if (priv_cmd.total_len > PRIVATE_COMMAND_MAX_LEN)
	{
		DHD_ERROR(("%s: too long priavte command\n", __FUNCTION__));
		ret = -EINVAL;
		goto exit;
	}
	command = kmalloc((priv_cmd.total_len + 1), GFP_KERNEL);
	if (!command)
	{
		DHD_ERROR(("%s: failed to allocate memory\n", __FUNCTION__));
		ret = -ENOMEM;
		goto exit;
	}
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto exit;
	}
	command[priv_cmd.total_len] = '\0';

	DHD_INFO(("%s: Android private cmd \"%s\" on %s\n", __FUNCTION__, command, ifr->ifr_name));

	if (strnicmp(command, CMD_START, strlen(CMD_START)) == 0) {
		DHD_INFO(("%s, Received regular START command\n", __FUNCTION__));
#ifdef SUPPORT_DEEP_SLEEP
		trigger_deep_sleep = 1;
#else
		bytes_written = wl_android_wifi_on(net);
#endif 
	}
	else if (strnicmp(command, CMD_SETFWPATH, strlen(CMD_SETFWPATH)) == 0) {
		bytes_written = wl_android_set_fwpath(net, command, priv_cmd.total_len);
	}

	if (!g_wifi_on) {
		DHD_ERROR(("%s: Ignore private cmd \"%s\" - iface %s is down\n",
			__FUNCTION__, command, ifr->ifr_name));
		ret = 0;
		goto exit;
	}

	if (strnicmp(command, CMD_STOP, strlen(CMD_STOP)) == 0) {
#ifdef SUPPORT_DEEP_SLEEP
		trigger_deep_sleep = 1;
#else
		bytes_written = wl_android_wifi_off(net);
#endif 
	}
	else if (strnicmp(command, CMD_SCAN_ACTIVE, strlen(CMD_SCAN_ACTIVE)) == 0) {
		
	}
	else if (strnicmp(command, CMD_SCAN_PASSIVE, strlen(CMD_SCAN_PASSIVE)) == 0) {
		
	}
	else if (strnicmp(command, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
		bytes_written = wl_android_get_rssi(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) == 0) {
		bytes_written = wl_android_get_link_speed(net, command, priv_cmd.total_len);
	}
#ifdef PKT_FILTER_SUPPORT
	else if (strnicmp(command, CMD_RXFILTER_START, strlen(CMD_RXFILTER_START)) == 0) {
		
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
		/*bytes_written = net_os_set_packet_filter(net, 1);*/
		
	}
	else if (strnicmp(command, CMD_RXFILTER_STOP, strlen(CMD_RXFILTER_STOP)) == 0) {
		
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
		/*bytes_written = net_os_set_packet_filter(net, 0);*/
		
	}
	else if (strnicmp(command, CMD_RXFILTER_ADD, strlen(CMD_RXFILTER_ADD)) == 0) {
		
#ifdef BCM4329_LOW_POWER
		if (LowPowerMode == 1) {
			data = (struct dd_pkt_filter_s *)&command[32];
			if ((data->id == ALLOW_IPV6_MULTICAST) || (data->id == ALLOW_IPV4_MULTICAST)) {
				WL_TRACE(("RXFILTER-ADD MULTICAST filter\n"));
				allowMulticast = false;
			}
		}
#endif
		wl_android_set_pktfilter(net, (struct dd_pkt_filter_s *)&command[32]);
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
		/*
		int filter_num = *(command + strlen(CMD_RXFILTER_ADD) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, TRUE, filter_num);
		*/
		
	}
	else if (strnicmp(command, CMD_RXFILTER_REMOVE, strlen(CMD_RXFILTER_REMOVE)) == 0) {
		
#ifdef BCM4329_LOW_POWER
		if (LowPowerMode == 1) {
			data = (struct dd_pkt_filter_s *)&command[32];
			if ((data->id == ALLOW_IPV6_MULTICAST) || (data->id == ALLOW_IPV4_MULTICAST)) {
				WL_TRACE(("RXFILTER-REMOVE MULTICAST filter\n"));
				allowMulticast = true;
			}
		}
#endif
		wl_android_set_pktfilter(net, (struct dd_pkt_filter_s *)&command[32]);
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
		/*
		int filter_num = *(command + strlen(CMD_RXFILTER_REMOVE) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, FALSE, filter_num);
		*/
		
	}
#endif 
	else if (strnicmp(command, CMD_BTCOEXSCAN_START, strlen(CMD_BTCOEXSCAN_START)) == 0) {
		
	}
	else if (strnicmp(command, CMD_BTCOEXSCAN_STOP, strlen(CMD_BTCOEXSCAN_STOP)) == 0) {
		
	}
	else if (strnicmp(command, CMD_BTCOEXMODE, strlen(CMD_BTCOEXMODE)) == 0) {
#ifdef WL_CFG80211
		bytes_written = wl_cfg80211_set_btcoex_dhcp(net, command);
#else
#ifdef PKT_FILTER_SUPPORT
		uint mode = *(command + strlen(CMD_BTCOEXMODE) + 1) - '0';

		if (mode == 1)
			net_os_enable_packet_filter(net, 0); 
		else
			net_os_enable_packet_filter(net, 1); 
#endif 
#endif 
	}
	else if (strnicmp(command, CMD_SETSUSPENDOPT, strlen(CMD_SETSUSPENDOPT)) == 0) {
		bytes_written = wl_android_set_suspendopt(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSUSPENDMODE, strlen(CMD_SETSUSPENDMODE)) == 0) {
		bytes_written = wl_android_set_suspendmode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
		uint band = *(command + strlen(CMD_SETBAND) + 1) - '0';
#ifdef WL_HOST_BAND_MGMT
		s32 ret = 0;
		if ((ret = wl_cfg80211_set_band(net, band)) < 0) {
			if (ret == BCME_UNSUPPORTED) {
				
				WL_ERR(("WL_HOST_BAND_MGMT defined, "
					"but roam_band iovar unsupported in the firmware\n"));
			} else {
				bytes_written = -1;
				goto exit;
			}
		}
		if ((band == WLC_BAND_AUTO) || (ret == BCME_UNSUPPORTED))
			bytes_written = wldev_set_band(net, band);
#else
		bytes_written = wldev_set_band(net, band);
#endif 
	}
	else if (strnicmp(command, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
		bytes_written = wl_android_get_band(net, command, priv_cmd.total_len);
	}
#if defined(HTC_TX_TRACKING)
	else if (strnicmp(command, CMD_TX_TRACKING, strlen(CMD_TX_TRACKING)) == 0) {
		bytes_written = wl_android_set_tx_tracking(net, command, priv_cmd.total_len);
	}
#endif
#ifdef WL_CFG80211
#ifndef CUSTOMER_SET_COUNTRY
	
	else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
#if 1 
		char country_code[3];
		country_code[0] = *(command + strlen(CMD_COUNTRY) + 1);
		country_code[1] = *(command + strlen(CMD_COUNTRY) + 2);
		country_code[2] = '\0';
#else
		char *country_code = command + strlen(CMD_COUNTRY) + 1;
#endif
		bytes_written = wldev_set_country(net, country_code);
	}
#endif 
#endif 

#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)
#ifdef ROAM_API
	else if (strnicmp(command, CMD_ROAMTRIGGER_SET,
		strlen(CMD_ROAMTRIGGER_SET)) == 0) {
		bytes_written = wl_android_set_roam_trigger(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMTRIGGER_GET,
		strlen(CMD_ROAMTRIGGER_GET)) == 0) {
		bytes_written = wl_android_get_roam_trigger(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMDELTA_SET,
		strlen(CMD_ROAMDELTA_SET)) == 0) {
		bytes_written = wl_android_set_roam_delta(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMDELTA_GET,
		strlen(CMD_ROAMDELTA_GET)) == 0) {
		bytes_written = wl_android_get_roam_delta(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMSCANPERIOD_SET,
		strlen(CMD_ROAMSCANPERIOD_SET)) == 0) {
		bytes_written = wl_android_set_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMSCANPERIOD_GET,
		strlen(CMD_ROAMSCANPERIOD_GET)) == 0) {
		bytes_written = wl_android_get_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_FULLROAMSCANPERIOD_SET,
		strlen(CMD_FULLROAMSCANPERIOD_SET)) == 0) {
		bytes_written = wl_android_set_full_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_FULLROAMSCANPERIOD_GET,
		strlen(CMD_FULLROAMSCANPERIOD_GET)) == 0) {
		bytes_written = wl_android_get_full_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_COUNTRYREV_SET,
		strlen(CMD_COUNTRYREV_SET)) == 0) {
		bytes_written = wl_android_set_country_rev(net, command,
		priv_cmd.total_len);
		wl_update_wiphybands(NULL);
	} else if (strnicmp(command, CMD_COUNTRYREV_GET,
		strlen(CMD_COUNTRYREV_GET)) == 0) {
		bytes_written = wl_android_get_country_rev(net, command,
		priv_cmd.total_len);
	}
#endif 
#ifdef WES_SUPPORT
	else if (strnicmp(command, CMD_GETROAMSCANCONTROL, strlen(CMD_GETROAMSCANCONTROL)) == 0) {
		bytes_written = wl_android_get_roam_scan_control(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETROAMSCANCONTROL, strlen(CMD_SETROAMSCANCONTROL)) == 0) {
		bytes_written = wl_android_set_roam_scan_control(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETROAMSCANCHANNELS, strlen(CMD_GETROAMSCANCHANNELS)) == 0) {
		bytes_written = wl_android_get_roam_scan_channels(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETROAMSCANCHANNELS, strlen(CMD_SETROAMSCANCHANNELS)) == 0) {
		bytes_written = wl_android_set_roam_scan_channels(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SENDACTIONFRAME, strlen(CMD_SENDACTIONFRAME)) == 0) {
		bytes_written = wl_android_send_action_frame(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_REASSOC, strlen(CMD_REASSOC)) == 0) {
		bytes_written = wl_android_reassoc(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANCHANNELTIME, strlen(CMD_GETSCANCHANNELTIME)) == 0) {
		bytes_written = wl_android_get_scan_channel_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANCHANNELTIME, strlen(CMD_SETSCANCHANNELTIME)) == 0) {
		bytes_written = wl_android_set_scan_channel_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANHOMETIME, strlen(CMD_GETSCANHOMETIME)) == 0) {
		bytes_written = wl_android_get_scan_home_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANHOMETIME, strlen(CMD_SETSCANHOMETIME)) == 0) {
		bytes_written = wl_android_set_scan_home_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANHOMEAWAYTIME, strlen(CMD_GETSCANHOMEAWAYTIME)) == 0) {
		bytes_written = wl_android_get_scan_home_away_time(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANHOMEAWAYTIME, strlen(CMD_SETSCANHOMEAWAYTIME)) == 0) {
		bytes_written = wl_android_set_scan_home_away_time(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANNPROBES, strlen(CMD_GETSCANNPROBES)) == 0) {
		bytes_written = wl_android_get_scan_nprobes(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANNPROBES, strlen(CMD_SETSCANNPROBES)) == 0) {
		bytes_written = wl_android_set_scan_nprobes(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETWESMODE, strlen(CMD_GETWESMODE)) == 0) {
		bytes_written = wl_android_get_wes_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETWESMODE, strlen(CMD_SETWESMODE)) == 0) {
		bytes_written = wl_android_set_wes_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETOKCMODE, strlen(CMD_GETOKCMODE)) == 0) {
		bytes_written = wl_android_get_okc_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETOKCMODE, strlen(CMD_SETOKCMODE)) == 0) {
		bytes_written = wl_android_set_okc_mode(net, command, priv_cmd.total_len);
	}
#endif 
#endif 

	
#if 0 
	else if (strnicmp(command, CMD_PNOSSIDCLR_SET, strlen(CMD_PNOSSIDCLR_SET)) == 0) {
		bytes_written = dhd_dev_pno_reset(net);
	}
	else if (strnicmp(command, CMD_PNOSETUP_SET, strlen(CMD_PNOSETUP_SET)) == 0) {
		bytes_written = wl_android_set_pno_setup(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_PNOENABLE_SET, strlen(CMD_PNOENABLE_SET)) == 0) {
		uint pfn_enabled = *(command + strlen(CMD_PNOENABLE_SET) + 1) - '0';
		bytes_written = dhd_dev_pno_enable(net, pfn_enabled);
	}
#endif 
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_MAC_ADDR, strlen(CMD_MAC_ADDR)) == 0) {
		bytes_written = wl_android_get_mac_addr(net, command, priv_cmd.total_len);
	}
#endif
	else if (strnicmp(command, CMD_P2P_DEV_ADDR, strlen(CMD_P2P_DEV_ADDR)) == 0) {
		bytes_written = wl_android_get_p2p_dev_addr(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_P2P_SET_NOA, strlen(CMD_P2P_SET_NOA)) == 0) {
		int skip = strlen(CMD_P2P_SET_NOA) + 1;
		bytes_written = wl_cfg80211_set_p2p_noa(net, command + skip,
			priv_cmd.total_len - skip);
	}
#ifdef WL_SDO
	else if (strnicmp(command, CMD_P2P_SD_OFFLOAD, strlen(CMD_P2P_SD_OFFLOAD)) == 0) {
		u8 *buf = command;
		u8 *cmd = NULL;
		int len;

		cmd = strsep((char **)&buf, " ");
		
		if (buf == NULL)
			len = 0;
		else
			len = strlen(buf);

		bytes_written = wl_cfg80211_sd_offload(net, cmd, buf, len);
	}
#endif 
#if !defined WL_ENABLE_P2P_IF
	else if (strnicmp(command, CMD_P2P_GET_NOA, strlen(CMD_P2P_GET_NOA)) == 0) {
		bytes_written = wl_cfg80211_get_p2p_noa(net, command, priv_cmd.total_len);
	}
#endif 
	else if (strnicmp(command, CMD_P2P_SET_PS, strlen(CMD_P2P_SET_PS)) == 0) {
		int skip = strlen(CMD_P2P_SET_PS) + 1;
		bytes_written = wl_cfg80211_set_p2p_ps(net, command + skip,
			priv_cmd.total_len - skip);
	}
	else if (strnicmp(command, CMD_GET_TX_FAIL, strlen(CMD_GET_TX_FAIL)) == 0) {
		bytes_written = wl_android_get_tx_fail(net, command, priv_cmd.total_len);
	}
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_SET_AP_WPS_P2P_IE,
		strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
		int skip = strlen(CMD_SET_AP_WPS_P2P_IE) + 3;
		bytes_written = wl_cfg80211_set_wps_p2p_ie(net, command + skip,
			priv_cmd.total_len - skip, *(command + skip - 2) - '0');
	} else if (strnicmp(command, CMD_P2P_SET_MPC, strlen(CMD_P2P_SET_MPC)) == 0) {
		int skip = strlen(CMD_P2P_SET_MPC) + 1;
		bytes_written = wl_cfg80211_set_mpc(net, command + skip,
			priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_DEAUTH_STA, strlen(CMD_DEAUTH_STA)) == 0) {
		int skip = strlen(CMD_DEAUTH_STA) + 1;
		bytes_written = wl_cfg80211_deauth_sta(net, command + skip,
			priv_cmd.total_len - skip);
	} 
#endif 
	else if (strnicmp(command, CMD_OKC_SET_PMK, strlen(CMD_OKC_SET_PMK)) == 0)
		bytes_written = wl_android_set_pmk(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_OKC_ENABLE, strlen(CMD_OKC_ENABLE)) == 0)
		bytes_written = wl_android_okc_enable(net, command, priv_cmd.total_len);
#if defined(CUSTOMER_HW4)||defined(CUSTOMER_HW2)
#ifdef SUPPORT_AMPDU_MPDU_CMD
	
	else if (strnicmp(command, CMD_AMPDU_MPDU, strlen(CMD_AMPDU_MPDU)) == 0) {
		int skip = strlen(CMD_AMPDU_MPDU) + 1;
		bytes_written = wl_android_set_ampdu_mpdu(net, (const char*)command+skip);
	}
#endif 
#ifdef SUPPORT_AUTO_CHANNEL
	else if (strnicmp(command, CMD_SET_HAPD_AUTO_CHANNEL,
		strlen(CMD_SET_HAPD_AUTO_CHANNEL)) == 0) {
		int skip = strlen(CMD_SET_HAPD_AUTO_CHANNEL) + 3;
		bytes_written = wl_android_set_auto_channel(net, (const char*)command+skip, command,
			priv_cmd.total_len);
	}
#endif 
#ifdef SUPPORT_HIDDEN_AP
	else if (strnicmp(command, CMD_SET_HAPD_MAX_NUM_STA,
		strlen(CMD_SET_HAPD_MAX_NUM_STA)) == 0) {
		int skip = strlen(CMD_SET_HAPD_MAX_NUM_STA) + 3;
		wl_android_set_max_num_sta(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SET_HAPD_SSID,
		strlen(CMD_SET_HAPD_SSID)) == 0) {
		int skip = strlen(CMD_SET_HAPD_SSID) + 3;
		wl_android_set_ssid(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SET_HAPD_HIDE_SSID,
		strlen(CMD_SET_HAPD_HIDE_SSID)) == 0) {
		int skip = strlen(CMD_SET_HAPD_HIDE_SSID) + 3;
		wl_android_set_hide_ssid(net, (const char*)command+skip);
	}
#endif 
#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
	else if (strnicmp(command, CMD_HAPD_STA_DISASSOC,
		strlen(CMD_HAPD_STA_DISASSOC)) == 0) {
		int skip = strlen(CMD_HAPD_STA_DISASSOC) + 1;
		wl_android_sta_diassoc(net, (const char*)command+skip);
	}
#endif 
	else if (strnicmp(command, CMD_CHANGE_RL, strlen(CMD_CHANGE_RL)) == 0)
		bytes_written = wl_android_ch_res_rl(net, true);
	else if (strnicmp(command, CMD_RESTORE_RL, strlen(CMD_RESTORE_RL)) == 0)
		bytes_written = wl_android_ch_res_rl(net, false);
#endif 
	else if (strnicmp(command, CMD_SETROAMMODE, strlen(CMD_SETROAMMODE)) == 0)
		bytes_written = wl_android_set_roam_mode(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_DTIM_SKIP_GET, strlen(CMD_DTIM_SKIP_GET)) == 0) {
		bytes_written = wl_android_get_dtim_skip(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_DTIM_SKIP_SET, strlen(CMD_DTIM_SKIP_SET)) == 0) {
		bytes_written = wl_android_set_dtim_skip(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_TXPOWER_SET, strlen(CMD_TXPOWER_SET)) == 0) {
		bytes_written = wl_android_set_txpower(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_POWER_MODE_SET, strlen(CMD_POWER_MODE_SET)) == 0) {
		bytes_written = wl_android_set_power_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_POWER_MODE_GET, strlen(CMD_POWER_MODE_GET)) == 0) {
		bytes_written = wl_android_get_power_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_AP_TXPOWER_SET, strlen(CMD_AP_TXPOWER_SET)) == 0) {
		bytes_written = wl_android_set_ap_txpower(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_AP_ASSOC_LIST_GET, strlen(CMD_AP_ASSOC_LIST_GET)) == 0) {
		bytes_written = wl_android_get_assoc_sta_list(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_AP_MAC_LIST_SET, strlen(CMD_AP_MAC_LIST_SET)) == 0) {
		bytes_written = wl_android_set_ap_mac_list(net, command + PROFILE_OFFSET);
	}
	else if (strnicmp(command, CMD_SCAN_MINRSSI_SET, strlen(CMD_SCAN_MINRSSI_SET)) == 0) {
		bytes_written = wl_android_set_scan_minrssi(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_LOW_RSSI_SET, strlen(CMD_LOW_RSSI_SET)) == 0) {
		bytes_written = wl_android_low_rssi_set(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_PFN_REMOVE, strlen(CMD_PFN_REMOVE)) == 0) {
		bytes_written = wl_android_del_pfn(net, command, priv_cmd.total_len);
	}
#if 0
	else if (strnicmp(command, CMD_GETCSCAN, strlen(CMD_GETCSCAN)) == 0) {
		bytes_written = wl_android_get_cscan(net, command, priv_cmd.total_len);
	}
#endif
	else if (strnicmp(command, CMD_GETWIFILOCK, strlen(CMD_GETWIFILOCK)) == 0) {
		bytes_written = wl_android_get_wifilock(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETWIFICALL, strlen(CMD_SETWIFICALL)) == 0) {
		bytes_written = wl_android_set_wificall(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETPROJECT, strlen(CMD_SETPROJECT)) == 0) {
		bytes_written = wl_android_set_project(net, command, priv_cmd.total_len);
	}
	
#ifdef BCM4329_LOW_POWER
	else if (strnicmp(command, CMD_SETLOWPOWERMODE, strlen(CMD_SETLOWPOWERMODE)) == 0) {
		bytes_written = wl_android_set_lowpowermode(net, command, priv_cmd.total_len);
	}
#endif
	
	else if (strnicmp(command, CMD_GATEWAYADD, strlen(CMD_GATEWAYADD)) == 0) {
		bytes_written = wl_android_gateway_add(net, command, priv_cmd.total_len);

	} else if (strnicmp(command, CMD_GET_AUTO_CHANNEL, strlen(CMD_GET_AUTO_CHANNEL)) == 0) {
		int skip = strlen(CMD_GET_AUTO_CHANNEL) + 1;
		block_ap_event = 1;
		bytes_written = wl_android_auto_channel(net, command + skip,
		priv_cmd.total_len - skip) + skip;
		block_ap_event = 0;
	}
#ifdef APSTA_CONCURRENT
	else if (strnicmp(command, CMD_GET_AP_STATUS, strlen(CMD_GET_AP_STATUS)) == 0) {
		bytes_written = wl_android_get_ap_status(net, command, priv_cmd.total_len);
	} 
	else if (strnicmp(command, CMD_SET_AP_CFG, strlen(CMD_SET_AP_CFG)) == 0) {
		bytes_written = wl_android_set_ap_cfg(net, command, priv_cmd.total_len);
	} 
	else if (strnicmp(command, CMD_SET_APSTA, strlen(CMD_SET_APSTA)) == 0) {
		bytes_written = wl_android_set_apsta(net, command, priv_cmd.total_len);
	} 
	else if (strnicmp(command, CMD_SET_BCN_TIMEOUT, strlen(CMD_SET_BCN_TIMEOUT)) == 0) {
		printk("[WLAN] %s CMD_SET_BCN_TIMEOUT\n",__FUNCTION__);
		bytes_written = wl_android_set_bcn_timeout(net, command, priv_cmd.total_len);
	} 
	else if (strnicmp(command, CMD_SCAN_SUPPRESS, strlen(CMD_SCAN_SUPPRESS)) == 0) {
		bytes_written = wl_android_scansuppress(net, command, priv_cmd.total_len);
	} 
	else if (strnicmp(command, CMD_SCAN_ABORT, strlen(CMD_SCAN_ABORT)) == 0) {
		bytes_written = wl_android_scanabort(net, command, priv_cmd.total_len);
	} 
else if (strnicmp(command, CMD_GET_CONAP_CHANNEL, strlen(CMD_GET_CONAP_CHANNEL)) == 0) {
    printf(" %s CMD_GET_CONAP_CHANNEL\n",__FUNCTION__);
    bytes_written = wl_android_get_conap_channel(net, command, priv_cmd.total_len);
} 
	else if (strnicmp(command, CMD_OLD_DNGL, strlen(CMD_OLD_DNGL)) == 0) {
    	printf(" %s CMD_DNGL_OLD\n",__FUNCTION__);
	    
		old_dongle = 1;
	}
#ifdef BRCM_WPSAP
	else if (strnicmp(command, CMD_SET_WSEC, strlen(CMD_SET_WSEC)) == 0) {
		printk("[WLAN] %s CMD_SET_WSEC\n",__FUNCTION__);
		bytes_written = wldev_set_ap_sta_registra_wsec(net, command, priv_cmd.total_len);  
	}
	else if (strnicmp(command, CMD_WPS_RESULT, strlen(CMD_WPS_RESULT)) == 0) {
		unsigned char result;
		result = *(command + PROFILE_OFFSET);
		printf("[WLAN] %s WPS_RESULT result = %d\n",__FUNCTION__,result);
		if(result == 1)
			wl_iw_send_priv_event(net, "WPS_SUCCESSFUL"); 
		else
			wl_iw_send_priv_event(net, "WPS_FAIL"); 
	}
        else if (strnicmp(command, "L2PE_RESULT", strlen("L2PE_RESULT")) == 0) {
		unsigned char result;
		result = *(command + PROFILE_OFFSET);
		printf("%s L2PE_RESULT result = %d\n", __FUNCTION__, result);
		if(result == 1)
			wl_iw_send_priv_event(net, "L2PE_SUCCESSFUL");
		else
			wl_iw_send_priv_event(net, "L2PE_FAIL");
	}
#endif 
#endif 
#ifdef BCMCCX
	else if (strnicmp(command, CMD_GETCCKM_RN, strlen(CMD_GETCCKM_RN)) == 0) {
		bytes_written = wl_android_get_cckm_rn(net, command);
	}
	else if (strnicmp(command, CMD_SETCCKM_KRK, strlen(CMD_SETCCKM_KRK)) == 0) {
		bytes_written = wl_android_set_cckm_krk(net, command);
	}
	else if (strnicmp(command, CMD_GET_ASSOC_RES_IES, strlen(CMD_GET_ASSOC_RES_IES)) == 0) {
		bytes_written = wl_android_get_assoc_res_ies(net, command);
	}
#endif
	else {
		DHD_ERROR(("Unknown PRIVATE command %s - ignored\n", command));
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
	}

	if (bytes_written >= 0) {
		if ((bytes_written == 0) && (priv_cmd.total_len > 0))
			command[0] = '\0';
		if (bytes_written >= priv_cmd.total_len) {
			DHD_ERROR(("%s: bytes_written = %d\n", __FUNCTION__, bytes_written));
			bytes_written = priv_cmd.total_len;
		} else {
			bytes_written++;
		}
		priv_cmd.used_len = bytes_written;
		if (copy_to_user(priv_cmd.buf, command, bytes_written)) {
			DHD_ERROR(("%s: failed to copy data to user buffer\n", __FUNCTION__));
			ret = -EFAULT;
		}
	}
	else {
		ret = bytes_written;
	}

exit:
	net_os_wake_unlock(net);
	if (command) {
		kfree(command);
	}

	return ret;
}

int wl_android_init(void)
{
	int ret = 0;

	dhd_msg_level |= DHD_ERROR_VAL;
#ifdef ENABLE_INSMOD_NO_FW_LOAD
	dhd_download_fw_on_driverload = FALSE;
#endif 
#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
	if (!iface_name[0]) {
		memset(iface_name, 0, IFNAMSIZ);
		bcm_strncpy_s(iface_name, IFNAMSIZ, "wlan", IFNAMSIZ);
	}
#endif 

	mutex_init(&wl_wificall_mutex);
	mutex_init(&wl_wifionoff_mutex);
	wlan_init_perf();

#ifdef WL_GENL
	wl_genl_init();
#endif

	return ret;
}

int wl_android_exit(void)
{
	int ret = 0;

#ifdef WL_GENL
	wl_genl_deinit();
#endif 

	wlan_deinit_perf();

	return ret;
}

void wl_android_post_init(void)
{
	if (!dhd_download_fw_on_driverload) {
		
		dhd_customer_gpio_wlan_ctrl(WLAN_RESET_OFF);
		g_wifi_on = 0;
	}
}

#ifdef WL_GENL
static int wl_genl_init(void)
{
	int ret;

	WL_DBG(("GEN Netlink Init\n\n"));

	
	ret = genl_register_family(&wl_genl_family);
	if (ret != 0)
		goto failure;

	
	ret = genl_register_ops(&wl_genl_family, &wl_genl_ops);
	if (ret != 0) {
		WL_ERR(("register ops failed: %i\n", ret));
		genl_unregister_family(&wl_genl_family);
		goto failure;
	}

	ret = genl_register_mc_group(&wl_genl_family, &wl_genl_mcast);
	if (ret != 0) {
		WL_ERR(("register mc_group failed: %i\n", ret));
		genl_unregister_ops(&wl_genl_family, &wl_genl_ops);
		genl_unregister_family(&wl_genl_family);
		goto failure;
	}

	return 0;

failure:
	WL_ERR(("Registering Netlink failed!!\n"));
	return -1;
}

static int wl_genl_deinit(void)
{
	if (genl_unregister_ops(&wl_genl_family, &wl_genl_ops) < 0)
		WL_ERR(("Unregister wl_genl_ops failed\n"));

	if (genl_unregister_family(&wl_genl_family) < 0)
		WL_ERR(("Unregister wl_genl_ops failed\n"));

	return 0;
}

s32
wl_genl_send_msg(
	struct net_device *ndev,
	int pid,
	u8 *string,
	u8 len,
	int mcast)
{
	int ret = 0;
	struct sk_buff *skb;
	void *msg;

	WL_DBG(("Enter \n"));
	skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (skb == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	msg = genlmsg_put(skb, 0, 0, &wl_genl_family, 0, BCM_GENL_CMD_MSG);
	if (msg == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ret = nla_put_string(skb, BCM_GENL_ATTR_MSG, string);
	if (ret != 0) {
		WL_ERR(("nla_put_string failed\n"));
		goto out;
	}

	if (mcast) {
		
		genlmsg_end(skb, msg);
		
		if (genlmsg_multicast(skb, 0, wl_genl_mcast.id, GFP_ATOMIC) < 0)
			WL_ERR(("genlmsg_multicast failed\n"));
	} else {
		NETLINK_CB(skb).dst_group = 0; 

		
		genlmsg_end(skb, msg);

		
		if (genlmsg_unicast(&init_net, skb, pid) < 0)
			WL_ERR(("genlmsg_unicast failed\n"));
	}

	return 0;
out:
	nlmsg_free(skb);
	return ret;
}

static s32
wl_genl_handle_msg(
	struct sk_buff *skb,
	struct genl_info *info)
{
	struct nlattr *na;
	u8 *data = NULL;

	WL_DBG(("Enter \n"));

	if (info == NULL) {
		return -EINVAL;
	}

	na = info->attrs[BCM_GENL_ATTR_MSG];
	if (!na) {
		WL_ERR(("nlattribute NULL\n"));
		return -EINVAL;
	}

	data = (char *)nla_data(na);
	if (!data) {
		WL_ERR(("Invalid data\n"));
		return -EINVAL;
	} else {
		
		WL_DBG(("%s: Data received from pid (%d) \n", __func__, info->snd_pid));
	}

	return 0;
}
#endif 

#if defined(CONFIG_WIFI_CONTROL_FUNC)

static int g_wifidev_registered = 0;
static struct semaphore wifi_control_sem;
static struct wifi_platform_data *wifi_control_data = NULL;
static struct resource *wifi_irqres = NULL;

static int wifi_add_dev(void);
static void wifi_del_dev(void);

int wl_android_wifictrl_func_add(void)
{
	int ret = 0;
	sema_init(&wifi_control_sem, 0);

	ret = wifi_add_dev();
	if (ret) {
		DHD_ERROR(("%s: platform_driver_register failed\n", __FUNCTION__));
		return ret;
	}
	g_wifidev_registered = 1;

	
	if (down_timeout(&wifi_control_sem,  msecs_to_jiffies(1000)) != 0) {
		ret = -EINVAL;
		DHD_ERROR(("%s: platform_driver_register timeout\n", __FUNCTION__));
	}

	return ret;
}

void wl_android_wifictrl_func_del(void)
{
	if (g_wifidev_registered)
	{
		wifi_del_dev();
		g_wifidev_registered = 0;
	}
}

void* wl_android_prealloc(int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	if (wifi_control_data && wifi_control_data->mem_prealloc) {
		alloc_ptr = wifi_control_data->mem_prealloc(section, size);
		if (alloc_ptr) {
			DHD_INFO(("success alloc section %d\n", section));
			if (size != 0L)
				bzero(alloc_ptr, size);
			return alloc_ptr;
		}
	}

	DHD_ERROR(("can't alloc section %d\n", section));
	return NULL;
}

int wifi_get_irq_number(unsigned long *irq_flags_ptr)
{
	if (wifi_irqres) {
		*irq_flags_ptr = wifi_irqres->flags & IRQF_TRIGGER_MASK;
		return (int)wifi_irqres->start;
	}
#ifdef CUSTOM_OOB_GPIO_NUM
	return CUSTOM_OOB_GPIO_NUM;
#else
	return -1;
#endif
}

int wifi_set_power(int on, unsigned long msec)
{
	
	struct mmc_host *mmc;
	struct msmsdcc_host *host;
	

	DHD_ERROR(("%s = %d\n", __FUNCTION__, on));
	if (wifi_control_data && wifi_control_data->set_power) {
		wifi_control_data->set_power(on);
	}
	
	if (!on) {
        printf("Check module_insert(%d) before Setting wifi Power off\n", module_insert);
		if (module_insert && gInstance && gInstance->func[0] && gInstance->func[0]->card) {
			mmc = gInstance->func[0]->card->host;
			host = (void *)mmc->private;
			clk_disable(host->clk);
			if (host->pclk)
				clk_disable(host->pclk);
			if (host->bus_clk)
				clk_disable(host->bus_clk);
			atomic_set(&host->clks_on, 0);
		}
	}
	
	if (msec)
		msleep(msec);
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
int wifi_get_mac_addr(unsigned char *buf)
{
	DHD_ERROR(("%s\n", __FUNCTION__));
	if (!buf)
		return -EINVAL;
	if (wifi_control_data && wifi_control_data->get_mac_addr) {
		return wifi_control_data->get_mac_addr(buf);
	}
	return -EOPNOTSUPP;
}
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
void *wifi_get_country_code(char *ccode)
{
	DHD_TRACE(("%s\n", __FUNCTION__));
	if (!ccode)
		return NULL;
	if (wifi_control_data && wifi_control_data->get_country_code) {
		return wifi_control_data->get_country_code(ccode);
	}
	return NULL;
}
#endif 

static int wifi_set_carddetect(int on)
{
	DHD_ERROR(("%s = %d\n", __FUNCTION__, on));
	if (wifi_control_data && wifi_control_data->set_carddetect) {
		wifi_control_data->set_carddetect(on);
	}
	return 0;
}

static int wifi_probe(struct platform_device *pdev)
{
	struct wifi_platform_data *wifi_ctrl =
		(struct wifi_platform_data *)(pdev->dev.platform_data);

	wifi_irqres = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "bcmdhd_wlan_irq");
	if (wifi_irqres == NULL)
		wifi_irqres = platform_get_resource_byname(pdev,
			IORESOURCE_IRQ, "bcm4329_wlan_irq");
	wifi_control_data = wifi_ctrl;
	
	if (wifi_control_data)
		bus_scale_table = wifi_control_data->bus_scale_table;
	

	wifi_set_power(1, 200);	
	wifi_set_carddetect(1);	

	up(&wifi_control_sem);
	return 0;
}

static int wifi_remove(struct platform_device *pdev)
{
	struct wifi_platform_data *wifi_ctrl =
		(struct wifi_platform_data *)(pdev->dev.platform_data);

	DHD_ERROR(("## %s\n", __FUNCTION__));
	wifi_control_data = wifi_ctrl;

	wifi_set_power(0, WIFI_TURNOFF_DELAY);	
	wifi_set_carddetect(0);	

	up(&wifi_control_sem);
	return 0;
}

static int wifi_suspend(struct platform_device *pdev, pm_message_t state)
{
	DHD_TRACE(("##> %s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 39)) && defined(OOB_INTR_ONLY) && 1
	bcmsdh_oob_intr_set(0);
#endif 
	return 0;
}

static int wifi_resume(struct platform_device *pdev)
{
	DHD_TRACE(("##> %s\n", __FUNCTION__));
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 39)) && defined(OOB_INTR_ONLY) && 1
	if (dhd_os_check_if_up(bcmsdh_get_drvdata()))
		bcmsdh_oob_intr_set(1);
#endif 
	return 0;
}

static struct platform_driver wifi_device = {
	.probe          = wifi_probe,
	.remove         = wifi_remove,
	.suspend        = wifi_suspend,
	.resume         = wifi_resume,
	.driver         = {
	.name   = "bcmdhd_wlan",
	}
};

static struct platform_driver wifi_device_legacy = {
	.probe          = wifi_probe,
	.remove         = wifi_remove,
	.suspend        = wifi_suspend,
	.resume         = wifi_resume,
	.driver         = {
	.name   = "bcm4329_wlan",
	}
};

static int wifi_add_dev(void)
{
	int ret = 0;
	DHD_TRACE(("## Calling platform_driver_register\n"));
	ret = platform_driver_register(&wifi_device);
	if (ret)
		return ret;

	ret = platform_driver_register(&wifi_device_legacy);
	return ret;
}

static void wifi_del_dev(void)
{
	DHD_TRACE(("## Unregister platform_driver_register\n"));
	platform_driver_unregister(&wifi_device);
	platform_driver_unregister(&wifi_device_legacy);
}
#endif 
