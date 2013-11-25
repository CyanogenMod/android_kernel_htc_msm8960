/*
 * DHD PROP_TXSTATUS Module.
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
 * $Id: dhd_wlfc.c 412994 2013-07-17 12:38:03Z $
 *
 */

#include <typedefs.h>
#include <osl.h>

#include <bcmutils.h>
#include <bcmendian.h>

#include <dngl_stats.h>
#include <dhd.h>

#include <dhd_bus.h>
#include <dhd_dbg.h>

#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <dhd_wlfc.h>
#endif




#define BUS_RETRIES 1	

#ifdef PROP_TXSTATUS
typedef struct dhd_wlfc_commit_info {
	uint8					needs_hdr;
	uint8					ac_fifo_credit_spent;
	ewlfc_packet_state_t	pkt_type;
	wlfc_mac_descriptor_t*	mac_entry;
	void*					p;
} dhd_wlfc_commit_info_t;
#endif 


#ifdef PROP_TXSTATUS

#ifdef QMONITOR
#define DHD_WLFC_QMON_COMPLETE(entry) dhd_qmon_txcomplete(&entry->qmon)
#else
#define DHD_WLFC_QMON_COMPLETE(entry)
#endif 

void
dhd_wlfc_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	int i;
	uint8* ea;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhdp->wlfc_state;
	wlfc_hanger_t* h;
	wlfc_mac_descriptor_t* mac_table;
	wlfc_mac_descriptor_t* interfaces;
	char* iftypes[] = {"STA", "AP", "WDS", "p2pGO", "p2pCL"};

	if (wlfc == NULL) {
		bcm_bprintf(strbuf, "wlfc not initialized yet\n");
		return;
	}
	h = (wlfc_hanger_t*)wlfc->hanger;
	if (h == NULL) {
		bcm_bprintf(strbuf, "wlfc-hanger not initialized yet\n");
	}

	mac_table = wlfc->destination_entries.nodes;
	interfaces = wlfc->destination_entries.interfaces;
	bcm_bprintf(strbuf, "---- wlfc stats ----\n");
	if (h) {
		bcm_bprintf(strbuf, "wlfc hanger (pushed,popped,f_push,"
			"f_pop,f_slot, pending) = (%d,%d,%d,%d,%d,%d)\n",
			h->pushed,
			h->popped,
			h->failed_to_push,
			h->failed_to_pop,
			h->failed_slotfind,
			(h->pushed - h->popped));
	}

	bcm_bprintf(strbuf, "wlfc fail(tlv,credit_rqst,mac_update,psmode_update), "
		"(dq_full,rollback_fail) = (%d,%d,%d,%d), (%d,%d)\n",
		wlfc->stats.tlv_parse_failed,
		wlfc->stats.credit_request_failed,
		wlfc->stats.mac_update_failed,
		wlfc->stats.psmode_update_failed,
		wlfc->stats.delayq_full_error,
		wlfc->stats.rollback_failed);

	bcm_bprintf(strbuf, "PKTS (credit,sent) "
		"(AC0[%d,%d],AC1[%d,%d],AC2[%d,%d],AC3[%d,%d],BC_MC[%d,%d])\n",
		wlfc->FIFO_credit[0], wlfc->stats.send_pkts[0],
		wlfc->FIFO_credit[1], wlfc->stats.send_pkts[1],
		wlfc->FIFO_credit[2], wlfc->stats.send_pkts[2],
		wlfc->FIFO_credit[3], wlfc->stats.send_pkts[3],
		wlfc->FIFO_credit[4], wlfc->stats.send_pkts[4]);

	bcm_bprintf(strbuf, "\n");
	for (i = 0; i < WLFC_MAX_IFNUM; i++) {
		if (interfaces[i].occupied) {
			char* iftype_desc;

			if (interfaces[i].iftype > WLC_E_IF_ROLE_P2P_CLIENT)
				iftype_desc = "<Unknown";
			else
				iftype_desc = iftypes[interfaces[i].iftype];

			ea = interfaces[i].ea;
			bcm_bprintf(strbuf, "INTERFACE[%d].ea = "
				"[%02x:%02x:%02x:%02x:%02x:%02x], if:%d, type: %s"
				"netif_flow_control:%s\n", i,
				ea[0], ea[1], ea[2], ea[3], ea[4], ea[5],
				interfaces[i].interface_id,
				iftype_desc, ((wlfc->hostif_flow_state[i] == OFF)
				? " OFF":" ON"));

			bcm_bprintf(strbuf, "INTERFACE[%d].DELAYQ(len,state,credit)"
				"= (%d,%s,%d)\n",
				i,
				interfaces[i].psq.len,
				((interfaces[i].state ==
				WLFC_STATE_OPEN) ? " OPEN":"CLOSE"),
				interfaces[i].requested_credit);

			bcm_bprintf(strbuf, "INTERFACE[%d].DELAYQ"
				"(sup,ac0),(sup,ac1),(sup,ac2),(sup,ac3) = "
				"(%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
				i,
				interfaces[i].psq.q[0].len,
				interfaces[i].psq.q[1].len,
				interfaces[i].psq.q[2].len,
				interfaces[i].psq.q[3].len,
				interfaces[i].psq.q[4].len,
				interfaces[i].psq.q[5].len,
				interfaces[i].psq.q[6].len,
				interfaces[i].psq.q[7].len);
		}
	}

	bcm_bprintf(strbuf, "\n");
	for (i = 0; i < WLFC_MAC_DESC_TABLE_SIZE; i++) {
		if (mac_table[i].occupied) {
			ea = mac_table[i].ea;
			bcm_bprintf(strbuf, "MAC_table[%d].ea = "
				"[%02x:%02x:%02x:%02x:%02x:%02x], if:%d \n", i,
				ea[0], ea[1], ea[2], ea[3], ea[4], ea[5],
				mac_table[i].interface_id);

			bcm_bprintf(strbuf, "MAC_table[%d].DELAYQ(len,state,credit)"
				"= (%d,%s,%d)\n",
				i,
				mac_table[i].psq.len,
				((mac_table[i].state ==
				WLFC_STATE_OPEN) ? " OPEN":"CLOSE"),
				mac_table[i].requested_credit);
#ifdef PROP_TXSTATUS_DEBUG
			bcm_bprintf(strbuf, "MAC_table[%d]: (opened, closed) = (%d, %d)\n",
				i, mac_table[i].opened_ct, mac_table[i].closed_ct);
#endif
			bcm_bprintf(strbuf, "MAC_table[%d].DELAYQ"
				"(sup,ac0),(sup,ac1),(sup,ac2),(sup,ac3) = "
				"(%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
				i,
				mac_table[i].psq.q[0].len,
				mac_table[i].psq.q[1].len,
				mac_table[i].psq.q[2].len,
				mac_table[i].psq.q[3].len,
				mac_table[i].psq.q[4].len,
				mac_table[i].psq.q[5].len,
				mac_table[i].psq.q[6].len,
				mac_table[i].psq.q[7].len);
		}
	}

#ifdef PROP_TXSTATUS_DEBUG
	{
		int avg;
		int moving_avg = 0;
		int moving_samples;

		if (wlfc->stats.latency_sample_count) {
			moving_samples = sizeof(wlfc->stats.deltas)/sizeof(uint32);

			for (i = 0; i < moving_samples; i++)
				moving_avg += wlfc->stats.deltas[i];
			moving_avg /= moving_samples;

			avg = (100 * wlfc->stats.total_status_latency) /
				wlfc->stats.latency_sample_count;
			bcm_bprintf(strbuf, "txstatus latency (average, last, moving[%d]) = "
				"(%d.%d, %03d, %03d)\n",
				moving_samples, avg/100, (avg - (avg/100)*100),
				wlfc->stats.latency_most_recent,
				moving_avg);
		}
	}

	bcm_bprintf(strbuf, "wlfc- fifo[0-5] credit stats: sent = (%d,%d,%d,%d,%d,%d), "
		"back = (%d,%d,%d,%d,%d,%d)\n",
		wlfc->stats.fifo_credits_sent[0],
		wlfc->stats.fifo_credits_sent[1],
		wlfc->stats.fifo_credits_sent[2],
		wlfc->stats.fifo_credits_sent[3],
		wlfc->stats.fifo_credits_sent[4],
		wlfc->stats.fifo_credits_sent[5],

		wlfc->stats.fifo_credits_back[0],
		wlfc->stats.fifo_credits_back[1],
		wlfc->stats.fifo_credits_back[2],
		wlfc->stats.fifo_credits_back[3],
		wlfc->stats.fifo_credits_back[4],
		wlfc->stats.fifo_credits_back[5]);
	{
		uint32 fifo_cr_sent = 0;
		uint32 fifo_cr_acked = 0;
		uint32 request_cr_sent = 0;
		uint32 request_cr_ack = 0;
		uint32 bc_mc_cr_ack = 0;

		for (i = 0; i < sizeof(wlfc->stats.fifo_credits_sent)/sizeof(uint32); i++) {
			fifo_cr_sent += wlfc->stats.fifo_credits_sent[i];
		}

		for (i = 0; i < sizeof(wlfc->stats.fifo_credits_back)/sizeof(uint32); i++) {
			fifo_cr_acked += wlfc->stats.fifo_credits_back[i];
		}

		for (i = 0; i < WLFC_MAC_DESC_TABLE_SIZE; i++) {
			if (wlfc->destination_entries.nodes[i].occupied) {
				request_cr_sent +=
					wlfc->destination_entries.nodes[i].dstncredit_sent_packets;
			}
		}
		for (i = 0; i < WLFC_MAX_IFNUM; i++) {
			if (wlfc->destination_entries.interfaces[i].occupied) {
				request_cr_sent +=
				wlfc->destination_entries.interfaces[i].dstncredit_sent_packets;
			}
		}
		for (i = 0; i < WLFC_MAC_DESC_TABLE_SIZE; i++) {
			if (wlfc->destination_entries.nodes[i].occupied) {
				request_cr_ack +=
					wlfc->destination_entries.nodes[i].dstncredit_acks;
			}
		}
		for (i = 0; i < WLFC_MAX_IFNUM; i++) {
			if (wlfc->destination_entries.interfaces[i].occupied) {
				request_cr_ack +=
					wlfc->destination_entries.interfaces[i].dstncredit_acks;
			}
		}
		bcm_bprintf(strbuf, "wlfc- (sent, status) => pq(%d,%d), vq(%d,%d),"
			"other:%d, bc_mc:%d, signal-only, (sent,freed): (%d,%d)",
			fifo_cr_sent, fifo_cr_acked,
			request_cr_sent, request_cr_ack,
			wlfc->destination_entries.other.dstncredit_acks,
			bc_mc_cr_ack,
			wlfc->stats.signal_only_pkts_sent, wlfc->stats.signal_only_pkts_freed);
	}
#endif 
	bcm_bprintf(strbuf, "\n");
	bcm_bprintf(strbuf, "wlfc- pkt((in,2bus,txstats,hdrpull),(dropped,hdr_only,wlc_tossed)"
		"(freed,free_err,rollback)) = "
		"((%d,%d,%d,%d),(%d,%d,%d),(%d,%d,%d))\n",
		wlfc->stats.pktin,
		wlfc->stats.pkt2bus,
		wlfc->stats.txstatus_in,
		wlfc->stats.dhd_hdrpulls,

		wlfc->stats.pktdropped,
		wlfc->stats.wlfc_header_only_pkt,
		wlfc->stats.wlc_tossed_pkts,

		wlfc->stats.pkt_freed,
		wlfc->stats.pkt_free_err, wlfc->stats.rollback);

	bcm_bprintf(strbuf, "wlfc- suppress((d11,wlc,err),enq(d11,wl,hq,mac?),retx(d11,wlc,hq)) = "
		"((%d,%d,%d),(%d,%d,%d,%d),(%d,%d,%d))\n",

		wlfc->stats.d11_suppress,
		wlfc->stats.wl_suppress,
		wlfc->stats.bad_suppress,

		wlfc->stats.psq_d11sup_enq,
		wlfc->stats.psq_wlsup_enq,
		wlfc->stats.psq_hostq_enq,
		wlfc->stats.mac_handle_notfound,

		wlfc->stats.psq_d11sup_retx,
		wlfc->stats.psq_wlsup_retx,
		wlfc->stats.psq_hostq_retx);
	bcm_bprintf(strbuf, "wlfc- generic error: %d", wlfc->stats.generic_error);

	return;
}

static void*
dhd_wlfc_hanger_create(osl_t *osh, int max_items)
{
	int i;
	wlfc_hanger_t* hanger;

	
	ASSERT(max_items == WLFC_HANGER_MAXITEMS);

	if ((hanger = (wlfc_hanger_t*)MALLOC(osh, WLFC_HANGER_SIZE(max_items))) == NULL)
		return NULL;

	memset(hanger, 0, WLFC_HANGER_SIZE(max_items));
	hanger->max_items = max_items;

	for (i = 0; i < hanger->max_items; i++) {
		hanger->items[i].state = WLFC_HANGER_ITEM_STATE_FREE;
	}
	return hanger;
}

static int
dhd_wlfc_hanger_delete(osl_t *osh, void* hanger)
{
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	if (h) {
		MFREE(osh, h, WLFC_HANGER_SIZE(h->max_items));
		return BCME_OK;
	}
	return BCME_BADARG;
}

static uint16
dhd_wlfc_hanger_get_free_slot(void* hanger)
{
	uint32 i;
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	if (h) {
		i = h->slot_pos + 1;
		if (i == h->max_items) {
			i = 0;
		}
		while (i != h->slot_pos) {
			if (h->items[i].state == WLFC_HANGER_ITEM_STATE_FREE) {
				h->slot_pos = i;
				return (uint16)i;
			}
			i++;
			if (i == h->max_items)
				i = 0;
		}
		h->failed_slotfind++;
	}
	return WLFC_HANGER_MAXITEMS;
}

static int
dhd_wlfc_hanger_get_genbit(void* hanger, void* pkt, uint32 slot_id, int* gen)
{
	int rc = BCME_OK;
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	*gen = 0xff;

	
	if (slot_id == WLFC_HANGER_MAXITEMS)
		return BCME_NOTFOUND;

	if (h) {
		if ((h->items[slot_id].state == WLFC_HANGER_ITEM_STATE_INUSE) ||
			(h->items[slot_id].state == WLFC_HANGER_ITEM_STATE_INUSE_SUPPRESSED)) {
			*gen = h->items[slot_id].gen;
		}
		else {
			rc = BCME_NOTFOUND;
		}
	}
	else
		rc = BCME_BADARG;
	return rc;
}

static int
dhd_wlfc_hanger_pushpkt(void* hanger, void* pkt, uint32 slot_id)
{
	int rc = BCME_OK;
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	if (h && (slot_id < WLFC_HANGER_MAXITEMS)) {
		if (h->items[slot_id].state == WLFC_HANGER_ITEM_STATE_FREE) {
			h->items[slot_id].state = WLFC_HANGER_ITEM_STATE_INUSE;
			h->items[slot_id].pkt = pkt;
			h->items[slot_id].identifier = slot_id;
			h->pushed++;
		}
		else {
			h->failed_to_push++;
			rc = BCME_NOTFOUND;
		}
	}
	else
		rc = BCME_BADARG;
	return rc;
}

static int
dhd_wlfc_hanger_poppkt(void* hanger, uint32 slot_id, void** pktout, int remove_from_hanger)
{
	int rc = BCME_OK;
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	
	if (slot_id == WLFC_HANGER_MAXITEMS)
		return BCME_NOTFOUND;

	if (h) {
		if (h->items[slot_id].state != WLFC_HANGER_ITEM_STATE_FREE) {
			*pktout = h->items[slot_id].pkt;
			if (remove_from_hanger) {
				h->items[slot_id].state =
					WLFC_HANGER_ITEM_STATE_FREE;
				h->items[slot_id].pkt = NULL;
				h->items[slot_id].identifier = 0;
				h->items[slot_id].gen = 0xff;
				h->popped++;
			}
		}
		else {
			h->failed_to_pop++;
			rc = BCME_NOTFOUND;
		}
	}
	else
		rc = BCME_BADARG;
	return rc;
}

static int
dhd_wlfc_hanger_mark_suppressed(void* hanger, uint32 slot_id, uint8 gen)
{
	int rc = BCME_OK;
	wlfc_hanger_t* h = (wlfc_hanger_t*)hanger;

	
	if (slot_id == WLFC_HANGER_MAXITEMS)
		return BCME_NOTFOUND;
	if (h) {
		h->items[slot_id].gen = gen;
		if (h->items[slot_id].state == WLFC_HANGER_ITEM_STATE_INUSE) {
			h->items[slot_id].state = WLFC_HANGER_ITEM_STATE_INUSE_SUPPRESSED;
		}
		else
			rc = BCME_BADARG;
	}
	else
		rc = BCME_BADARG;

	return rc;
}

static int
_dhd_wlfc_pushheader(athost_wl_status_info_t* ctx, void* p, bool tim_signal,
	uint8 tim_bmp, uint8 mac_handle, uint32 htodtag)
{
	uint32 wl_pktinfo = 0;
	uint8* wlh;
	uint8 dataOffset;
	uint8 fillers;
	uint8 tim_signal_len = 0;

	struct bdc_header *h;

	if (tim_signal) {
		tim_signal_len = 1 + 1 + WLFC_CTL_VALUE_LEN_PENDING_TRAFFIC_BMP;
	}

	
	dataOffset = WLFC_CTL_VALUE_LEN_PKTTAG + 2 + tim_signal_len;
	fillers = ROUNDUP(dataOffset, 4) - dataOffset;
	dataOffset += fillers;

	PKTPUSH(ctx->osh, p, dataOffset);
	wlh = (uint8*) PKTDATA(ctx->osh, p);

	wl_pktinfo = htol32(htodtag);

	wlh[0] = WLFC_CTL_TYPE_PKTTAG;
	wlh[1] = WLFC_CTL_VALUE_LEN_PKTTAG;
	memcpy(&wlh[2], &wl_pktinfo, sizeof(uint32));

	if (tim_signal_len) {
		wlh[dataOffset - fillers - tim_signal_len ] =
			WLFC_CTL_TYPE_PENDING_TRAFFIC_BMP;
		wlh[dataOffset - fillers - tim_signal_len + 1] =
			WLFC_CTL_VALUE_LEN_PENDING_TRAFFIC_BMP;
		wlh[dataOffset - fillers - tim_signal_len + 2] = mac_handle;
		wlh[dataOffset - fillers - tim_signal_len + 3] = tim_bmp;
	}
	if (fillers)
		memset(&wlh[dataOffset - fillers], WLFC_CTL_TYPE_FILLER, fillers);

	PKTPUSH(ctx->osh, p, BDC_HEADER_LEN);
	h = (struct bdc_header *)PKTDATA(ctx->osh, p);
	h->flags = (BDC_PROTO_VER << BDC_FLAG_VER_SHIFT);
	if (PKTSUMNEEDED(p))
		h->flags |= BDC_FLAG_SUM_NEEDED;


	h->priority = (PKTPRIO(p) & BDC_PRIORITY_MASK);
	h->flags2 = 0;
	h->dataOffset = dataOffset >> 2;
	BDC_SET_IF_IDX(h, DHD_PKTTAG_IF(PKTTAG(p)));
	return BCME_OK;
}

static int
_dhd_wlfc_pullheader(athost_wl_status_info_t* ctx, void* pktbuf)
{
	struct bdc_header *h;

	if (PKTLEN(ctx->osh, pktbuf) < BDC_HEADER_LEN) {
		WLFC_DBGMESG(("%s: rx data too short (%d < %d)\n", __FUNCTION__,
		           PKTLEN(ctx->osh, pktbuf), BDC_HEADER_LEN));
		return BCME_ERROR;
	}
	h = (struct bdc_header *)PKTDATA(ctx->osh, pktbuf);

	
	PKTPULL(ctx->osh, pktbuf, BDC_HEADER_LEN);

	if (PKTLEN(ctx->osh, pktbuf) < (h->dataOffset << 2)) {
		WLFC_DBGMESG(("%s: rx data too short (%d < %d)\n", __FUNCTION__,
		           PKTLEN(ctx->osh, pktbuf), (h->dataOffset << 2)));
		return BCME_ERROR;
	}

	
	PKTPULL(ctx->osh, pktbuf, (h->dataOffset << 2));
	return BCME_OK;
}

static wlfc_mac_descriptor_t*
_dhd_wlfc_find_table_entry(athost_wl_status_info_t* ctx, void* p)
{
	int i;
	wlfc_mac_descriptor_t* table = ctx->destination_entries.nodes;
	uint8 ifid = DHD_PKTTAG_IF(PKTTAG(p));
	uint8* dstn = DHD_PKTTAG_DSTN(PKTTAG(p));
	wlfc_mac_descriptor_t* entry = NULL;
	int iftype = ctx->destination_entries.interfaces[ifid].iftype;

	if ((iftype == WLC_E_IF_ROLE_STA || ETHER_ISMULTI(dstn) ||
		iftype == WLC_E_IF_ROLE_P2P_CLIENT) &&
		(ctx->destination_entries.interfaces[ifid].occupied)) {
			entry = &ctx->destination_entries.interfaces[ifid];
	}

	if (entry != NULL && ETHER_ISMULTI(dstn))
		return entry;

	for (i = 0; i < WLFC_MAC_DESC_TABLE_SIZE; i++) {
		if (table[i].occupied) {
			if (table[i].interface_id == ifid) {
				if (!memcmp(table[i].ea, dstn, ETHER_ADDR_LEN)) {
					entry = &table[i];
					break;
				}
			}
		}
	}

	return entry != NULL ? entry : &ctx->destination_entries.other;
}

static int
_dhd_wlfc_rollback_packet_toq(athost_wl_status_info_t* ctx,
	void* p, ewlfc_packet_state_t pkt_type, uint32 hslot)
{
	wlfc_mac_descriptor_t* entry;
	void* pktout;
	int rc = BCME_OK;
	int prec;

	entry = _dhd_wlfc_find_table_entry(ctx, p);
	prec = DHD_PKTTAG_FIFO(PKTTAG(p));
	if (entry != NULL) {
		if (pkt_type == eWLFC_PKTTYPE_SUPPRESSED) {
			
			if (WLFC_PKTQ_PENQ_HEAD(&entry->psq, ((prec << 1) + 1), p) == NULL) {
				WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
				rc = BCME_ERROR;
			}
		}
		else {
			
			rc = _dhd_wlfc_pullheader(ctx, p);
			if (rc != BCME_OK)          {
				WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
				
				dhd_wlfc_hanger_poppkt(ctx->hanger, hslot, &pktout, 1);
				PKTFREE(ctx->osh, p, TRUE);
				ctx->stats.rollback_failed++;
				return BCME_ERROR;
			}

			if (pkt_type == eWLFC_PKTTYPE_DELAYED) {
				
				if (WLFC_PKTQ_PENQ_HEAD(&entry->psq, (prec << 1), p) == NULL) {
					WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
					rc = BCME_ERROR;
				}
			}

			
			dhd_wlfc_hanger_poppkt(ctx->hanger, hslot, &pktout, 1);

			
			WLFC_DECR_SEQCOUNT(entry, prec);
		}
		if (!DHD_PKTTAG_CREDITCHECK(PKTTAG(p))) {
			entry->requested_credit++;
		}
	}
	else {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		rc = BCME_ERROR;
	}
	if (rc != BCME_OK)
		ctx->stats.rollback_failed++;
	else
		ctx->stats.rollback++;

	return rc;
}

static void
_dhd_wlfc_flow_control_check(athost_wl_status_info_t* ctx, struct pktq* pq, uint8 if_id)
{
	dhd_pub_t *dhdp;

	ASSERT(ctx);

	dhdp = (dhd_pub_t *)ctx->dhdp;

	if (dhdp && dhdp->skip_fc && dhdp->skip_fc())
		return;

	if ((pq->len <= WLFC_FLOWCONTROL_LOWATER) && (ctx->hostif_flow_state[if_id] == ON)) {
		
		ctx->hostif_flow_state[if_id] = OFF;
		WLFC_DBGMESG(("F"));

		dhd_txflowcontrol(ctx->dhdp, if_id, OFF);

		ctx->toggle_host_if = 0;
	}
	if ((pq->len >= WLFC_FLOWCONTROL_HIWATER) && (ctx->hostif_flow_state[if_id] == OFF)) {
		
		ctx->hostif_flow_state[if_id] = ON;
		WLFC_DBGMESG(("N"));

		dhd_txflowcontrol(ctx->dhdp, if_id, ON);

		ctx->host_ifidx = if_id;
		ctx->toggle_host_if = 1;
	}

	return;
}

static int
_dhd_wlfc_send_signalonly_packet(athost_wl_status_info_t* ctx, wlfc_mac_descriptor_t* entry,
	uint8 ta_bmp)
{
	int rc = BCME_OK;
	void* p = NULL;
	int dummylen = ((dhd_pub_t *)ctx->dhdp)->hdrlen+ 12;

	
	p = PKTGET(ctx->osh, dummylen, TRUE);
	if (p) {
		PKTPULL(ctx->osh, p, dummylen);
		DHD_PKTTAG_SET_H2DTAG(PKTTAG(p), 0);
		_dhd_wlfc_pushheader(ctx, p, TRUE, ta_bmp, entry->mac_handle, 0);
		DHD_PKTTAG_SETSIGNALONLY(PKTTAG(p), 1);
#ifdef PROP_TXSTATUS_DEBUG
		ctx->stats.signal_only_pkts_sent++;
#endif
		rc = dhd_bus_txdata(((dhd_pub_t *)ctx->dhdp)->bus, p);
		if (rc != BCME_OK) {
			PKTFREE(ctx->osh, p, TRUE);
		}
	}
	else {
		DHD_ERROR(("%s: couldn't allocate new %d-byte packet\n",
		           __FUNCTION__, dummylen));
		rc = BCME_NOMEM;
	}
	return rc;
}

static bool
_dhd_wlfc_traffic_pending_check(athost_wl_status_info_t* ctx, wlfc_mac_descriptor_t* entry,
	int prec)
{
	bool rc = FALSE;

	if (entry->state == WLFC_STATE_CLOSE) {
		if ((pktq_plen(&entry->psq, (prec << 1)) == 0) &&
			(pktq_plen(&entry->psq, ((prec << 1) + 1)) == 0)) {

			if (entry->traffic_pending_bmp & NBITVAL(prec)) {
				rc = TRUE;
				entry->traffic_pending_bmp =
					entry->traffic_pending_bmp & ~ NBITVAL(prec);
			}
		}
		else {
			if (!(entry->traffic_pending_bmp & NBITVAL(prec))) {
				rc = TRUE;
				entry->traffic_pending_bmp =
					entry->traffic_pending_bmp | NBITVAL(prec);
			}
		}
	}
	if (rc) {
		
		if (entry->traffic_lastreported_bmp != entry->traffic_pending_bmp) {
			entry->send_tim_signal = 1;
			_dhd_wlfc_send_signalonly_packet(ctx, entry, entry->traffic_pending_bmp);
			entry->traffic_lastreported_bmp = entry->traffic_pending_bmp;
			entry->send_tim_signal = 0;
		}
		else {
			rc = FALSE;
		}
	}
	return rc;
}

static int
_dhd_wlfc_enque_suppressed(athost_wl_status_info_t* ctx, int prec, void* p)
{
	wlfc_mac_descriptor_t* entry;

	entry = _dhd_wlfc_find_table_entry(ctx, p);
	if (entry == NULL) {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		return BCME_NOTFOUND;
	}
	if (WLFC_PKTQ_PENQ(&entry->psq, ((prec << 1) + 1), p) == NULL) {
		ctx->stats.delayq_full_error++;
		
		WLFC_DBGMESG(("s"));
		return BCME_ERROR;
	}
	
	_dhd_wlfc_traffic_pending_check(ctx, entry, prec);
	_dhd_wlfc_flow_control_check(ctx, &entry->psq, DHD_PKTTAG_IF(PKTTAG(p)));
	return BCME_OK;
}

static int
_dhd_wlfc_pretx_pktprocess(athost_wl_status_info_t* ctx,
	wlfc_mac_descriptor_t* entry, void* p, int header_needed, uint32* slot)
{
	int rc = BCME_OK;
	int hslot = WLFC_HANGER_MAXITEMS;
	bool send_tim_update = FALSE;
	uint32 htod = 0;
	uint8 free_ctr;

	*slot = hslot;

	if (entry == NULL) {
		entry = _dhd_wlfc_find_table_entry(ctx, p);
	}

	if (entry == NULL) {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		return BCME_ERROR;
	}
	if (entry->send_tim_signal) {
		send_tim_update = TRUE;
		entry->send_tim_signal = 0;
		entry->traffic_lastreported_bmp = entry->traffic_pending_bmp;
	}
	if (header_needed) {
		hslot = dhd_wlfc_hanger_get_free_slot(ctx->hanger);
		free_ctr = WLFC_SEQCOUNT(entry, DHD_PKTTAG_FIFO(PKTTAG(p)));
		DHD_PKTTAG_SET_H2DTAG(PKTTAG(p), htod);
		WLFC_PKTFLAG_SET_GENERATION(htod, entry->generation);
		entry->transit_count++;
	}
	else {
		hslot = WLFC_PKTID_HSLOT_GET(DHD_PKTTAG_H2DTAG(PKTTAG(p)));
		free_ctr = WLFC_PKTID_FREERUNCTR_GET(DHD_PKTTAG_H2DTAG(PKTTAG(p)));
	}
	WLFC_PKTID_HSLOT_SET(htod, hslot);
	WLFC_PKTID_FREERUNCTR_SET(htod, free_ctr);
	DHD_PKTTAG_SETPKTDIR(PKTTAG(p), 1);
	WL_TXSTATUS_SET_FLAGS(htod, WLFC_PKTFLAG_PKTFROMHOST);
	WL_TXSTATUS_SET_FIFO(htod, DHD_PKTTAG_FIFO(PKTTAG(p)));


	if (!DHD_PKTTAG_CREDITCHECK(PKTTAG(p))) {
		WLFC_PKTFLAG_SET_PKTREQUESTED(htod);
	}
	else {
		WLFC_PKTFLAG_CLR_PKTREQUESTED(htod);
	}
	if (header_needed) {
		rc = _dhd_wlfc_pushheader(ctx, p, send_tim_update,
			entry->traffic_lastreported_bmp, entry->mac_handle, htod);
		if (rc == BCME_OK) {
			DHD_PKTTAG_SET_H2DTAG(PKTTAG(p), htod);
			rc = dhd_wlfc_hanger_pushpkt(ctx->hanger, p, hslot);
			if (rc == BCME_OK) {
				
				WLFC_INCR_SEQCOUNT(entry, DHD_PKTTAG_FIFO(PKTTAG(p)));
#ifdef PROP_TXSTATUS_DEBUG
				((wlfc_hanger_t*)(ctx->hanger))->items[hslot].push_time =
					OSL_SYSUPTIME();
#endif
			}
			else {
				WLFC_DBGMESG(("%s() hanger_pushpkt() failed, rc: %d\n",
					__FUNCTION__, rc));
			}
		}
	}
	else {
		int gen;

		
		rc = _dhd_wlfc_pullheader(ctx, p);
		if (rc == BCME_OK) {
			hslot = WLFC_PKTID_HSLOT_GET(DHD_PKTTAG_H2DTAG(PKTTAG(p)));
			dhd_wlfc_hanger_get_genbit(ctx->hanger, p, hslot, &gen);

			WLFC_PKTFLAG_SET_GENERATION(htod, gen);
			free_ctr = WLFC_PKTID_FREERUNCTR_GET(DHD_PKTTAG_H2DTAG(PKTTAG(p)));
			
			_dhd_wlfc_pushheader(ctx, p, send_tim_update,
				entry->traffic_lastreported_bmp, entry->mac_handle, htod);
		}
	}
	*slot = hslot;
	return rc;
}

static int
_dhd_wlfc_is_destination_closed(athost_wl_status_info_t* ctx,
	wlfc_mac_descriptor_t* entry, int prec)
{
	if (ctx->destination_entries.interfaces[entry->interface_id].iftype ==
		WLC_E_IF_ROLE_P2P_GO) {
		if ((entry->state == WLFC_STATE_CLOSE) && (entry->requested_credit == 0) &&
			(entry->requested_packet == 0))
			return 1;
	}
	
	if (((entry->state == WLFC_STATE_CLOSE) && (entry->requested_credit == 0) &&
		(entry->requested_packet == 0)) ||
		(!(entry->ac_bitmap & (1 << prec))))
		return 1;

	return 0;
}

static void*
_dhd_wlfc_deque_delayedq(athost_wl_status_info_t* ctx,
	int prec, uint8* ac_credit_spent, uint8* needs_hdr, wlfc_mac_descriptor_t** entry_out)
{
	wlfc_mac_descriptor_t* entry;
	wlfc_mac_descriptor_t* table;
	uint8 token_pos;
	int total_entries;
	void* p = NULL;
	int pout;
	int i;

	*entry_out = NULL;
	token_pos = ctx->token_pos[prec];
	
	*ac_credit_spent = 1;
	*needs_hdr = 1;

	
	table = (wlfc_mac_descriptor_t*)&ctx->destination_entries;
	total_entries = sizeof(ctx->destination_entries)/sizeof(wlfc_mac_descriptor_t);

	for (i = 0; i < total_entries; i++) {
		entry = &table[(token_pos + i) % total_entries];
		if (entry->occupied && !entry->deleting) {
			if (!_dhd_wlfc_is_destination_closed(ctx, entry, prec)) {
				p = pktq_mdeq(&entry->psq,
					NBITVAL((prec << 1) + 1), &pout);
					*needs_hdr = 0;

				if (p == NULL) {
					if (entry->suppressed == TRUE) {
						if ((entry->suppr_transit_count <=
							entry->suppress_count)) {
							entry->suppressed = FALSE;
						} else {
							return NULL;
						}
					}
					
					p = pktq_mdeq(&entry->psq,
						NBITVAL((prec << 1)),
						&pout);
					*needs_hdr = 1;
				}

				if (p != NULL) {
					
					if (entry->requested_credit > 0) {
						entry->requested_credit--;
#ifdef PROP_TXSTATUS_DEBUG
						entry->dstncredit_sent_packets++;
#endif
						if (entry->state == WLFC_STATE_CLOSE)
							*ac_credit_spent = 0;
					}
					else if (entry->requested_packet > 0) {
						entry->requested_packet--;
						DHD_PKTTAG_SETONETIMEPKTRQST(PKTTAG(p));
						if (entry->state == WLFC_STATE_CLOSE)
							*ac_credit_spent = 0;
					}
					
					ctx->token_pos[prec] =
						(token_pos + i + 1) % total_entries;
					*entry_out = entry;
					_dhd_wlfc_flow_control_check(ctx, &entry->psq,
						DHD_PKTTAG_IF(PKTTAG(p)));
					_dhd_wlfc_traffic_pending_check(ctx, entry, prec);
					return p;
				}
			}
		}
	}
	return NULL;
}

void *
_dhd_wlfc_pktq_peek_tail(struct pktq *pq, int *prec_out)
{
	int prec;

	ASSERT(pq);

	if (pq->len == 0)
		return NULL;

	for (prec = 0; prec < pq->hi_prec; prec++)
		
		if (((prec & 1) == 0) && pq->q[prec].head)
			break;

	if (prec_out)
		*prec_out = prec;

	return (pq->q[prec].tail);
}

bool
_dhd_wlfc_prec_enq_with_drop(dhd_pub_t *dhdp, struct pktq *pq, void *pkt, int prec)
{
	void *p = NULL;
	int eprec = -1;		

	ASSERT(dhdp && pq && pkt);
	ASSERT(prec >= 0 && prec < pq->num_prec);

	if (!pktq_pfull(pq, prec) && !pktq_full(pq)) {
		pktq_penq(pq, prec, pkt);
		return TRUE;
	}

	
	if (pktq_pfull(pq, prec))
		eprec = prec;
	else if (pktq_full(pq)) {
		p = _dhd_wlfc_pktq_peek_tail(pq, &eprec);
		if (!p) {
			WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
			return FALSE;
		}
		if ((eprec > prec) || (eprec < 0)) {
			if (!pktq_pempty(pq, prec)) {
				eprec = prec;
			} else {
				return FALSE;
			}
		}
	}

	
	if (eprec >= 0) {
		
		ASSERT(!pktq_pempty(pq, eprec));
		
		dhd_prec_drop_pkts(dhdp->osh, pq, eprec);
	}

	
	p = pktq_penq(pq, prec, pkt);
	if (!p) {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		return FALSE;
	}

	return TRUE;
}

static int
_dhd_wlfc_enque_delayq(athost_wl_status_info_t* ctx, void* pktbuf, int prec)
{
	wlfc_mac_descriptor_t* entry;

	if (pktbuf != NULL) {
		entry = _dhd_wlfc_find_table_entry(ctx, pktbuf);

		if (entry == NULL) {
			WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
			return BCME_ERROR;
		}

		if (_dhd_wlfc_prec_enq_with_drop(ctx->dhdp, &entry->psq, pktbuf, (prec << 1))
			== FALSE) {
			WLFC_DBGMESG(("D"));
			
			PKTFREE(ctx->osh, pktbuf, TRUE);
			ctx->stats.delayq_full_error++;
			return BCME_ERROR;
		}

#ifdef QMONITOR
		dhd_qmon_tx(&entry->qmon);
#endif
		_dhd_wlfc_traffic_pending_check(ctx, entry, prec);

	}
	return BCME_OK;
}

bool ifpkt_fn(void* p, int ifid)
{
	return (ifid == DHD_PKTTAG_IF(PKTTAG(p)));
}

static int
_dhd_wlfc_mac_entry_update(athost_wl_status_info_t* ctx, wlfc_mac_descriptor_t* entry,
	ewlfc_mac_entry_action_t action, uint8 ifid, uint8 iftype, uint8* ea)
{
	int rc = BCME_OK;

#ifdef QMONITOR
	dhd_qmon_reset(&entry->qmon);
#endif

	if (action == eWLFC_MAC_ENTRY_ACTION_ADD) {
		entry->occupied = 1;
		entry->state = WLFC_STATE_OPEN;
		entry->requested_credit = 0;
		entry->interface_id = ifid;
		entry->iftype = iftype;
		entry->ac_bitmap = 0xff; 
		
		if (ea != NULL)
			memcpy(&entry->ea[0], ea, ETHER_ADDR_LEN);
		pktq_init(&entry->psq, WLFC_PSQ_PREC_COUNT, WLFC_PSQ_LEN);
	}
	else if (action == eWLFC_MAC_ENTRY_ACTION_UPDATE) {
		entry->occupied = 1;
		entry->state = WLFC_STATE_OPEN;
		entry->requested_credit = 0;
		entry->interface_id = ifid;
		entry->iftype = iftype;
		entry->ac_bitmap = 0xff; 
		
		if (ea != NULL)
			memcpy(&entry->ea[0], ea, ETHER_ADDR_LEN);
	}
	else if (action == eWLFC_MAC_ENTRY_ACTION_DEL) {
		entry->deleting = 1;
		dhd_wlfc_cleanup(ctx->dhdp, ifpkt_fn, ifid);
		_dhd_wlfc_flow_control_check(ctx, &entry->psq, ifid);
		entry->deleting = 0;

		entry->occupied = 0;
		entry->suppressed = 0;
		entry->state = WLFC_STATE_CLOSE;
		entry->requested_credit = 0;
		entry->transit_count = 0;
		entry->suppr_transit_count = 0;
		entry->suppress_count = 0;
		memset(&entry->ea[0], 0, ETHER_ADDR_LEN);

	}
	return rc;
}

int
_dhd_wlfc_borrow_credit(athost_wl_status_info_t* ctx, uint8 available_credit_map, int borrower_ac)
{
	int lender_ac;
	int rc = BCME_ERROR;

	if (ctx == NULL || available_credit_map == 0) {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		return BCME_BADARG;
	}

	
	for (lender_ac = 0; lender_ac <= AC_COUNT; lender_ac++) {
		if ((available_credit_map && (1 << lender_ac)) &&
		   (ctx->FIFO_credit[lender_ac] > 0)) {
			ctx->credits_borrowed[borrower_ac][lender_ac]++;
			ctx->FIFO_credit[lender_ac]--;
			rc = BCME_OK;
			break;
		}
	}

	return rc;
}

int
dhd_wlfc_interface_entry_update(void* state,
	ewlfc_mac_entry_action_t action, uint8 ifid, uint8 iftype, uint8* ea)
{
	athost_wl_status_info_t* ctx = (athost_wl_status_info_t*)state;
	wlfc_mac_descriptor_t* entry;
	int ret;

	if (ifid >= WLFC_MAX_IFNUM)
		return BCME_BADARG;

	entry = &ctx->destination_entries.interfaces[ifid];
	ret = _dhd_wlfc_mac_entry_update(ctx, entry, action, ifid, iftype, ea);
	return ret;
}

int
dhd_wlfc_FIFOcreditmap_update(void* state, uint8* credits)
{
	athost_wl_status_info_t* ctx = (athost_wl_status_info_t*)state;

	
	ctx->FIFO_credit[0] = credits[0];
	ctx->FIFO_credit[1] = credits[1];
	ctx->FIFO_credit[2] = credits[2];
	ctx->FIFO_credit[3] = credits[3];
	
	ctx->FIFO_credit[4] = credits[4];
	
	ctx->FIFO_credit[5] = 0;
	return BCME_OK;
}

int
_dhd_wlfc_handle_packet_commit(athost_wl_status_info_t* ctx, int ac,
    dhd_wlfc_commit_info_t *commit_info, f_commitpkt_t fcommit, void* commit_ctx)
{
	uint32 hslot;
	int	rc;

	DHD_PKTTAG_SETCREDITCHECK(PKTTAG(commit_info->p), commit_info->ac_fifo_credit_spent);
	rc = _dhd_wlfc_pretx_pktprocess(ctx, commit_info->mac_entry, commit_info->p,
	     commit_info->needs_hdr, &hslot);

	if (rc == BCME_OK)
		rc = fcommit(commit_ctx, commit_info->p);
	else
		ctx->stats.generic_error++;

	if (rc == BCME_OK) {
		ctx->stats.pkt2bus++;
		if (commit_info->ac_fifo_credit_spent) {
			ctx->stats.send_pkts[ac]++;
			WLFC_HOST_FIFO_CREDIT_INC_SENTCTRS(ctx, ac);
		}
	} else if (rc == BCME_NORESOURCE)
		rc = BCME_ERROR;
	else {
		rc = _dhd_wlfc_rollback_packet_toq(ctx,	commit_info->p,
		     (commit_info->pkt_type), hslot);

		rc = BCME_ERROR;
	}

	return rc;
}

int
dhd_wlfc_commit_packets(void* state, f_commitpkt_t fcommit, void* commit_ctx, void *pktbuf)
{
	int ac;
	int credit;
	int rc;
	dhd_wlfc_commit_info_t  commit_info;
	athost_wl_status_info_t* ctx = (athost_wl_status_info_t*)state;
	int credit_count = 0;
	int bus_retry_count = 0;
	uint8 ac_available = 0;  

	if ((state == NULL) ||
		(fcommit == NULL)) {
		WLFC_DBGMESG(("Error: %s():%d\n", __FUNCTION__, __LINE__));
		return BCME_BADARG;
	}

	memset(&commit_info, 0, sizeof(commit_info));


	if (pktbuf) {
		ac = DHD_PKTTAG_FIFO(PKTTAG(pktbuf));
		if (ETHER_ISMULTI(DHD_PKTTAG_DSTN(PKTTAG(pktbuf)))) {
				ASSERT(ac == AC_COUNT);
			commit_info.needs_hdr = 1;
			commit_info.mac_entry = NULL;
			commit_info.pkt_type = eWLFC_PKTTYPE_NEW;
			commit_info.p = pktbuf;
			if (ctx->FIFO_credit[ac]) {
				rc = _dhd_wlfc_handle_packet_commit(ctx, ac, &commit_info,
					fcommit, commit_ctx);

			
				if (rc == BCME_OK) {
					if (commit_info.ac_fifo_credit_spent) {
						(void) _dhd_wlfc_borrow_credit(ctx,
							ac_available, ac);
						credit_count--;
					}
				} else {
					bus_retry_count++;
					if (bus_retry_count >= BUS_RETRIES) {
						DHD_ERROR((" %s: bus error %d\n",
							__FUNCTION__, rc));
						return rc;
					}
				}
			}
		}
		else {
			
			rc = _dhd_wlfc_enque_delayq(ctx, pktbuf, ac);
		}
	}

	for (ac = AC_COUNT; ac >= 0; ac--) {

		bool bQueueIdle = TRUE;

		commit_info.needs_hdr = 1;
		commit_info.mac_entry = NULL;
		commit_info.pkt_type = eWLFC_PKTTYPE_NEW;

		for (credit = 0; credit < ctx->FIFO_credit[ac];) {
			commit_info.p = _dhd_wlfc_deque_delayedq(ctx, ac,
			                &(commit_info.ac_fifo_credit_spent),
			                &(commit_info.needs_hdr),
			                &(commit_info.mac_entry));

			if (commit_info.p == NULL)
				break;

			bQueueIdle = FALSE;

			commit_info.pkt_type = (commit_info.needs_hdr) ? eWLFC_PKTTYPE_DELAYED :
				eWLFC_PKTTYPE_SUPPRESSED;

			rc = _dhd_wlfc_handle_packet_commit(ctx, ac, &commit_info,
			     fcommit, commit_ctx);

			
			if (rc == BCME_OK) {
				if (commit_info.ac_fifo_credit_spent) {
					credit++;
				}
			}
			else {
				bus_retry_count++;
				if (bus_retry_count >= BUS_RETRIES) {
					DHD_ERROR(("%s: bus error %d\n", __FUNCTION__, rc));
					ctx->FIFO_credit[ac] -= credit;
					return rc;
				}
			}
		}

		ctx->FIFO_credit[ac] -= credit;


		
		if (bQueueIdle) {
			ac_available |= (1 << ac);
			credit_count += ctx->FIFO_credit[ac];
		}
	}

	if ((ac_available & WLFC_AC_BE_TRAFFIC_ONLY) == WLFC_AC_BE_TRAFFIC_ONLY) {

		if (ctx->allow_credit_borrow) {
			ac = 1;  
		}
		else {
			int delta;
			int curr_t = OSL_SYSUPTIME();

			if (curr_t > ctx->borrow_defer_timestamp)
				delta = curr_t - ctx->borrow_defer_timestamp;
			else
				delta = 0xffffffff + curr_t - ctx->borrow_defer_timestamp;

			if (delta >= WLFC_BORROW_DEFER_PERIOD_MS) {
				
				ctx->allow_credit_borrow = TRUE;
				ctx->borrow_defer_timestamp = 0;
			}
			return BCME_OK;
		}
	}
	else {
		
		ctx->allow_credit_borrow = FALSE;
		ctx->borrow_defer_timestamp = OSL_SYSUPTIME();
		return BCME_OK;
	}

	for (; (credit_count > 0);) {

		commit_info.p = _dhd_wlfc_deque_delayedq(ctx, ac,
		                &(commit_info.ac_fifo_credit_spent),
		                &(commit_info.needs_hdr),
		                &(commit_info.mac_entry));
		if (commit_info.p == NULL)
			break;

		commit_info.pkt_type = (commit_info.needs_hdr) ? eWLFC_PKTTYPE_DELAYED :
			eWLFC_PKTTYPE_SUPPRESSED;

		rc = _dhd_wlfc_handle_packet_commit(ctx, ac, &commit_info,
		     fcommit, commit_ctx);

		
		if (rc == BCME_OK) {
			if (commit_info.ac_fifo_credit_spent) {
				(void) _dhd_wlfc_borrow_credit(ctx, ac_available, ac);
				credit_count--;
			}
		}
		else {
			bus_retry_count++;
			if (bus_retry_count >= BUS_RETRIES) {
				DHD_ERROR(("%s: bus error %d\n", __FUNCTION__, rc));
				return rc;
			}
		}
	}
	return BCME_OK;
}

static uint8
dhd_wlfc_find_mac_desc_id_from_mac(dhd_pub_t *dhdp, uint8* ea)
{
	wlfc_mac_descriptor_t* table =
		((athost_wl_status_info_t*)dhdp->wlfc_state)->destination_entries.nodes;
	uint8 table_index;

	if (ea != NULL) {
		for (table_index = 0; table_index < WLFC_MAC_DESC_TABLE_SIZE; table_index++) {
			if ((memcmp(ea, &table[table_index].ea[0], ETHER_ADDR_LEN) == 0) &&
				table[table_index].occupied)
				return table_index;
		}
	}
	return WLFC_MAC_DESC_ID_INVALID;
}

void
dhd_wlfc_txcomplete(dhd_pub_t *dhd, void *txp, bool success)
{
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	void* p;
	int fifo_id;

	if (DHD_PKTTAG_SIGNALONLY(PKTTAG(txp))) {
#ifdef PROP_TXSTATUS_DEBUG
		wlfc->stats.signal_only_pkts_freed++;
#endif
		
		if (success)
			PKTFREE(wlfc->osh, txp, TRUE);
		return;
	}
	if (!success) {
		WLFC_DBGMESG(("At: %s():%d, bus_complete() failure for %p, htod_tag:0x%08x\n",
			__FUNCTION__, __LINE__, txp, DHD_PKTTAG_H2DTAG(PKTTAG(txp))));
		dhd_wlfc_hanger_poppkt(wlfc->hanger, WLFC_PKTID_HSLOT_GET(DHD_PKTTAG_H2DTAG
			(PKTTAG(txp))), &p, 1);

		
		dhd_txcomplete(dhd, txp, FALSE);

		
		if (DHD_PKTTAG_CREDITCHECK(PKTTAG(txp))) {
			int lender, credit_returned = 0; 

			fifo_id = DHD_PKTTAG_FIFO(PKTTAG(txp));

			
			for (lender = AC_COUNT; lender >= 0; lender--) {
				if (wlfc->credits_borrowed[fifo_id][lender] > 0) {
					wlfc->FIFO_credit[lender]++;
					wlfc->credits_borrowed[fifo_id][lender]--;
					credit_returned = 1;
					break;
				}
			}

			if (!credit_returned) {
				wlfc->FIFO_credit[fifo_id]++;
			}
		}

		PKTFREE(wlfc->osh, txp, TRUE);
	}
	return;
}

static int
dhd_wlfc_compressed_txstatus_update(dhd_pub_t *dhd, uint8* pkt_info, uint8 len)
{
	uint8 	status_flag;
	uint32	status;
	int		ret;
	int		remove_from_hanger = 1;
	void*	pktbuf;
	uint8	fifo_id;
	uint8 count = 0;
	uint32 status_g;
	uint32 hslot, hcnt;
	wlfc_mac_descriptor_t* entry = NULL;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;

	memcpy(&status, pkt_info, sizeof(uint32));
	status_flag = WL_TXSTATUS_GET_FLAGS(status);
	status_g = status & 0xff000000;
	hslot = (status & 0x00ffff00) >> 8;
	hcnt = status & 0xff;
	len =	pkt_info[4];

	wlfc->stats.txstatus_in++;

	if (status_flag == WLFC_CTL_PKTFLAG_DISCARD) {
		wlfc->stats.pkt_freed++;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_D11SUPPRESS) {
		wlfc->stats.d11_suppress++;
		remove_from_hanger = 0;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_WLSUPPRESS) {
		wlfc->stats.wl_suppress++;
		remove_from_hanger = 0;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_TOSSED_BYWLC) {
		wlfc->stats.wlc_tossed_pkts++;
	}
	while (count < len) {
		status = (status_g << 24) | (hslot << 8) | (hcnt);
		count++;
		hslot++;
		hcnt++;

		ret = dhd_wlfc_hanger_poppkt(wlfc->hanger,
			WLFC_PKTID_HSLOT_GET(status), &pktbuf, remove_from_hanger);
		if (ret != BCME_OK) {
			
			continue;
		}

		entry = _dhd_wlfc_find_table_entry(wlfc, pktbuf);

		if (!remove_from_hanger) {
			
			if (!entry->suppressed || entry->generation != WLFC_PKTID_GEN(status)) {
				entry->suppressed = TRUE;
				entry->suppress_count = pktq_mlen(&entry->psq,
					NBITVAL((WL_TXSTATUS_GET_FIFO(status) << 1) + 1));
				entry->suppr_transit_count = entry->transit_count;
			}
			entry->generation = WLFC_PKTID_GEN(status);
		}

#ifdef PROP_TXSTATUS_DEBUG
		{
			uint32 new_t = OSL_SYSUPTIME();
			uint32 old_t;
			uint32 delta;
			old_t = ((wlfc_hanger_t*)(wlfc->hanger))->items[
				WLFC_PKTID_HSLOT_GET(status)].push_time;


			wlfc->stats.latency_sample_count++;
			if (new_t > old_t)
				delta = new_t - old_t;
			else
				delta = 0xffffffff + new_t - old_t;
			wlfc->stats.total_status_latency += delta;
			wlfc->stats.latency_most_recent = delta;

			wlfc->stats.deltas[wlfc->stats.idx_delta++] = delta;
			if (wlfc->stats.idx_delta == sizeof(wlfc->stats.deltas)/sizeof(uint32))
				wlfc->stats.idx_delta = 0;
		}
#endif 

		fifo_id = DHD_PKTTAG_FIFO(PKTTAG(pktbuf));

		
		if (DHD_PKTTAG_CREDITCHECK(PKTTAG(pktbuf))) {
			if (wlfc->proptxstatus_mode == WLFC_FCMODE_IMPLIED_CREDIT) {

				int lender, credit_returned = 0; 

				
				for (lender = AC_COUNT; lender >= 0; lender--)	{
					if (wlfc->credits_borrowed[fifo_id][lender] > 0) {
						wlfc->FIFO_credit[lender]++;
						wlfc->credits_borrowed[fifo_id][lender]--;
						credit_returned = 1;
						break;
					}
				}

				if (!credit_returned) {
					wlfc->FIFO_credit[fifo_id]++;
				}
			}
		}
		else {
			if (!entry) {

				entry = _dhd_wlfc_find_table_entry(wlfc, pktbuf);
			}
			if (!DHD_PKTTAG_ONETIMEPKTRQST(PKTTAG(pktbuf)))
				entry->requested_credit++;
#ifdef PROP_TXSTATUS_DEBUG
			entry->dstncredit_acks++;
#endif
		}
		if ((status_flag == WLFC_CTL_PKTFLAG_D11SUPPRESS) ||
			(status_flag == WLFC_CTL_PKTFLAG_WLSUPPRESS)) {

			ret = _dhd_wlfc_enque_suppressed(wlfc, fifo_id, pktbuf);
			if (ret != BCME_OK) {
				
				dhd_wlfc_hanger_poppkt(wlfc->hanger, WLFC_PKTID_HSLOT_GET(status),
				&pktbuf, 1);

				
				dhd_txcomplete(dhd, pktbuf, FALSE);
				entry->transit_count--;
				DHD_WLFC_QMON_COMPLETE(entry);
				if (entry->suppressed) {
					entry->suppr_transit_count--;
				}
				PKTFREE(wlfc->osh, pktbuf, TRUE);
			} else {
				

				dhd_wlfc_hanger_mark_suppressed(wlfc->hanger,
				WLFC_PKTID_HSLOT_GET(status), WLFC_PKTID_GEN(status));
				entry->suppress_count++;
			}
		}
		else {
			dhd_txcomplete(dhd, pktbuf, TRUE);
			entry->transit_count--;
			DHD_WLFC_QMON_COMPLETE(entry);

			if (entry->suppressed) {
				entry->suppr_transit_count--;
			}
			
			PKTFREE(wlfc->osh, pktbuf, TRUE);
		}
	}
	return BCME_OK;
}

static int
dhd_wlfc_txstatus_update(dhd_pub_t *dhd, uint8* pkt_info)
{
	uint8 	status_flag;
	uint32	status;
	int		ret;
	int		remove_from_hanger = 1;
	void*	pktbuf;
	uint8	fifo_id;
	wlfc_mac_descriptor_t* entry = NULL;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;

	memcpy(&status, pkt_info, sizeof(uint32));
	status_flag = WL_TXSTATUS_GET_FLAGS(status);
	wlfc->stats.txstatus_in++;

	if (status_flag == WLFC_CTL_PKTFLAG_DISCARD) {
		wlfc->stats.pkt_freed++;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_D11SUPPRESS) {
		wlfc->stats.d11_suppress++;
		remove_from_hanger = 0;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_WLSUPPRESS) {
		wlfc->stats.wl_suppress++;
		remove_from_hanger = 0;
	}

	else if (status_flag == WLFC_CTL_PKTFLAG_TOSSED_BYWLC) {
		wlfc->stats.wlc_tossed_pkts++;
	}

	ret = dhd_wlfc_hanger_poppkt(wlfc->hanger,
		WLFC_PKTID_HSLOT_GET(status), &pktbuf, remove_from_hanger);
	if (ret != BCME_OK) {
		
		return ret;
	}

	entry = _dhd_wlfc_find_table_entry(wlfc, pktbuf);

	if (!remove_from_hanger) {
		
		if (!entry->suppressed || entry->generation != WLFC_PKTID_GEN(status)) {
			entry->suppressed = TRUE;
			entry->suppress_count = pktq_mlen(&entry->psq,
				NBITVAL((WL_TXSTATUS_GET_FIFO(status) << 1) + 1));
			entry->suppr_transit_count = entry->transit_count;
		}
		entry->generation = WLFC_PKTID_GEN(status);
	}

#ifdef PROP_TXSTATUS_DEBUG
	{
		uint32 new_t = OSL_SYSUPTIME();
		uint32 old_t;
		uint32 delta;
		old_t = ((wlfc_hanger_t*)(wlfc->hanger))->items[
			WLFC_PKTID_HSLOT_GET(status)].push_time;


		wlfc->stats.latency_sample_count++;
		if (new_t > old_t)
			delta = new_t - old_t;
		else
			delta = 0xffffffff + new_t - old_t;
		wlfc->stats.total_status_latency += delta;
		wlfc->stats.latency_most_recent = delta;

		wlfc->stats.deltas[wlfc->stats.idx_delta++] = delta;
		if (wlfc->stats.idx_delta == sizeof(wlfc->stats.deltas)/sizeof(uint32))
			wlfc->stats.idx_delta = 0;
	}
#endif 

	fifo_id = DHD_PKTTAG_FIFO(PKTTAG(pktbuf));

	
	if (DHD_PKTTAG_CREDITCHECK(PKTTAG(pktbuf))) {
		if (wlfc->proptxstatus_mode == WLFC_FCMODE_IMPLIED_CREDIT) {

			int lender, credit_returned = 0; 

			
			for (lender = AC_COUNT; lender >= 0; lender--)	{
				if (wlfc->credits_borrowed[fifo_id][lender] > 0) {
					wlfc->FIFO_credit[lender]++;
					wlfc->credits_borrowed[fifo_id][lender]--;
					credit_returned = 1;
					break;
				}
			}

			if (!credit_returned) {
				wlfc->FIFO_credit[fifo_id]++;
			}
		}
	}
	else {
		if (!entry) {

			entry = _dhd_wlfc_find_table_entry(wlfc, pktbuf);
		}
		if (!DHD_PKTTAG_ONETIMEPKTRQST(PKTTAG(pktbuf)))
			entry->requested_credit++;
#ifdef PROP_TXSTATUS_DEBUG
		entry->dstncredit_acks++;
#endif
	}
	if ((status_flag == WLFC_CTL_PKTFLAG_D11SUPPRESS) ||
		(status_flag == WLFC_CTL_PKTFLAG_WLSUPPRESS)) {

		ret = _dhd_wlfc_enque_suppressed(wlfc, fifo_id, pktbuf);
		if (ret != BCME_OK) {
			
			dhd_wlfc_hanger_poppkt(wlfc->hanger, WLFC_PKTID_HSLOT_GET(status),
			&pktbuf, 1);

			
			dhd_txcomplete(dhd, pktbuf, FALSE);
			entry->transit_count--;
			DHD_WLFC_QMON_COMPLETE(entry);
			if (entry->suppressed) {
				entry->suppr_transit_count--;
			}
			PKTFREE(wlfc->osh, pktbuf, TRUE);
		} else {
			

			dhd_wlfc_hanger_mark_suppressed(wlfc->hanger,
			WLFC_PKTID_HSLOT_GET(status), WLFC_PKTID_GEN(status));
			entry->suppress_count++;
		}
	}
	else {
		dhd_txcomplete(dhd, pktbuf, TRUE);
		entry->transit_count--;
		DHD_WLFC_QMON_COMPLETE(entry);

		
		if (entry->suppressed) {
			entry->suppr_transit_count--;
		}
		
		PKTFREE(wlfc->osh, pktbuf, TRUE);
	}
	return BCME_OK;
}

static int
dhd_wlfc_fifocreditback_indicate(dhd_pub_t *dhd, uint8* credits)
{
	int i;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	for (i = 0; i < WLFC_CTL_VALUE_LEN_FIFO_CREDITBACK; i++) {
#ifdef PROP_TXSTATUS_DEBUG
		wlfc->stats.fifo_credits_back[i] += credits[i];
#endif
		
		if (wlfc->proptxstatus_mode == WLFC_FCMODE_EXPLICIT_CREDIT)
		{
			int lender; 

			
			for (lender = AC_COUNT; (lender >= 0) && (credits[i] > 0); lender--) {
				if (wlfc->credits_borrowed[i][lender] > 0) {
					if (credits[i] >= wlfc->credits_borrowed[i][lender]) {
						credits[i] -= wlfc->credits_borrowed[i][lender];
						wlfc->FIFO_credit[lender] +=
						    wlfc->credits_borrowed[i][lender];
						wlfc->credits_borrowed[i][lender] = 0;
					}
					else {
						wlfc->credits_borrowed[i][lender] -= credits[i];
						wlfc->FIFO_credit[lender] += credits[i];
						credits[i] = 0;
					}
				}
			}

			
			if (credits[i] > 0) {
				wlfc->FIFO_credit[i] += credits[i];
			}
		}
	}

	return BCME_OK;
}

static int
dhd_wlfc_dbg_senum_check(dhd_pub_t *dhd, uint8 *value)
{
	uint32 timestamp;

	(void)dhd;

	bcopy(&value[2], &timestamp, sizeof(uint32));
	DHD_INFO(("RXPKT: SEQ: %d, timestamp %d\n", value[1], timestamp));
	return BCME_OK;
}


static int
dhd_wlfc_rssi_indicate(dhd_pub_t *dhd, uint8* rssi)
{
	(void)dhd;
	(void)rssi;
	return BCME_OK;
}

static int
dhd_wlfc_mac_table_update(dhd_pub_t *dhd, uint8* value, uint8 type)
{
	int rc;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	uint8 existing_index;
	uint8 table_index;
	uint8 ifid;
	uint8* ea;

	WLFC_DBGMESG(("%s(), mac [%02x:%02x:%02x:%02x:%02x:%02x],%s,idx:%d,id:0x%02x\n",
		__FUNCTION__, value[2], value[3], value[4], value[5], value[6], value[7],
		((type == WLFC_CTL_TYPE_MACDESC_ADD) ? "ADD":"DEL"),
		WLFC_MAC_DESC_GET_LOOKUP_INDEX(value[0]), value[0]));

	table = wlfc->destination_entries.nodes;
	table_index = WLFC_MAC_DESC_GET_LOOKUP_INDEX(value[0]);
	ifid = value[1];
	ea = &value[2];

	if (type == WLFC_CTL_TYPE_MACDESC_ADD) {
		existing_index = dhd_wlfc_find_mac_desc_id_from_mac(dhd, &value[2]);
		if (existing_index == WLFC_MAC_DESC_ID_INVALID) {
			
			if (!table[table_index].occupied) {
				table[table_index].mac_handle = value[0];
				rc = _dhd_wlfc_mac_entry_update(wlfc, &table[table_index],
				eWLFC_MAC_ENTRY_ACTION_ADD, ifid,
				wlfc->destination_entries.interfaces[ifid].iftype,
				ea);
			}
			else {
				
				wlfc->stats.mac_update_failed++;
			}
		}
		else {
			if (existing_index != table_index) {
				
				table[existing_index].occupied = 0;
				table[existing_index].state = WLFC_STATE_CLOSE;
				table[existing_index].requested_credit = 0;
				table[existing_index].interface_id = 0;
			}
		}
	}
	if (type == WLFC_CTL_TYPE_MACDESC_DEL) {
		if (table[table_index].occupied) {
				rc = _dhd_wlfc_mac_entry_update(wlfc, &table[table_index],
					eWLFC_MAC_ENTRY_ACTION_DEL, ifid,
					wlfc->destination_entries.interfaces[ifid].iftype,
					ea);
		}
		else {
			
			wlfc->stats.mac_update_failed++;
		}
	}
	BCM_REFERENCE(rc);
	return BCME_OK;
}

static int
dhd_wlfc_psmode_update(dhd_pub_t *dhd, uint8* value, uint8 type)
{
	
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	wlfc_mac_descriptor_t* desc;
	uint8 mac_handle = value[0];
	int i;

	table = wlfc->destination_entries.nodes;
	desc = &table[WLFC_MAC_DESC_GET_LOOKUP_INDEX(mac_handle)];
	if (desc->occupied) {
		
		desc->requested_credit = 0;
		if (type == WLFC_CTL_TYPE_MAC_OPEN) {
			desc->state = WLFC_STATE_OPEN;
			DHD_WLFC_CTRINC_MAC_OPEN(desc);
		}
		else {
			desc->state = WLFC_STATE_CLOSE;
			DHD_WLFC_CTRINC_MAC_CLOSE(desc);
			for (i = AC_BE; i < AC_COUNT; i++) {
				_dhd_wlfc_traffic_pending_check(wlfc, desc, i);
			}
		}
	}
	else {
		wlfc->stats.psmode_update_failed++;
	}
	return BCME_OK;
}

static int
dhd_wlfc_interface_update(dhd_pub_t *dhd, uint8* value, uint8 type)
{
	
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	uint8 if_id = value[0];

	if (if_id < WLFC_MAX_IFNUM) {
		table = wlfc->destination_entries.interfaces;
		if (table[if_id].occupied) {
			if (type == WLFC_CTL_TYPE_INTERFACE_OPEN) {
				table[if_id].state = WLFC_STATE_OPEN;
				
			}
			else {
				table[if_id].state = WLFC_STATE_CLOSE;
				
			}
			return BCME_OK;
		}
	}
	wlfc->stats.interface_update_failed++;

	return BCME_OK;
}

static int
dhd_wlfc_credit_request(dhd_pub_t *dhd, uint8* value)
{
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	wlfc_mac_descriptor_t* desc;
	uint8 mac_handle;
	uint8 credit;

	table = wlfc->destination_entries.nodes;
	mac_handle = value[1];
	credit = value[0];

	desc = &table[WLFC_MAC_DESC_GET_LOOKUP_INDEX(mac_handle)];
	if (desc->occupied) {
		desc->requested_credit = credit;

		desc->ac_bitmap = value[2];
	}
	else {
		wlfc->stats.credit_request_failed++;
	}
	return BCME_OK;
}

static int
dhd_wlfc_packet_request(dhd_pub_t *dhd, uint8* value)
{
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	wlfc_mac_descriptor_t* desc;
	uint8 mac_handle;
	uint8 packet_count;

	table = wlfc->destination_entries.nodes;
	mac_handle = value[1];
	packet_count = value[0];

	desc = &table[WLFC_MAC_DESC_GET_LOOKUP_INDEX(mac_handle)];
	if (desc->occupied) {
		desc->requested_packet = packet_count;

		desc->ac_bitmap = value[2];
	}
	else {
		wlfc->stats.packet_request_failed++;
	}
	return BCME_OK;
}

static void
dhd_wlfc_reorderinfo_indicate(uint8 *val, uint8 len, uchar *info_buf, uint *info_len)
{
	if (info_len) {
		if (info_buf) {
			bcopy(val, info_buf, len);
			*info_len = len;
		}
		else
			*info_len = 0;
	}
}

int
dhd_wlfc_parse_header_info(dhd_pub_t *dhd, void* pktbuf, int tlv_hdr_len, uchar *reorder_info_buf,
	uint *reorder_info_len)
{
	uint8 type, len;
	uint8* value;
	uint8* tmpbuf;
	uint16 remainder = tlv_hdr_len;
	uint16 processed = 0;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	tmpbuf = (uint8*)PKTDATA(dhd->osh, pktbuf);
	if (remainder) {
		while ((processed < (WLFC_MAX_PENDING_DATALEN * 2)) && (remainder > 0)) {
			type = tmpbuf[processed];
			if (type == WLFC_CTL_TYPE_FILLER) {
				remainder -= 1;
				processed += 1;
				continue;
			}

			len  = tmpbuf[processed + 1];
			value = &tmpbuf[processed + 2];

			if (remainder < (2 + len))
				break;

			remainder -= 2 + len;
			processed += 2 + len;
			if (type == WLFC_CTL_TYPE_TXSTATUS)
				dhd_wlfc_txstatus_update(dhd, value);
			if (type == WLFC_CTL_TYPE_COMP_TXSTATUS)
				dhd_wlfc_compressed_txstatus_update(dhd, value, len);

			else if (type == WLFC_CTL_TYPE_HOST_REORDER_RXPKTS)
				dhd_wlfc_reorderinfo_indicate(value, len, reorder_info_buf,
					reorder_info_len);
			else if (type == WLFC_CTL_TYPE_FIFO_CREDITBACK)
				dhd_wlfc_fifocreditback_indicate(dhd, value);

			else if (type == WLFC_CTL_TYPE_RSSI)
				dhd_wlfc_rssi_indicate(dhd, value);

			else if (type == WLFC_CTL_TYPE_MAC_REQUEST_CREDIT)
				dhd_wlfc_credit_request(dhd, value);

			else if (type == WLFC_CTL_TYPE_MAC_REQUEST_PACKET)
				dhd_wlfc_packet_request(dhd, value);

			else if ((type == WLFC_CTL_TYPE_MAC_OPEN) ||
				(type == WLFC_CTL_TYPE_MAC_CLOSE))
				dhd_wlfc_psmode_update(dhd, value, type);

			else if ((type == WLFC_CTL_TYPE_MACDESC_ADD) ||
				(type == WLFC_CTL_TYPE_MACDESC_DEL))
				dhd_wlfc_mac_table_update(dhd, value, type);

			else if (type == WLFC_CTL_TYPE_TRANS_ID)
				dhd_wlfc_dbg_senum_check(dhd, value);

			else if ((type == WLFC_CTL_TYPE_INTERFACE_OPEN) ||
				(type == WLFC_CTL_TYPE_INTERFACE_CLOSE)) {
				dhd_wlfc_interface_update(dhd, value, type);
			}
		}
		if (remainder != 0) {
			
			wlfc->stats.tlv_parse_failed++;
		}
	}
	return BCME_OK;
}

int
dhd_wlfc_init(dhd_pub_t *dhd)
{
	char iovbuf[12]; 
	
	uint32 tlv = dhd->wlfc_enabled?
		WLFC_FLAGS_RSSI_SIGNALS |
		WLFC_FLAGS_XONXOFF_SIGNALS |
		WLFC_FLAGS_CREDIT_STATUS_SIGNALS |
		WLFC_FLAGS_HOST_PROPTXSTATUS_ACTIVE |
		WLFC_FLAGS_HOST_RXRERODER_ACTIVE : 0;
		



	
	bcm_mkiovar("tlv", (char *)&tlv, 4, iovbuf, sizeof(iovbuf));
	if (dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0) < 0) {
		DHD_ERROR(("dhd_wlfc_init(): failed to enable/disable bdcv2 tlv signaling\n"));
	}
	else {
		DHD_ERROR(("dhd_wlfc_init(): successfully %s bdcv2 tlv signaling, %d\n",
			dhd->wlfc_enabled?"enabled":"disabled", tlv));
	}
	return BCME_OK;
}

int
dhd_wlfc_enable(dhd_pub_t *dhd)
{
	int i;
	athost_wl_status_info_t* wlfc;

	if (!dhd->wlfc_enabled || dhd->wlfc_state)
		return BCME_OK;

	
	dhd->wlfc_state = MALLOC(dhd->osh, sizeof(athost_wl_status_info_t));
	if (dhd->wlfc_state == NULL)
		return BCME_NOMEM;

	
	wlfc = (athost_wl_status_info_t*)dhd->wlfc_state;
	memset(wlfc, 0, sizeof(athost_wl_status_info_t));

	
	wlfc->osh = dhd->osh;
	wlfc->dhdp = dhd;

	wlfc->hanger =
		dhd_wlfc_hanger_create(dhd->osh, WLFC_HANGER_MAXITEMS);
	if (wlfc->hanger == NULL) {
		MFREE(dhd->osh, dhd->wlfc_state, sizeof(athost_wl_status_info_t));
		dhd->wlfc_state = NULL;
		DHD_ERROR(("Failed to malloc dhd->wlfc_state\n"));
		return BCME_NOMEM;
	}

	
	for (i = 0; i < WLFC_MAX_IFNUM; i++) {
		wlfc->hostif_flow_state[i] = OFF;
	}

	wlfc->destination_entries.other.state = WLFC_STATE_OPEN;
	
	wlfc->destination_entries.other.ac_bitmap = 0x1f;
	wlfc->destination_entries.other.interface_id = 0;

	wlfc->proptxstatus_mode = WLFC_FCMODE_EXPLICIT_CREDIT;

	wlfc->allow_credit_borrow = TRUE;
	wlfc->borrow_defer_timestamp = 0;

	return BCME_OK;
}

void
dhd_wlfc_cleanup(dhd_pub_t *dhd, ifpkt_cb_t fn, int arg)
{
	int i;
	int total_entries;
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;
	wlfc_mac_descriptor_t* table;
	wlfc_hanger_t* h;
	int prec;
	void *pkt = NULL;
	struct pktq *txq = NULL;
	if (dhd->wlfc_state == NULL)
		return;
	
	txq = dhd_bus_txq(dhd->bus);
	
	h = (wlfc_hanger_t*)wlfc->hanger;
	total_entries = sizeof(wlfc->destination_entries)/sizeof(wlfc_mac_descriptor_t);
	
	table = (wlfc_mac_descriptor_t*)&wlfc->destination_entries;

	for (i = 0; i < total_entries; i++) {
		if (table[i].occupied && (fn == NULL || (arg == table[i].interface_id))) {
			if (table[i].psq.len) {
				WLFC_DBGMESG(("%s(): DELAYQ[%d].len = %d\n",
					__FUNCTION__, i, table[i].psq.len));
				
				pktq_flush(wlfc->osh, &table[i].psq, TRUE, fn, arg);
			}
			if (fn == NULL)
				table[i].occupied = 0;
		}
	}
	for (prec = 0; prec < txq->num_prec; prec++) {
		pkt = pktq_pdeq_with_fn(txq, prec, fn, arg);
		while (pkt) {
			for (i = 0; i < h->max_items; i++) {
				if (pkt == h->items[i].pkt) {
					if (h->items[i].state == WLFC_HANGER_ITEM_STATE_INUSE) {
						PKTFREE(wlfc->osh, h->items[i].pkt, TRUE);
						h->items[i].state = WLFC_HANGER_ITEM_STATE_FREE;
					} else if (h->items[i].state ==
						WLFC_HANGER_ITEM_STATE_INUSE_SUPPRESSED) {
						
						h->items[i].state = WLFC_HANGER_ITEM_STATE_FREE;
					}
					break;
				}
			}
			pkt = pktq_pdeq(txq, prec);
		}
	}
	
	for (i = 0; i < h->max_items; i++) {
		if (h->items[i].state == WLFC_HANGER_ITEM_STATE_INUSE) {
			if (fn == NULL || (*fn)(h->items[i].pkt, arg)) {
				PKTFREE(wlfc->osh, h->items[i].pkt, TRUE);
				h->items[i].state = WLFC_HANGER_ITEM_STATE_FREE;
			}
		} else if (h->items[i].state == WLFC_HANGER_ITEM_STATE_INUSE_SUPPRESSED) {
			if (fn == NULL || (*fn)(h->items[i].pkt, arg)) {
				
				h->items[i].state = WLFC_HANGER_ITEM_STATE_FREE;
			}
		}
	}
	return;
}

void
dhd_wlfc_deinit(dhd_pub_t *dhd)
{
	
	athost_wl_status_info_t* wlfc = (athost_wl_status_info_t*)
		dhd->wlfc_state;

	dhd_os_wlfc_block(dhd);
	if (dhd->wlfc_state == NULL) {
		dhd_os_wlfc_unblock(dhd);
		return;
	}

#ifdef PROP_TXSTATUS_DEBUG
	{
		int i;
		wlfc_hanger_t* h = (wlfc_hanger_t*)wlfc->hanger;
		for (i = 0; i < h->max_items; i++) {
			if (h->items[i].state != WLFC_HANGER_ITEM_STATE_FREE) {
				WLFC_DBGMESG(("%s() pkt[%d] = 0x%p, FIFO_credit_used:%d\n",
					__FUNCTION__, i, h->items[i].pkt,
					DHD_PKTTAG_CREDITCHECK(PKTTAG(h->items[i].pkt))));
			}
		}
	}
#endif
	
	dhd_wlfc_hanger_delete(dhd->osh, wlfc->hanger);

	
	MFREE(dhd->osh, dhd->wlfc_state, sizeof(athost_wl_status_info_t));
	dhd->wlfc_state = NULL;
	dhd_os_wlfc_unblock(dhd);

	return;
}
#endif 
