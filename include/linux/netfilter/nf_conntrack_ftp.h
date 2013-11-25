#ifndef _NF_CONNTRACK_FTP_H
#define _NF_CONNTRACK_FTP_H

enum nf_ct_ftp_type {
	
	NF_CT_FTP_PORT,
	
	NF_CT_FTP_PASV,
	
	NF_CT_FTP_EPRT,
	
	NF_CT_FTP_EPSV,
};

#ifdef __KERNEL__

#define FTP_PORT	21

#define NUM_SEQ_TO_REMEMBER 2
struct nf_ct_ftp_master {
	
	u_int32_t seq_aft_nl[IP_CT_DIR_MAX][NUM_SEQ_TO_REMEMBER];
	
	int seq_aft_nl_num[IP_CT_DIR_MAX];
};

struct nf_conntrack_expect;

extern unsigned int (*nf_nat_ftp_hook)(struct sk_buff *skb,
				       enum ip_conntrack_info ctinfo,
				       enum nf_ct_ftp_type type,
				       unsigned int matchoff,
				       unsigned int matchlen,
				       struct nf_conntrack_expect *exp);
#endif 

#endif 
