/*
 * Common function shared by Linux WEXT, cfg80211 and p2p drivers
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
 * $Id: wldev_common.c,v 1.1.4.1.2.14 2011-02-09 01:40:07 $
 */

#include <osl.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

#include <wldev_common.h>
#include <bcmutils.h>
#ifdef CUSTOMER_HW_ONE
#include <wlioctl.h>
#include <wldev_common.h>
#include <wl_cfg80211.h>
#include <wl_cfgp2p.h>

#ifdef SOFTAP
#define WL_SOFTAP(x) printf x

static struct ap_profile ap_cfg;

extern struct net_device *ap_net_dev;
extern int init_ap_profile_from_string(char *param_str, struct ap_profile *ap_cfg);
extern int set_apsta_cfg(struct net_device *dev, struct ap_profile *ap);
extern int set_ap_channel(struct net_device *dev, struct ap_profile *ap);
extern int wait_for_ap_ready(int sec);
extern void wl_iw_restart_apsta(struct ap_profile *ap);
extern int wl_iw_set_ap_security(struct net_device *dev, struct ap_profile *ap);
#endif
#ifdef APSTA_CONCURRENT
extern int is_screen_off;
#define SCAN_HOME_TIME      45
#define SCAN_ASSOC_TIME     40
#define SCAN_PASSIVE_TIME   100
#define SCAN_NPROBES        2
#define SCAN_SRL            15
#define SCAN_LRL            15

#define APSTA_SCAN_HOME_TIME        20
#define APSTA_SCAN_ASSOC_TIME       20
#define APSTA_SCAN_PASSIVE_TIME     50
#define APSTA_SCAN_NPROBES          1
#define APSTA_SCAN_SRL              7
#define APSTA_SCAN_LRL              7
#endif
int scan_suppress_flag = 0;

static s32 wldev_ioctl_no_memset(struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set);
extern int wl_softap_stop(struct net_device *dev);
extern void wl_cfg80211_abort_connecting(void);

extern struct wl_priv *wlcfg_drv_priv;
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

extern int rxglom_fail_count;
extern int max_cntl_timeout;


#endif
#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i
#define htodchanspec(i) i
#define dtohchanspec(i) i

#define	WLDEV_ERROR(args)						\
	do {										\
		printk(KERN_ERR "[WLAN] WLDEV-ERROR) %s : ", __func__);	\
		printk args;							\
	} while (0)

extern int dhd_ioctl_entry_local(struct net_device *net, wl_ioctl_t *ioc, int cmd);

s32 wldev_ioctl(
	struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set)
{
	s32 ret = 0;
	struct wl_ioctl ioc;


	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;
	ioc.set = set;

	ret = dhd_ioctl_entry_local(dev, &ioc, cmd);

	return ret;
}

static s32 wldev_mkiovar(
	s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, u32 buflen)
{
	s32 iolen = 0;

	iolen = bcm_mkiovar(iovar_name, param, paramlen, iovar_buf, buflen);
	return iolen;
}

s32 wldev_iovar_getbuf(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, struct mutex* buf_sync)
{
	s32 ret = 0;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
	ret = wldev_ioctl(dev, WLC_GET_VAR, buf, buflen, FALSE);
	if (buf_sync)
		mutex_unlock(buf_sync);
	return ret;
}


s32 wldev_iovar_setbuf(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, struct mutex* buf_sync)
{
	s32 ret = 0;
	s32 iovar_len;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	iovar_len = wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
	if (iovar_len > 0)
		ret = wldev_ioctl(dev, WLC_SET_VAR, buf, iovar_len, TRUE);
	else
		ret = BCME_BUFTOOSHORT;

	if (buf_sync)
		mutex_unlock(buf_sync);
	return ret;
}

s32 wldev_iovar_setint(
	struct net_device *dev, s8 *iovar, s32 val)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];

	val = htod32(val);
	memset(iovar_buf, 0, sizeof(iovar_buf));
	return wldev_iovar_setbuf(dev, iovar, &val, sizeof(val), iovar_buf,
		sizeof(iovar_buf), NULL);
}


s32 wldev_iovar_getint(
	struct net_device *dev, s8 *iovar, s32 *pval)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	s32 err;

	memset(iovar_buf, 0, sizeof(iovar_buf));
	err = wldev_iovar_getbuf(dev, iovar, pval, sizeof(*pval), iovar_buf,
		sizeof(iovar_buf), NULL);
	if (err == 0)
	{
		memcpy(pval, iovar_buf, sizeof(*pval));
		*pval = dtoh32(*pval);
	}
	return err;
}

s32 wldev_mkiovar_bsscfg(
	const s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, s32 buflen, s32 bssidx)
{
	const s8 *prefix = "bsscfg:";
	s8 *p;
	u32 prefixlen;
	u32 namelen;
	u32 iolen;

	if (bssidx == 0) {
		return wldev_mkiovar((s8*)iovar_name, (s8 *)param, paramlen,
			(s8 *) iovar_buf, buflen);
	}

	prefixlen = (u32) strlen(prefix); 
	namelen = (u32) strlen(iovar_name) + 1; 
	iolen = prefixlen + namelen + sizeof(u32) + paramlen;

	if (buflen < 0 || iolen > (u32)buflen)
	{
		WLDEV_ERROR(("%s: buffer is too short\n", __FUNCTION__));
		return BCME_BUFTOOSHORT;
	}

	p = (s8 *)iovar_buf;

	
	memcpy(p, prefix, prefixlen);
	p += prefixlen;

	
	memcpy(p, iovar_name, namelen);
	p += namelen;

	
	bssidx = htod32(bssidx);
	memcpy(p, &bssidx, sizeof(u32));
	p += sizeof(u32);

	
	if (paramlen)
		memcpy(p, param, paramlen);

	return iolen;

}

s32 wldev_iovar_getbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx, struct mutex* buf_sync)
{
	s32 ret = 0;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}

	wldev_mkiovar_bsscfg(iovar_name, param, paramlen, buf, buflen, bsscfg_idx);
	ret = wldev_ioctl(dev, WLC_GET_VAR, buf, buflen, FALSE);
	if (buf_sync) {
		mutex_unlock(buf_sync);
	}
	return ret;

}

s32 wldev_iovar_setbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx, struct mutex* buf_sync)
{
	s32 ret = 0;
	s32 iovar_len;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	iovar_len = wldev_mkiovar_bsscfg(iovar_name, param, paramlen, buf, buflen, bsscfg_idx);
	if (iovar_len > 0)
		ret = wldev_ioctl(dev, WLC_SET_VAR, buf, iovar_len, TRUE);
	else {
		ret = BCME_BUFTOOSHORT;
	}

	if (buf_sync) {
		mutex_unlock(buf_sync);
	}
	return ret;
}

s32 wldev_iovar_setint_bsscfg(
	struct net_device *dev, s8 *iovar, s32 val, s32 bssidx)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];

	val = htod32(val);
	memset(iovar_buf, 0, sizeof(iovar_buf));
	return wldev_iovar_setbuf_bsscfg(dev, iovar, &val, sizeof(val), iovar_buf,
		sizeof(iovar_buf), bssidx, NULL);
}


s32 wldev_iovar_getint_bsscfg(
	struct net_device *dev, s8 *iovar, s32 *pval, s32 bssidx)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	s32 err;

	memset(iovar_buf, 0, sizeof(iovar_buf));
	err = wldev_iovar_getbuf_bsscfg(dev, iovar, pval, sizeof(*pval), iovar_buf,
		sizeof(iovar_buf), bssidx, NULL);
	if (err == 0)
	{
		memcpy(pval, iovar_buf, sizeof(*pval));
		*pval = dtoh32(*pval);
	}
	return err;
}

int wldev_get_link_speed(
	struct net_device *dev, int *plink_speed)
{
	int error;

	if (!plink_speed)
		return -ENOMEM;
	error = wldev_ioctl(dev, WLC_GET_RATE, plink_speed, sizeof(int), 0);
	if (unlikely(error))
		return error;

	
	*plink_speed *= 500;
	return error;
}

int wldev_get_rssi(
	struct net_device *dev, int *prssi)
{
	scb_val_t scb_val;
	int error;

	if (!prssi)
		return -ENOMEM;
	bzero(&scb_val, sizeof(scb_val_t));

	error = wldev_ioctl(dev, WLC_GET_RSSI, &scb_val, sizeof(scb_val_t), 0);
	if (unlikely(error))
		return error;

	*prssi = dtoh32(scb_val.val);
	return error;
}

int wldev_get_ssid(
	struct net_device *dev, wlc_ssid_t *pssid)
{
	int error;

	if (!pssid)
		return -ENOMEM;
	error = wldev_ioctl(dev, WLC_GET_SSID, pssid, sizeof(wlc_ssid_t), 0);
	if (unlikely(error))
		return error;
	pssid->SSID_len = dtoh32(pssid->SSID_len);
	return error;
}

int wldev_get_band(
	struct net_device *dev, uint *pband)
{
	int error;

	error = wldev_ioctl(dev, WLC_GET_BAND, pband, sizeof(uint), 0);
	return error;
}

int wldev_set_band(
	struct net_device *dev, uint band)
{
	int error = -1;

	if ((band == WLC_BAND_AUTO) || (band == WLC_BAND_5G) || (band == WLC_BAND_2G)) {
		error = wldev_ioctl(dev, WLC_SET_BAND, &band, sizeof(band), true);
		if (!error)
			dhd_bus_band_set(dev, band);
	}
#ifdef CUSTOMER_HW_ONE
	wl_cfg80211_abort_connecting();
#endif
	return error;
}

int wldev_set_country(
	struct net_device *dev, char *country_code, bool notify, bool user_enforced)
{
	int error = -1;
	wl_country_t cspec = {{0}, 0, {0}};
	scb_val_t scbval;
	char smbuf[WLC_IOCTL_SMLEN];
#ifdef CUSTOMER_HW_ONE
	uint32 chan_buf[WL_NUMCHANNELS];
	wl_uint32_list_t *list;
	channel_info_t ci;
	int retry = 0;
	int chan = 1;
#endif
	if (!country_code)
		return error;

	bzero(&scbval, sizeof(scb_val_t));
	error = wldev_iovar_getbuf(dev, "country", NULL, 0, &cspec, sizeof(cspec), NULL);
	if (error < 0) {
		WLDEV_ERROR(("%s: get country failed = %d\n", __FUNCTION__, error));
		return error;
	}

	if ((error < 0) ||
	    (strncmp(country_code, cspec.ccode, WLC_CNTRY_BUF_SZ) != 0)) {

		if (user_enforced) {
			bzero(&scbval, sizeof(scb_val_t));
			error = wldev_ioctl(dev, WLC_DISASSOC, &scbval, sizeof(scb_val_t), true);
			if (error < 0) {
				WLDEV_ERROR(("%s: set country failed due to Disassoc error %d\n",
					__FUNCTION__, error));
				return error;
			}
		}

#ifdef CUSTOMER_HW_ONE
		error = wldev_ioctl(dev, WLC_SET_CHANNEL, &chan, sizeof(chan), 1);
		if (error < 0) {
			WLDEV_ERROR(("%s: set country failed due to channel 1 error %d\n",
				__FUNCTION__, error));
			return error;
		}
	}

get_channel_retry:
	if ((error = wldev_ioctl(dev, WLC_GET_CHANNEL, &ci, sizeof(ci), 0))) {
		WLDEV_ERROR(("%s: get channel fail!\n", __FUNCTION__));
		return error;
	}
	ci.scan_channel = dtoh32(ci.scan_channel);
	if (ci.scan_channel) {
		retry++;
		printf("%s: scan in progress, retry %d!\n", __FUNCTION__, retry);
		if (retry > 3)
			return -EBUSY;
		bcm_mdelay(1000);
		goto get_channel_retry;
	}
#endif
		cspec.rev = -1;
		memcpy(cspec.country_abbrev, country_code, WLC_CNTRY_BUF_SZ);
		memcpy(cspec.ccode, country_code, WLC_CNTRY_BUF_SZ);
		get_customized_country_code((char *)&cspec.country_abbrev, &cspec);
		error = wldev_iovar_setbuf(dev, "country", &cspec, sizeof(cspec),
			smbuf, sizeof(smbuf), NULL);
		if (error < 0) {
#ifdef CUSTOMER_HW_ONE
			if (strcmp(cspec.country_abbrev, DEF_COUNTRY_CODE) != 0) {
				strcpy(country_code, DEF_COUNTRY_CODE);
				retry = 0;
				goto get_channel_retry;
			}
			else {
#endif
				WLDEV_ERROR(("%s: set country for %s as %s rev %d failed\n",
					__FUNCTION__, country_code, cspec.ccode, cspec.rev));
				return error;
			}
#ifdef CUSTOMER_HW_ONE
		}
		
		else {
			if (strcmp(country_code, DEF_COUNTRY_CODE) != 0) {
				list = (wl_uint32_list_t *)(void *)chan_buf;
				list->count = htod32(WL_NUMCHANNELS);
				if ((error = wldev_ioctl_no_memset(dev, WLC_GET_VALID_CHANNELS, &chan_buf, sizeof(chan_buf), 0))) {
					WLDEV_ERROR(("%s: get channel list fail! %d\n", __FUNCTION__, error));
					return error;
				}
				
				printf("%s: channel_count = %d\n", __FUNCTION__, list->count);
				if (list->count == 0) {
					strcpy(country_code, DEF_COUNTRY_CODE);
					retry = 0;
					goto get_channel_retry;
				}
		}	
#endif
		dhd_bus_country_set(dev, &cspec, notify);
		WLDEV_ERROR(("%s: set country for %s as %s rev %d\n",
			__FUNCTION__, country_code, cspec.ccode, cspec.rev));
	}
#ifdef CUSTOMER_HW_ONE
	wl_cfg80211_abort_connecting();
#endif
	return 0;
}

#define VSDB_BW_ALLOCATE_ENABLE 1
int wldev_miracast_tuning(
	struct net_device *dev, int mode)
{
	int error = 0;
	int ampdu_mpdu;
	int roam_off;
#ifdef VSDB_BW_ALLOCATE_ENABLE
	int mchan_algo;
	int mchan_bw;
#endif 

	WLDEV_ERROR(("mode: %d\n", mode));

	if (mode == 0) {
		
		ampdu_mpdu = -1;	
#if defined(ROAM_ENABLE)
		roam_off = 0;	
#elif defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1;	
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 0;	
		mchan_bw = 50;	
#endif 
	}
	else if (mode == 1) {
		
		ampdu_mpdu = 8;	
#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1; 
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 1;	
		mchan_bw = 35;	
#endif 
	}
	else if (mode == 2) {
		
		ampdu_mpdu = -1;	
#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1; 
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 0;	
		mchan_bw = 50;	
#endif 
	}
	else {
		WLDEV_ERROR(("Unknown mode: %d\n", mode));
		return -1;
	}

	
	error = wldev_iovar_setint(dev, "ampdu_mpdu", ampdu_mpdu);
	if (error) {
		WLDEV_ERROR(("Failed to set ampdu_mpdu: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}

#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
	error = wldev_iovar_setint(dev, "roam_off", roam_off);
	if (error) {
		WLDEV_ERROR(("Failed to set roam_off: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}
#endif 

#ifdef VSDB_BW_ALLOCATE_ENABLE
	error = wldev_iovar_setint(dev, "mchan_algo", mchan_algo);
	if (error) {
		WLDEV_ERROR(("Failed to set mchan_algo: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}

	error = wldev_iovar_setint(dev, "mchan_bw", mchan_bw);
	if (error) {
		WLDEV_ERROR(("Failed to set mchan_bw: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}
#endif 

	return error;
}

#ifdef CUSTOMER_HW_ONE

static s32 wldev_ioctl_no_memset(
	struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set)
{
	s32 ret = 0;
	struct wl_ioctl ioc;

	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;
	ioc.set = set;
	ret = dhd_ioctl_entry_local(dev, &ioc, cmd);

	return ret;
}

#ifdef APSTA_CONCURRENT
static int wldev_set_pktfilter_enable_by_id(struct net_device *dev, int pkt_id, int enable)
{
	wl_pkt_filter_enable_t	enable_parm;
	char smbuf[WLC_IOCTL_SMLEN];
	int res;

	
	enable_parm.id = htod32(pkt_id);
	enable_parm.enable = htod32(enable);
	res = wldev_iovar_setbuf(dev, "pkt_filter_enable", &enable_parm, sizeof(enable_parm),
		smbuf, sizeof(smbuf), NULL);
	if (res < 0) {
		WLDEV_ERROR(("%s: set pkt_filter_enable failed, error:%d\n", __FUNCTION__, res));
		return res;
	}
	
	return 0;
}

int wldev_set_pktfilter_enable(struct net_device *dev, int enable)
{
	wldev_set_pktfilter_enable_by_id(dev, 100, enable);
        printf("%s: pkt_filter id:100 %s\n", __FUNCTION__, (enable)?"enable":"disable");
	wldev_set_pktfilter_enable_by_id(dev, 101, enable);
        printf("%s: pkt_filter id:101 %s\n", __FUNCTION__, (enable)?"enable":"disable");
	wldev_set_pktfilter_enable_by_id(dev, 102, enable);
        printf("%s: pkt_filter id:102 %s\n", __FUNCTION__, (enable)?"enable":"disable");
	wldev_set_pktfilter_enable_by_id(dev, 104, enable);
        printf("%s: pkt_filter id:104 %s\n", __FUNCTION__, (enable)?"enable":"disable");
    wldev_set_pktfilter_enable_by_id(dev, 105, enable);
        printf("%s: pkt_filter id:105 %s\n", __FUNCTION__, (enable)?"enable":"disable");

	if(!is_screen_off){
    wldev_set_pktfilter_enable_by_id(dev, 106, enable);
        printf("%s: pkt_filter id:106 %s\n", __FUNCTION__, (enable)?"enable":"disable");
	}
	if (!enable) {
		wldev_set_pktfilter_enable_by_id(dev, 107, enable);
		printf("%s: pkt_filter id:107 %s\n", __FUNCTION__, (enable)?"enable":"disable");

		wldev_set_pktfilter_enable_by_id(dev, 108, enable);
		printf("%s: pkt_filter id:108 %s\n", __FUNCTION__, (enable)?"enable":"disable");
	}
	return 0;
}
#ifdef SOFTAP
int
wldev_set_apsta_cfg(struct net_device *dev, char *param_str)
{
	int res = 0;

	printf("%s: enter\n", __FUNCTION__);

        if (!dev) {
                 WLDEV_ERROR(("%s: dev is null\n", __FUNCTION__));
                 return -1;
  	}
	
	init_ap_profile_from_string(param_str, &ap_cfg);

        if ((res = set_apsta_cfg(dev, &ap_cfg)) < 0)
                 WLDEV_ERROR(("%s failed to set_apsta_cfg %d\n", __FUNCTION__, res));
	
	return res;
}
void
wldev_restart_ap(struct net_device *dev)
{
	wl_iw_restart_apsta(&ap_cfg);
}

#endif  

int
wldev_get_ap_status(struct net_device *dev)
{
	int res = 0;
	int ap = 0;
	int apsta = 0;
       	char smbuf[WLC_IOCTL_SMLEN];

	printf("%s: enter\n", __FUNCTION__);

        if (!dev) {
                WLDEV_ERROR(("%s: dev is null\n", __FUNCTION__));
                return -1;
        }

        if ((res = wldev_ioctl(dev, WLC_GET_AP, &ap, sizeof(ap), 0))) {
                WLDEV_ERROR(("%s fail to get ap\n", __FUNCTION__));
		ap = 0;
	}

        if ((res = wldev_iovar_getbuf(dev, "apsta", &apsta, sizeof(apsta), smbuf, sizeof(smbuf), NULL)) < 0 ){
                WLDEV_ERROR(("%s fail to get apsta \n", __FUNCTION__));
        } else {
		memcpy(&apsta, smbuf, sizeof(apsta));
		apsta = dtoh32(apsta);
	}

	return (ap||apsta);
}

int wldev_start_stop_scansuppress(struct net_device *dev)
{
	int res = 0;
	int enable = 1;
	if (!dev) {
		WLDEV_ERROR(("%s: dev is null\n", __FUNCTION__));
		return (res = -1);;
	}

	if(scan_suppress_flag){
		if ((res = wldev_ioctl(dev, WLC_SET_SCANSUPPRESS, &enable, sizeof(enable), 1))) {
			WLDEV_ERROR(("%s fail to SET_SCANSUPPRESS enable\n", __FUNCTION__));
			return (res = -1);
		}
		printf("Successful Enable scan suppress!!\n");
		return 0;
	}
		
	return 0;
}

int
wldev_set_scansuppress(struct net_device *dev,int enable)
{
	int res = 0;
	int scan_unassoc_time ;

	printf("%s: enter\n", __FUNCTION__);

	if (!dev) {
		WLDEV_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -1;
	}

	printf("wldev_set_scansuppress enable[%d]\n", enable);

	if(enable){
		scan_unassoc_time = 20;
		
		if((res =  wldev_ioctl(dev, WLC_SET_SCAN_UNASSOC_TIME, (char *)&scan_unassoc_time,sizeof(scan_unassoc_time), 1))) {
					WLDEV_ERROR(("%s fail to set  WLC_SET_SCAN_UNASSOC_TIME\n", __FUNCTION__));
		}

		if ((res = wldev_ioctl(dev, WLC_SET_SCANSUPPRESS, &enable, sizeof(enable), 1))) {
			WLDEV_ERROR(("%s fail to get ap\n", __FUNCTION__));
		}
		scan_suppress_flag = 1;
		printf("Successful enable scan suppress!!\n");
	}else{
		scan_unassoc_time = 80;
		
		if ((res =  wldev_ioctl(dev, WLC_SET_SCAN_UNASSOC_TIME, (char *)&scan_unassoc_time,sizeof(scan_unassoc_time), 1))) {
					WLDEV_ERROR(("%s fail to set  WLC_SET_SCAN_UNASSOC_TIME\n", __FUNCTION__));
		}

		if ((res = wldev_ioctl(dev, WLC_SET_SCANSUPPRESS, &enable, sizeof(enable), 1))) {
			WLDEV_ERROR(("%s fail to get ap\n", __FUNCTION__));		
		}
		scan_suppress_flag = 0;
		
		printf("Successful disable scan suppress!!\n");
	}
	
	return 0;
}


#ifdef APSTA_CONCURRENT
void wldev_adj_apsta_scan_param(struct net_device *dev,int enable)
{
    int res;

    if(enable){
        
        int scan_home_time = APSTA_SCAN_HOME_TIME;
        int scan_assoc_time = APSTA_SCAN_ASSOC_TIME;
        int scan_passive_time = APSTA_SCAN_PASSIVE_TIME;
        int scan_nprobes = APSTA_SCAN_NPROBES;
        int srl = APSTA_SCAN_SRL;
        int lrl = APSTA_SCAN_LRL;

         
        if((res =  wldev_ioctl(dev, WLC_SET_SCAN_HOME_TIME, (char *)&scan_home_time,sizeof(scan_home_time), 1)))
            WLDEV_ERROR(("%s fail to  WLC_SET_SCAN_HOME_TIME\n", __FUNCTION__));
		    
         
        if((res =  wldev_ioctl(dev, WLC_SET_SCAN_CHANNEL_TIME, (char *)&scan_assoc_time,sizeof(scan_assoc_time), 1)))
            WLDEV_ERROR(("%s fail to WLC_SET_SCAN_CHANNEL_TIME\n", __FUNCTION__));

         
        if((res =  wldev_ioctl(dev, WLC_SET_SCAN_PASSIVE_TIME, (char *)&scan_passive_time,sizeof(scan_passive_time), 1)))
            WLDEV_ERROR(("%s fail WLC_SET_SCAN_PASSIVE_TIME\n", __FUNCTION__));
		
         
        if((res =  wldev_ioctl(dev, WLC_SET_SCAN_NPROBES, (char *)&scan_nprobes,sizeof(scan_nprobes), 1)))
            WLDEV_ERROR(("%s fail to WLC_SET_SCAN_NPROBES\n", __FUNCTION__));

         
        if((res =  wldev_ioctl(dev, WLC_SET_SRL, (char *)&srl,sizeof(srl), 1)))
            WLDEV_ERROR(("%s fail to WLC_SET_SRL\n", __FUNCTION__));
		
         
        if((res =  wldev_ioctl(dev, WLC_SET_LRL, (char *)&lrl,sizeof(lrl), 1)))
            WLDEV_ERROR(("%s fail to WLC_SET_LRL\n", __FUNCTION__));

    }else{
        
        int scan_home_time = SCAN_HOME_TIME;
        int scan_assoc_time = SCAN_ASSOC_TIME;
        int scan_passive_time = SCAN_PASSIVE_TIME;
        int scan_nprobes = SCAN_NPROBES;
        int srl = SCAN_SRL;
        int lrl = SCAN_LRL;
  
         
		if((res =  wldev_ioctl(dev, WLC_SET_SCAN_HOME_TIME, (char *)&scan_home_time,sizeof(scan_home_time), 1))) {
					WLDEV_ERROR(("%s fail to WLC_SET_SCAN_HOME_TIME\n", __FUNCTION__));
		}
         
		if((res =  wldev_ioctl(dev, WLC_SET_SCAN_CHANNEL_TIME, (char *)&scan_assoc_time,sizeof(scan_assoc_time), 1))) {
					WLDEV_ERROR(("%s fail to WLC_SET_SCAN_CHANNEL_TIME\n", __FUNCTION__));
		}
         
		if((res =  wldev_ioctl(dev, WLC_SET_SCAN_PASSIVE_TIME, (char *)&scan_passive_time,sizeof(scan_passive_time), 1))) {
					WLDEV_ERROR(("%s fail to WLC_SET_SCAN_PASSIVE_TIME\n", __FUNCTION__));
		}
         
		if((res =  wldev_ioctl(dev, WLC_SET_SCAN_NPROBES, (char *)&scan_nprobes,sizeof(scan_nprobes), 1))) {
					WLDEV_ERROR(("%s fail WLC_SET_SCAN_NPROBES\n", __FUNCTION__));
		}

         
		if((res =  wldev_ioctl(dev, WLC_SET_SRL, (char *)&srl,sizeof(srl), 1))) {
					WLDEV_ERROR(("%s fail to WLC_SET_SRL\n", __FUNCTION__));
		}
         
		if((res =  wldev_ioctl(dev, WLC_SET_LRL, (char *)&lrl,sizeof(lrl), 1))) {
					WLDEV_ERROR(("%s fail to  WLC_SET_LRL\n", __FUNCTION__));
		}

    }
    
}

s32 wldev_set_ssid(struct net_device *dev,int *channel)
{
	wlc_ssid_t ap_ssid;
	bss_setbuf_t bss_setbuf;
	char smbuf[WLC_IOCTL_SMLEN];

	int res = 0;
	printf("  %s  ap_cfg.ssid[%s]\n",__FUNCTION__, ap_cfg.ssid);
	ap_ssid.SSID_len = strlen(ap_cfg.ssid);
	strncpy(ap_ssid.SSID, ap_cfg.ssid, ap_ssid.SSID_len);

	bss_setbuf.cfg = 1;
	bss_setbuf.val = 0;  
	
	if ((res = wldev_iovar_setbuf_bsscfg(dev, "bss", &bss_setbuf, sizeof(bss_setbuf), 
			smbuf, sizeof(smbuf), 1, NULL)) < 0){
			printf("%s: ERROR:%d, set bss down failed\n", __FUNCTION__, res);
	}
	
	bcm_mdelay(50);

	res = wldev_ioctl(ap_net_dev, WLC_SET_CHANNEL, channel, sizeof(*channel), 1);
	if (res < 0) {
		printf("%s set channel fial res[%d]",__FUNCTION__,res);
	}


	bss_setbuf.cfg = 1;
	bss_setbuf.val = 1;  
	bcm_mdelay(50);

	if ((res = wldev_iovar_setbuf_bsscfg(dev, "bss", &bss_setbuf, sizeof(bss_setbuf), 
			smbuf, sizeof(smbuf), 1, NULL)) < 0){
			printf("%s: ERROR:%d, set bss up failed\n", __FUNCTION__, res);
	}
			printf("%s: up Conap part of apsta concurrent\n", __FUNCTION__);
		
	return res;

}
#endif

int
wldev_set_apsta(struct net_device *dev, bool enable)
{
   	int res = 0;
   	int mpc = 0;
   	int concr_mode = 0;
	int roam_off;
   	char smbuf[WLC_IOCTL_SMLEN];
	bss_setbuf_t bss_setbuf;
	int frameburst;

        memset(smbuf, 0, sizeof(smbuf));

	printf("%s: enter\n", __FUNCTION__);

   	if (!dev) {
                  WLDEV_ERROR(("%s: dev is null\n", __FUNCTION__));
                  return -1;
   	}

	if (enable){
		
		wait_for_ap_ready(1);

		if ( ap_net_dev == NULL ) {
                        WLDEV_ERROR(("%s ap_net_dev == NULL\n", __FUNCTION__));
                        goto fail;
		}

		concr_mode = 1;
		if ((res = wldev_iovar_setint(dev, "concr_mode_set", concr_mode))) {
			printf("%s fail to set concr_mode res[%d]\n", __FUNCTION__,res);
		}


        rxglom_fail_count = RXGLOM_CONCUR_MODE_FAIL_COUNT;
        max_cntl_timeout =  MAX_CONCUR_MODE_CNTL_TIMEOUT;


		roam_off = 1;
		if((res = wldev_iovar_setint(dev, "roam_off", roam_off)))
			printf("%s fail to set roam_off res[%d]\n", __FUNCTION__,res);
		
   		mpc = 0;
		if ((res = wldev_iovar_setint(dev, "mpc", mpc))) {
			WLDEV_ERROR(("%s fail to set mpc\n", __FUNCTION__));
			goto fail;
		}	

		if ((res = wl_iw_set_ap_security(ap_net_dev, &ap_cfg)) != 0) {
			WLDEV_ERROR((" %s ERROR setting SOFTAP security in :%d\n", __FUNCTION__, res));
			goto fail;
		} 

		if(wl_get_drv_status(wlcfg_drv_priv,CONNECTED,dev))
		{
			u32 chanspec = 0;
			int err;
			if(wldev_iovar_getint(dev, "chanspec", (s32 *)&chanspec) == BCME_OK)
			{
				printf("%s get Chanspec [%0x]\n",__func__ ,chanspec);
				if((err = wldev_iovar_setint(ap_net_dev, "chanspec", chanspec)) == BCME_BADCHAN) {
					printf("%s set Chanspec failed\n",__func__);
				} else
					printf("%s set Chanspec OK\n",__func__);
			}
			else
				printf("%s get Chanspec failed\n",__func__);

		}
		else
			printf("%s Sta is not connected with any AP\n",__func__);

			bss_setbuf.cfg = 1;
			bss_setbuf.val = 1;  

			if ((res = wldev_iovar_setbuf_bsscfg(dev, "bss", &bss_setbuf, sizeof(bss_setbuf), smbuf, sizeof(smbuf), 1, NULL)) < 0){
				WLDEV_ERROR(("%s: ERROR:%d, set bss up failed\n", __FUNCTION__, res));
				goto fail;
        	}

	        bcm_mdelay(500);

            printf("prepare set frameburst \n");
            frameburst = 1;
            if ((res = wldev_ioctl(dev, WLC_SET_FAKEFRAG, &frameburst, sizeof(frameburst), 0))) {
                printf("%s fail to set frameburst !!\n", __FUNCTION__);
            }

		if ((res = wldev_iovar_setint(dev, "allmulti", 1))) {
            		WLDEV_ERROR(("%s: ERROR:%d, set allmulti failed\n", __FUNCTION__, res));
            		goto fail;
		}

#if !defined(WLCREDALL)
		if ((res = wldev_iovar_setint(dev, "bus:credall", 1))) {
			WLDEV_ERROR(("%s: ERROR:%d, set credall failed\n", __FUNCTION__, res));
			goto fail;
		}
#endif


		set_ap_channel(dev,&ap_cfg);
		ap_net_dev->operstate = IF_OPER_UP;
	} else {
		if ((res = wl_softap_stop(ap_net_dev))){
           		WLDEV_ERROR(("%s: ERROR:%d, call wl_softap_stop failed\n", __FUNCTION__, res));
           		goto fail;
		}

		concr_mode = 0;
		if ((res = wldev_iovar_setint(dev, "concr_mode_set", concr_mode))) {
				printf("%s fail to set concr_mode res[%d]\n", __FUNCTION__,res);
			}


        rxglom_fail_count = RXGLOM_FAIL_COUNT;
        max_cntl_timeout =  MAX_CNTL_TIMEOUT;
        scan_suppress_flag = 0;


	
	roam_off = 0;
	if((res = wldev_iovar_setint(dev, "roam_off", roam_off)))
			printf("%s fail to set roam_off res[%d]\n", __FUNCTION__,res);

	mpc = 1;
	     	if ((res = wldev_iovar_setint(dev, "mpc", mpc))) {
        	   	WLDEV_ERROR(("%s fail to set mpc\n", __FUNCTION__));
           		goto fail;
	      	}

#if !defined(WLCREDALL)
		if ((res = wldev_iovar_setint(dev, "bus:credall", 0))) {
			WLDEV_ERROR(("%s fail to set credall\n", __FUNCTION__));
			goto fail;
		}
#endif

        printf("prepare set frameburst \n");
        frameburst = 0;
        if ((res = wldev_ioctl(dev, WLC_SET_FAKEFRAG, &frameburst, sizeof(frameburst), 0))) {
            printf("%s fail to set frameburst !!\n", __FUNCTION__);
        }


        wlcfg_drv_priv->dongle_connected = 0;
        wldev_adj_apsta_scan_param(dev,wlcfg_drv_priv->dongle_connected);
	}

fail:
    return res;
}

int
wldev_get_conap_ctrl_channel(struct net_device *dev,uint8 *ctrl_channel)
{
    struct wl_bss_info *bss = NULL;
    int err;
    

    	*(u32 *) wlcfg_drv_priv->extra_buf = htod32(WL_EXTRA_BUF_MAX);
    	if ((err = wldev_ioctl(ap_net_dev, WLC_GET_BSS_INFO, wlcfg_drv_priv->extra_buf,
    		WL_EXTRA_BUF_MAX, false))) {
    			printf("Failed to get hostapd bss info, use temp channel \n");
    			return -1;
    	}
    	else {
    			bss = (struct wl_bss_info *) (wlcfg_drv_priv->extra_buf + 4);
    			*ctrl_channel =  bss->ctl_ch;
    			
    			printf(" %s Valid BSS Found. ctrl_channel:%d \n",__func__ ,*ctrl_channel);
    			return 0;
    	}

     
}

#ifdef BRCM_WPSAP
int wldev_set_ap_sta_registra_wsec(struct net_device *dev, char *command, int total_len)
{
	int bytes_written = 0;
	int wsec = 0;

        if ( !ap_net_dev ) return 0;

	wldev_iovar_getint(ap_net_dev, "wsec", &wsec);
	WLDEV_ERROR(("### %s devname[%s],got wsec(bset)=0x%x\n", __FUNCTION__, ap_net_dev->name, wsec));

	wsec |= SES_OW_ENABLED;
	WLDEV_ERROR(("### %s devname[%s],wsec=0x%x\n", __FUNCTION__, ap_net_dev->name, wsec));

	wldev_iovar_setint(ap_net_dev, "wsec", wsec);
	WLDEV_ERROR(("### %s devname[%s] seting\n", __FUNCTION__, ap_net_dev->name));

	wldev_iovar_getint(ap_net_dev, "wsec", &wsec);
	WLDEV_ERROR(("### %s devname[%s],got(aset) wsec=0x%x\n", __FUNCTION__, ap_net_dev->name, wsec));

	return bytes_written;
}
#endif 


void wldev_san_check_channel(struct net_device *ndev,int *errcode)
{   
    int error = 0;
    uint8 consta_ctrl_channel = 0;
    uint8 conap_ctrl_channel = 0;
    struct wl_bss_info *bss = NULL;

    printf("Enter %s\n",__FUNCTION__);
        
    if (!ndev ){
        printf("%s NULL ConSTA netdev[%p]",__FUNCTION__,ndev);
        *errcode = 1;
            
    }   
            
    if (!ap_net_dev ){
        printf("%s NULL ConAP netdev[%p]",__FUNCTION__,ap_net_dev);
        *errcode = 2;
    }

	
	if ((error = wldev_ioctl(ndev, WLC_GET_BSS_INFO, wlcfg_drv_priv->extra_buf,
        WL_EXTRA_BUF_MAX, false))) {
        printf("Failed to get conssta bss info \n");
        *errcode = 3;
    }
    else {
        bss = (struct wl_bss_info *) (wlcfg_drv_priv->extra_buf + 4);
        consta_ctrl_channel =  bss->ctl_ch;
        printf(" %s Valid CONSTA BSS Found. consta_ctrl_channel:%d \n",__func__ ,consta_ctrl_channel);
    }
    bcm_mdelay(50);

	
    *(u32 *) wlcfg_drv_priv->extra_buf = htod32(WL_EXTRA_BUF_MAX);
    if ((error = wldev_ioctl(ap_net_dev, WLC_GET_BSS_INFO, wlcfg_drv_priv->extra_buf,
        WL_EXTRA_BUF_MAX, false))) {
            printf("Failed to get conap bss info \n");
            *errcode = 4;
    }
    else {
        bss = (struct wl_bss_info *) (wlcfg_drv_priv->extra_buf + 4);
        conap_ctrl_channel =  bss->ctl_ch;
        printf(" %s Valid CONAP BSS Found. conap_ctrl_channel:%d \n",__func__ ,conap_ctrl_channel);
    }

    if(consta_ctrl_channel == conap_ctrl_channel){
        printf("ConSTA,ConAP Ctrl Channel same \n");
        *errcode = 0;
    }
    else{
        printf("ConSTA,ConAP Channel NOT same!!! \n");
        *errcode = consta_ctrl_channel;
    }
    printf("Leave %s errcode[%d]",__FUNCTION__,*errcode);
}
#endif 

void
wldev_set_scanabort(struct net_device *dev)
{

	s8 iovar_buf[WLC_IOCTL_SMLEN];
	int ret = 0;

	memset(iovar_buf, 0, sizeof(iovar_buf));
	ret = wldev_iovar_setbuf(dev, "scanabort", NULL, 0, iovar_buf,
		sizeof(iovar_buf), NULL);
	if (ret)
		printf("%s failed ret = %d\n", __func__, ret);

}

#endif
