
#include <linux/module.h>
#include <linux/tcp.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <linux/netfilter/nf_conntrack_proto_gre.h>
#include <linux/netfilter/nf_conntrack_pptp.h>

#define NF_NAT_PPTP_VERSION "3.0"

#define REQ_CID(req, off)		(*(__be16 *)((char *)(req) + (off)))

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@gnumonks.org>");
MODULE_DESCRIPTION("Netfilter NAT helper module for PPTP");
MODULE_ALIAS("ip_nat_pptp");

static void pptp_nat_expected(struct nf_conn *ct,
			      struct nf_conntrack_expect *exp)
{
	struct net *net = nf_ct_net(ct);
	const struct nf_conn *master = ct->master;
	struct nf_conntrack_expect *other_exp;
	struct nf_conntrack_tuple t;
	const struct nf_ct_pptp_master *ct_pptp_info;
	const struct nf_nat_pptp *nat_pptp_info;
	struct nf_nat_ipv4_range range;

	ct_pptp_info = &nfct_help(master)->help.ct_pptp_info;
	nat_pptp_info = &nfct_nat(master)->help.nat_pptp_info;

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(ct_pptp_info) || (!ct_pptp_info))
		printk(KERN_ERR "[NET] ct_pptp_info is NULL in %s!\n", __func__);

	if (IS_ERR(nat_pptp_info) || (!nat_pptp_info))
		printk(KERN_ERR "[NET] nat_pptp_info is NULL in %s!\n", __func__);
#endif

	
	if (exp->dir == IP_CT_DIR_ORIGINAL) {
		pr_debug("we are PNS->PAC\n");
		
		t.src.l3num = AF_INET;
		t.src.u3.ip = master->tuplehash[!exp->dir].tuple.src.u3.ip;
		t.src.u.gre.key = ct_pptp_info->pac_call_id;
		t.dst.u3.ip = master->tuplehash[!exp->dir].tuple.dst.u3.ip;
		t.dst.u.gre.key = ct_pptp_info->pns_call_id;
		t.dst.protonum = IPPROTO_GRE;
	} else {
		pr_debug("we are PAC->PNS\n");
		
		t.src.l3num = AF_INET;
		t.src.u3.ip = master->tuplehash[!exp->dir].tuple.src.u3.ip;
		t.src.u.gre.key = nat_pptp_info->pns_call_id;
		t.dst.u3.ip = master->tuplehash[!exp->dir].tuple.dst.u3.ip;
		t.dst.u.gre.key = nat_pptp_info->pac_call_id;
		t.dst.protonum = IPPROTO_GRE;
	}

	pr_debug("trying to unexpect other dir: ");
	nf_ct_dump_tuple_ip(&t);
	other_exp = nf_ct_expect_find_get(net, nf_ct_zone(ct), &t);
	if (other_exp) {
		nf_ct_unexpect_related(other_exp);
		nf_ct_expect_put(other_exp);
		pr_debug("success\n");
	} else {
		pr_debug("not found!\n");
	}

	
	BUG_ON(ct->status & IPS_NAT_DONE_MASK);

	
	range.flags = NF_NAT_RANGE_MAP_IPS;
	range.min_ip = range.max_ip
		= ct->master->tuplehash[!exp->dir].tuple.dst.u3.ip;
	if (exp->dir == IP_CT_DIR_ORIGINAL) {
		range.flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
		range.min = range.max = exp->saved_proto;
	}
	nf_nat_setup_info(ct, &range, NF_NAT_MANIP_SRC);

	
	range.flags = NF_NAT_RANGE_MAP_IPS;
	range.min_ip = range.max_ip
		= ct->master->tuplehash[!exp->dir].tuple.src.u3.ip;
	if (exp->dir == IP_CT_DIR_REPLY) {
		range.flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
		range.min = range.max = exp->saved_proto;
	}
	nf_nat_setup_info(ct, &range, NF_NAT_MANIP_DST);
}

static int
pptp_outbound_pkt(struct sk_buff *skb,
		  struct nf_conn *ct,
		  enum ip_conntrack_info ctinfo,
		  struct PptpControlHeader *ctlh,
		  union pptp_ctrl_union *pptpReq)

{
	struct nf_ct_pptp_master *ct_pptp_info;
	struct nf_nat_pptp *nat_pptp_info;
	u_int16_t msg;
	__be16 new_callid;
	unsigned int cid_off;

	ct_pptp_info  = &nfct_help(ct)->help.ct_pptp_info;
	nat_pptp_info = &nfct_nat(ct)->help.nat_pptp_info;

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(ct_pptp_info) || (!ct_pptp_info))
		printk(KERN_ERR "[NET] ct_pptp_info is NULL in %s!\n", __func__);

	if (IS_ERR(nat_pptp_info) || (!nat_pptp_info))
		printk(KERN_ERR "[NET] nat_pptp_info is NULL in %s!\n", __func__);
#endif

	new_callid = ct_pptp_info->pns_call_id;

	switch (msg = ntohs(ctlh->messageType)) {
	case PPTP_OUT_CALL_REQUEST:
		cid_off = offsetof(union pptp_ctrl_union, ocreq.callID);

		
		nat_pptp_info->pns_call_id = ct_pptp_info->pns_call_id;

		new_callid = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port;

		
		ct_pptp_info->pns_call_id = new_callid;
		break;
	case PPTP_IN_CALL_REPLY:
		cid_off = offsetof(union pptp_ctrl_union, icack.callID);
		break;
	case PPTP_CALL_CLEAR_REQUEST:
		cid_off = offsetof(union pptp_ctrl_union, clrreq.callID);
		break;
	default:
		pr_debug("unknown outbound packet 0x%04x:%s\n", msg,
			 msg <= PPTP_MSG_MAX ? pptp_msg_name[msg] :
					       pptp_msg_name[0]);
		
	case PPTP_SET_LINK_INFO:
		
	case PPTP_START_SESSION_REQUEST:
	case PPTP_START_SESSION_REPLY:
	case PPTP_STOP_SESSION_REQUEST:
	case PPTP_STOP_SESSION_REPLY:
	case PPTP_ECHO_REQUEST:
	case PPTP_ECHO_REPLY:
		
		return NF_ACCEPT;
	}

	pr_debug("altering call id from 0x%04x to 0x%04x\n",
		 ntohs(REQ_CID(pptpReq, cid_off)), ntohs(new_callid));

	
	if (nf_nat_mangle_tcp_packet(skb, ct, ctinfo,
				     cid_off + sizeof(struct pptp_pkt_hdr) +
				     sizeof(struct PptpControlHeader),
				     sizeof(new_callid), (char *)&new_callid,
				     sizeof(new_callid)) == 0)
		return NF_DROP;
	return NF_ACCEPT;
}

static void
pptp_exp_gre(struct nf_conntrack_expect *expect_orig,
	     struct nf_conntrack_expect *expect_reply)
{
	const struct nf_conn *ct = expect_orig->master;
	struct nf_ct_pptp_master *ct_pptp_info;
	struct nf_nat_pptp *nat_pptp_info;

	ct_pptp_info  = &nfct_help(ct)->help.ct_pptp_info;
	nat_pptp_info = &nfct_nat(ct)->help.nat_pptp_info;

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(ct_pptp_info) || (!ct_pptp_info))
		printk(KERN_ERR "[NET] ct_pptp_info is NULL in %s!\n", __func__);

	if (IS_ERR(nat_pptp_info) || (!nat_pptp_info))
		printk(KERN_ERR "[NET] nat_pptp_info is NULL in %s!\n", __func__);
#endif

	
	nat_pptp_info->pac_call_id = ct_pptp_info->pac_call_id;

	
	expect_orig->saved_proto.gre.key = ct_pptp_info->pns_call_id;
	expect_orig->tuple.src.u.gre.key = nat_pptp_info->pns_call_id;
	expect_orig->tuple.dst.u.gre.key = ct_pptp_info->pac_call_id;
	expect_orig->dir = IP_CT_DIR_ORIGINAL;

	
	expect_reply->saved_proto.gre.key = nat_pptp_info->pns_call_id;
	expect_reply->tuple.src.u.gre.key = nat_pptp_info->pac_call_id;
	expect_reply->tuple.dst.u.gre.key = ct_pptp_info->pns_call_id;
	expect_reply->dir = IP_CT_DIR_REPLY;
}

static int
pptp_inbound_pkt(struct sk_buff *skb,
		 struct nf_conn *ct,
		 enum ip_conntrack_info ctinfo,
		 struct PptpControlHeader *ctlh,
		 union pptp_ctrl_union *pptpReq)
{
	const struct nf_nat_pptp *nat_pptp_info;
	u_int16_t msg;
	__be16 new_pcid;
	unsigned int pcid_off;

	nat_pptp_info = &nfct_nat(ct)->help.nat_pptp_info;
	new_pcid = nat_pptp_info->pns_call_id;

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(nat_pptp_info) || (!nat_pptp_info))
		printk(KERN_ERR "[NET] nat_pptp_info is NULL in %s!\n", __func__);
#endif

	switch (msg = ntohs(ctlh->messageType)) {
	case PPTP_OUT_CALL_REPLY:
		pcid_off = offsetof(union pptp_ctrl_union, ocack.peersCallID);
		break;
	case PPTP_IN_CALL_CONNECT:
		pcid_off = offsetof(union pptp_ctrl_union, iccon.peersCallID);
		break;
	case PPTP_IN_CALL_REQUEST:
		
		return NF_ACCEPT;
	case PPTP_WAN_ERROR_NOTIFY:
		pcid_off = offsetof(union pptp_ctrl_union, wanerr.peersCallID);
		break;
	case PPTP_CALL_DISCONNECT_NOTIFY:
		pcid_off = offsetof(union pptp_ctrl_union, disc.callID);
		break;
	case PPTP_SET_LINK_INFO:
		pcid_off = offsetof(union pptp_ctrl_union, setlink.peersCallID);
		break;
	default:
		pr_debug("unknown inbound packet %s\n",
			 msg <= PPTP_MSG_MAX ? pptp_msg_name[msg] :
					       pptp_msg_name[0]);
		
	case PPTP_START_SESSION_REQUEST:
	case PPTP_START_SESSION_REPLY:
	case PPTP_STOP_SESSION_REQUEST:
	case PPTP_STOP_SESSION_REPLY:
	case PPTP_ECHO_REQUEST:
	case PPTP_ECHO_REPLY:
		
		return NF_ACCEPT;
	}


	
	pr_debug("altering peer call id from 0x%04x to 0x%04x\n",
		 ntohs(REQ_CID(pptpReq, pcid_off)), ntohs(new_pcid));

	if (nf_nat_mangle_tcp_packet(skb, ct, ctinfo,
				     pcid_off + sizeof(struct pptp_pkt_hdr) +
				     sizeof(struct PptpControlHeader),
				     sizeof(new_pcid), (char *)&new_pcid,
				     sizeof(new_pcid)) == 0)
		return NF_DROP;
	return NF_ACCEPT;
}

static int __init nf_nat_helper_pptp_init(void)
{
	nf_nat_need_gre();

	BUG_ON(nf_nat_pptp_hook_outbound != NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_outbound, pptp_outbound_pkt);

	BUG_ON(nf_nat_pptp_hook_inbound != NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_inbound, pptp_inbound_pkt);

	BUG_ON(nf_nat_pptp_hook_exp_gre != NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_exp_gre, pptp_exp_gre);

	BUG_ON(nf_nat_pptp_hook_expectfn != NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_expectfn, pptp_nat_expected);
	return 0;
}

static void __exit nf_nat_helper_pptp_fini(void)
{
	RCU_INIT_POINTER(nf_nat_pptp_hook_expectfn, NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_exp_gre, NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_inbound, NULL);
	RCU_INIT_POINTER(nf_nat_pptp_hook_outbound, NULL);
	synchronize_rcu();
}

module_init(nf_nat_helper_pptp_init);
module_exit(nf_nat_helper_pptp_fini);
