/*
 * linux/include/linux/sunrpc/auth.h
 *
 * Declarations for the RPC client authentication machinery.
 *
 * Copyright (C) 1996, Olaf Kirch <okir@monad.swb.de>
 */

#ifndef _LINUX_SUNRPC_AUTH_H
#define _LINUX_SUNRPC_AUTH_H

#ifdef __KERNEL__

#include <linux/sunrpc/sched.h>
#include <linux/sunrpc/msg_prot.h>
#include <linux/sunrpc/xdr.h>

#include <linux/atomic.h>
#include <linux/rcupdate.h>

#define UNX_MAXNODENAME	32

struct auth_cred {
	uid_t	uid;
	gid_t	gid;
	struct group_info *group_info;
	const char *principal;
	unsigned char machine_cred : 1;
};

struct rpc_auth;
struct rpc_credops;
struct rpc_cred {
	struct hlist_node	cr_hash;	
	struct list_head	cr_lru;		
	struct rcu_head		cr_rcu;
	struct rpc_auth *	cr_auth;
	const struct rpc_credops *cr_ops;
#ifdef RPC_DEBUG
	unsigned long		cr_magic;	
#endif
	unsigned long		cr_expire;	
	unsigned long		cr_flags;	
	atomic_t		cr_count;	

	uid_t			cr_uid;

	
};
#define RPCAUTH_CRED_NEW	0
#define RPCAUTH_CRED_UPTODATE	1
#define RPCAUTH_CRED_HASHED	2
#define RPCAUTH_CRED_NEGATIVE	3

#define RPCAUTH_CRED_MAGIC	0x0f4aa4f0

struct rpc_cred_cache;
struct rpc_authops;
struct rpc_auth {
	unsigned int		au_cslack;	
	unsigned int		au_rslack;
				
	unsigned int		au_verfsize;

	unsigned int		au_flags;	
	const struct rpc_authops *au_ops;		
	rpc_authflavor_t	au_flavor;	
	atomic_t		au_count;	

	struct rpc_cred_cache *	au_credcache;
	
};

#define RPCAUTH_LOOKUP_NEW		0x01	

struct rpc_authops {
	struct module		*owner;
	rpc_authflavor_t	au_flavor;	
	char *			au_name;
	struct rpc_auth *	(*create)(struct rpc_clnt *, rpc_authflavor_t);
	void			(*destroy)(struct rpc_auth *);

	struct rpc_cred *	(*lookup_cred)(struct rpc_auth *, struct auth_cred *, int);
	struct rpc_cred *	(*crcreate)(struct rpc_auth*, struct auth_cred *, int);
	int			(*pipes_create)(struct rpc_auth *);
	void			(*pipes_destroy)(struct rpc_auth *);
};

struct rpc_credops {
	const char *		cr_name;	
	int			(*cr_init)(struct rpc_auth *, struct rpc_cred *);
	void			(*crdestroy)(struct rpc_cred *);

	int			(*crmatch)(struct auth_cred *, struct rpc_cred *, int);
	struct rpc_cred *	(*crbind)(struct rpc_task *, struct rpc_cred *, int);
	__be32 *		(*crmarshal)(struct rpc_task *, __be32 *);
	int			(*crrefresh)(struct rpc_task *);
	__be32 *		(*crvalidate)(struct rpc_task *, __be32 *);
	int			(*crwrap_req)(struct rpc_task *, kxdreproc_t,
						void *, __be32 *, void *);
	int			(*crunwrap_resp)(struct rpc_task *, kxdrdproc_t,
						void *, __be32 *, void *);
};

extern const struct rpc_authops	authunix_ops;
extern const struct rpc_authops	authnull_ops;

int __init		rpc_init_authunix(void);
int __init		rpc_init_generic_auth(void);
int __init		rpcauth_init_module(void);
void			rpcauth_remove_module(void);
void			rpc_destroy_generic_auth(void);
void 			rpc_destroy_authunix(void);

struct rpc_cred *	rpc_lookup_cred(void);
struct rpc_cred *	rpc_lookup_machine_cred(const char *service_name);
int			rpcauth_register(const struct rpc_authops *);
int			rpcauth_unregister(const struct rpc_authops *);
struct rpc_auth *	rpcauth_create(rpc_authflavor_t, struct rpc_clnt *);
void			rpcauth_release(struct rpc_auth *);
struct rpc_cred *	rpcauth_lookup_credcache(struct rpc_auth *, struct auth_cred *, int);
void			rpcauth_init_cred(struct rpc_cred *, const struct auth_cred *, struct rpc_auth *, const struct rpc_credops *);
struct rpc_cred *	rpcauth_lookupcred(struct rpc_auth *, int);
struct rpc_cred *	rpcauth_generic_bind_cred(struct rpc_task *, struct rpc_cred *, int);
void			put_rpccred(struct rpc_cred *);
__be32 *		rpcauth_marshcred(struct rpc_task *, __be32 *);
__be32 *		rpcauth_checkverf(struct rpc_task *, __be32 *);
int			rpcauth_wrap_req(struct rpc_task *task, kxdreproc_t encode, void *rqstp, __be32 *data, void *obj);
int			rpcauth_unwrap_resp(struct rpc_task *task, kxdrdproc_t decode, void *rqstp, __be32 *data, void *obj);
int			rpcauth_refreshcred(struct rpc_task *);
void			rpcauth_invalcred(struct rpc_task *);
int			rpcauth_uptodatecred(struct rpc_task *);
int			rpcauth_init_credcache(struct rpc_auth *);
void			rpcauth_destroy_credcache(struct rpc_auth *);
void			rpcauth_clear_credcache(struct rpc_cred_cache *);

static inline
struct rpc_cred *	get_rpccred(struct rpc_cred *cred)
{
	atomic_inc(&cred->cr_count);
	return cred;
}

#endif 
#endif 
