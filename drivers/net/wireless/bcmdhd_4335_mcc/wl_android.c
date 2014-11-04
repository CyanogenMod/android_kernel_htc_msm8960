/*
 * Linux cfg80211 driver - Android related functions
 *
 * Copyright (C) 1999-2013, Broadcom Corporation
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
 * $Id: wl_android.c 420671 2013-08-28 11:37:19Z $
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <wl_android.h>
#include <wldev_common.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include <linux_osl.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#ifdef PNO_SUPPORT
#include <dhd_pno.h>
#endif
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
#endif 

#ifdef CUSTOMER_HW_ONE
#include <wl_iw.h>
#include <sdio.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/clk.h>
#include "bcmsdh_sdmmc.h"
#include "../../../mmc/host/msm_sdcc.h"
extern PBCMSDH_SDMMC_INSTANCE gInstance;
static uint32 last_txframes = 0xffffffff;
static uint32 last_txretrans = 0xffffffff;
static uint32 last_txerror = 0xffffffff;
int block_ap_event = 0;
static int g_wifi_on = TRUE;
extern int dhdcdc_wifiLock;
static struct mac_list_set android_mac_list_buf;
static struct mflist android_ap_black_list;
static int android_ap_macmode = MACLIST_MODE_DISABLED;
extern struct net_device *ap_net_dev;
#ifdef APSTA_CONCURRENT
extern struct wl_priv *wlcfg_drv_priv;
int old_dongle = 0;
#endif
extern int net_os_send_hang_message(struct net_device *dev);
int wl_cfg80211_get_mac_addr(struct net_device *net, struct ether_addr *mac_addr);
void wl_cfg80211_set_btcoex_done(struct net_device *dev);
int wl_cfg80211_set_apsta_concurrent(struct net_device *dev, bool enable);
extern bool check_hang_already(struct net_device *dev);
static int wl_android_get_mac_addr(struct net_device *ndev, char *command, int total_len);
static int wl_android_get_tx_fail(struct net_device *dev, char *command, int total_len);
static int wl_android_get_dtim_skip(struct net_device *dev, char *command, int total_len);
static int wl_android_set_dtim_skip(struct net_device *dev, char *command, int total_len);
static int wl_android_set_txpower(struct net_device *dev, char *command, int total_len);
static int wl_android_set_power_mode(struct net_device *dev, char *command, int total_len);
static int wl_android_get_power_mode(struct net_device *dev, char *command, int total_len);
static int wl_android_set_ap_txpower(struct net_device *dev, char *command, int total_len);
static int wl_android_get_assoc_sta_list(struct net_device *dev, char *buf, int len);
static int wl_android_set_ap_mac_list(struct net_device *dev, void *buf);
static int wl_android_set_scan_minrssi(struct net_device *dev, char *command, int total_len);
static int wl_android_low_rssi_set(struct net_device *dev, char *command, int total_len);
static int wl_android_del_pfn(struct net_device *dev, char *command, int total_len);
static int wl_android_get_wifilock(struct net_device *ndev, char *command, int total_len);
static int wl_android_set_wificall(struct net_device *ndev, char *command, int total_len);
static int wl_android_set_project(struct net_device *ndev, char *command, int total_len);
static int wl_android_gateway_add(struct net_device *ndev, char *command, int total_len);
static int wl_android_auto_channel(struct net_device *dev, char *command, int total_len);
static int wl_android_get_ap_status(struct net_device *net, char *command, int total_len);
static int wl_android_set_ap_cfg(struct net_device *net, char *command, int total_len);
static int wl_android_set_apsta(struct net_device *net, char *command, int total_len);
static int wl_android_set_bcn_timeout(struct net_device *dev, char *command, int total_len);
static int wl_android_scansuppress(struct net_device *net, char *command, int total_len);
extern s32 wl_cfg80211_scan_abort(struct net_device *ndev);
static int wl_android_scanabort(struct net_device *net, char *command, int total_len);
extern int wldev_get_conap_ctrl_channel(struct net_device *dev,uint8 *ctrl_channel);
static int wl_android_get_conap_channel(struct net_device *net, char *buf, int len);
static void wlan_init_perf(void);
static void wlan_deinit_perf(void);
void wl_cfg80211_send_priv_event(struct net_device *dev, char *flag);
extern s32 wl_cfg80211_set_scan_abort(struct net_device *ndev);

extern char project_type[33];
static int wl_android_wifi_call = 0;
static int last_auto_channel = 6;
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
extern int msm_otg_setclk( int on);
static int assoc_count_buff = 0;
extern int sta_event_sent;

#define TRAFFIC_SUPER_HIGH_WATER_MARK	2600
#define TRAFFIC_HIGH_WATER_MARK			2300
#define TRAFFIC_LOW_WATER_MARK			256
typedef enum traffic_ind {
	TRAFFIC_STATS_NORMAL = 0,
	TRAFFIC_STATS_HIGH,
	TRAFFIC_STATS_SUPER_HIGH,
} traffic_ind_t;

#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif

#ifdef CONFIG_PERFLOCK
struct perf_lock *wlan_perf_lock;
#endif

struct msm_bus_scale_pdata *bus_scale_table = NULL;
uint32_t bus_perf_client = 0;

static int screen_off = 0;
int sta_connected = 0;
static int traffic_stats_flag = TRAFFIC_STATS_NORMAL;
static unsigned long current_traffic_count = 0;
static unsigned long last_traffic_count = 0;
static unsigned long last_traffic_count_jiffies = 0;

#define TX_FAIL_CHECK_COUNT		100

#ifdef HTC_TX_TRACKING
static int wl_android_set_tx_tracking(struct net_device *dev, char *command, int total_len);
static uint old_tx_stat_chk;
static uint old_tx_stat_chk_prd;
static uint old_tx_stat_chk_ratio;
static uint old_tx_stat_chk_num;
#endif

#define CMD_MAC_ADDR		"MACADDR"
#define CMD_P2P_SET_MPC 	"P2P_MPC_SET"
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
#define CMD_PFN_REMOVE		"PFN_REMOVE"
#define CMD_GET_AUTO_CHANNEL	"AUTOCHANNELGET"

#endif

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
#define CMD_P2P_DEV_ADDR	"P2P_DEV_ADDR"
#define CMD_SETFWPATH		"SETFWPATH"
#define CMD_SETBAND		"SETBAND"
#define CMD_GETBAND		"GETBAND"
#define CMD_COUNTRY		"COUNTRY"
#define CMD_P2P_SET_NOA		"P2P_SET_NOA"
#if !defined WL_ENABLE_P2P_IF
#define CMD_P2P_GET_NOA			"P2P_GET_NOA"
#endif 
#define CMD_P2P_SD_OFFLOAD		"P2P_SD_"
#define CMD_P2P_SET_PS		"P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE 		"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE 	"SETROAMMODE"
#define CMD_SETIBSSBEACONOUIDATA	"SETIBSSBEACONOUIDATA"
#define CMD_MIRACAST		"MIRACAST"

#if defined(WL_SUPPORT_AUTO_CHANNEL)
#define CMD_GET_BEST_CHANNELS	"GET_BEST_CHANNELS"
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
#define CMD_WLS_BATCHING	"WLS_BATCHING"
#endif 

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"

#define	CMD_HAPD_MAC_FILTER	"HAPD_MAC_FILTER"
#define MACLIST_MODE_DISABLED   0
#define MACLIST_MODE_DENY       1
#define MACLIST_MODE_ALLOW      2

#define MAX_NUM_OF_ASSOCLIST    64

#define MAX_NUM_MAC_FILT        10


#define MIRACAST_MODE_OFF	0
#define MIRACAST_MODE_SOURCE	1
#define MIRACAST_MODE_SINK	2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE	8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif

static LIST_HEAD(miracast_resume_list);
static u8 miracast_cur_mode;

struct io_cfg {
	s8 *iovar;
	s32 param;
	u32 ioctl;
	void *arg;
	u32 len;
	struct list_head list;
};

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
	[BCM_GENL_ATTR_STRING] = { .type = NLA_NUL_STRING },
	[BCM_GENL_ATTR_MSG] = { .type = NLA_BINARY },
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
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr);
int wl_cfg80211_set_btcoex_dhcp(struct net_device *dev, char *command);
int wl_cfg80211_get_ioctl_version(void);
#else
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr)
{ return 0; }
int wl_cfg80211_set_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_get_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_p2p_ps(struct net_device *net, char* buf, int len)
{ return 0; }
#endif 
extern int dhd_os_check_if_up(void *dhdp);
#ifdef BCMLXSDMMC
extern void *bcmsdh_get_drvdata(void);
#endif 


#ifdef ENABLE_4335BT_WAR
extern int bcm_bt_lock(int cookie);
extern void bcm_bt_unlock(int cookie);
static int lock_cookie_wifi = 'W' | 'i'<<8 | 'F'<<16 | 'i'<<24;	
#endif 

extern bool ap_fw_loaded;
#if defined(CUSTOMER_HW2)
extern char iface_name[IFNAMSIZ];
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
		DHD_ERROR(("%s: wldev_get_ssid failed\n", __FUNCTION__));
	} else {
		memcpy(command, ssid.SSID, ssid.SSID_len);
		bytes_written = ssid.SSID_len;
	}
	bytes_written += snprintf(&command[bytes_written], total_len, " rssi %d", rssi);
	DHD_INFO(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));

#ifdef CUSTOMER_HW_ONE
	wl_android_traffic_monitor(net);
#endif
	return bytes_written;
}

static int wl_android_set_suspendopt(struct net_device *dev, char *command, int total_len)
{
	int suspend_flag;
	int ret_now;
	int ret = 0;

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
	return ret;
}

static int wl_android_set_suspendmode(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;

#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(DHD_USE_EARLYSUSPEND)
	int suspend_flag;

	suspend_flag = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';
	if (suspend_flag != 0)
		suspend_flag = 1;

#ifdef CUSTOMER_HW_ONE
	if (suspend_flag == 1)
		dhdcdc_wifiLock = 0;
	else if (suspend_flag == 0)
		dhdcdc_wifiLock = 1;
#endif
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


#if defined(PNO_SUPPORT) && !defined(WL_SCHED_SCAN) && defined(CUSTOMER_HW_ONE)
#define PARAM_SIZE 50
#define VALUE_SIZE 50
static int
wls_parse_batching_cmd(struct net_device *dev, char *command, int total_len)
{
	int err = BCME_OK;
	uint i, tokens;
	char *pos, *pos2, *token, *token2, *delim;
	char param[PARAM_SIZE], value[VALUE_SIZE];
	struct dhd_pno_batch_params batch_params;
	DHD_ERROR(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));
	if (total_len < strlen(CMD_WLS_BATCHING)) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		err = BCME_ERROR;
		goto exit;
	}
	pos = command + strlen(CMD_WLS_BATCHING) + 1;
	memset(&batch_params, 0, sizeof(struct dhd_pno_batch_params));

	if (!strncmp(pos, PNO_BATCHING_SET, strlen(PNO_BATCHING_SET))) {
		pos += strlen(PNO_BATCHING_SET) + 1;
		while ((token = strsep(&pos, PNO_PARAMS_DELIMETER)) != NULL) {
			memset(param, 0, sizeof(param));
			memset(value, 0, sizeof(value));
			if (token == NULL || !*token)
				break;
			if (*token == '\0')
				continue;
			delim = strchr(token, PNO_PARAM_VALUE_DELLIMETER);
			if (delim != NULL)
				*delim = ' ';

			tokens = sscanf(token, "%s %s", param, value);
			if (!strncmp(param, PNO_PARAM_SCANFREQ, strlen(PNO_PARAM_MSCAN))) {
				batch_params.scan_fr = simple_strtol(value, NULL, 0);
				DHD_PNO(("scan_freq : %d\n", batch_params.scan_fr));
			} else if (!strncmp(param, PNO_PARAM_BESTN, strlen(PNO_PARAM_MSCAN))) {
				batch_params.bestn = simple_strtol(value, NULL, 0);
				DHD_PNO(("bestn : %d\n", batch_params.bestn));
			} else if (!strncmp(param, PNO_PARAM_MSCAN, strlen(PNO_PARAM_MSCAN))) {
				batch_params.mscan = simple_strtol(value, NULL, 0);
				DHD_PNO(("mscan : %d\n", batch_params.mscan));
			} else if (!strncmp(param, PNO_PARAM_CHANNEL, strlen(PNO_PARAM_MSCAN))) {
				i = 0;
				pos2 = value;
				tokens = sscanf(value, "<%s>", value);
				if (tokens != 1) {
					err = BCME_ERROR;
					DHD_ERROR(("%s : invalid format for channel"
					" <> params\n", __FUNCTION__));
					goto exit;
				}
					while ((token2 = strsep(&pos2,
					PNO_PARAM_CHANNEL_DELIMETER)) != NULL) {
					if (token2 == NULL || !*token2)
						break;
					if (*token2 == '\0')
						continue;
					if (*token2 == 'A' || *token2 == 'B') {
						batch_params.band = (*token2 == 'A')?
							WLC_BAND_5G : WLC_BAND_2G;
						DHD_PNO(("band : %s\n",
							(*token2 == 'A')? "A" : "B"));
					} else {
						batch_params.chan_list[i++] =
						simple_strtol(token2, NULL, 0);
						batch_params.nchan++;
						DHD_PNO(("channel :%d\n",
						batch_params.chan_list[i-1]));
					}
				 }
			} else if (!strncmp(param, PNO_PARAM_RTT, strlen(PNO_PARAM_MSCAN))) {
				batch_params.rtt = simple_strtol(value, NULL, 0);
				DHD_PNO(("rtt : %d\n", batch_params.rtt));
			} else {
				DHD_ERROR(("%s : unknown param: %s\n", __FUNCTION__, param));
				err = BCME_ERROR;
				goto exit;
			}
		}
		err = dhd_dev_pno_set_for_batch(dev, &batch_params);
		if (err < 0) {
			DHD_ERROR(("failed to configure batch scan\n"));
		}
	} else if (!strncmp(pos, PNO_BATCHING_GET, strlen(PNO_BATCHING_GET))) {
		err = dhd_dev_pno_get_for_batch(dev, command, total_len);
		if (err < 0) {
			DHD_ERROR(("failed to getting batching results\n"));
		} else {
			err = strlen(command);
		}
	} else if (!strncmp(pos, PNO_BATCHING_STOP, strlen(PNO_BATCHING_STOP))) {
		err = dhd_dev_pno_stop_for_batch(dev);
		if (err < 0) {
			DHD_ERROR(("failed to stop batching scan\n"));
		}
	} else {
		DHD_ERROR(("%s : unknown command\n", __FUNCTION__));
		err = BCME_ERROR;
		goto exit;
	}
exit:
	return err;
}
#ifndef WL_SCHED_SCAN
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
	DHD_ERROR(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));

	if (total_len < (strlen(CMD_PNOSETUP_SET) + sizeof(cmd_tlv_t))) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		goto exit_proc;
	}
#ifdef PNO_SET_DEBUG
	memcpy(command, pno_in_example, sizeof(pno_in_example));
	total_len = sizeof(pno_in_example);
#endif
	str_ptr = command + strlen(CMD_PNOSETUP_SET);
	tlv_size_left = total_len - strlen(CMD_PNOSETUP_SET);

	cmd_tlv_temp = (cmd_tlv_t *)str_ptr;
	memset(ssids_local, 0, sizeof(ssids_local));

	if ((cmd_tlv_temp->prefix == PNO_TLV_PREFIX) &&
		(cmd_tlv_temp->version == PNO_TLV_VERSION) &&
		(cmd_tlv_temp->subtype == PNO_TLV_SUBTYPE_LEGACY_PNO)) {

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
			DHD_ERROR(("%s: pno_time=%d\n", __FUNCTION__, pno_time));

			if (str_ptr[0] != 0) {
				if ((str_ptr[0] != PNO_TLV_FREQ_REPEAT)) {
					DHD_ERROR(("%s pno repeat : corrupted field\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_repeat = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_PNO(("%s :got pno_repeat=%d\n", __FUNCTION__, pno_repeat));
				if (str_ptr[0] != PNO_TLV_FREQ_EXPO_MAX) {
					DHD_ERROR(("%s FREQ_EXPO_MAX corrupted field size\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_freq_expo_max = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_ERROR(("%s: pno_freq_expo_max=%d\n",
					__FUNCTION__, pno_freq_expo_max));
			}
		}
	} else {
		DHD_ERROR(("%s get wrong TLV command\n", __FUNCTION__));
		goto exit_proc;
	}

	res = dhd_dev_pno_set_for_ssid(dev, ssids_local, nssid, pno_time, pno_repeat,
		pno_freq_expo_max, NULL, 0);
exit_proc:
	return res;
}
#endif 
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

	error = wldev_iovar_setbuf(dev, "cckm_krk", key, sizeof(key),
		iovar_buf, WLC_IOCTL_MEDLEN, NULL);
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

	error = wldev_iovar_getbuf(dev, "assoc_info", NULL, 0, buf, WL_ASSOC_INFO_MAX, NULL);
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
	bytes_written = sizeof(u32);

	
	if (resp_ies_len) {
		error = wldev_iovar_getbuf(dev, "assoc_resp_ies", NULL, 0,
			buf, WL_ASSOC_INFO_MAX, NULL);
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

#ifndef CUSTOMER_HW_ONE
static int
wl_android_set_ap_mac_list(struct net_device *dev, int macmode, struct maclist *maclist)
{
	int i, j, match;
	int ret	= 0;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	
	if ((ret = wldev_ioctl(dev, WLC_SET_MACMODE, &macmode, sizeof(macmode), true)) != 0) {
		DHD_ERROR(("%s : WLC_SET_MACMODE error=%d\n", __FUNCTION__, ret));
		return ret;
	}
	if (macmode != MACLIST_MODE_DISABLED) {
		
		if ((ret = wldev_ioctl(dev, WLC_SET_MACLIST, maclist,
			sizeof(int) + sizeof(struct ether_addr) * maclist->count, true)) != 0) {
			DHD_ERROR(("%s : WLC_SET_MACLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		
		assoc_maclist->count = MAX_NUM_OF_ASSOCLIST;
		if ((ret = wldev_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist,
			sizeof(mac_buf), false)) != 0) {
			DHD_ERROR(("%s : WLC_GET_ASSOCLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		
		if (assoc_maclist->count) {
			
			for (i = 0; i < assoc_maclist->count; i++) {
				match = 0;
				
				for (j = 0; j < maclist->count; j++) {
					DHD_INFO(("%s : associated="MACDBG " list="MACDBG "\n",
					__FUNCTION__, MAC2STRDBG(assoc_maclist->ea[i].octet),
					MAC2STRDBG(maclist->ea[j].octet)));
					if (memcmp(assoc_maclist->ea[i].octet,
						maclist->ea[j].octet, ETHER_ADDR_LEN) == 0) {
						match = 1;
						break;
					}
				}
				
				
				if ((macmode == MACLIST_MODE_ALLOW && !match) ||
					(macmode == MACLIST_MODE_DENY && match)) {
					scb_val_t scbval;

					scbval.val = htod32(1);
					memcpy(&scbval.ea, &assoc_maclist->ea[i],
						ETHER_ADDR_LEN);
					if ((ret = wldev_ioctl(dev,
						WLC_SCB_DEAUTHENTICATE_FOR_REASON,
						&scbval, sizeof(scb_val_t), true)) != 0)
						DHD_ERROR(("%s WLC_SCB_DEAUTHENTICATE error=%d\n",
							__FUNCTION__, ret));
				}
			}
		}
	}
	return ret;
}

static int
wl_android_set_mac_address_filter(struct net_device *dev, const char* str)
{
	int i;
	int ret = 0;
	int macnum = 0;
	int macmode = MACLIST_MODE_DISABLED;
	struct maclist *list;
	char eabuf[ETHER_ADDR_STR_LEN];

	
	

	
	macmode = bcm_atoi(strsep((char**)&str, " "));

	if (macmode < MACLIST_MODE_DISABLED || macmode > MACLIST_MODE_ALLOW) {
		DHD_ERROR(("%s : invalid macmode %d\n", __FUNCTION__, macmode));
		return -1;
	}

	macnum = bcm_atoi(strsep((char**)&str, " "));
	if (macnum < 0 || macnum > MAX_NUM_MAC_FILT) {
		DHD_ERROR(("%s : invalid number of MAC address entries %d\n",
			__FUNCTION__, macnum));
		return -1;
	}
	
	list = (struct maclist*)kmalloc(sizeof(int) +
		sizeof(struct ether_addr) * macnum, GFP_KERNEL);
	if (!list) {
		DHD_ERROR(("%s : failed to allocate memory\n", __FUNCTION__));
		return -1;
	}
	
	list->count = htod32(macnum);
	bzero((char *)eabuf, ETHER_ADDR_STR_LEN);
	for (i = 0; i < list->count; i++) {
		strncpy(eabuf, strsep((char**)&str, " "), ETHER_ADDR_STR_LEN - 1);
		if (!(ret = bcm_ether_atoe(eabuf, &list->ea[i]))) {
			DHD_ERROR(("%s : mac parsing err index=%d, addr=%s\n",
				__FUNCTION__, i, eabuf));
			list->count--;
			break;
		}
		DHD_INFO(("%s : %d/%d MACADDR=%s", __FUNCTION__, i, list->count, eabuf));
	}
	
	if ((ret = wl_android_set_ap_mac_list(dev, macmode, list)) != 0)
		DHD_ERROR(("%s : Setting MAC list failed error=%d\n", __FUNCTION__, ret));

	kfree(list);

	return 0;
}
#endif

extern int msm_otg_setclk( int on);


int wl_android_wifi_on(struct net_device *dev)
{
	int ret = 0;
	int retry = POWERUP_MAX_RETRY;

	printf("%s in\n", __FUNCTION__);
	if (!dev) {
		DHD_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

#ifdef CUSTOMER_HW_ONE
#if defined(HTC_TX_TRACKING)
	old_tx_stat_chk = 0xff;
	old_tx_stat_chk_prd= 0xff;
	old_tx_stat_chk_ratio = 0xff;
	old_tx_stat_chk_num = 0xff;
	last_txframes = 0xffffffff;
	last_txretrans = 0xffffffff;
	last_txerror = 0xffffffff;
#endif
	smp_mb();

	mutex_lock(&wl_wifionoff_mutex);
#else
	dhd_net_if_lock(dev);
#endif
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
		g_wifi_on = TRUE;
	}

#ifdef CUSTOMER_HW_ONE
	if(bus_scale_table) {
		bus_perf_client =
			msm_bus_scale_register_client(bus_scale_table);
		if (!bus_perf_client)
			printf("%s: Failed to register BUS "
					"scaling client!!\n", __func__);
	}
	
exit:
	mutex_unlock(&wl_wifionoff_mutex);
#else
exit:
	dhd_net_if_unlock(dev);
#endif

	return ret;
}

int wl_android_wifi_off(struct net_device *dev)
{
	int ret = 0;

	printf("%s in\n", __FUNCTION__); 
	if (!dev) {
		DHD_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

#ifdef CUSTOMER_HW_ONE
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
#else
	dhd_net_if_lock(dev);
#endif
	if (g_wifi_on) {
		ret = dhd_dev_reset(dev, TRUE);
		sdioh_stop(NULL);
		dhd_customer_gpio_wlan_ctrl(WLAN_RESET_OFF);
		g_wifi_on = FALSE;
	}
#ifdef CUSTOMER_HW_ONE
	wlan_unlock_multi_core(dev);
	wlan_unlock_perf();
	if (bus_perf_client)
		msm_bus_scale_unregister_client(bus_perf_client);

	
	mutex_unlock(&wl_wifionoff_mutex);
	bcm_mdelay(500);
#else
	dhd_net_if_unlock(dev);
#endif
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

	wldev_iovar_setint(dev, "ccx_enable", 0);

	return error;
}



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

int wl_android_set_ibss_beacon_ouidata(struct net_device *dev, char *command, int total_len)
{
	char ie_buf[VNDR_IE_MAX_LEN];
	char *ioctl_buf = NULL;
	char hex[] = "XX";
	char *pcmd = NULL;
	int ielen = 0, datalen = 0, idx = 0, tot_len = 0;
	vndr_ie_setbuf_t *vndr_ie = NULL;
	s32 iecount;
	uint32 pktflag;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	s32 err = BCME_OK;

	if (wl_cfg80211_ibss_vsie_delete(dev) != BCME_OK) {
		return -EINVAL;
	}

	pcmd = command + strlen(CMD_SETIBSSBEACONOUIDATA) + 1;
	for (idx = 0; idx < DOT11_OUI_LEN; idx++) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx] =  (uint8)simple_strtoul(hex, NULL, 16);
	}
	pcmd++;
	while ((*pcmd != '\0') && (idx < VNDR_IE_MAX_LEN)) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx++] =  (uint8)simple_strtoul(hex, NULL, 16);
		datalen++;
	}
	tot_len = sizeof(vndr_ie_setbuf_t) + (datalen - 1);
	vndr_ie = (vndr_ie_setbuf_t *) kzalloc(tot_len, kflags);
	if (!vndr_ie) {
		WL_ERR(("IE memory alloc failed\n"));
		return -ENOMEM;
	}
	
	strncpy(vndr_ie->cmd, "add", VNDR_IE_CMD_LEN - 1);
	vndr_ie->cmd[VNDR_IE_CMD_LEN - 1] = '\0';

	
	iecount = htod32(1);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.iecount, &iecount, sizeof(s32));

	
	pktflag = htod32(VNDR_IE_BEACON_FLAG | VNDR_IE_PRBRSP_FLAG);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].pktflag, &pktflag,
		sizeof(u32));
	
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.id = (uchar) DOT11_MNG_PROPR_ID;

	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.oui, &ie_buf,
		DOT11_OUI_LEN);
	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.data,
		&ie_buf[DOT11_OUI_LEN], datalen);

	ielen = DOT11_OUI_LEN + datalen;
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.len = (uchar) ielen;

	ioctl_buf = kmalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (!ioctl_buf) {
		WL_ERR(("ioctl memory alloc failed\n"));
		if (vndr_ie) {
			kfree(vndr_ie);
		}
		return -ENOMEM;
	}
	memset(ioctl_buf, 0, WLC_IOCTL_MEDLEN);	
	err = wldev_iovar_setbuf(dev, "ie", vndr_ie, tot_len, ioctl_buf, WLC_IOCTL_MEDLEN, NULL);


	if (err != BCME_OK) {
		err = -EINVAL;
		if (vndr_ie) {
			kfree(vndr_ie);
		}
	}
	else {
		
		wl_cfg80211_ibss_vsie_set_buffer(vndr_ie, tot_len);
	}

	if (ioctl_buf) {
		kfree(ioctl_buf);
	}

	return err;
}

static int
wl_android_iolist_add(struct net_device *dev, struct list_head *head, struct io_cfg *config)
{
	struct io_cfg *resume_cfg;
	s32 ret;

	resume_cfg = kzalloc(sizeof(struct io_cfg), GFP_KERNEL);
	if (!resume_cfg)
		return -ENOMEM;

	if (config->iovar) {
		ret = wldev_iovar_getint(dev, config->iovar, &resume_cfg->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to get current %s value\n",
				__FUNCTION__, config->iovar));
			goto error;
		}

		ret = wldev_iovar_setint(dev, config->iovar, config->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}

		resume_cfg->iovar = config->iovar;
	} else {
		resume_cfg->arg = kzalloc(config->len, GFP_KERNEL);
		if (!resume_cfg->arg) {
			ret = -ENOMEM;
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl, resume_cfg->arg, config->len, false);
		if (ret) {
			DHD_ERROR(("%s: Failed to get ioctl %d\n", __FUNCTION__,
				config->ioctl));
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl + 1, config->arg, config->len, true);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}
		if (config->ioctl + 1 == WLC_SET_PM)
			wl_cfg80211_update_power_mode(dev);
		resume_cfg->ioctl = config->ioctl;
		resume_cfg->len = config->len;
	}

	list_add(&resume_cfg->list, head);

	return 0;
error:
	kfree(resume_cfg->arg);
	kfree(resume_cfg);
	return ret;
}

static void
wl_android_iolist_resume(struct net_device *dev, struct list_head *head)
{
	struct io_cfg *config;
	struct list_head *cur, *q;
	s32 ret = 0;

	list_for_each_safe(cur, q, head) {
		config = list_entry(cur, struct io_cfg, list);
		if (config->iovar) {
			if (!ret)
				ret = wldev_iovar_setint(dev, config->iovar,
					config->param);
		} else {
			if (!ret)
				ret = wldev_ioctl(dev, config->ioctl + 1,
					config->arg, config->len, true);
			if (config->ioctl + 1 == WLC_SET_PM)
				wl_cfg80211_update_power_mode(dev);
			kfree(config->arg);
		}
		list_del(cur);
		kfree(config);
	}
}

static int
wl_android_set_miracast(struct net_device *dev, char *command, int total_len)
{
	int mode, val;
	int ret = 0;
	struct io_cfg config;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	DHD_INFO(("%s: enter miracast mode %d\n", __FUNCTION__, mode));

	if (miracast_cur_mode == mode)
		return 0;

	wl_android_iolist_resume(dev, &miracast_resume_list);
	miracast_cur_mode = MIRACAST_MODE_OFF;

	switch (mode) {
	case MIRACAST_MODE_SOURCE:
		
		config.iovar = "mchan_algo";
		config.param = MIRACAST_MCHAN_ALGO;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		
		config.iovar = "mchan_bw";
		config.param = MIRACAST_MCHAN_BW;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		
		config.iovar = "ampdu_mpdu";
		config.param = MIRACAST_AMPDU_SIZE;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;
		
	case MIRACAST_MODE_SINK:
		
		config.iovar = "roam_off";
		config.param = 1;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;
		
		val = 0;
		config.iovar = NULL;
		config.ioctl = WLC_GET_PM;
		config.arg = &val;
		config.len = sizeof(int);
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		break;
	case MIRACAST_MODE_OFF:
	default:
		break;
	}
	miracast_cur_mode = mode;

	return 0;

resume:
	DHD_ERROR(("%s: turnoff miracast mode because of err%d\n", __FUNCTION__, ret));
	wl_android_iolist_resume(dev, &miracast_resume_list);
	return ret;
}

int wl_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd)
{
#define PRIVATE_COMMAND_MAX_LEN	8192
	int ret = 0;
	char *command = NULL;
	int bytes_written = 0;
	android_wifi_priv_cmd priv_cmd;
#if defined(CUSTOMER_HW_ONE) && defined(BCM4329_LOW_POWER)
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

	DHD_ERROR(("%s: Android private cmd \"%s\" on %s\n", __FUNCTION__, command, ifr->ifr_name));

	if (strnicmp(command, CMD_START, strlen(CMD_START)) == 0) {
		DHD_INFO(("%s, Received regular START command\n", __FUNCTION__));
		bytes_written = wl_android_wifi_on(net);
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
		bytes_written = wl_android_wifi_off(net);
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
#if 1
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
#else
		bytes_written = net_os_enable_packet_filter(net, 1);
#endif
	}
	else if (strnicmp(command, CMD_RXFILTER_STOP, strlen(CMD_RXFILTER_STOP)) == 0) {
#if 1
				snprintf(command, 3, "OK");
				bytes_written = strlen("OK");
#else
		bytes_written = net_os_enable_packet_filter(net, 0);
#endif
	}
	else if (strnicmp(command, CMD_RXFILTER_ADD, strlen(CMD_RXFILTER_ADD)) == 0) {
		
#ifdef CUSTOMER_HW_ONE
#ifdef BCM4329_LOW_POWER
		if (LowPowerMode == 1) {
			data = (struct dd_pkt_filter_s *)&command[32];
			if ((data->id == ALLOW_IPV6_MULTICAST) || (data->id == ALLOW_IPV4_MULTICAST)) {
				WL_TRACE(("RXFILTER-ADD MULTICAST filter\n"));
				allowMulticast = false;
			}
		}
#endif
		
		DHD_ERROR(("RXFILTER-ADD MULTICAST filter\n"));
		wl_android_enable_pktfilter(net,1);
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");		
#else
		int filter_num = *(command + strlen(CMD_RXFILTER_ADD) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, TRUE, filter_num);
#endif
	}
	else if (strnicmp(command, CMD_RXFILTER_REMOVE, strlen(CMD_RXFILTER_REMOVE)) == 0) {
#ifdef CUSTOMER_HW_ONE
#ifdef BCM4329_LOW_POWER
		if (LowPowerMode == 1) {
			data = (struct dd_pkt_filter_s *)&command[32];
			if ((data->id == ALLOW_IPV6_MULTICAST) || (data->id == ALLOW_IPV4_MULTICAST)) {
				WL_TRACE(("RXFILTER-REMOVE MULTICAST filter\n"));
				allowMulticast = true;
			}
		}
#endif
		
		DHD_ERROR(("RXFILTER-REMOVE MULTICAST filter\n"));
		wl_android_enable_pktfilter(net,0);
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
#else
		int filter_num = *(command + strlen(CMD_RXFILTER_REMOVE) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, FALSE, filter_num);
#endif
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
#ifdef WL_CFG80211
	
	else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
#ifdef CUSTOMER_HW_ONE
		char country_code[3];
		country_code[0] = *(command + strlen(CMD_COUNTRY) + 1);
		country_code[1] = *(command + strlen(CMD_COUNTRY) + 2);
		country_code[2] = '\0';
#else
		char *country_code = command + strlen(CMD_COUNTRY) + 1;
#endif
		bytes_written = wldev_set_country(net, country_code, true, true);
	}
#endif 


#if defined(PNO_SUPPORT) && !defined(WL_SCHED_SCAN) && defined(CUSTOMER_HW_ONE)
	else if (strnicmp(command, CMD_PNOSSIDCLR_SET, strlen(CMD_PNOSSIDCLR_SET)) == 0) {
		bytes_written = dhd_dev_pno_stop_for_ssid(net);
	}
#ifndef WL_SCHED_SCAN
	else if (strnicmp(command, CMD_PNOSETUP_SET, strlen(CMD_PNOSETUP_SET)) == 0) {
		bytes_written = wl_android_set_pno_setup(net, command, priv_cmd.total_len);
	}
#endif 
	else if (strnicmp(command, CMD_PNOENABLE_SET, strlen(CMD_PNOENABLE_SET)) == 0) {
		int enable = *(command + strlen(CMD_PNOENABLE_SET) + 1) - '0';
        printf("enter %s enable = %d\n ",__FUNCTION__,enable);
		bytes_written = (enable)? 0 : dhd_dev_pno_stop_for_ssid(net);
	}
	else if (strnicmp(command, CMD_WLS_BATCHING, strlen(CMD_WLS_BATCHING)) == 0) {
		bytes_written = wls_parse_batching_cmd(net, command, priv_cmd.total_len);
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
		u8 *cmd_id = NULL;
		int len;

		cmd_id = strsep((char **)&buf, " ");
		
		if (buf == NULL)
			len = 0;
		else
			len = strlen(buf);

		bytes_written = wl_cfg80211_sd_offload(net, cmd_id, buf, len);
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
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_SET_AP_WPS_P2P_IE,
		strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
		int skip = strlen(CMD_SET_AP_WPS_P2P_IE) + 3;
		bytes_written = wl_cfg80211_set_wps_p2p_ie(net, command + skip,
			priv_cmd.total_len - skip, *(command + skip - 2) - '0');
	}
#endif 
	else if (strnicmp(command, CMD_OKC_SET_PMK, strlen(CMD_OKC_SET_PMK)) == 0)
		bytes_written = wl_android_set_pmk(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_OKC_ENABLE, strlen(CMD_OKC_ENABLE)) == 0)
		bytes_written = wl_android_okc_enable(net, command, priv_cmd.total_len);
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
#if defined(WL_SUPPORT_AUTO_CHANNEL)
	else if (strnicmp(command, CMD_GET_BEST_CHANNELS,
		strlen(CMD_GET_BEST_CHANNELS)) == 0) {
		bytes_written = wl_cfg80211_get_best_channels(net, command,
			priv_cmd.total_len);
	}
#endif 
#ifndef CUSTOMER_HW_ONE
	else if (strnicmp(command, CMD_HAPD_MAC_FILTER, strlen(CMD_HAPD_MAC_FILTER)) == 0) {
		int skip = strlen(CMD_HAPD_MAC_FILTER) + 1;
		wl_android_set_mac_address_filter(net, (const char*)command+skip);
	}
#endif
	else if (strnicmp(command, CMD_SETROAMMODE, strlen(CMD_SETROAMMODE)) == 0)
		bytes_written = wl_android_set_roam_mode(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_MIRACAST, strlen(CMD_MIRACAST)) == 0)
		bytes_written = wl_android_set_miracast(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_SETIBSSBEACONOUIDATA,
		strlen(CMD_SETIBSSBEACONOUIDATA)) == 0)
		bytes_written = wl_android_set_ibss_beacon_ouidata(net, command,
			priv_cmd.total_len);
#ifdef CUSTOMER_HW_ONE
#if defined(HTC_TX_TRACKING)
	else if (strnicmp(command, CMD_TX_TRACKING, strlen(CMD_TX_TRACKING)) == 0) {
		bytes_written = wl_android_set_tx_tracking(net, command, priv_cmd.total_len);
	}
#endif
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_MAC_ADDR, strlen(CMD_MAC_ADDR)) == 0) {
		bytes_written = wl_android_get_mac_addr(net, command, priv_cmd.total_len);
	}
#endif
	else if (strnicmp(command, CMD_GET_TX_FAIL, strlen(CMD_GET_TX_FAIL)) == 0) {
		bytes_written = wl_android_get_tx_fail(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_P2P_SET_MPC, strlen(CMD_P2P_SET_MPC)) == 0) {
		int skip = strlen(CMD_P2P_SET_MPC) + 1;
		bytes_written = wl_cfg80211_set_mpc(net, command + skip, priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_DEAUTH_STA, strlen(CMD_DEAUTH_STA)) == 0) {
		int skip = strlen(CMD_DEAUTH_STA) + 1;
		bytes_written = wl_cfg80211_deauth_sta(net, command + skip, priv_cmd.total_len - skip);
	} 
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
		printf("%s CMD_SET_BCN_TIMEOUT\n",__FUNCTION__);
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
		printf("%s CMD_SET_WSEC\n",__FUNCTION__);
		bytes_written = wldev_set_ap_sta_registra_wsec(net, command, priv_cmd.total_len);  
	}
	else if (strnicmp(command, CMD_WPS_RESULT, strlen(CMD_WPS_RESULT)) == 0) {
		unsigned char result;
		result = *(command + PROFILE_OFFSET);
		printf("%s WPS_RESULT result = %d\n",__FUNCTION__,result);
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

#ifdef ENABLE_INSMOD_NO_FW_LOAD
	dhd_download_fw_on_driverload = FALSE;
#endif 
#if defined(CUSTOMER_HW2)
	if (!iface_name[0]) {
		memset(iface_name, 0, IFNAMSIZ);
		bcm_strncpy_s(iface_name, IFNAMSIZ, "wlan", IFNAMSIZ);
	}
#endif 
#ifdef CUSTOMER_HW_ONE
	mutex_init(&wl_wificall_mutex);
	mutex_init(&wl_wifionoff_mutex);
	wlan_init_perf();
	dhd_msg_level |= DHD_ERROR_VAL;
#endif
#ifdef WL_GENL
	wl_genl_init();
#endif

	return ret;
}

int wl_android_exit(void)
{
	int ret = 0;

#ifdef CUSTOMER_HW_ONE
	wlan_deinit_perf();
#endif
#ifdef WL_GENL
	wl_genl_deinit();
#endif 

	return ret;
}

void wl_android_post_init(void)
{

#ifdef ENABLE_4335BT_WAR
	bcm_bt_unlock(lock_cookie_wifi);
	printk("%s: btlock released\n", __FUNCTION__);
#endif 

	if (!dhd_download_fw_on_driverload) {
		
		dhd_customer_gpio_wlan_ctrl(WLAN_RESET_OFF);
		g_wifi_on = FALSE;
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

s32 wl_event_to_bcm_event(u16 event_type)
{
	u16 event = -1;

	switch (event_type) {
		case WLC_E_SERVICE_FOUND:
			event = BCM_E_SVC_FOUND;
			break;
		case WLC_E_P2PO_ADD_DEVICE:
			event = BCM_E_DEV_FOUND;
			break;
		case WLC_E_P2PO_DEL_DEVICE:
			event = BCM_E_DEV_LOST;
			break;
	

		default:
			WL_ERR(("Event not supported\n"));
	}

	return event;
}

s32
wl_genl_send_msg(
	struct net_device *ndev,
	u32 event_type,
	u8 *buf,
	u16 len,
	u8 *subhdr,
	u16 subhdr_len)
{
	int ret = 0;
	struct sk_buff *skb;
	void *msg;
	u32 attr_type = 0;
	bcm_event_hdr_t *hdr = NULL;
	int mcast = 1; 
	int pid = 0;
	u8 *ptr = NULL, *p = NULL;
	u32 tot_len = sizeof(bcm_event_hdr_t) + subhdr_len + len;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;


	WL_DBG(("Enter \n"));

	
	if (event_type == 0)
		attr_type = BCM_GENL_ATTR_STRING;
	else
		attr_type = BCM_GENL_ATTR_MSG;

	skb = genlmsg_new(NLMSG_GOODSIZE, kflags);
	if (skb == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	msg = genlmsg_put(skb, 0, 0, &wl_genl_family, 0, BCM_GENL_CMD_MSG);
	if (msg == NULL) {
		ret = -ENOMEM;
		goto out;
	}


	if (attr_type == BCM_GENL_ATTR_STRING) {
		if (subhdr || subhdr_len) {
			WL_ERR(("No sub hdr support for the ATTR STRING type \n"));
			ret =  -EINVAL;
			goto out;
		}

		ret = nla_put_string(skb, BCM_GENL_ATTR_STRING, buf);
		if (ret != 0) {
			WL_ERR(("nla_put_string failed\n"));
			goto out;
		}
	} else {
		

		
		p = ptr = kzalloc(tot_len, kflags);
		if (!ptr) {
			ret = -ENOMEM;
			WL_ERR(("ENOMEM!!\n"));
			goto out;
		}

		
		hdr = (bcm_event_hdr_t *)ptr;
		hdr->event_type = wl_event_to_bcm_event(event_type);
		hdr->len = len + subhdr_len;
		ptr += sizeof(bcm_event_hdr_t);

		
		if (subhdr && subhdr_len) {
			memcpy(ptr, subhdr, subhdr_len);
			ptr += subhdr_len;
		}

		
		if (buf && len) {
			memcpy(ptr, buf, len);
		}

		ret = nla_put(skb, BCM_GENL_ATTR_MSG, tot_len, p);
		if (ret != 0) {
			WL_ERR(("nla_put_string failed\n"));
			goto out;
		}
	}

	if (mcast) {
		int err = 0;
		
		genlmsg_end(skb, msg);
		
		if ((err = genlmsg_multicast(skb, 0, wl_genl_mcast.id, GFP_ATOMIC)) < 0)
			WL_ERR(("genlmsg_multicast for attr(%d) failed. Error:%d \n",
				attr_type, err));
		else
			WL_DBG(("Multicast msg sent successfully. attr_type:%d len:%d \n",
				attr_type, tot_len));
	} else {
		NETLINK_CB(skb).dst_group = 0; 

		
		genlmsg_end(skb, msg);

		
		if (genlmsg_unicast(&init_net, skb, pid) < 0)
			WL_ERR(("genlmsg_unicast failed\n"));
	}

out:
	if (p)
		kfree(p);
	if (ret)
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
		
#if !defined(WL_CFG80211_P2P_DEV_IF) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
		WL_DBG(("%s: Data received from pid (%d) \n", __func__,
			info->snd_pid));
#else
		WL_DBG(("%s: Data received from pid (%d) \n", __func__,
			info->snd_portid));
#endif 
	}

	return 0;
}
#endif 

#if defined(CONFIG_WIFI_CONTROL_FUNC)

bool g_wifi_poweron = FALSE;
static int g_wifidev_registered = 0;
static struct semaphore wifi_control_sem;
static struct wifi_platform_data *wifi_control_data = NULL;
static struct resource *wifi_irqres = NULL;
static struct regulator *wifi_regulator = NULL;

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
	int ret = 0;
#ifdef CUSTOMER_HW_ONE
	struct mmc_host *mmc;
	struct msmsdcc_host *host;
#endif
	DHD_ERROR(("%s = %d\n", __FUNCTION__, on));
	printf("%s = %d\n", __FUNCTION__, on);
	if (wifi_regulator && on)
		ret = regulator_enable(wifi_regulator);
	if (wifi_control_data && wifi_control_data->set_power) {
#ifdef ENABLE_4335BT_WAR
		if (on) {
			printk("WiFi: trying to acquire BT lock\n");
			if (bcm_bt_lock(lock_cookie_wifi) != 0)
				printk("** WiFi: timeout in acquiring bt lock**\n");
			printk("%s: btlock acquired\n", __FUNCTION__);
		}
		else {
			
			bcm_bt_unlock(lock_cookie_wifi);
		}
#endif 
		ret = wifi_control_data->set_power(on);
	}

	if (wifi_regulator && !on)
		ret = regulator_disable(wifi_regulator);

#ifdef CUSTOMER_HW_ONE
	if (!on) {
		if (gInstance && gInstance->func[0] && gInstance->func[0]->card) {
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
#endif
	if (msec && !ret)
		OSL_SLEEP(msec);
	return ret;
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

static struct resource *get_wifi_irqres_from_of(struct platform_device *pdev)
{
	static struct resource gpio_wifi_irqres;
	int irq;
	int gpio = of_get_gpio(pdev->dev.of_node, 0);
	if (gpio < 0)
		return NULL;
	irq = gpio_to_irq(gpio);
	if (irq < 0)
		return NULL;

	gpio_wifi_irqres.name = "bcmdhd_wlan_irq";
	gpio_wifi_irqres.start = irq;
	gpio_wifi_irqres.end = irq;
	gpio_wifi_irqres.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL |
		IORESOURCE_IRQ_SHAREABLE;

	return &gpio_wifi_irqres;
}

static int wifi_probe(struct platform_device *pdev)
{
	int err;
	struct regulator *regulator;
	struct wifi_platform_data *wifi_ctrl =
		(struct wifi_platform_data *)(pdev->dev.platform_data);

	if (!wifi_ctrl) {
		regulator = regulator_get(&pdev->dev, "wlreg_on");
		if (IS_ERR(regulator))
			return PTR_ERR(regulator);
		wifi_regulator = regulator;
	}

	wifi_irqres = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "bcmdhd_wlan_irq");
	if (wifi_irqres == NULL)
		wifi_irqres = platform_get_resource_byname(pdev,
			IORESOURCE_IRQ, "bcm4329_wlan_irq");
	if (wifi_irqres == NULL)
		wifi_irqres = get_wifi_irqres_from_of(pdev);
	wifi_control_data = wifi_ctrl;
#ifdef CUSTOMER_HW_ONE
	if (wifi_control_data)
		bus_scale_table = wifi_control_data->bus_scale_table;
#endif
	err = wifi_set_power(1, 200);	
	if (unlikely(err)) {
		DHD_ERROR(("%s: set_power failed. err=%d\n", __FUNCTION__, err));
		wifi_set_power(0, WIFI_TURNOFF_DELAY);
		
	} else {
		wifi_set_carddetect(1);	
		g_wifi_poweron = TRUE;
	}

	up(&wifi_control_sem);
	return 0;
}

static int wifi_remove(struct platform_device *pdev)
{
	struct wifi_platform_data *wifi_ctrl =
		(struct wifi_platform_data *)(pdev->dev.platform_data);
	struct io_cfg *cur, *q;

	DHD_ERROR(("## %s\n", __FUNCTION__));
	wifi_control_data = wifi_ctrl;

	if (g_wifi_poweron) {
	wifi_set_power(0, WIFI_TURNOFF_DELAY);	
	wifi_set_carddetect(0);	
		g_wifi_poweron = FALSE;
		list_for_each_entry_safe(cur, q, &miracast_resume_list, list) {
			list_del(&cur->list);
			kfree(cur);
		}
	}
	if (wifi_regulator) {
		regulator_put(wifi_regulator);
		wifi_regulator = NULL;
	}

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

static const struct of_device_id wifi_device_dt_match[] = {
	{ .compatible = "android,bcmdhd_wlan", },
	{},
};
MODULE_DEVICE_TABLE(of, wifi_device_dt_match);

static struct platform_driver wifi_device = {
	.probe          = wifi_probe,
	.remove         = wifi_remove,
	.suspend        = wifi_suspend,
	.resume         = wifi_resume,
	.driver         = {
	.name   = "bcmdhd_wlan",
	.of_match_table = wifi_device_dt_match,
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

#ifdef CUSTOMER_HW_ONE
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

static void wlan_init_perf(void)
{
#ifdef CONFIG_PERFLOCK
	wlan_perf_lock = perflock_acquire("bcmdhd");
	perf_lock_init(wlan_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "bcmdhd");
#endif
}

static void wlan_deinit_perf(void)
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
	char buf[32];

	sprintf(buf, "PERF_LOCK cpu=%u", (nr_cpu_ids <= 2)? nr_cpu_ids: 2);
	wl_cfg80211_send_priv_event(dev, buf);
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
    unsigned long jiffies_diff = 0;

	
	dhd_get_txrx_stats(dev, &rx_packets_count, &tx_packets_count);
	current_traffic_count = rx_packets_count + tx_packets_count;

	if ((current_traffic_count >= last_traffic_count && jiffies > last_traffic_count_jiffies) || screen_off || !sta_connected) {
        
        if (screen_off || !sta_connected) {
            printf("set traffic = 0 and relase performace lock when %s", screen_off? "screen off": "disconnected");
            traffic_diff = 0;
        }
        else {
        
            jiffies_diff = jiffies - last_traffic_count_jiffies;
            if (jiffies_diff < 7*HZ) {
                traffic_diff = (current_traffic_count - last_traffic_count) / jiffies_diff * HZ;
            }
            else {
                traffic_diff = 0;
            }
        }
        switch (traffic_stats_flag) {
        case TRAFFIC_STATS_NORMAL:
			if (traffic_diff > TRAFFIC_HIGH_WATER_MARK) {
				traffic_stats_flag = TRAFFIC_STATS_HIGH;
				wlan_lock_perf();
				printf("lock cpu here, traffic-count=%ld\n", traffic_diff);
                if (traffic_diff > TRAFFIC_SUPER_HIGH_WATER_MARK) {
                    traffic_stats_flag = TRAFFIC_STATS_SUPER_HIGH;
                    wlan_lock_multi_core(dev);
                    printf("lock 2nd cpu here, traffic-count=%ld\n", traffic_diff);
                }
			}
            break;
        case TRAFFIC_STATS_HIGH:
            if (traffic_diff > TRAFFIC_SUPER_HIGH_WATER_MARK) {
                traffic_stats_flag = TRAFFIC_STATS_SUPER_HIGH;
                wlan_lock_multi_core(dev);
				printf("lock 2nd cpu here, traffic-count=%ld\n", traffic_diff);
            }
            else if (traffic_diff < TRAFFIC_LOW_WATER_MARK) {
				traffic_stats_flag = TRAFFIC_STATS_NORMAL;
				wlan_unlock_perf();
				printf("unlock cpu here, traffic-count=%ld\n", traffic_diff);
			}
            break;
        case TRAFFIC_STATS_SUPER_HIGH:
			if (traffic_diff < TRAFFIC_SUPER_HIGH_WATER_MARK) {
                traffic_stats_flag = TRAFFIC_STATS_HIGH;
                wlan_unlock_multi_core(dev);
				printf("unlock 2nd cpu here, traffic-count=%ld\n", traffic_diff);
                if (traffic_diff < TRAFFIC_LOW_WATER_MARK) {
                    traffic_stats_flag = TRAFFIC_STATS_NORMAL;
                    wlan_unlock_perf();
                    printf("unlock cpu here, traffic-count=%ld\n", traffic_diff);
                }
            }
            break;
        default:
            break;
        }
	}
    last_traffic_count_jiffies = jiffies;
	last_traffic_count = current_traffic_count;
	
}

#if defined(HTC_TX_TRACKING)
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

#ifdef CUSTOMER_HW_ONE
	if (!sta_event_sent && assoc_count_buff && (assoc_count_buff != maclist->count)) {
		wl_cfg80211_send_priv_event(dev, "STA_LEAVE");
	}

	assoc_count_buff = maclist->count;
	sta_event_sent = 0;
#endif
	memset(buf, 0x0, len);
	memcpy(buf, mac_lst, len);
	return len;
}

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
		
		case 40:
			dhdhtc_set_power_control(0, DHDHTC_POWER_CTRL_KDDI_APK);
			dhdhtc_update_wifi_power_mode(screen_off);
			break;
		case 41:
			dhdhtc_set_power_control(1, DHDHTC_POWER_CTRL_KDDI_APK);
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
		default:
			DHD_ERROR(("%s: not support mode: %d\n", __func__, pm_val));
			break;

	}

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

#ifdef APSTA_CONCURRENT
static int wl_android_get_ap_status(struct net_device *net, char *command, int total_len)
{
	printf("%s: enter, command=%s\n", __FUNCTION__, command);
	
	return wldev_get_ap_status(net);
}

static int wl_android_set_ap_cfg(struct net_device *net, char *command, int total_len)
{
	int res;
	printf("%s: enter, command=%s\n", __FUNCTION__, command);

	wl_cfg80211_set_apsta_concurrent(net, TRUE);

    if((res = wldev_set_apsta_cfg(net, (command + strlen(CMD_SET_AP_CFG) + 1))) < 0){
        DHD_ERROR(("%s fail to set wldev_set_apsta_cfg res[%d], send hang.\n",__FUNCTION__,res));
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

static int wl_android_scanabort(struct net_device *net, char *command, int total_len)
{

	wldev_set_scanabort(net);
	wl_cfg80211_set_scan_abort(net);

    return 0;
}

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

#endif
