/*
 * DHD Protocol Module for CDC and BDC.
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
 * $Id: dhd_cdc.c 416698 2013-08-06 07:53:34Z $
 *
 * BDC is like CDC, except it includes a header for data packets to convey
 * packet priority over the bus, and flags (e.g. to indicate checksum status
 * for dongle offload.)
 */

#include <typedefs.h>
#include <osl.h>

#include <bcmutils.h>
#include <bcmcdc.h>
#include <bcmendian.h>

#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_proto.h>
#include <dhd_bus.h>
#include <dhd_dbg.h>


#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <dhd_wlfc.h>
#endif


#define RETRIES 2		
#define BUS_HEADER_LEN	(24+DHD_SDALIGN)	
#define ROUND_UP_MARGIN	2048	

typedef struct dhd_prot {
	uint16 reqid;
	uint8 pending;
	uint32 lastcmd;
	uint8 bus_header[BUS_HEADER_LEN];
	cdc_ioctl_t msg;
	unsigned char buf[WLC_IOCTL_MAXLEN + ROUND_UP_MARGIN];
} dhd_prot_t;


static int
dhdcdc_msg(dhd_pub_t *dhd)
{
	int err = 0;
	dhd_prot_t *prot = dhd->prot;
	int len = ltoh32(prot->msg.len) + sizeof(cdc_ioctl_t);

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	DHD_OS_WAKE_LOCK(dhd);

	if (len > CDC_MAX_MSG_SIZE)
		len = CDC_MAX_MSG_SIZE;

	
	err = dhd_bus_txctl(dhd->bus, (uchar*)&prot->msg, len);

	DHD_OS_WAKE_UNLOCK(dhd);
	return err;
}

static int
dhdcdc_cmplt(dhd_pub_t *dhd, uint32 id, uint32 len)
{
	int ret;
	int cdc_len = len + sizeof(cdc_ioctl_t);
	dhd_prot_t *prot = dhd->prot;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#if defined(CUSTOMER_HW4) || defined(CUSTOMER_HW2)
		DHD_OS_WAKE_LOCK(dhd);
#endif 

	if ((dhd->busstate == DHD_BUS_DOWN) || dhd->hang_was_sent || dhd->os_stopped) {
		DHD_ERROR(("%s : bus is down. we have nothing to do\n", __FUNCTION__));
#if defined(CUSTOMER_HW4) || defined(CUSTOMER_HW2)
		DHD_OS_WAKE_UNLOCK(dhd);
#endif 
		return -EIO;
	}


	do {
		ret = dhd_bus_rxctl(dhd->bus, (uchar*)&prot->msg, cdc_len);
		if (ret < 0)
			break;
	} while (CDC_IOC_ID(ltoh32(prot->msg.flags)) != id);


#if defined(CUSTOMER_HW4) || defined(CUSTOMER_HW2)
	DHD_OS_WAKE_UNLOCK(dhd);
#endif 
	return ret;
}

static int
dhdcdc_query_ioctl(dhd_pub_t *dhd, int ifidx, uint cmd, void *buf, uint len, uint8 action)
{
	dhd_prot_t *prot = dhd->prot;
	cdc_ioctl_t *msg = &prot->msg;
	int ret = 0, retries = 0;
	uint32 id, flags = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
	DHD_CTL(("%s: cmd %d len %d\n", __FUNCTION__, cmd, len));


	
	if (cmd == WLC_GET_VAR && buf)
	{
		if (!strcmp((char *)buf, "bcmerrorstr"))
		{
			strncpy((char *)buf, bcmerrorstr(dhd->dongle_error), BCME_STRLEN);
			goto done;
		}
		else if (!strcmp((char *)buf, "bcmerror"))
		{
			*(int *)buf = dhd->dongle_error;
			goto done;
		}
	}

	memset(msg, 0, sizeof(cdc_ioctl_t));

	msg->cmd = htol32(cmd);
	msg->len = htol32(len);
	msg->flags = (++prot->reqid << CDCF_IOC_ID_SHIFT);
	CDC_SET_IF_IDX(msg, ifidx);
	
	action &= WL_IOCTL_ACTION_MASK;
	msg->flags |= (action << CDCF_IOC_ACTION_SHIFT);
	msg->flags = htol32(msg->flags);

	if (buf)
		memcpy(prot->buf, buf, len);

	if ((ret = dhdcdc_msg(dhd)) < 0) {
		if (!dhd->hang_was_sent)
		DHD_ERROR(("dhdcdc_query_ioctl: dhdcdc_msg failed w/status %d\n", ret));
		goto done;
	}

retry:
	
	if ((ret = dhdcdc_cmplt(dhd, prot->reqid, len)) < 0)
		goto done;

	flags = ltoh32(msg->flags);
	id = (flags & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT;

	if ((id < prot->reqid) && (++retries < RETRIES))
		goto retry;
	if (id != prot->reqid) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, ifidx), __FUNCTION__, id, prot->reqid));
		ret = -EINVAL;
		goto done;
	}

	
	if (buf)
	{
		if (ret < (int)len)
			len = ret;
		memcpy(buf, (void*) prot->buf, len);
	}

	
	if (flags & CDCF_IOC_ERROR)
	{
		ret = ltoh32(msg->status);
		
		dhd->dongle_error = ret;
	}

done:
	return ret;
}


static int
dhdcdc_set_ioctl(dhd_pub_t *dhd, int ifidx, uint cmd, void *buf, uint len, uint8 action)
{
	dhd_prot_t *prot = dhd->prot;
	cdc_ioctl_t *msg = &prot->msg;
	int ret = 0;
	uint32 flags, id;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
	DHD_CTL(("%s: cmd %d len %d\n", __FUNCTION__, cmd, len));

	if (dhd->busstate == DHD_BUS_DOWN) {
		DHD_ERROR(("%s : bus is down. we have nothing to do\n", __FUNCTION__));
		return -EIO;
	}

	
	if (dhd->hang_was_sent) {
		DHD_ERROR(("%s: HANG was sent up earlier. Not talking to the chip\n",
			__FUNCTION__));
		return -EIO;
	}


	memset(msg, 0, sizeof(cdc_ioctl_t));

	msg->cmd = htol32(cmd);
	msg->len = htol32(len);
	msg->flags = (++prot->reqid << CDCF_IOC_ID_SHIFT);
	CDC_SET_IF_IDX(msg, ifidx);
	
	action &= WL_IOCTL_ACTION_MASK;
	msg->flags |= (action << CDCF_IOC_ACTION_SHIFT) | CDCF_IOC_SET;
	msg->flags = htol32(msg->flags);

	if (buf)
		memcpy(prot->buf, buf, len);

	if ((ret = dhdcdc_msg(dhd)) < 0) {
		DHD_ERROR(("%s: dhdcdc_msg failed w/status %d\n", __FUNCTION__, ret));
		goto done;
	}

	if ((ret = dhdcdc_cmplt(dhd, prot->reqid, len)) < 0)
		goto done;

	flags = ltoh32(msg->flags);
	id = (flags & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT;

	if (id != prot->reqid) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, ifidx), __FUNCTION__, id, prot->reqid));
		ret = -EINVAL;
		goto done;
	}

	
	if (flags & CDCF_IOC_ERROR)
	{
		ret = ltoh32(msg->status);
		
		dhd->dongle_error = ret;
	}

done:
	return ret;
}

#ifdef CUSTOMER_HW_ONE
#ifdef PNO_SUPPORT
#define htod32(i) i
#define htod16(i) i

typedef struct pfn_ssid {
	char		ssid[32];			
	int32		ssid_len;		
	uint32		weight;
} pfn_ssid_t;

typedef struct pfn_ssid_set {
	pfn_ssid_t	pfn_ssids[MAX_PFN_NUMBER];
} pfn_ssid_set_t;

static pfn_ssid_set_t pfn_ssid_set;

int dhd_set_pfn_ssid(char * ssid, int ssid_len)
{
	uint32 i, lightest, weightest, samessid = 0xffff;

	if ((ssid_len < 1) || (ssid_len > 32)) {
		printf("Invaild ssid length!\n");
		return -1;
	}
	printf("pfn: set ssid = %s\n", ssid);
	lightest = 0;
	weightest =	0;
	for (i = 0; i < MAX_PFN_NUMBER; i++)
	{
		if (pfn_ssid_set.pfn_ssids[i].weight < pfn_ssid_set.pfn_ssids[lightest].weight)
			lightest = i;

		if (pfn_ssid_set.pfn_ssids[i].weight > pfn_ssid_set.pfn_ssids[weightest].weight)
			weightest = i;

		if (!strcmp(ssid, pfn_ssid_set.pfn_ssids[i].ssid))
			samessid = i;
	}

	printf("lightest is %d, weightest is %d, samessid = %d\n", lightest, weightest, samessid);

	if (samessid != 0xffff) {
		if (samessid == weightest) {
			printf("connect to latest ssid, ignore!\n");
			return 0;
		}
		pfn_ssid_set.pfn_ssids[samessid].weight = pfn_ssid_set.pfn_ssids[weightest].weight + 1;
		return 0;
	}
	memset(&pfn_ssid_set.pfn_ssids[lightest], 0, sizeof(pfn_ssid_t));

	strncpy(pfn_ssid_set.pfn_ssids[lightest].ssid, ssid, ssid_len);
	pfn_ssid_set.pfn_ssids[lightest].ssid_len = ssid_len;
	pfn_ssid_set.pfn_ssids[lightest].weight = pfn_ssid_set.pfn_ssids[weightest].weight + 1;
	
	return 0;
}

int dhd_del_pfn_ssid(char * ssid, int ssid_len)
{
	uint32 i;

	if ((ssid_len < 1) || (ssid_len > 32)) {
		printf("Invaild ssid length!\n");
		return -1;
	}
	
	for (i = 0; i < MAX_PFN_NUMBER; i++) {
		if (!strcmp(ssid, pfn_ssid_set.pfn_ssids[i].ssid))
			break;
	}

	if (i == MAX_PFN_NUMBER) {
		printf("del ssid [%s] not found!\n", ssid);
		return 0;
	}

	memset(&pfn_ssid_set.pfn_ssids[i], 0, sizeof(pfn_ssid_t));
	printf("del ssid [%s] complete!\n", ssid);

	return 0;
}
extern int dhd_pno_enable(dhd_pub_t *dhd, int pfn_enabled);
extern int dhd_pno_clean(dhd_pub_t *dhd);
extern int dhd_pno_set_ssid(dhd_pub_t *dhd, wlc_ssid_t* ssids_local, int nssid,
                       ushort  scan_fr, int pno_repeat, int pno_freq_expo_max);
char project_type[33];
int dhd_set_pfn(dhd_pub_t *dhd, int enabled)
{
	wlc_ssid_t ssids_local[MAX_PFN_NUMBER];
	int i;	
	int config_network = 0;
	int ret = 0;

	memset(ssids_local, 0, sizeof(ssids_local));
	
	if (enabled) {
		for (i = 0; i < MAX_PFN_NUMBER; i++) {
			if (pfn_ssid_set.pfn_ssids[i].ssid[0]) {
				strncpy((char *)ssids_local[i].SSID, pfn_ssid_set.pfn_ssids[i].ssid,
			        sizeof(ssids_local[i].SSID));
				ssids_local[i].SSID_len = pfn_ssid_set.pfn_ssids[i].ssid_len;
				config_network++;
			}
		}

		if (!config_network) {
			return 0;
		}

		
	        if (project_type != NULL && !strnicmp(project_type, "KT", strlen("KT")) ) {
				ret = dhd_pno_set_ssid(dhd, ssids_local, config_network, 120, 0, 0);
				if(ret < 0){
					DHD_ERROR(("%s Set dhd_pno_set_ssid error %d\n", __FUNCTION__, ret));
					return ret;
				}
        	} else {
				ret = dhd_pno_set_ssid(dhd, ssids_local, config_network, PFN_SCAN_FREQ, 0, 0);
				if(ret < 0){
					DHD_ERROR(("%s Set dhd_pno_set_ssid error %d\n", __FUNCTION__, ret));
					return ret;
				}
        	}

		
		ret = dhd_pno_enable(dhd, 1);
		if(ret < 0){
			DHD_ERROR(("%s Set dhd_pno_enable error %d\n", __FUNCTION__, ret));
			return ret;
		}
	} else{
		ret = dhd_pno_enable(dhd, 0);
		if(ret < 0){
			DHD_ERROR(("%s Set dhd_pno_enable error %d\n", __FUNCTION__, ret));
			return ret;
		}
	}
	return 0;
}

void dhd_clear_pfn()
{
	memset(&pfn_ssid_set, 0, sizeof(pfn_ssid_set_t));
}
#endif

#endif
int
dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, wl_ioctl_t * ioc, void * buf, int len)
{
	dhd_prot_t *prot = dhd->prot;
	int ret = -1;
	uint8 action;

	if ((dhd->busstate == DHD_BUS_DOWN) || dhd->hang_was_sent || dhd->os_stopped) {
		DHD_ERROR(("%s : bus is down. we have nothing to do\n", __FUNCTION__));
		goto done;
	}

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ASSERT(len <= WLC_IOCTL_MAXLEN);

	if (len > WLC_IOCTL_MAXLEN)
		goto done;

	if (prot->pending == TRUE) {
		DHD_ERROR(("CDC packet is pending!!!! cmd=0x%x (%lu) lastcmd=0x%x (%lu)\n",
			ioc->cmd, (unsigned long)ioc->cmd, prot->lastcmd,
			(unsigned long)prot->lastcmd));
		if ((ioc->cmd == WLC_SET_VAR) || (ioc->cmd == WLC_GET_VAR)) {
			DHD_TRACE(("iovar cmd=%s\n", (char*)buf));
		}
		goto done;
	}

	prot->pending = TRUE;
	prot->lastcmd = ioc->cmd;
	action = ioc->set;
	if (action & WL_IOCTL_ACTION_SET)
		ret = dhdcdc_set_ioctl(dhd, ifidx, ioc->cmd, buf, len, action);
	else {
		ret = dhdcdc_query_ioctl(dhd, ifidx, ioc->cmd, buf, len, action);
		if (ret > 0)
			ioc->used = ret - sizeof(cdc_ioctl_t);
	}

	
	if (ret >= 0)
		ret = 0;
	else {
		cdc_ioctl_t *msg = &prot->msg;
		ioc->needed = ltoh32(msg->len); 
	}

	
	if ((!ret) && (ioc->cmd == WLC_SET_VAR) && (!strcmp(buf, "wme_dp"))) {
		int slen, val = 0;

		slen = strlen("wme_dp") + 1;
		if (len >= (int)(slen + sizeof(int)))
			bcopy(((char *)buf + slen), &val, sizeof(int));
		dhd->wme_dp = (uint8) ltoh32(val);
	}

	prot->pending = FALSE;

done:

	return ret;
}

int
dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name,
                  void *params, int plen, void *arg, int len, bool set)
{
	return BCME_UNSUPPORTED;
}

void
dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	bcm_bprintf(strbuf, "Protocol CDC: reqid %d\n", dhdp->prot->reqid);
#ifdef PROP_TXSTATUS
	if (dhdp->wlfc_state)
		dhd_wlfc_dump(dhdp, strbuf);
#endif
}

#define PKTBUF pktbuf
#ifdef CUSTOMER_HW_ONE
#define RAISE_BK_PRIO 1
#endif
void
dhd_prot_hdrpush(dhd_pub_t *dhd, int ifidx, void *PKTBUF)
{
#ifdef BDC
	struct bdc_header *h;
#endif 

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#ifdef BDC
	

	PKTPUSH(dhd->osh, PKTBUF, BDC_HEADER_LEN);

	h = (struct bdc_header *)PKTDATA(dhd->osh, PKTBUF);

	h->flags = (BDC_PROTO_VER << BDC_FLAG_VER_SHIFT);
	if (PKTSUMNEEDED(PKTBUF))
		h->flags |= BDC_FLAG_SUM_NEEDED;


	h->priority = (PKTPRIO(PKTBUF) & BDC_PRIORITY_MASK);
#ifdef RAISE_BK_PRIO
		if ((dhd_APUP == true) && (h->priority < 3))
			h->priority = 3;
#endif
	h->flags2 = 0;
	h->dataOffset = 0;
#endif 
	BDC_SET_IF_IDX(h, ifidx);
}
#undef PKTBUF	

int
dhd_prot_hdrpull(dhd_pub_t *dhd, int *ifidx, void *pktbuf, uchar *reorder_buf_info,
	uint *reorder_info_len)
{
#ifdef BDC
	struct bdc_header *h;
#endif
	uint8 data_offset = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#ifdef BDC
	if (reorder_info_len)
		*reorder_info_len = 0;
	

	if (PKTLEN(dhd->osh, pktbuf) < BDC_HEADER_LEN) {
		DHD_ERROR(("%s: rx data too short (%d < %d)\n", __FUNCTION__,
		           PKTLEN(dhd->osh, pktbuf), BDC_HEADER_LEN));
		return BCME_ERROR;
	}

	h = (struct bdc_header *)PKTDATA(dhd->osh, pktbuf);

	if (!ifidx) {
		
		data_offset = h->dataOffset;
		PKTPULL(dhd->osh, pktbuf, BDC_HEADER_LEN);
		goto exit;
	}

	if ((*ifidx = BDC_GET_IF_IDX(h)) >= DHD_MAX_IFS) {
		DHD_ERROR(("%s: rx data ifnum out of range (%d)\n",
		           __FUNCTION__, *ifidx));
		return BCME_ERROR;
	}

	if (((h->flags & BDC_FLAG_VER_MASK) >> BDC_FLAG_VER_SHIFT) != BDC_PROTO_VER) {
		DHD_ERROR(("%s: non-BDC packet received, flags = 0x%x\n",
		           dhd_ifname(dhd, *ifidx), h->flags));
		if (((h->flags & BDC_FLAG_VER_MASK) >> BDC_FLAG_VER_SHIFT) == BDC_PROTO_VER_1)
			h->dataOffset = 0;
		else
		return BCME_ERROR;
	}

	if (h->flags & BDC_FLAG_SUM_GOOD) {
		DHD_INFO(("%s: BDC packet received with good rx-csum, flags 0x%x\n",
		          dhd_ifname(dhd, *ifidx), h->flags));
		PKTSETSUMGOOD(pktbuf, TRUE);
	}

	PKTSETPRIO(pktbuf, (h->priority & BDC_PRIORITY_MASK));
	data_offset = h->dataOffset;
	PKTPULL(dhd->osh, pktbuf, BDC_HEADER_LEN);
#endif 

#ifdef PROP_TXSTATUS
	dhd_os_wlfc_block(dhd);
	if (dhd->wlfc_state &&
		((athost_wl_status_info_t*)dhd->wlfc_state)->proptxstatus_mode
		!= WLFC_FCMODE_NONE &&
		(!DHD_PKTTAG_PKTDIR(PKTTAG(pktbuf)))) {
		dhd_wlfc_parse_header_info(dhd, pktbuf, (data_offset << 2),
			reorder_buf_info, reorder_info_len);
		((athost_wl_status_info_t*)dhd->wlfc_state)->stats.dhd_hdrpulls++;

	}
	dhd_os_wlfc_unblock(dhd);
#endif 

exit:
		PKTPULL(dhd->osh, pktbuf, (data_offset << 2));
	return 0;
}

#if defined(PROP_TXSTATUS)
void
dhd_wlfc_trigger_pktcommit(dhd_pub_t *dhd)
{
	dhd_os_wlfc_block(dhd);
	if (dhd->wlfc_state &&
		(((athost_wl_status_info_t*)dhd->wlfc_state)->proptxstatus_mode
		!= WLFC_FCMODE_NONE)) {
		dhd_wlfc_commit_packets(dhd->wlfc_state, (f_commitpkt_t)dhd_bus_txdata,
			(void *)dhd->bus, NULL);
	}
	dhd_os_wlfc_unblock(dhd);
}
#endif


int
dhd_prot_attach(dhd_pub_t *dhd)
{
	dhd_prot_t *cdc;

	if (!(cdc = (dhd_prot_t *)DHD_OS_PREALLOC(dhd->osh, DHD_PREALLOC_PROT,
		sizeof(dhd_prot_t)))) {
			DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
			goto fail;
		}
	memset(cdc, 0, sizeof(dhd_prot_t));

	
	if ((uintptr)(&cdc->msg + 1) != (uintptr)cdc->buf) {
		DHD_ERROR(("dhd_prot_t is not correctly defined\n"));
		goto fail;
	}

	dhd->prot = cdc;
#ifdef BDC
	dhd->hdrlen += BDC_HEADER_LEN;
#endif
	dhd->maxctl = WLC_IOCTL_MAXLEN + sizeof(cdc_ioctl_t) + ROUND_UP_MARGIN;
	return 0;

fail:
#ifndef CONFIG_DHD_USE_STATIC_BUF
	if (cdc != NULL)
		MFREE(dhd->osh, cdc, sizeof(dhd_prot_t));
#endif 
	return BCME_NOMEM;
}

void
dhd_prot_detach(dhd_pub_t *dhd)
{
#ifdef PROP_TXSTATUS
	dhd_wlfc_deinit(dhd);
	if (dhd->plat_deinit)
		dhd->plat_deinit((void *)dhd);
#endif
#ifndef CONFIG_DHD_USE_STATIC_BUF
	MFREE(dhd->osh, dhd->prot, sizeof(dhd_prot_t));
#endif 
	dhd->prot = NULL;
}

void
dhd_prot_dstats(dhd_pub_t *dhd)
{
	
	dhd->dstats.tx_packets = dhd->tx_packets;
	dhd->dstats.tx_errors = dhd->tx_errors;
	dhd->dstats.rx_packets = dhd->rx_packets;
	dhd->dstats.rx_errors = dhd->rx_errors;
	dhd->dstats.rx_dropped = dhd->rx_dropped;
	dhd->dstats.multicast = dhd->rx_multicast;
	return;
}

int
dhd_prot_init(dhd_pub_t *dhd)
{
	int ret = 0;
	wlc_rev_info_t revinfo;
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));


	
	memset(&revinfo, 0, sizeof(revinfo));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_REVINFO, &revinfo, sizeof(revinfo), FALSE, 0);
	if (ret < 0)
		goto done;


#if defined(WL_CFG80211)
	if (dhd_download_fw_on_driverload)
#endif 
		ret = dhd_preinit_ioctls(dhd);
	
	dhd->iswl = TRUE;

done:
	return ret;
}

void
dhd_prot_stop(dhd_pub_t *dhd)
{
	
}


static void
dhd_get_hostreorder_pkts(void *osh, struct reorder_info *ptr, void **pkt,
	uint32 *pkt_count, void **pplast, uint8 start, uint8 end)
{
	uint i;
	void *plast = NULL, *p;
	uint32 pkt_cnt = 0;

	if (ptr->pend_pkts == 0) {
		DHD_REORDER(("%s: no packets in reorder queue \n", __FUNCTION__));
		*pplast = NULL;
		*pkt_count = 0;
		*pkt = NULL;
		return;
	}
	if (start == end)
		i = ptr->max_idx + 1;
	else {
		if (start > end)
			i = ((ptr->max_idx + 1) - start) + end;
		else
			i = end - start;
	}
	while (i) {
		p = (void *)(ptr->p[start]);
		ptr->p[start] = NULL;

		if (p != NULL) {
			if (plast == NULL)
				*pkt = p;
			else
				PKTSETNEXT(osh, plast, p);

			plast = p;
			pkt_cnt++;
		}
		i--;
		if (start++ == ptr->max_idx)
			start = 0;
	}
	*pplast = plast;
	*pkt_count = (uint32)pkt_cnt;
}

int
dhd_process_pkt_reorder_info(dhd_pub_t *dhd, uchar *reorder_info_buf, uint reorder_info_len,
	void **pkt, uint32 *pkt_count)
{
	uint8 flow_id, max_idx, cur_idx, exp_idx;
	struct reorder_info *ptr;
	uint8 flags;
	void *cur_pkt, *plast = NULL;
	uint32 cnt = 0;

	if (pkt == NULL) {
		if (pkt_count != NULL)
			*pkt_count = 0;
		return 0;
	}

	flow_id = reorder_info_buf[WLHOST_REORDERDATA_FLOWID_OFFSET];
	flags = reorder_info_buf[WLHOST_REORDERDATA_FLAGS_OFFSET];

	DHD_REORDER(("flow_id %d, flags 0x%02x, idx(%d, %d, %d)\n", flow_id, flags,
		reorder_info_buf[WLHOST_REORDERDATA_CURIDX_OFFSET],
		reorder_info_buf[WLHOST_REORDERDATA_EXPIDX_OFFSET],
		reorder_info_buf[WLHOST_REORDERDATA_MAXIDX_OFFSET]));

	
	if (flags == 0xFF) {
		DHD_ERROR(("%s: invalid flags...so ignore this packet\n", __FUNCTION__));
		*pkt_count = 1;
		return 0;
	}

	cur_pkt = *pkt;
	*pkt = NULL;

	ptr = dhd->reorder_bufs[flow_id];
	if (flags & WLHOST_REORDERDATA_DEL_FLOW) {
		uint32 buf_size = sizeof(struct reorder_info);

		DHD_REORDER(("%s: Flags indicating to delete a flow id %d\n",
			__FUNCTION__, flow_id));

		if (ptr == NULL) {
			DHD_REORDER(("%s: received flags to cleanup, but no flow (%d) yet\n",
				__FUNCTION__, flow_id));
			*pkt_count = 1;
			*pkt = cur_pkt;
			return 0;
		}

		dhd_get_hostreorder_pkts(dhd->osh, ptr, pkt, &cnt, &plast,
			ptr->exp_idx, ptr->exp_idx);
		
		if (plast) {
			PKTSETNEXT(dhd->osh, plast, cur_pkt);
			cnt++;
		}
		else {
			if (cnt != 0) {
				DHD_ERROR(("%s: del flow: something fishy, pending packets %d\n",
					__FUNCTION__, cnt));
			}
			*pkt = cur_pkt;
			cnt = 1;
		}
		buf_size += ((ptr->max_idx + 1) * sizeof(void *));
		MFREE(dhd->osh, ptr, buf_size);
		dhd->reorder_bufs[flow_id] = NULL;
		*pkt_count = cnt;
		return 0;
	}
	
	if (ptr == NULL) {
		uint32 buf_size_alloc = sizeof(reorder_info_t);
		max_idx = reorder_info_buf[WLHOST_REORDERDATA_MAXIDX_OFFSET];

		buf_size_alloc += ((max_idx + 1) * sizeof(void*));
		

		DHD_REORDER(("%s: alloc buffer of size %d size, reorder info id %d, maxidx %d\n",
			__FUNCTION__, buf_size_alloc, flow_id, max_idx));
		ptr = (struct reorder_info *)MALLOC(dhd->osh, buf_size_alloc);
		if (ptr == NULL) {
			DHD_ERROR(("%s: Malloc failed to alloc buffer\n", __FUNCTION__));
			*pkt_count = 1;
			return 0;
		}
		bzero(ptr, buf_size_alloc);
		dhd->reorder_bufs[flow_id] = ptr;
		ptr->p = (void *)(ptr+1);
		ptr->max_idx = max_idx;
	}
	if (flags & WLHOST_REORDERDATA_NEW_HOLE)  {
		DHD_REORDER(("%s: new hole, so cleanup pending buffers\n", __FUNCTION__));
		if (ptr->pend_pkts) {
			dhd_get_hostreorder_pkts(dhd->osh, ptr, pkt, &cnt, &plast,
				ptr->exp_idx, ptr->exp_idx);
			ptr->pend_pkts = 0;
		}
		ptr->cur_idx = reorder_info_buf[WLHOST_REORDERDATA_CURIDX_OFFSET];
		ptr->exp_idx = reorder_info_buf[WLHOST_REORDERDATA_EXPIDX_OFFSET];
		ptr->max_idx = reorder_info_buf[WLHOST_REORDERDATA_MAXIDX_OFFSET];
		ptr->p[ptr->cur_idx] = cur_pkt;
		ptr->pend_pkts++;
		*pkt_count = cnt;
	}
	else if (flags & WLHOST_REORDERDATA_CURIDX_VALID) {
		cur_idx = reorder_info_buf[WLHOST_REORDERDATA_CURIDX_OFFSET];
		exp_idx = reorder_info_buf[WLHOST_REORDERDATA_EXPIDX_OFFSET];


		if ((exp_idx == ptr->exp_idx) && (cur_idx != ptr->exp_idx)) {
			
			
			if (ptr->p[cur_idx] != NULL) {
				DHD_REORDER(("%s: HOLE: ERROR buffer pending..free it\n",
					__FUNCTION__));
				PKTFREE(dhd->osh, ptr->p[cur_idx], TRUE);
				ptr->p[cur_idx] = NULL;
			}
			ptr->p[cur_idx] = cur_pkt;
			ptr->pend_pkts++;
			ptr->cur_idx = cur_idx;
			DHD_REORDER(("%s: fill up a hole..pending packets is %d\n",
				__FUNCTION__, ptr->pend_pkts));
			*pkt_count = 0;
			*pkt = NULL;
		}
		else if (ptr->exp_idx == cur_idx) {
			
			DHD_REORDER(("%s: got the right one now, cur_idx is %d\n",
				__FUNCTION__, cur_idx));
			if (ptr->p[cur_idx] != NULL) {
				DHD_REORDER(("%s: Error buffer pending..free it\n",
					__FUNCTION__));
				PKTFREE(dhd->osh, ptr->p[cur_idx], TRUE);
				ptr->p[cur_idx] = NULL;
			}
			ptr->p[cur_idx] = cur_pkt;
			ptr->pend_pkts++;

			ptr->cur_idx = cur_idx;
			ptr->exp_idx = exp_idx;

			dhd_get_hostreorder_pkts(dhd->osh, ptr, pkt, &cnt, &plast,
				cur_idx, exp_idx);
			ptr->pend_pkts -= (uint8)cnt;
			*pkt_count = cnt;
			DHD_REORDER(("%s: freeing up buffers %d, still pending %d\n",
				__FUNCTION__, cnt, ptr->pend_pkts));
		}
		else {
			uint8 end_idx;
			bool flush_current = FALSE;
			
			DHD_REORDER(("%s:, flow %d, both moved, cur %d(%d), exp %d(%d)\n",
				__FUNCTION__, flow_id, ptr->cur_idx, cur_idx,
				ptr->exp_idx, exp_idx));
			if (flags & WLHOST_REORDERDATA_FLUSH_ALL)
				end_idx = ptr->exp_idx;
			else
				end_idx = exp_idx;

			
			dhd_get_hostreorder_pkts(dhd->osh, ptr, pkt, &cnt, &plast,
				ptr->exp_idx, end_idx);

			if (cur_idx == ptr->max_idx) {
				if (exp_idx == 0)
					flush_current = TRUE;
			} else {
				if (exp_idx == cur_idx + 1)
					flush_current = TRUE;
			}
			if (flush_current) {
				if (plast)
					PKTSETNEXT(dhd->osh, plast, cur_pkt);
				else
					*pkt = cur_pkt;
				cnt++;
			}
			else {
				ptr->p[cur_idx] = cur_pkt;
				ptr->pend_pkts++;
			}
			ptr->exp_idx = exp_idx;
			ptr->cur_idx = cur_idx;
			*pkt_count = cnt;
		}
	}
	else {
		uint8 end_idx;
		
		exp_idx = reorder_info_buf[WLHOST_REORDERDATA_EXPIDX_OFFSET];

		DHD_REORDER(("%s: move the window, cur_idx is %d, exp is %d, new exp is %d\n",
			__FUNCTION__, ptr->cur_idx, ptr->exp_idx, exp_idx));
		if (flags & WLHOST_REORDERDATA_FLUSH_ALL)
			end_idx =  ptr->exp_idx;
		else
			end_idx =  exp_idx;

		dhd_get_hostreorder_pkts(dhd->osh, ptr, pkt, &cnt, &plast, ptr->exp_idx, end_idx);
		ptr->pend_pkts -= (uint8)cnt;
		if (plast)
			PKTSETNEXT(dhd->osh, plast, cur_pkt);
		else
			*pkt = cur_pkt;
		cnt++;
		*pkt_count = cnt;
		
		ptr->exp_idx = exp_idx;
	}
	return 0;
}
