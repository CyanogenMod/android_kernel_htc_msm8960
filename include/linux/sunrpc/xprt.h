/*
 *  linux/include/linux/sunrpc/xprt.h
 *
 *  Declarations for the RPC transport interface.
 *
 *  Copyright (C) 1995, 1996 Olaf Kirch <okir@monad.swb.de>
 */

#ifndef _LINUX_SUNRPC_XPRT_H
#define _LINUX_SUNRPC_XPRT_H

#include <linux/uio.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/ktime.h>
#include <linux/sunrpc/sched.h>
#include <linux/sunrpc/xdr.h>
#include <linux/sunrpc/msg_prot.h>

#ifdef __KERNEL__

#define RPC_MIN_SLOT_TABLE	(2U)
#define RPC_DEF_SLOT_TABLE	(16U)
#define RPC_MAX_SLOT_TABLE_LIMIT	(65536U)
#define RPC_MAX_SLOT_TABLE	RPC_MAX_SLOT_TABLE_LIMIT

struct rpc_timeout {
	unsigned long		to_initval,		
				to_maxval,		
				to_increment;		
	unsigned int		to_retries;		
	unsigned char		to_exponential;
};

enum rpc_display_format_t {
	RPC_DISPLAY_ADDR = 0,
	RPC_DISPLAY_PORT,
	RPC_DISPLAY_PROTO,
	RPC_DISPLAY_HEX_ADDR,
	RPC_DISPLAY_HEX_PORT,
	RPC_DISPLAY_NETID,
	RPC_DISPLAY_MAX,
};

struct rpc_task;
struct rpc_xprt;
struct seq_file;

struct rpc_rqst {
	struct rpc_xprt *	rq_xprt;		
	struct xdr_buf		rq_snd_buf;		
	struct xdr_buf		rq_rcv_buf;		

	struct rpc_task *	rq_task;	
	struct rpc_cred *	rq_cred;	
	__be32			rq_xid;		
	int			rq_cong;	
	u32			rq_seqno;	
	int			rq_enc_pages_num;
	struct page		**rq_enc_pages;	
	void (*rq_release_snd_buf)(struct rpc_rqst *); 
	struct list_head	rq_list;

	__u32 *			rq_buffer;	
	size_t			rq_callsize,
				rq_rcvsize;
	size_t			rq_xmit_bytes_sent;	
	size_t			rq_reply_bytes_recvd;	
							

	struct xdr_buf		rq_private_buf;		
	unsigned long		rq_majortimeo;	
	unsigned long		rq_timeout;	
	ktime_t			rq_rtt;		
	unsigned int		rq_retries;	
	unsigned int		rq_connect_cookie;
	
	u32			rq_bytes_sent;	

	ktime_t			rq_xtime;	
	int			rq_ntrans;

#if defined(CONFIG_SUNRPC_BACKCHANNEL)
	struct list_head	rq_bc_list;	
	unsigned long		rq_bc_pa_state;	
	struct list_head	rq_bc_pa_list;	
#endif 
};
#define rq_svec			rq_snd_buf.head
#define rq_slen			rq_snd_buf.len

struct rpc_xprt_ops {
	void		(*set_buffer_size)(struct rpc_xprt *xprt, size_t sndsize, size_t rcvsize);
	int		(*reserve_xprt)(struct rpc_xprt *xprt, struct rpc_task *task);
	void		(*release_xprt)(struct rpc_xprt *xprt, struct rpc_task *task);
	void		(*rpcbind)(struct rpc_task *task);
	void		(*set_port)(struct rpc_xprt *xprt, unsigned short port);
	void		(*connect)(struct rpc_task *task);
	void *		(*buf_alloc)(struct rpc_task *task, size_t size);
	void		(*buf_free)(void *buffer);
	int		(*send_request)(struct rpc_task *task);
	void		(*set_retrans_timeout)(struct rpc_task *task);
	void		(*timer)(struct rpc_task *task);
	void		(*release_request)(struct rpc_task *task);
	void		(*close)(struct rpc_xprt *xprt);
	void		(*destroy)(struct rpc_xprt *xprt);
	void		(*print_stats)(struct rpc_xprt *xprt, struct seq_file *seq);
};

#define XPRT_TRANSPORT_BC       (1 << 31)
enum xprt_transports {
	XPRT_TRANSPORT_UDP	= IPPROTO_UDP,
	XPRT_TRANSPORT_TCP	= IPPROTO_TCP,
	XPRT_TRANSPORT_BC_TCP	= IPPROTO_TCP | XPRT_TRANSPORT_BC,
	XPRT_TRANSPORT_RDMA	= 256,
	XPRT_TRANSPORT_LOCAL	= 257,
};

struct rpc_xprt {
	atomic_t		count;		
	struct rpc_xprt_ops *	ops;		

	const struct rpc_timeout *timeout;	
	struct sockaddr_storage	addr;		
	size_t			addrlen;	
	int			prot;		

	unsigned long		cong;		
	unsigned long		cwnd;		

	size_t			max_payload;	
	unsigned int		tsh_size;	

	struct rpc_wait_queue	binding;	
	struct rpc_wait_queue	sending;	
	struct rpc_wait_queue	pending;	
	struct rpc_wait_queue	backlog;	
	struct list_head	free;		
	unsigned int		max_reqs;	
	unsigned int		min_reqs;	
	atomic_t		num_reqs;	
	unsigned long		state;		
	unsigned char		shutdown   : 1,	
				resvport   : 1; 
	unsigned int		bind_index;	

	unsigned long		bind_timeout,
				reestablish_timeout;
	unsigned int		connect_cookie;	

	struct work_struct	task_cleanup;
	struct timer_list	timer;
	unsigned long		last_used,
				idle_timeout;

	spinlock_t		transport_lock;	
	spinlock_t		reserve_lock;	
	u32			xid;		
	struct rpc_task *	snd_task;	
	struct svc_xprt		*bc_xprt;	
#if defined(CONFIG_SUNRPC_BACKCHANNEL)
	struct svc_serv		*bc_serv;       
						
	unsigned int		bc_alloc_count;	
	spinlock_t		bc_pa_lock;	
	struct list_head	bc_pa_list;	
#endif 
	struct list_head	recv;

	struct {
		unsigned long		bind_count,	
					connect_count,	
					connect_start,	
					connect_time,	
					sends,		
					recvs,		
					bad_xids,	
					max_slots;	

		unsigned long long	req_u,		
					bklog_u,	
					sending_u,	
					pending_u;	
	} stat;

	struct net		*xprt_net;
	const char		*servername;
	const char		*address_strings[RPC_DISPLAY_MAX];
};

#if defined(CONFIG_SUNRPC_BACKCHANNEL)
#define	RPC_BC_PA_IN_USE	0x0001		
						
#endif 

#if defined(CONFIG_SUNRPC_BACKCHANNEL)
static inline int bc_prealloc(struct rpc_rqst *req)
{
	return test_bit(RPC_BC_PA_IN_USE, &req->rq_bc_pa_state);
}
#else
static inline int bc_prealloc(struct rpc_rqst *req)
{
	return 0;
}
#endif 

struct xprt_create {
	int			ident;		
	struct net *		net;
	struct sockaddr *	srcaddr;	
	struct sockaddr *	dstaddr;	
	size_t			addrlen;
	const char		*servername;
	struct svc_xprt		*bc_xprt;	
};

struct xprt_class {
	struct list_head	list;
	int			ident;		
	struct rpc_xprt *	(*setup)(struct xprt_create *);
	struct module		*owner;
	char			name[32];
};

struct rpc_xprt		*xprt_create_transport(struct xprt_create *args);
void			xprt_connect(struct rpc_task *task);
void			xprt_reserve(struct rpc_task *task);
int			xprt_reserve_xprt(struct rpc_xprt *xprt, struct rpc_task *task);
int			xprt_reserve_xprt_cong(struct rpc_xprt *xprt, struct rpc_task *task);
int			xprt_prepare_transmit(struct rpc_task *task);
void			xprt_transmit(struct rpc_task *task);
void			xprt_end_transmit(struct rpc_task *task);
int			xprt_adjust_timeout(struct rpc_rqst *req);
void			xprt_release_xprt(struct rpc_xprt *xprt, struct rpc_task *task);
void			xprt_release_xprt_cong(struct rpc_xprt *xprt, struct rpc_task *task);
void			xprt_release(struct rpc_task *task);
struct rpc_xprt *	xprt_get(struct rpc_xprt *xprt);
void			xprt_put(struct rpc_xprt *xprt);
struct rpc_xprt *	xprt_alloc(struct net *net, size_t size,
				unsigned int num_prealloc,
				unsigned int max_req);
void			xprt_free(struct rpc_xprt *);

static inline __be32 *xprt_skip_transport_header(struct rpc_xprt *xprt, __be32 *p)
{
	return p + xprt->tsh_size;
}

int			xprt_register_transport(struct xprt_class *type);
int			xprt_unregister_transport(struct xprt_class *type);
int			xprt_load_transport(const char *);
void			xprt_set_retrans_timeout_def(struct rpc_task *task);
void			xprt_set_retrans_timeout_rtt(struct rpc_task *task);
void			xprt_wake_pending_tasks(struct rpc_xprt *xprt, int status);
void			xprt_wait_for_buffer_space(struct rpc_task *task, rpc_action action);
void			xprt_write_space(struct rpc_xprt *xprt);
void			xprt_adjust_cwnd(struct rpc_task *task, int result);
struct rpc_rqst *	xprt_lookup_rqst(struct rpc_xprt *xprt, __be32 xid);
void			xprt_complete_rqst(struct rpc_task *task, int copied);
void			xprt_release_rqst_cong(struct rpc_task *task);
void			xprt_disconnect_done(struct rpc_xprt *xprt);
void			xprt_force_disconnect(struct rpc_xprt *xprt);
void			xprt_conditional_disconnect(struct rpc_xprt *xprt, unsigned int cookie);

#define XPRT_LOCKED		(0)
#define XPRT_CONNECTED		(1)
#define XPRT_CONNECTING		(2)
#define XPRT_CLOSE_WAIT		(3)
#define XPRT_BOUND		(4)
#define XPRT_BINDING		(5)
#define XPRT_CLOSING		(6)
#define XPRT_CONNECTION_ABORT	(7)
#define XPRT_CONNECTION_CLOSE	(8)

static inline void xprt_set_connected(struct rpc_xprt *xprt)
{
	set_bit(XPRT_CONNECTED, &xprt->state);
}

static inline void xprt_clear_connected(struct rpc_xprt *xprt)
{
	clear_bit(XPRT_CONNECTED, &xprt->state);
}

static inline int xprt_connected(struct rpc_xprt *xprt)
{
	return test_bit(XPRT_CONNECTED, &xprt->state);
}

static inline int xprt_test_and_set_connected(struct rpc_xprt *xprt)
{
	return test_and_set_bit(XPRT_CONNECTED, &xprt->state);
}

static inline int xprt_test_and_clear_connected(struct rpc_xprt *xprt)
{
	return test_and_clear_bit(XPRT_CONNECTED, &xprt->state);
}

static inline void xprt_clear_connecting(struct rpc_xprt *xprt)
{
	smp_mb__before_clear_bit();
	clear_bit(XPRT_CONNECTING, &xprt->state);
	smp_mb__after_clear_bit();
}

static inline int xprt_connecting(struct rpc_xprt *xprt)
{
	return test_bit(XPRT_CONNECTING, &xprt->state);
}

static inline int xprt_test_and_set_connecting(struct rpc_xprt *xprt)
{
	return test_and_set_bit(XPRT_CONNECTING, &xprt->state);
}

static inline void xprt_set_bound(struct rpc_xprt *xprt)
{
	test_and_set_bit(XPRT_BOUND, &xprt->state);
}

static inline int xprt_bound(struct rpc_xprt *xprt)
{
	return test_bit(XPRT_BOUND, &xprt->state);
}

static inline void xprt_clear_bound(struct rpc_xprt *xprt)
{
	clear_bit(XPRT_BOUND, &xprt->state);
}

static inline void xprt_clear_binding(struct rpc_xprt *xprt)
{
	smp_mb__before_clear_bit();
	clear_bit(XPRT_BINDING, &xprt->state);
	smp_mb__after_clear_bit();
}

static inline int xprt_test_and_set_binding(struct rpc_xprt *xprt)
{
	return test_and_set_bit(XPRT_BINDING, &xprt->state);
}

#endif 

#endif 
