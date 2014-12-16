#ifndef LLC_PDU_H
#define LLC_PDU_H
/*
 * Copyright (c) 1997 by Procom Technology,Inc.
 * 		 2001-2003 by Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 *
 * This program can be redistributed or modified under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * This program is distributed without any warranty or implied warranty
 * of merchantability or fitness for a particular purpose.
 *
 * See the GNU General Public License for more details.
 */

#include <linux/if_ether.h>
#include <linux/if_tr.h>

#define LLC_PDU_LEN_I	4       
#define LLC_PDU_LEN_S	4
#define LLC_PDU_LEN_U	3       
#define LLC_GLOBAL_SAP	0xFF
#define LLC_NULL_SAP	0x00	
#define LLC_MGMT_INDIV	0x02	
#define LLC_MGMT_GRP	0x03	
#define LLC_RDE_SAP	0xA6	

#define LLC_ISO_RESERVED_SAP	0x02
#define LLC_SAP_GROUP_DSAP	0x01
#define LLC_SAP_RESP_SSAP	0x01

#define LLC_PDU_GROUP_DSAP_MASK    0x01
#define LLC_PDU_IS_GROUP_DSAP(pdu)      \
	((pdu->dsap & LLC_PDU_GROUP_DSAP_MASK) ? 0 : 1)
#define LLC_PDU_IS_INDIV_DSAP(pdu)      \
	(!(pdu->dsap & LLC_PDU_GROUP_DSAP_MASK) ? 0 : 1)

#define LLC_PDU_CMD_RSP_MASK	0x01
#define LLC_PDU_CMD		0
#define LLC_PDU_RSP		1
#define LLC_PDU_IS_CMD(pdu)    ((pdu->ssap & LLC_PDU_RSP) ? 0 : 1)
#define LLC_PDU_IS_RSP(pdu)    ((pdu->ssap & LLC_PDU_RSP) ? 1 : 0)

#define LLC_PDU_TYPE_I_MASK    0x01	
#define LLC_PDU_TYPE_S_MASK    0x03
#define LLC_PDU_TYPE_U_MASK    0x03	
#define LLC_PDU_TYPE_MASK      0x03

#define LLC_PDU_TYPE_I	0	
#define LLC_PDU_TYPE_S	1	
#define LLC_PDU_TYPE_U	3	

#define LLC_PDU_TYPE_IS_I(pdu) \
	((!(pdu->ctrl_1 & LLC_PDU_TYPE_I_MASK)) ? 1 : 0)

#define LLC_PDU_TYPE_IS_U(pdu) \
	(((pdu->ctrl_1 & LLC_PDU_TYPE_U_MASK) == LLC_PDU_TYPE_U) ? 1 : 0)

#define LLC_PDU_TYPE_IS_S(pdu) \
	(((pdu->ctrl_1 & LLC_PDU_TYPE_S_MASK) == LLC_PDU_TYPE_S) ? 1 : 0)

#define LLC_U_PF_BIT_MASK      0x10	
#define LLC_U_PF_IS_1(pdu)     ((pdu->ctrl_1 & LLC_U_PF_BIT_MASK) ? 1 : 0)
#define LLC_U_PF_IS_0(pdu)     ((!(pdu->ctrl_1 & LLC_U_PF_BIT_MASK)) ? 1 : 0)

#define LLC_U_PDU_CMD_MASK     0xEC	
#define LLC_U_PDU_CMD(pdu)     (pdu->ctrl_1 & LLC_U_PDU_CMD_MASK)
#define LLC_U_PDU_RSP(pdu)     (pdu->ctrl_1 & LLC_U_PDU_CMD_MASK)

#define LLC_1_PDU_CMD_UI       0x00	
#define LLC_1_PDU_CMD_XID      0xAC
#define LLC_1_PDU_CMD_TEST     0xE0

#define LLC_2_PDU_CMD_SABME    0x6C	
#define LLC_2_PDU_CMD_DISC     0x40
#define LLC_2_PDU_RSP_UA       0x60
#define LLC_2_PDU_RSP_DM       0x0C
#define LLC_2_PDU_RSP_FRMR     0x84



#define LLC_XID_FMT_ID		0x81	

#define LLC_XID_CLASS_ZEROS_MASK	0xE0	
#define LLC_XID_CLASS_MASK		0x1F	

#define LLC_XID_NULL_CLASS_1	0x01	
#define LLC_XID_NULL_CLASS_2	0x03
#define LLC_XID_NULL_CLASS_3	0x05
#define LLC_XID_NULL_CLASS_4	0x07

#define LLC_XID_NNULL_TYPE_1	0x01	
#define LLC_XID_NNULL_TYPE_2	0x02
#define LLC_XID_NNULL_TYPE_3	0x04
#define LLC_XID_NNULL_TYPE_1_2	0x03
#define LLC_XID_NNULL_TYPE_1_3	0x05
#define LLC_XID_NNULL_TYPE_2_3	0x06
#define LLC_XID_NNULL_ALL		0x07

#define LLC_XID_RW_MASK	0xFE	

#define LLC_XID_MIN_RW	0x02	


#define LLC_2_SEQ_NBR_MODULO   ((u8) 128)

#define LLC_I_GET_NS(pdu)     (u8)((pdu->ctrl_1 & 0xFE) >> 1)
#define LLC_I_GET_NR(pdu)     (u8)((pdu->ctrl_2 & 0xFE) >> 1)

#define LLC_I_PF_BIT_MASK      0x01

#define LLC_I_PF_IS_0(pdu)     ((!(pdu->ctrl_2 & LLC_I_PF_BIT_MASK)) ? 1 : 0)
#define LLC_I_PF_IS_1(pdu)     ((pdu->ctrl_2 & LLC_I_PF_BIT_MASK) ? 1 : 0)


#define LLC_S_PDU_CMD_MASK     0x0C
#define LLC_S_PDU_CMD(pdu)     (pdu->ctrl_1 & LLC_S_PDU_CMD_MASK)
#define LLC_S_PDU_RSP(pdu)     (pdu->ctrl_1 & LLC_S_PDU_CMD_MASK)

#define LLC_2_PDU_CMD_RR       0x00	
#define LLC_2_PDU_RSP_RR       0x00	
#define LLC_2_PDU_CMD_REJ      0x08	
#define LLC_2_PDU_RSP_REJ      0x08	
#define LLC_2_PDU_CMD_RNR      0x04	
#define LLC_2_PDU_RSP_RNR      0x04	

#define LLC_S_PF_BIT_MASK      0x01
#define LLC_S_PF_IS_0(pdu)     ((!(pdu->ctrl_2 & LLC_S_PF_BIT_MASK)) ? 1 : 0)
#define LLC_S_PF_IS_1(pdu)     ((pdu->ctrl_2 & LLC_S_PF_BIT_MASK) ? 1 : 0)

#define PDU_SUPV_GET_Nr(pdu)   ((pdu->ctrl_2 & 0xFE) >> 1)
#define PDU_GET_NEXT_Vr(sn)    (++sn & ~LLC_2_SEQ_NBR_MODULO)


#define FRMR_INFO_LENGTH       5	

#define FRMR_INFO_SET_REJ_CNTRL(info,rej_ctrl) \
	info->rej_pdu_ctrl = ((*((u8 *) rej_ctrl) & \
				LLC_PDU_TYPE_U) != LLC_PDU_TYPE_U ? \
				(u16)*((u16 *) rej_ctrl) : \
				(((u16) *((u8 *) rej_ctrl)) & 0x00FF))

#define FRMR_INFO_SET_Vs(info,vs) (info->curr_ssv = (((u8) vs) << 1))
#define FRMR_INFO_SET_Vr(info,vr) (info->curr_rsv = (((u8) vr) << 1))

#define FRMR_INFO_SET_C_R_BIT(info, cr)  (info->curr_rsv |= (((u8) cr) & 0x01))

#define FRMR_INFO_SET_INVALID_PDU_CTRL_IND(info, ind) \
       (info->ind_bits = ((info->ind_bits & 0xFE) | (((u8) ind) & 0x01)))

#define FRMR_INFO_SET_INVALID_PDU_INFO_IND(info, ind) \
       (info->ind_bits = ( (info->ind_bits & 0xFD) | (((u8) ind) & 0x02)))

#define FRMR_INFO_SET_PDU_INFO_2LONG_IND(info, ind) \
       (info->ind_bits = ( (info->ind_bits & 0xFB) | (((u8) ind) & 0x04)))

#define FRMR_INFO_SET_PDU_INVALID_Nr_IND(info, ind) \
       (info->ind_bits = ( (info->ind_bits & 0xF7) | (((u8) ind) & 0x08)))

#define FRMR_INFO_SET_PDU_INVALID_Ns_IND(info, ind) \
       (info->ind_bits = ( (info->ind_bits & 0xEF) | (((u8) ind) & 0x10)))

struct llc_pdu_sn {
	u8 dsap;
	u8 ssap;
	u8 ctrl_1;
	u8 ctrl_2;
} __packed;

static inline struct llc_pdu_sn *llc_pdu_sn_hdr(struct sk_buff *skb)
{
	return (struct llc_pdu_sn *)skb_network_header(skb);
}

struct llc_pdu_un {
	u8 dsap;
	u8 ssap;
	u8 ctrl_1;
} __packed;

static inline struct llc_pdu_un *llc_pdu_un_hdr(struct sk_buff *skb)
{
	return (struct llc_pdu_un *)skb_network_header(skb);
}

static inline void llc_pdu_header_init(struct sk_buff *skb, u8 type,
				       u8 ssap, u8 dsap, u8 cr)
{
	const int hlen = type == LLC_PDU_TYPE_U ? 3 : 4;
	struct llc_pdu_un *pdu;

	skb_push(skb, hlen);
	skb_reset_network_header(skb);
	pdu = llc_pdu_un_hdr(skb);
	pdu->dsap = dsap;
	pdu->ssap = ssap;
	pdu->ssap |= cr;
}

static inline void llc_pdu_decode_sa(struct sk_buff *skb, u8 *sa)
{
	if (skb->protocol == htons(ETH_P_802_2))
		memcpy(sa, eth_hdr(skb)->h_source, ETH_ALEN);
	else if (skb->protocol == htons(ETH_P_TR_802_2)) {
		memcpy(sa, tr_hdr(skb)->saddr, ETH_ALEN);
		*sa &= 0x7F;
	}
}

static inline void llc_pdu_decode_da(struct sk_buff *skb, u8 *da)
{
	if (skb->protocol == htons(ETH_P_802_2))
		memcpy(da, eth_hdr(skb)->h_dest, ETH_ALEN);
	else if (skb->protocol == htons(ETH_P_TR_802_2))
		memcpy(da, tr_hdr(skb)->daddr, ETH_ALEN);
}

static inline void llc_pdu_decode_ssap(struct sk_buff *skb, u8 *ssap)
{
	*ssap = llc_pdu_un_hdr(skb)->ssap & 0xFE;
}

static inline void llc_pdu_decode_dsap(struct sk_buff *skb, u8 *dsap)
{
	*dsap = llc_pdu_un_hdr(skb)->dsap & 0xFE;
}

static inline void llc_pdu_init_as_ui_cmd(struct sk_buff *skb)
{
	struct llc_pdu_un *pdu = llc_pdu_un_hdr(skb);

	pdu->ctrl_1  = LLC_PDU_TYPE_U;
	pdu->ctrl_1 |= LLC_1_PDU_CMD_UI;
}

static inline void llc_pdu_init_as_test_cmd(struct sk_buff *skb)
{
	struct llc_pdu_un *pdu = llc_pdu_un_hdr(skb);

	pdu->ctrl_1  = LLC_PDU_TYPE_U;
	pdu->ctrl_1 |= LLC_1_PDU_CMD_TEST;
	pdu->ctrl_1 |= LLC_U_PF_BIT_MASK;
}

static inline void llc_pdu_init_as_test_rsp(struct sk_buff *skb,
					    struct sk_buff *ev_skb)
{
	struct llc_pdu_un *pdu = llc_pdu_un_hdr(skb);

	pdu->ctrl_1  = LLC_PDU_TYPE_U;
	pdu->ctrl_1 |= LLC_1_PDU_CMD_TEST;
	pdu->ctrl_1 |= LLC_U_PF_BIT_MASK;
	if (ev_skb->protocol == htons(ETH_P_802_2)) {
		struct llc_pdu_un *ev_pdu = llc_pdu_un_hdr(ev_skb);
		int dsize;

		dsize = ntohs(eth_hdr(ev_skb)->h_proto) - 3;
		memcpy(((u8 *)pdu) + 3, ((u8 *)ev_pdu) + 3, dsize);
		skb_put(skb, dsize);
	}
}

struct llc_xid_info {
	u8 fmt_id;	
	u8 type;	
	u8 rw;		
} __packed;

static inline void llc_pdu_init_as_xid_cmd(struct sk_buff *skb,
					   u8 svcs_supported, u8 rx_window)
{
	struct llc_xid_info *xid_info;
	struct llc_pdu_un *pdu = llc_pdu_un_hdr(skb);

	pdu->ctrl_1	 = LLC_PDU_TYPE_U;
	pdu->ctrl_1	|= LLC_1_PDU_CMD_XID;
	pdu->ctrl_1	|= LLC_U_PF_BIT_MASK;
	xid_info	 = (struct llc_xid_info *)(((u8 *)&pdu->ctrl_1) + 1);
	xid_info->fmt_id = LLC_XID_FMT_ID;	
	xid_info->type	 = svcs_supported;
	xid_info->rw	 = rx_window << 1;	
	skb_put(skb, sizeof(struct llc_xid_info));
}

static inline void llc_pdu_init_as_xid_rsp(struct sk_buff *skb,
					   u8 svcs_supported, u8 rx_window)
{
	struct llc_xid_info *xid_info;
	struct llc_pdu_un *pdu = llc_pdu_un_hdr(skb);

	pdu->ctrl_1	 = LLC_PDU_TYPE_U;
	pdu->ctrl_1	|= LLC_1_PDU_CMD_XID;
	pdu->ctrl_1	|= LLC_U_PF_BIT_MASK;

	xid_info	 = (struct llc_xid_info *)(((u8 *)&pdu->ctrl_1) + 1);
	xid_info->fmt_id = LLC_XID_FMT_ID;
	xid_info->type	 = svcs_supported;
	xid_info->rw	 = rx_window << 1;
	skb_put(skb, sizeof(struct llc_xid_info));
}

struct llc_frmr_info {
	u16 rej_pdu_ctrl;	
	u8  curr_ssv;		
	u8  curr_rsv;		
	u8  ind_bits;		
} __packed;

extern void llc_pdu_set_cmd_rsp(struct sk_buff *skb, u8 type);
extern void llc_pdu_set_pf_bit(struct sk_buff *skb, u8 bit_value);
extern void llc_pdu_decode_pf_bit(struct sk_buff *skb, u8 *pf_bit);
extern void llc_pdu_init_as_disc_cmd(struct sk_buff *skb, u8 p_bit);
extern void llc_pdu_init_as_i_cmd(struct sk_buff *skb, u8 p_bit, u8 ns, u8 nr);
extern void llc_pdu_init_as_rej_cmd(struct sk_buff *skb, u8 p_bit, u8 nr);
extern void llc_pdu_init_as_rnr_cmd(struct sk_buff *skb, u8 p_bit, u8 nr);
extern void llc_pdu_init_as_rr_cmd(struct sk_buff *skb, u8 p_bit, u8 nr);
extern void llc_pdu_init_as_sabme_cmd(struct sk_buff *skb, u8 p_bit);
extern void llc_pdu_init_as_dm_rsp(struct sk_buff *skb, u8 f_bit);
extern void llc_pdu_init_as_frmr_rsp(struct sk_buff *skb,
				     struct llc_pdu_sn *prev_pdu,
				     u8 f_bit, u8 vs, u8 vr, u8 vzyxw);
extern void llc_pdu_init_as_rr_rsp(struct sk_buff *skb, u8 f_bit, u8 nr);
extern void llc_pdu_init_as_rej_rsp(struct sk_buff *skb, u8 f_bit, u8 nr);
extern void llc_pdu_init_as_rnr_rsp(struct sk_buff *skb, u8 f_bit, u8 nr);
extern void llc_pdu_init_as_ua_rsp(struct sk_buff *skb, u8 f_bit);
#endif 
