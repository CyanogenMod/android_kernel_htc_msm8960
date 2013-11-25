/*
 * linux/include/linux/sunrpc/gss_api.h
 *
 * Somewhat simplified version of the gss api.
 *
 * Dug Song <dugsong@monkey.org>
 * Andy Adamson <andros@umich.edu>
 * Bruce Fields <bfields@umich.edu>
 * Copyright (c) 2000 The Regents of the University of Michigan
 */

#ifndef _LINUX_SUNRPC_GSS_API_H
#define _LINUX_SUNRPC_GSS_API_H

#ifdef __KERNEL__
#include <linux/sunrpc/xdr.h>
#include <linux/uio.h>

struct gss_ctx {
	struct gss_api_mech	*mech_type;
	void			*internal_ctx_id;
};

#define GSS_C_NO_BUFFER		((struct xdr_netobj) 0)
#define GSS_C_NO_CONTEXT	((struct gss_ctx *) 0)
#define GSS_C_NULL_OID		((struct xdr_netobj) 0)

#define GSS_OID_MAX_LEN 32

int gss_import_sec_context(
		const void*		input_token,
		size_t			bufsize,
		struct gss_api_mech	*mech,
		struct gss_ctx		**ctx_id,
		gfp_t			gfp_mask);
u32 gss_get_mic(
		struct gss_ctx		*ctx_id,
		struct xdr_buf		*message,
		struct xdr_netobj	*mic_token);
u32 gss_verify_mic(
		struct gss_ctx		*ctx_id,
		struct xdr_buf		*message,
		struct xdr_netobj	*mic_token);
u32 gss_wrap(
		struct gss_ctx		*ctx_id,
		int			offset,
		struct xdr_buf		*outbuf,
		struct page		**inpages);
u32 gss_unwrap(
		struct gss_ctx		*ctx_id,
		int			offset,
		struct xdr_buf		*inbuf);
u32 gss_delete_sec_context(
		struct gss_ctx		**ctx_id);

u32 gss_svc_to_pseudoflavor(struct gss_api_mech *, u32 service);
u32 gss_pseudoflavor_to_service(struct gss_api_mech *, u32 pseudoflavor);
char *gss_service_to_auth_domain_name(struct gss_api_mech *, u32 service);

struct pf_desc {
	u32	pseudoflavor;
	u32	service;
	char	*name;
	char	*auth_domain_name;
};


struct gss_api_mech {
	struct list_head	gm_list;
	struct module		*gm_owner;
	struct xdr_netobj	gm_oid;
	char			*gm_name;
	const struct gss_api_ops *gm_ops;
	
	int			gm_pf_num;
	struct pf_desc *	gm_pfs;
	
	const char		*gm_upcall_enctypes;
};

struct gss_api_ops {
	int (*gss_import_sec_context)(
			const void		*input_token,
			size_t			bufsize,
			struct gss_ctx		*ctx_id,
			gfp_t			gfp_mask);
	u32 (*gss_get_mic)(
			struct gss_ctx		*ctx_id,
			struct xdr_buf		*message,
			struct xdr_netobj	*mic_token);
	u32 (*gss_verify_mic)(
			struct gss_ctx		*ctx_id,
			struct xdr_buf		*message,
			struct xdr_netobj	*mic_token);
	u32 (*gss_wrap)(
			struct gss_ctx		*ctx_id,
			int			offset,
			struct xdr_buf		*outbuf,
			struct page		**inpages);
	u32 (*gss_unwrap)(
			struct gss_ctx		*ctx_id,
			int			offset,
			struct xdr_buf		*buf);
	void (*gss_delete_sec_context)(
			void			*internal_ctx_id);
};

int gss_mech_register(struct gss_api_mech *);
void gss_mech_unregister(struct gss_api_mech *);

struct gss_api_mech * gss_mech_get_by_OID(struct xdr_netobj *);

struct gss_api_mech *gss_mech_get_by_name(const char *);

struct gss_api_mech *gss_mech_get_by_pseudoflavor(u32);

int gss_mech_list_pseudoflavors(u32 *);

struct gss_api_mech * gss_mech_get(struct gss_api_mech *);

void gss_mech_put(struct gss_api_mech *);

#endif 
#endif 

