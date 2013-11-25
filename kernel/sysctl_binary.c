#include <linux/stat.h>
#include <linux/sysctl.h>
#include "../fs/xfs/xfs_sysctl.h"
#include <linux/sunrpc/debug.h>
#include <linux/string.h>
#include <net/ip_vs.h>
#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/fs.h>
#include <linux/nsproxy.h>
#include <linux/pid_namespace.h>
#include <linux/file.h>
#include <linux/ctype.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#ifdef CONFIG_SYSCTL_SYSCALL

struct bin_table;
typedef ssize_t bin_convert_t(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen);

static bin_convert_t bin_dir;
static bin_convert_t bin_string;
static bin_convert_t bin_intvec;
static bin_convert_t bin_ulongvec;
static bin_convert_t bin_uuid;
static bin_convert_t bin_dn_node_address;

#define CTL_DIR   bin_dir
#define CTL_STR   bin_string
#define CTL_INT   bin_intvec
#define CTL_ULONG bin_ulongvec
#define CTL_UUID  bin_uuid
#define CTL_DNADR bin_dn_node_address

#define BUFSZ 256

struct bin_table {
	bin_convert_t		*convert;
	int			ctl_name;
	const char		*procname;
	const struct bin_table	*child;
};

static const struct bin_table bin_random_table[] = {
	{ CTL_INT,	RANDOM_POOLSIZE,	"poolsize" },
	{ CTL_INT,	RANDOM_ENTROPY_COUNT,	"entropy_avail" },
	{ CTL_INT,	RANDOM_READ_THRESH,	"read_wakeup_threshold" },
	{ CTL_INT,	RANDOM_WRITE_THRESH,	"write_wakeup_threshold" },
	{ CTL_UUID,	RANDOM_BOOT_ID,		"boot_id" },
	{ CTL_UUID,	RANDOM_UUID,		"uuid" },
	{}
};

static const struct bin_table bin_pty_table[] = {
	{ CTL_INT,	PTY_MAX,	"max" },
	{ CTL_INT,	PTY_NR,		"nr" },
	{}
};

static const struct bin_table bin_kern_table[] = {
	{ CTL_STR,	KERN_OSTYPE,			"ostype" },
	{ CTL_STR,	KERN_OSRELEASE,			"osrelease" },
	
	{ CTL_STR,	KERN_VERSION,			"version" },
	
	
	{ CTL_STR,	KERN_NODENAME,			"hostname" },
	{ CTL_STR,	KERN_DOMAINNAME,		"domainname" },

	{ CTL_INT,	KERN_PANIC,			"panic" },
	{ CTL_INT,	KERN_REALROOTDEV,		"real-root-dev" },

	{ CTL_STR,	KERN_SPARC_REBOOT,		"reboot-cmd" },
	{ CTL_INT,	KERN_CTLALTDEL,			"ctrl-alt-del" },
	{ CTL_INT,	KERN_PRINTK,			"printk" },

	
	
	
	{ CTL_INT,	KERN_PPC_POWERSAVE_NAP,		"powersave-nap" },

	{ CTL_STR,	KERN_MODPROBE,			"modprobe" },
	{ CTL_INT,	KERN_SG_BIG_BUFF,		"sg-big-buff" },
	{ CTL_INT,	KERN_ACCT,			"acct" },
	

	
	

	{ CTL_ULONG,	KERN_SHMMAX,			"shmmax" },
	{ CTL_INT,	KERN_MSGMAX,			"msgmax" },
	{ CTL_INT,	KERN_MSGMNB,			"msgmnb" },
	
	{ CTL_INT,	KERN_SYSRQ,			"sysrq" },
	{ CTL_INT,	KERN_MAX_THREADS,		"threads-max" },
	{ CTL_DIR,	KERN_RANDOM,			"random",	bin_random_table },
	{ CTL_ULONG,	KERN_SHMALL,			"shmall" },
	{ CTL_INT,	KERN_MSGMNI,			"msgmni" },
	{ CTL_INT,	KERN_SEM,			"sem" },
	{ CTL_INT,	KERN_SPARC_STOP_A,		"stop-a" },
	{ CTL_INT,	KERN_SHMMNI,			"shmmni" },

	{ CTL_INT,	KERN_OVERFLOWUID,		"overflowuid" },
	{ CTL_INT,	KERN_OVERFLOWGID,		"overflowgid" },

	{ CTL_STR,	KERN_HOTPLUG,			"hotplug", },
	{ CTL_INT,	KERN_IEEE_EMULATION_WARNINGS,	"ieee_emulation_warnings" },

	{ CTL_INT,	KERN_S390_USER_DEBUG_LOGGING,	"userprocess_debug" },
	{ CTL_INT,	KERN_CORE_USES_PID,		"core_uses_pid" },
	
	{ CTL_INT,	KERN_CADPID,			"cad_pid" },
	{ CTL_INT,	KERN_PIDMAX,			"pid_max" },
	{ CTL_STR,	KERN_CORE_PATTERN,		"core_pattern" },
	{ CTL_INT,	KERN_PANIC_ON_OOPS,		"panic_on_oops" },
	{ CTL_INT,	KERN_HPPA_PWRSW,		"soft-power" },
	{ CTL_INT,	KERN_HPPA_UNALIGNED,		"unaligned-trap" },

	{ CTL_INT,	KERN_PRINTK_RATELIMIT,		"printk_ratelimit" },
	{ CTL_INT,	KERN_PRINTK_RATELIMIT_BURST,	"printk_ratelimit_burst" },

	{ CTL_DIR,	KERN_PTY,			"pty",		bin_pty_table },
	{ CTL_INT,	KERN_NGROUPS_MAX,		"ngroups_max" },
	{ CTL_INT,	KERN_SPARC_SCONS_PWROFF,	"scons-poweroff" },
	
	{ CTL_INT,	KERN_UNKNOWN_NMI_PANIC,		"unknown_nmi_panic" },
	{ CTL_INT,	KERN_BOOTLOADER_TYPE,		"bootloader_type" },
	{ CTL_INT,	KERN_RANDOMIZE,			"randomize_va_space" },

	{ CTL_INT,	KERN_SPIN_RETRY,		"spin_retry" },
	
	{ CTL_INT,	KERN_IA64_UNALIGNED,		"ignore-unaligned-usertrap" },
	{ CTL_INT,	KERN_COMPAT_LOG,		"compat-log" },
	{ CTL_INT,	KERN_MAX_LOCK_DEPTH,		"max_lock_depth" },
	{ CTL_INT,	KERN_PANIC_ON_NMI,		"panic_on_unrecovered_nmi" },
	{ CTL_INT,	KERN_BOOT_REASON,		"boot_reason" },
	{}
};

static const struct bin_table bin_vm_table[] = {
	{ CTL_INT,	VM_OVERCOMMIT_MEMORY,		"overcommit_memory" },
	{ CTL_INT,	VM_PAGE_CLUSTER,		"page-cluster" },
	{ CTL_INT,	VM_DIRTY_BACKGROUND,		"dirty_background_ratio" },
	{ CTL_INT,	VM_DIRTY_RATIO,			"dirty_ratio" },
	
	
	{ CTL_INT,	VM_NR_PDFLUSH_THREADS,		"nr_pdflush_threads" },
	{ CTL_INT,	VM_OVERCOMMIT_RATIO,		"overcommit_ratio" },
	
	
	{ CTL_INT,	VM_SWAPPINESS,			"swappiness" },
	{ CTL_INT,	VM_LOWMEM_RESERVE_RATIO,	"lowmem_reserve_ratio" },
	{ CTL_INT,	VM_MIN_FREE_KBYTES,		"min_free_kbytes" },
	{ CTL_INT,	VM_MAX_MAP_COUNT,		"max_map_count" },
	{ CTL_INT,	VM_LAPTOP_MODE,			"laptop_mode" },
	{ CTL_INT,	VM_BLOCK_DUMP,			"block_dump" },
	{ CTL_INT,	VM_HUGETLB_GROUP,		"hugetlb_shm_group" },
	{ CTL_INT,	VM_VFS_CACHE_PRESSURE,	"vfs_cache_pressure" },
	{ CTL_INT,	VM_LEGACY_VA_LAYOUT,		"legacy_va_layout" },
	
	{ CTL_INT,	VM_DROP_PAGECACHE,		"drop_caches" },
	{ CTL_INT,	VM_PERCPU_PAGELIST_FRACTION,	"percpu_pagelist_fraction" },
	{ CTL_INT,	VM_ZONE_RECLAIM_MODE,		"zone_reclaim_mode" },
	{ CTL_INT,	VM_MIN_UNMAPPED,		"min_unmapped_ratio" },
	{ CTL_INT,	VM_PANIC_ON_OOM,		"panic_on_oom" },
	{ CTL_INT,	VM_VDSO_ENABLED,		"vdso_enabled" },
	{ CTL_INT,	VM_MIN_SLAB,			"min_slab_ratio" },

	{}
};

static const struct bin_table bin_net_core_table[] = {
	{ CTL_INT,	NET_CORE_WMEM_MAX,	"wmem_max" },
	{ CTL_INT,	NET_CORE_RMEM_MAX,	"rmem_max" },
	{ CTL_INT,	NET_CORE_WMEM_DEFAULT,	"wmem_default" },
	{ CTL_INT,	NET_CORE_RMEM_DEFAULT,	"rmem_default" },
	
	{ CTL_INT,	NET_CORE_MAX_BACKLOG,	"netdev_max_backlog" },
	
	{ CTL_INT,	NET_CORE_MSG_COST,	"message_cost" },
	{ CTL_INT,	NET_CORE_MSG_BURST,	"message_burst" },
	{ CTL_INT,	NET_CORE_OPTMEM_MAX,	"optmem_max" },
	
	
	
	
	
	
	{ CTL_INT,	NET_CORE_DEV_WEIGHT,	"dev_weight" },
	{ CTL_INT,	NET_CORE_SOMAXCONN,	"somaxconn" },
	{ CTL_INT,	NET_CORE_BUDGET,	"netdev_budget" },
	{ CTL_INT,	NET_CORE_AEVENT_ETIME,	"xfrm_aevent_etime" },
	{ CTL_INT,	NET_CORE_AEVENT_RSEQTH,	"xfrm_aevent_rseqth" },
	{ CTL_INT,	NET_CORE_WARNINGS,	"warnings" },
	{},
};

static const struct bin_table bin_net_unix_table[] = {
	
	
	{ CTL_INT,	NET_UNIX_MAX_DGRAM_QLEN,	"max_dgram_qlen" },
	{}
};

static const struct bin_table bin_net_ipv4_route_table[] = {
	{ CTL_INT,	NET_IPV4_ROUTE_FLUSH,			"flush" },
	
	
	{ CTL_INT,	NET_IPV4_ROUTE_GC_THRESH,		"gc_thresh" },
	{ CTL_INT,	NET_IPV4_ROUTE_MAX_SIZE,		"max_size" },
	{ CTL_INT,	NET_IPV4_ROUTE_GC_MIN_INTERVAL,		"gc_min_interval" },
	{ CTL_INT,	NET_IPV4_ROUTE_GC_MIN_INTERVAL_MS,	"gc_min_interval_ms" },
	{ CTL_INT,	NET_IPV4_ROUTE_GC_TIMEOUT,		"gc_timeout" },
	
	{ CTL_INT,	NET_IPV4_ROUTE_REDIRECT_LOAD,		"redirect_load" },
	{ CTL_INT,	NET_IPV4_ROUTE_REDIRECT_NUMBER,		"redirect_number" },
	{ CTL_INT,	NET_IPV4_ROUTE_REDIRECT_SILENCE,	"redirect_silence" },
	{ CTL_INT,	NET_IPV4_ROUTE_ERROR_COST,		"error_cost" },
	{ CTL_INT,	NET_IPV4_ROUTE_ERROR_BURST,		"error_burst" },
	{ CTL_INT,	NET_IPV4_ROUTE_GC_ELASTICITY,		"gc_elasticity" },
	{ CTL_INT,	NET_IPV4_ROUTE_MTU_EXPIRES,		"mtu_expires" },
	{ CTL_INT,	NET_IPV4_ROUTE_MIN_PMTU,		"min_pmtu" },
	{ CTL_INT,	NET_IPV4_ROUTE_MIN_ADVMSS,		"min_adv_mss" },
	{}
};

static const struct bin_table bin_net_ipv4_conf_vars_table[] = {
	{ CTL_INT,	NET_IPV4_CONF_FORWARDING,		"forwarding" },
	{ CTL_INT,	NET_IPV4_CONF_MC_FORWARDING,		"mc_forwarding" },

	{ CTL_INT,	NET_IPV4_CONF_ACCEPT_REDIRECTS,		"accept_redirects" },
	{ CTL_INT,	NET_IPV4_CONF_SECURE_REDIRECTS,		"secure_redirects" },
	{ CTL_INT,	NET_IPV4_CONF_SEND_REDIRECTS,		"send_redirects" },
	{ CTL_INT,	NET_IPV4_CONF_SHARED_MEDIA,		"shared_media" },
	{ CTL_INT,	NET_IPV4_CONF_RP_FILTER,		"rp_filter" },
	{ CTL_INT,	NET_IPV4_CONF_ACCEPT_SOURCE_ROUTE,	"accept_source_route" },
	{ CTL_INT,	NET_IPV4_CONF_PROXY_ARP,		"proxy_arp" },
	{ CTL_INT,	NET_IPV4_CONF_MEDIUM_ID,		"medium_id" },
	{ CTL_INT,	NET_IPV4_CONF_BOOTP_RELAY,		"bootp_relay" },
	{ CTL_INT,	NET_IPV4_CONF_LOG_MARTIANS,		"log_martians" },
	{ CTL_INT,	NET_IPV4_CONF_TAG,			"tag" },
	{ CTL_INT,	NET_IPV4_CONF_ARPFILTER,		"arp_filter" },
	{ CTL_INT,	NET_IPV4_CONF_ARP_ANNOUNCE,		"arp_announce" },
	{ CTL_INT,	NET_IPV4_CONF_ARP_IGNORE,		"arp_ignore" },
	{ CTL_INT,	NET_IPV4_CONF_ARP_ACCEPT,		"arp_accept" },
	{ CTL_INT,	NET_IPV4_CONF_ARP_NOTIFY,		"arp_notify" },

	{ CTL_INT,	NET_IPV4_CONF_NOXFRM,			"disable_xfrm" },
	{ CTL_INT,	NET_IPV4_CONF_NOPOLICY,			"disable_policy" },
	{ CTL_INT,	NET_IPV4_CONF_FORCE_IGMP_VERSION,	"force_igmp_version" },
	{ CTL_INT,	NET_IPV4_CONF_PROMOTE_SECONDARIES,	"promote_secondaries" },
	{}
};

static const struct bin_table bin_net_ipv4_conf_table[] = {
	{ CTL_DIR,	NET_PROTO_CONF_ALL,	"all",		bin_net_ipv4_conf_vars_table },
	{ CTL_DIR,	NET_PROTO_CONF_DEFAULT,	"default",	bin_net_ipv4_conf_vars_table },
	{ CTL_DIR,	0, NULL, bin_net_ipv4_conf_vars_table },
	{}
};

static const struct bin_table bin_net_neigh_vars_table[] = {
	{ CTL_INT,	NET_NEIGH_MCAST_SOLICIT,	"mcast_solicit" },
	{ CTL_INT,	NET_NEIGH_UCAST_SOLICIT,	"ucast_solicit" },
	{ CTL_INT,	NET_NEIGH_APP_SOLICIT,		"app_solicit" },
	
	{ CTL_INT,	NET_NEIGH_REACHABLE_TIME,	"base_reachable_time" },
	{ CTL_INT,	NET_NEIGH_DELAY_PROBE_TIME,	"delay_first_probe_time" },
	{ CTL_INT,	NET_NEIGH_GC_STALE_TIME,	"gc_stale_time" },
	{ CTL_INT,	NET_NEIGH_UNRES_QLEN,		"unres_qlen" },
	{ CTL_INT,	NET_NEIGH_PROXY_QLEN,		"proxy_qlen" },
	
	
	
	{ CTL_INT,	NET_NEIGH_GC_INTERVAL,		"gc_interval" },
	{ CTL_INT,	NET_NEIGH_GC_THRESH1,		"gc_thresh1" },
	{ CTL_INT,	NET_NEIGH_GC_THRESH2,		"gc_thresh2" },
	{ CTL_INT,	NET_NEIGH_GC_THRESH3,		"gc_thresh3" },
	{ CTL_INT,	NET_NEIGH_RETRANS_TIME_MS,	"retrans_time_ms" },
	{ CTL_INT,	NET_NEIGH_REACHABLE_TIME_MS,	"base_reachable_time_ms" },
	{}
};

static const struct bin_table bin_net_neigh_table[] = {
	{ CTL_DIR,	NET_PROTO_CONF_DEFAULT, "default", bin_net_neigh_vars_table },
	{ CTL_DIR,	0, NULL, bin_net_neigh_vars_table },
	{}
};

static const struct bin_table bin_net_ipv4_netfilter_table[] = {
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_MAX,		"ip_conntrack_max" },

	
	
	
	
	
	
	
	

	
	
	
	

	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_BUCKETS,		"ip_conntrack_buckets" },
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_LOG_INVALID,	"ip_conntrack_log_invalid" },
	
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_TCP_LOOSE,	"ip_conntrack_tcp_loose" },
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_TCP_BE_LIBERAL,	"ip_conntrack_tcp_be_liberal" },
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_TCP_MAX_RETRANS,	"ip_conntrack_tcp_max_retrans" },

	
	
	
	
	
	
	

	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_COUNT,		"ip_conntrack_count" },
	{ CTL_INT,	NET_IPV4_NF_CONNTRACK_CHECKSUM,		"ip_conntrack_checksum" },
	{}
};

static const struct bin_table bin_net_ipv4_table[] = {
	{CTL_INT,	NET_IPV4_FORWARD,			"ip_forward" },

	{ CTL_DIR,	NET_IPV4_CONF,		"conf",		bin_net_ipv4_conf_table },
	{ CTL_DIR,	NET_IPV4_NEIGH,		"neigh",	bin_net_neigh_table },
	{ CTL_DIR,	NET_IPV4_ROUTE,		"route",	bin_net_ipv4_route_table },
	
	{ CTL_DIR,	NET_IPV4_NETFILTER,	"netfilter",	bin_net_ipv4_netfilter_table },

	{ CTL_INT,	NET_IPV4_TCP_TIMESTAMPS,		"tcp_timestamps" },
	{ CTL_INT,	NET_IPV4_TCP_WINDOW_SCALING,		"tcp_window_scaling" },
	{ CTL_INT,	NET_IPV4_TCP_SACK,			"tcp_sack" },
	{ CTL_INT,	NET_IPV4_TCP_RETRANS_COLLAPSE,		"tcp_retrans_collapse" },
	{ CTL_INT,	NET_IPV4_DEFAULT_TTL,			"ip_default_ttl" },
	
	{ CTL_INT,	NET_IPV4_NO_PMTU_DISC,			"ip_no_pmtu_disc" },
	{ CTL_INT,	NET_IPV4_NONLOCAL_BIND,			"ip_nonlocal_bind" },
	{ CTL_INT,	NET_IPV4_TCP_SYN_RETRIES,		"tcp_syn_retries" },
	{ CTL_INT,	NET_TCP_SYNACK_RETRIES,			"tcp_synack_retries" },
	{ CTL_INT,	NET_TCP_MAX_ORPHANS,			"tcp_max_orphans" },
	{ CTL_INT,	NET_TCP_MAX_TW_BUCKETS,			"tcp_max_tw_buckets" },
	{ CTL_INT,	NET_IPV4_DYNADDR,			"ip_dynaddr" },
	{ CTL_INT,	NET_IPV4_TCP_KEEPALIVE_TIME,		"tcp_keepalive_time" },
	{ CTL_INT,	NET_IPV4_TCP_KEEPALIVE_PROBES,		"tcp_keepalive_probes" },
	{ CTL_INT,	NET_IPV4_TCP_KEEPALIVE_INTVL,		"tcp_keepalive_intvl" },
	{ CTL_INT,	NET_IPV4_TCP_RETRIES1,			"tcp_retries1" },
	{ CTL_INT,	NET_IPV4_TCP_RETRIES2,			"tcp_retries2" },
	{ CTL_INT,	NET_IPV4_TCP_FIN_TIMEOUT,		"tcp_fin_timeout" },
	{ CTL_INT,	NET_TCP_SYNCOOKIES,			"tcp_syncookies" },
	{ CTL_INT,	NET_TCP_TW_RECYCLE,			"tcp_tw_recycle" },
	{ CTL_INT,	NET_TCP_ABORT_ON_OVERFLOW,		"tcp_abort_on_overflow" },
	{ CTL_INT,	NET_TCP_STDURG,				"tcp_stdurg" },
	{ CTL_INT,	NET_TCP_RFC1337,			"tcp_rfc1337" },
	{ CTL_INT,	NET_TCP_MAX_SYN_BACKLOG,		"tcp_max_syn_backlog" },
	{ CTL_INT,	NET_IPV4_LOCAL_PORT_RANGE,		"ip_local_port_range" },
	{ CTL_INT,	NET_IPV4_IGMP_MAX_MEMBERSHIPS,		"igmp_max_memberships" },
	{ CTL_INT,	NET_IPV4_IGMP_MAX_MSF,			"igmp_max_msf" },
	{ CTL_INT,	NET_IPV4_INET_PEER_THRESHOLD,		"inet_peer_threshold" },
	{ CTL_INT,	NET_IPV4_INET_PEER_MINTTL,		"inet_peer_minttl" },
	{ CTL_INT,	NET_IPV4_INET_PEER_MAXTTL,		"inet_peer_maxttl" },
	{ CTL_INT,	NET_IPV4_INET_PEER_GC_MINTIME,		"inet_peer_gc_mintime" },
	{ CTL_INT,	NET_IPV4_INET_PEER_GC_MAXTIME,		"inet_peer_gc_maxtime" },
	{ CTL_INT,	NET_TCP_ORPHAN_RETRIES,			"tcp_orphan_retries" },
	{ CTL_INT,	NET_TCP_FACK,				"tcp_fack" },
	{ CTL_INT,	NET_TCP_REORDERING,			"tcp_reordering" },
	{ CTL_INT,	NET_TCP_ECN,				"tcp_ecn" },
	{ CTL_INT,	NET_TCP_DSACK,				"tcp_dsack" },
	{ CTL_INT,	NET_TCP_MEM,				"tcp_mem" },
	{ CTL_INT,	NET_TCP_WMEM,				"tcp_wmem" },
	{ CTL_INT,	NET_TCP_RMEM,				"tcp_rmem" },
	{ CTL_INT,	NET_TCP_APP_WIN,			"tcp_app_win" },
	{ CTL_INT,	NET_TCP_ADV_WIN_SCALE,			"tcp_adv_win_scale" },
	{ CTL_INT,	NET_TCP_TW_REUSE,			"tcp_tw_reuse" },
	{ CTL_INT,	NET_TCP_FRTO,				"tcp_frto" },
	{ CTL_INT,	NET_TCP_FRTO_RESPONSE,			"tcp_frto_response" },
	{ CTL_INT,	NET_TCP_LOW_LATENCY,			"tcp_low_latency" },
	{ CTL_INT,	NET_TCP_NO_METRICS_SAVE,		"tcp_no_metrics_save" },
	{ CTL_INT,	NET_TCP_MODERATE_RCVBUF,		"tcp_moderate_rcvbuf" },
	{ CTL_INT,	NET_TCP_TSO_WIN_DIVISOR,		"tcp_tso_win_divisor" },
	{ CTL_STR,	NET_TCP_CONG_CONTROL,			"tcp_congestion_control" },
	{ CTL_INT,	NET_TCP_ABC,				"tcp_abc" },
	{ CTL_INT,	NET_TCP_MTU_PROBING,			"tcp_mtu_probing" },
	{ CTL_INT,	NET_TCP_BASE_MSS,			"tcp_base_mss" },
	{ CTL_INT,	NET_IPV4_TCP_WORKAROUND_SIGNED_WINDOWS,	"tcp_workaround_signed_windows" },
	{ CTL_INT,	NET_TCP_DMA_COPYBREAK,			"tcp_dma_copybreak" },
	{ CTL_INT,	NET_TCP_SLOW_START_AFTER_IDLE,		"tcp_slow_start_after_idle" },
	{ CTL_INT,	NET_CIPSOV4_CACHE_ENABLE,		"cipso_cache_enable" },
	{ CTL_INT,	NET_CIPSOV4_CACHE_BUCKET_SIZE,		"cipso_cache_bucket_size" },
	{ CTL_INT,	NET_CIPSOV4_RBM_OPTFMT,			"cipso_rbm_optfmt" },
	{ CTL_INT,	NET_CIPSOV4_RBM_STRICTVALID,		"cipso_rbm_strictvalid" },
	
	{ CTL_STR,	NET_TCP_ALLOWED_CONG_CONTROL,		"tcp_allowed_congestion_control" },
	{ CTL_INT,	NET_TCP_MAX_SSTHRESH,			"tcp_max_ssthresh" },

	{ CTL_INT,	NET_IPV4_ICMP_ECHO_IGNORE_ALL,		"icmp_echo_ignore_all" },
	{ CTL_INT,	NET_IPV4_ICMP_ECHO_IGNORE_BROADCASTS,	"icmp_echo_ignore_broadcasts" },
	{ CTL_INT,	NET_IPV4_ICMP_IGNORE_BOGUS_ERROR_RESPONSES,	"icmp_ignore_bogus_error_responses" },
	{ CTL_INT,	NET_IPV4_ICMP_ERRORS_USE_INBOUND_IFADDR,	"icmp_errors_use_inbound_ifaddr" },
	{ CTL_INT,	NET_IPV4_ICMP_RATELIMIT,		"icmp_ratelimit" },
	{ CTL_INT,	NET_IPV4_ICMP_RATEMASK,			"icmp_ratemask" },

	{ CTL_INT,	NET_IPV4_IPFRAG_HIGH_THRESH,		"ipfrag_high_thresh" },
	{ CTL_INT,	NET_IPV4_IPFRAG_LOW_THRESH,		"ipfrag_low_thresh" },
	{ CTL_INT,	NET_IPV4_IPFRAG_TIME,			"ipfrag_time" },

	{ CTL_INT,	NET_IPV4_IPFRAG_SECRET_INTERVAL,	"ipfrag_secret_interval" },
	

	{ CTL_INT,	2088 ,		"ip_queue_maxlen" },

	
	
	
	
	
	
	
	
	
	
	
	{}
};

static const struct bin_table bin_net_ipx_table[] = {
	{ CTL_INT,	NET_IPX_PPROP_BROADCASTING,	"ipx_pprop_broadcasting" },
	
	{}
};

static const struct bin_table bin_net_atalk_table[] = {
	{ CTL_INT,	NET_ATALK_AARP_EXPIRY_TIME,		"aarp-expiry-time" },
	{ CTL_INT,	NET_ATALK_AARP_TICK_TIME,		"aarp-tick-time" },
	{ CTL_INT,	NET_ATALK_AARP_RETRANSMIT_LIMIT,	"aarp-retransmit-limit" },
	{ CTL_INT,	NET_ATALK_AARP_RESOLVE_TIME,		"aarp-resolve-time" },
	{},
};

static const struct bin_table bin_net_netrom_table[] = {
	{ CTL_INT,	NET_NETROM_DEFAULT_PATH_QUALITY,		"default_path_quality" },
	{ CTL_INT,	NET_NETROM_OBSOLESCENCE_COUNT_INITIALISER,	"obsolescence_count_initialiser" },
	{ CTL_INT,	NET_NETROM_NETWORK_TTL_INITIALISER,		"network_ttl_initialiser" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_TIMEOUT,			"transport_timeout" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_MAXIMUM_TRIES,		"transport_maximum_tries" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_ACKNOWLEDGE_DELAY,		"transport_acknowledge_delay" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_BUSY_DELAY,		"transport_busy_delay" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_REQUESTED_WINDOW_SIZE,	"transport_requested_window_size" },
	{ CTL_INT,	NET_NETROM_TRANSPORT_NO_ACTIVITY_TIMEOUT,	"transport_no_activity_timeout" },
	{ CTL_INT,	NET_NETROM_ROUTING_CONTROL,			"routing_control" },
	{ CTL_INT,	NET_NETROM_LINK_FAILS_COUNT,			"link_fails_count" },
	{ CTL_INT,	NET_NETROM_RESET,				"reset" },
	{}
};

static const struct bin_table bin_net_ax25_param_table[] = {
	{ CTL_INT,	NET_AX25_IP_DEFAULT_MODE,	"ip_default_mode" },
	{ CTL_INT,	NET_AX25_DEFAULT_MODE,		"ax25_default_mode" },
	{ CTL_INT,	NET_AX25_BACKOFF_TYPE,		"backoff_type" },
	{ CTL_INT,	NET_AX25_CONNECT_MODE,		"connect_mode" },
	{ CTL_INT,	NET_AX25_STANDARD_WINDOW,	"standard_window_size" },
	{ CTL_INT,	NET_AX25_EXTENDED_WINDOW,	"extended_window_size" },
	{ CTL_INT,	NET_AX25_T1_TIMEOUT,		"t1_timeout" },
	{ CTL_INT,	NET_AX25_T2_TIMEOUT,		"t2_timeout" },
	{ CTL_INT,	NET_AX25_T3_TIMEOUT,		"t3_timeout" },
	{ CTL_INT,	NET_AX25_IDLE_TIMEOUT,		"idle_timeout" },
	{ CTL_INT,	NET_AX25_N2,			"maximum_retry_count" },
	{ CTL_INT,	NET_AX25_PACLEN,		"maximum_packet_length" },
	{ CTL_INT,	NET_AX25_PROTOCOL,		"protocol" },
	{ CTL_INT,	NET_AX25_DAMA_SLAVE_TIMEOUT,	"dama_slave_timeout" },
	{}
};

static const struct bin_table bin_net_ax25_table[] = {
	{ CTL_DIR,	0, NULL, bin_net_ax25_param_table },
	{}
};

static const struct bin_table bin_net_rose_table[] = {
	{ CTL_INT,	NET_ROSE_RESTART_REQUEST_TIMEOUT,	"restart_request_timeout" },
	{ CTL_INT,	NET_ROSE_CALL_REQUEST_TIMEOUT,		"call_request_timeout" },
	{ CTL_INT,	NET_ROSE_RESET_REQUEST_TIMEOUT,		"reset_request_timeout" },
	{ CTL_INT,	NET_ROSE_CLEAR_REQUEST_TIMEOUT,		"clear_request_timeout" },
	{ CTL_INT,	NET_ROSE_ACK_HOLD_BACK_TIMEOUT,		"acknowledge_hold_back_timeout" },
	{ CTL_INT,	NET_ROSE_ROUTING_CONTROL,		"routing_control" },
	{ CTL_INT,	NET_ROSE_LINK_FAIL_TIMEOUT,		"link_fail_timeout" },
	{ CTL_INT,	NET_ROSE_MAX_VCS,			"maximum_virtual_circuits" },
	{ CTL_INT,	NET_ROSE_WINDOW_SIZE,			"window_size" },
	{ CTL_INT,	NET_ROSE_NO_ACTIVITY_TIMEOUT,		"no_activity_timeout" },
	{}
};

static const struct bin_table bin_net_ipv6_conf_var_table[] = {
	{ CTL_INT,	NET_IPV6_FORWARDING,			"forwarding" },
	{ CTL_INT,	NET_IPV6_HOP_LIMIT,			"hop_limit" },
	{ CTL_INT,	NET_IPV6_MTU,				"mtu" },
	{ CTL_INT,	NET_IPV6_ACCEPT_RA,			"accept_ra" },
	{ CTL_INT,	NET_IPV6_ACCEPT_REDIRECTS,		"accept_redirects" },
	{ CTL_INT,	NET_IPV6_AUTOCONF,			"autoconf" },
	{ CTL_INT,	NET_IPV6_DAD_TRANSMITS,			"dad_transmits" },
	{ CTL_INT,	NET_IPV6_RTR_SOLICITS,			"router_solicitations" },
	{ CTL_INT,	NET_IPV6_RTR_SOLICIT_INTERVAL,		"router_solicitation_interval" },
	{ CTL_INT,	NET_IPV6_RTR_SOLICIT_DELAY,		"router_solicitation_delay" },
	{ CTL_INT,	NET_IPV6_USE_TEMPADDR,			"use_tempaddr" },
	{ CTL_INT,	NET_IPV6_TEMP_VALID_LFT,		"temp_valid_lft" },
	{ CTL_INT,	NET_IPV6_TEMP_PREFERED_LFT,		"temp_prefered_lft" },
	{ CTL_INT,	NET_IPV6_REGEN_MAX_RETRY,		"regen_max_retry" },
	{ CTL_INT,	NET_IPV6_MAX_DESYNC_FACTOR,		"max_desync_factor" },
	{ CTL_INT,	NET_IPV6_MAX_ADDRESSES,			"max_addresses" },
	{ CTL_INT,	NET_IPV6_FORCE_MLD_VERSION,		"force_mld_version" },
	{ CTL_INT,	NET_IPV6_ACCEPT_RA_DEFRTR,		"accept_ra_defrtr" },
	{ CTL_INT,	NET_IPV6_ACCEPT_RA_PINFO,		"accept_ra_pinfo" },
	{ CTL_INT,	NET_IPV6_ACCEPT_RA_RTR_PREF,		"accept_ra_rtr_pref" },
	{ CTL_INT,	NET_IPV6_RTR_PROBE_INTERVAL,		"router_probe_interval" },
	{ CTL_INT,	NET_IPV6_ACCEPT_RA_RT_INFO_MAX_PLEN,	"accept_ra_rt_info_max_plen" },
	{ CTL_INT,	NET_IPV6_PROXY_NDP,			"proxy_ndp" },
	{ CTL_INT,	NET_IPV6_ACCEPT_SOURCE_ROUTE,		"accept_source_route" },
	{}
};

static const struct bin_table bin_net_ipv6_conf_table[] = {
	{ CTL_DIR,	NET_PROTO_CONF_ALL,		"all",	bin_net_ipv6_conf_var_table },
	{ CTL_DIR,	NET_PROTO_CONF_DEFAULT, 	"default", bin_net_ipv6_conf_var_table },
	{ CTL_DIR,	0, NULL, bin_net_ipv6_conf_var_table },
	{}
};

static const struct bin_table bin_net_ipv6_route_table[] = {
	
	{ CTL_INT,	NET_IPV6_ROUTE_GC_THRESH,		"gc_thresh" },
	{ CTL_INT,	NET_IPV6_ROUTE_MAX_SIZE,		"max_size" },
	{ CTL_INT,	NET_IPV6_ROUTE_GC_MIN_INTERVAL,		"gc_min_interval" },
	{ CTL_INT,	NET_IPV6_ROUTE_GC_TIMEOUT,		"gc_timeout" },
	{ CTL_INT,	NET_IPV6_ROUTE_GC_INTERVAL,		"gc_interval" },
	{ CTL_INT,	NET_IPV6_ROUTE_GC_ELASTICITY,		"gc_elasticity" },
	{ CTL_INT,	NET_IPV6_ROUTE_MTU_EXPIRES,		"mtu_expires" },
	{ CTL_INT,	NET_IPV6_ROUTE_MIN_ADVMSS,		"min_adv_mss" },
	{ CTL_INT,	NET_IPV6_ROUTE_GC_MIN_INTERVAL_MS,	"gc_min_interval_ms" },
	{}
};

static const struct bin_table bin_net_ipv6_icmp_table[] = {
	{ CTL_INT,	NET_IPV6_ICMP_RATELIMIT,	"ratelimit" },
	{}
};

static const struct bin_table bin_net_ipv6_table[] = {
	{ CTL_DIR,	NET_IPV6_CONF,		"conf",		bin_net_ipv6_conf_table },
	{ CTL_DIR,	NET_IPV6_NEIGH,		"neigh",	bin_net_neigh_table },
	{ CTL_DIR,	NET_IPV6_ROUTE,		"route",	bin_net_ipv6_route_table },
	{ CTL_DIR,	NET_IPV6_ICMP,		"icmp",		bin_net_ipv6_icmp_table },
	{ CTL_INT,	NET_IPV6_BINDV6ONLY,		"bindv6only" },
	{ CTL_INT,	NET_IPV6_IP6FRAG_HIGH_THRESH,	"ip6frag_high_thresh" },
	{ CTL_INT,	NET_IPV6_IP6FRAG_LOW_THRESH,	"ip6frag_low_thresh" },
	{ CTL_INT,	NET_IPV6_IP6FRAG_TIME,		"ip6frag_time" },
	{ CTL_INT,	NET_IPV6_IP6FRAG_SECRET_INTERVAL,	"ip6frag_secret_interval" },
	{ CTL_INT,	NET_IPV6_MLD_MAX_MSF,		"mld_max_msf" },
	{ CTL_INT,	2088 ,		"ip6_queue_maxlen" },
	{}
};

static const struct bin_table bin_net_x25_table[] = {
	{ CTL_INT,	NET_X25_RESTART_REQUEST_TIMEOUT,	"restart_request_timeout" },
	{ CTL_INT,	NET_X25_CALL_REQUEST_TIMEOUT,		"call_request_timeout" },
	{ CTL_INT,	NET_X25_RESET_REQUEST_TIMEOUT,	"reset_request_timeout" },
	{ CTL_INT,	NET_X25_CLEAR_REQUEST_TIMEOUT,	"clear_request_timeout" },
	{ CTL_INT,	NET_X25_ACK_HOLD_BACK_TIMEOUT,	"acknowledgement_hold_back_timeout" },
	{ CTL_INT,	NET_X25_FORWARD,			"x25_forward" },
	{}
};

static const struct bin_table bin_net_tr_table[] = {
	{ CTL_INT,	NET_TR_RIF_TIMEOUT,	"rif_timeout" },
	{}
};


static const struct bin_table bin_net_decnet_conf_vars[] = {
	{ CTL_INT,	NET_DECNET_CONF_DEV_FORWARDING,	"forwarding" },
	{ CTL_INT,	NET_DECNET_CONF_DEV_PRIORITY,	"priority" },
	{ CTL_INT,	NET_DECNET_CONF_DEV_T2,		"t2" },
	{ CTL_INT,	NET_DECNET_CONF_DEV_T3,		"t3" },
	{}
};

static const struct bin_table bin_net_decnet_conf[] = {
	{ CTL_DIR, NET_DECNET_CONF_ETHER,    "ethernet", bin_net_decnet_conf_vars },
	{ CTL_DIR, NET_DECNET_CONF_GRE,	     "ipgre",    bin_net_decnet_conf_vars },
	{ CTL_DIR, NET_DECNET_CONF_X25,	     "x25",      bin_net_decnet_conf_vars },
	{ CTL_DIR, NET_DECNET_CONF_PPP,	     "ppp",      bin_net_decnet_conf_vars },
	{ CTL_DIR, NET_DECNET_CONF_DDCMP,    "ddcmp",    bin_net_decnet_conf_vars },
	{ CTL_DIR, NET_DECNET_CONF_LOOPBACK, "loopback", bin_net_decnet_conf_vars },
	{ CTL_DIR, 0,			     NULL,	 bin_net_decnet_conf_vars },
	{}
};

static const struct bin_table bin_net_decnet_table[] = {
	{ CTL_DIR,	NET_DECNET_CONF,		"conf",	bin_net_decnet_conf },
	{ CTL_DNADR,	NET_DECNET_NODE_ADDRESS,	"node_address" },
	{ CTL_STR,	NET_DECNET_NODE_NAME,		"node_name" },
	{ CTL_STR,	NET_DECNET_DEFAULT_DEVICE,	"default_device" },
	{ CTL_INT,	NET_DECNET_TIME_WAIT,		"time_wait" },
	{ CTL_INT,	NET_DECNET_DN_COUNT,		"dn_count" },
	{ CTL_INT,	NET_DECNET_DI_COUNT,		"di_count" },
	{ CTL_INT,	NET_DECNET_DR_COUNT,		"dr_count" },
	{ CTL_INT,	NET_DECNET_DST_GC_INTERVAL,	"dst_gc_interval" },
	{ CTL_INT,	NET_DECNET_NO_FC_MAX_CWND,	"no_fc_max_cwnd" },
	{ CTL_INT,	NET_DECNET_MEM,		"decnet_mem" },
	{ CTL_INT,	NET_DECNET_RMEM,		"decnet_rmem" },
	{ CTL_INT,	NET_DECNET_WMEM,		"decnet_wmem" },
	{ CTL_INT,	NET_DECNET_DEBUG_LEVEL,	"debug" },
	{}
};

static const struct bin_table bin_net_sctp_table[] = {
	{ CTL_INT,	NET_SCTP_RTO_INITIAL,		"rto_initial" },
	{ CTL_INT,	NET_SCTP_RTO_MIN,		"rto_min" },
	{ CTL_INT,	NET_SCTP_RTO_MAX,		"rto_max" },
	{ CTL_INT,	NET_SCTP_RTO_ALPHA,		"rto_alpha_exp_divisor" },
	{ CTL_INT,	NET_SCTP_RTO_BETA,		"rto_beta_exp_divisor" },
	{ CTL_INT,	NET_SCTP_VALID_COOKIE_LIFE,	"valid_cookie_life" },
	{ CTL_INT,	NET_SCTP_ASSOCIATION_MAX_RETRANS,	"association_max_retrans" },
	{ CTL_INT,	NET_SCTP_PATH_MAX_RETRANS,	"path_max_retrans" },
	{ CTL_INT,	NET_SCTP_MAX_INIT_RETRANSMITS,	"max_init_retransmits" },
	{ CTL_INT,	NET_SCTP_HB_INTERVAL,		"hb_interval" },
	{ CTL_INT,	NET_SCTP_PRESERVE_ENABLE,	"cookie_preserve_enable" },
	{ CTL_INT,	NET_SCTP_MAX_BURST,		"max_burst" },
	{ CTL_INT,	NET_SCTP_ADDIP_ENABLE,		"addip_enable" },
	{ CTL_INT,	NET_SCTP_PRSCTP_ENABLE,		"prsctp_enable" },
	{ CTL_INT,	NET_SCTP_SNDBUF_POLICY,		"sndbuf_policy" },
	{ CTL_INT,	NET_SCTP_SACK_TIMEOUT,		"sack_timeout" },
	{ CTL_INT,	NET_SCTP_RCVBUF_POLICY,		"rcvbuf_policy" },
	{}
};

static const struct bin_table bin_net_llc_llc2_timeout_table[] = {
	{ CTL_INT,	NET_LLC2_ACK_TIMEOUT,	"ack" },
	{ CTL_INT,	NET_LLC2_P_TIMEOUT,	"p" },
	{ CTL_INT,	NET_LLC2_REJ_TIMEOUT,	"rej" },
	{ CTL_INT,	NET_LLC2_BUSY_TIMEOUT,	"busy" },
	{}
};

static const struct bin_table bin_net_llc_station_table[] = {
	{ CTL_INT,	NET_LLC_STATION_ACK_TIMEOUT,	"ack_timeout" },
	{}
};

static const struct bin_table bin_net_llc_llc2_table[] = {
	{ CTL_DIR,	NET_LLC2,		"timeout",	bin_net_llc_llc2_timeout_table },
	{}
};

static const struct bin_table bin_net_llc_table[] = {
	{ CTL_DIR,	NET_LLC2,		"llc2",		bin_net_llc_llc2_table },
	{ CTL_DIR,	NET_LLC_STATION,	"station",	bin_net_llc_station_table },
	{}
};

static const struct bin_table bin_net_netfilter_table[] = {
	{ CTL_INT,	NET_NF_CONNTRACK_MAX,			"nf_conntrack_max" },
	
	
	
	
	
	
	
	
	
	
	
	
	{ CTL_INT,	NET_NF_CONNTRACK_BUCKETS,		"nf_conntrack_buckets" },
	{ CTL_INT,	NET_NF_CONNTRACK_LOG_INVALID,		"nf_conntrack_log_invalid" },
	
	{ CTL_INT,	NET_NF_CONNTRACK_TCP_LOOSE,		"nf_conntrack_tcp_loose" },
	{ CTL_INT,	NET_NF_CONNTRACK_TCP_BE_LIBERAL,	"nf_conntrack_tcp_be_liberal" },
	{ CTL_INT,	NET_NF_CONNTRACK_TCP_MAX_RETRANS,	"nf_conntrack_tcp_max_retrans" },
	
	
	
	
	
	
	
	{ CTL_INT,	NET_NF_CONNTRACK_COUNT,			"nf_conntrack_count" },
	
	
	{ CTL_INT,	NET_NF_CONNTRACK_FRAG6_LOW_THRESH,	"nf_conntrack_frag6_low_thresh" },
	{ CTL_INT,	NET_NF_CONNTRACK_FRAG6_HIGH_THRESH,	"nf_conntrack_frag6_high_thresh" },
	{ CTL_INT,	NET_NF_CONNTRACK_CHECKSUM,		"nf_conntrack_checksum" },

	{}
};

static const struct bin_table bin_net_irda_table[] = {
	{ CTL_INT,	NET_IRDA_DISCOVERY,		"discovery" },
	{ CTL_STR,	NET_IRDA_DEVNAME,		"devname" },
	{ CTL_INT,	NET_IRDA_DEBUG,			"debug" },
	{ CTL_INT,	NET_IRDA_FAST_POLL,		"fast_poll_increase" },
	{ CTL_INT,	NET_IRDA_DISCOVERY_SLOTS,	"discovery_slots" },
	{ CTL_INT,	NET_IRDA_DISCOVERY_TIMEOUT,	"discovery_timeout" },
	{ CTL_INT,	NET_IRDA_SLOT_TIMEOUT,		"slot_timeout" },
	{ CTL_INT,	NET_IRDA_MAX_BAUD_RATE,		"max_baud_rate" },
	{ CTL_INT,	NET_IRDA_MIN_TX_TURN_TIME,	"min_tx_turn_time" },
	{ CTL_INT,	NET_IRDA_MAX_TX_DATA_SIZE,	"max_tx_data_size" },
	{ CTL_INT,	NET_IRDA_MAX_TX_WINDOW,		"max_tx_window" },
	{ CTL_INT,	NET_IRDA_MAX_NOREPLY_TIME,	"max_noreply_time" },
	{ CTL_INT,	NET_IRDA_WARN_NOREPLY_TIME,	"warn_noreply_time" },
	{ CTL_INT,	NET_IRDA_LAP_KEEPALIVE_TIME,	"lap_keepalive_time" },
	{}
};

static const struct bin_table bin_net_table[] = {
	{ CTL_DIR,	NET_CORE,		"core",		bin_net_core_table },
	
	
	{ CTL_DIR,	NET_UNIX,		"unix",		bin_net_unix_table },
	{ CTL_DIR,	NET_IPV4,		"ipv4",		bin_net_ipv4_table },
	{ CTL_DIR,	NET_IPX,		"ipx",		bin_net_ipx_table },
	{ CTL_DIR,	NET_ATALK,		"appletalk",	bin_net_atalk_table },
	{ CTL_DIR,	NET_NETROM,		"netrom",	bin_net_netrom_table },
	{ CTL_DIR,	NET_AX25,		"ax25",		bin_net_ax25_table },
	
	{ CTL_DIR,	NET_ROSE,		"rose",		bin_net_rose_table },
	{ CTL_DIR,	NET_IPV6,		"ipv6",		bin_net_ipv6_table },
	{ CTL_DIR,	NET_X25,		"x25",		bin_net_x25_table },
	{ CTL_DIR,	NET_TR,			"token-ring",	bin_net_tr_table },
	{ CTL_DIR,	NET_DECNET,		"decnet",	bin_net_decnet_table },
	
	{ CTL_DIR,	NET_SCTP,		"sctp",		bin_net_sctp_table },
	{ CTL_DIR,	NET_LLC,		"llc",		bin_net_llc_table },
	{ CTL_DIR,	NET_NETFILTER,		"netfilter",	bin_net_netfilter_table },
	
	{ CTL_DIR,	NET_IRDA,		"irda",		bin_net_irda_table },
	{ CTL_INT,	2089,			"nf_conntrack_max" },
	{}
};

static const struct bin_table bin_fs_quota_table[] = {
	{ CTL_INT,	FS_DQ_LOOKUPS,		"lookups" },
	{ CTL_INT,	FS_DQ_DROPS,		"drops" },
	{ CTL_INT,	FS_DQ_READS,		"reads" },
	{ CTL_INT,	FS_DQ_WRITES,		"writes" },
	{ CTL_INT,	FS_DQ_CACHE_HITS,	"cache_hits" },
	{ CTL_INT,	FS_DQ_ALLOCATED,	"allocated_dquots" },
	{ CTL_INT,	FS_DQ_FREE,		"free_dquots" },
	{ CTL_INT,	FS_DQ_SYNCS,		"syncs" },
	{ CTL_INT,	FS_DQ_WARNINGS,		"warnings" },
	{}
};

static const struct bin_table bin_fs_xfs_table[] = {
	{ CTL_INT,	XFS_SGID_INHERIT,	"irix_sgid_inherit" },
	{ CTL_INT,	XFS_SYMLINK_MODE,	"irix_symlink_mode" },
	{ CTL_INT,	XFS_PANIC_MASK,		"panic_mask" },

	{ CTL_INT,	XFS_ERRLEVEL,		"error_level" },
	{ CTL_INT,	XFS_SYNCD_TIMER,	"xfssyncd_centisecs" },
	{ CTL_INT,	XFS_INHERIT_SYNC,	"inherit_sync" },
	{ CTL_INT,	XFS_INHERIT_NODUMP,	"inherit_nodump" },
	{ CTL_INT,	XFS_INHERIT_NOATIME,	"inherit_noatime" },
	{ CTL_INT,	XFS_BUF_TIMER,		"xfsbufd_centisecs" },
	{ CTL_INT,	XFS_BUF_AGE,		"age_buffer_centisecs" },
	{ CTL_INT,	XFS_INHERIT_NOSYM,	"inherit_nosymlinks" },
	{ CTL_INT,	XFS_ROTORSTEP,	"rotorstep" },
	{ CTL_INT,	XFS_INHERIT_NODFRG,	"inherit_nodefrag" },
	{ CTL_INT,	XFS_FILESTREAM_TIMER,	"filestream_centisecs" },
	{ CTL_INT,	XFS_STATS_CLEAR,	"stats_clear" },
	{}
};

static const struct bin_table bin_fs_ocfs2_nm_table[] = {
	{ CTL_STR,	1, "hb_ctl_path" },
	{}
};

static const struct bin_table bin_fs_ocfs2_table[] = {
	{ CTL_DIR,	1,	"nm",	bin_fs_ocfs2_nm_table },
	{}
};

static const struct bin_table bin_inotify_table[] = {
	{ CTL_INT,	INOTIFY_MAX_USER_INSTANCES,	"max_user_instances" },
	{ CTL_INT,	INOTIFY_MAX_USER_WATCHES,	"max_user_watches" },
	{ CTL_INT,	INOTIFY_MAX_QUEUED_EVENTS,	"max_queued_events" },
	{}
};

static const struct bin_table bin_fs_table[] = {
	{ CTL_INT,	FS_NRINODE,		"inode-nr" },
	{ CTL_INT,	FS_STATINODE,		"inode-state" },
	
	
	
	
	{ CTL_INT,	FS_MAXFILE,		"file-max" },
	{ CTL_INT,	FS_DENTRY,		"dentry-state" },
	
	
	{ CTL_INT,	FS_OVERFLOWUID,		"overflowuid" },
	{ CTL_INT,	FS_OVERFLOWGID,		"overflowgid" },
	{ CTL_INT,	FS_LEASES,		"leases-enable" },
	{ CTL_INT,	FS_DIR_NOTIFY,		"dir-notify-enable" },
	{ CTL_INT,	FS_LEASE_TIME,		"lease-break-time" },
	{ CTL_DIR,	FS_DQSTATS,		"quota",	bin_fs_quota_table },
	{ CTL_DIR,	FS_XFS,			"xfs",		bin_fs_xfs_table },
	{ CTL_ULONG,	FS_AIO_NR,		"aio-nr" },
	{ CTL_ULONG,	FS_AIO_MAX_NR,		"aio-max-nr" },
	{ CTL_DIR,	FS_INOTIFY,		"inotify",	bin_inotify_table },
	{ CTL_DIR,	FS_OCFS2,		"ocfs2",	bin_fs_ocfs2_table },
	{ CTL_INT,	KERN_SETUID_DUMPABLE,	"suid_dumpable" },
	{}
};

static const struct bin_table bin_ipmi_table[] = {
	{ CTL_INT,	DEV_IPMI_POWEROFF_POWERCYCLE,	"poweroff_powercycle" },
	{}
};

static const struct bin_table bin_mac_hid_files[] = {
	
	
	{ CTL_INT,	DEV_MAC_HID_MOUSE_BUTTON_EMULATION,	"mouse_button_emulation" },
	{ CTL_INT,	DEV_MAC_HID_MOUSE_BUTTON2_KEYCODE,	"mouse_button2_keycode" },
	{ CTL_INT,	DEV_MAC_HID_MOUSE_BUTTON3_KEYCODE,	"mouse_button3_keycode" },
	
	{}
};

static const struct bin_table bin_raid_table[] = {
	{ CTL_INT,	DEV_RAID_SPEED_LIMIT_MIN,	"speed_limit_min" },
	{ CTL_INT,	DEV_RAID_SPEED_LIMIT_MAX,	"speed_limit_max" },
	{}
};

static const struct bin_table bin_scsi_table[] = {
	{ CTL_INT, DEV_SCSI_LOGGING_LEVEL, "logging_level" },
	{}
};

static const struct bin_table bin_dev_table[] = {
	
	
	
	{ CTL_DIR,	DEV_RAID,	"raid",		bin_raid_table },
	{ CTL_DIR,	DEV_MAC_HID,	"mac_hid",	bin_mac_hid_files },
	{ CTL_DIR,	DEV_SCSI,	"scsi",		bin_scsi_table },
	{ CTL_DIR,	DEV_IPMI,	"ipmi",		bin_ipmi_table },
	{}
};

static const struct bin_table bin_bus_isa_table[] = {
	{ CTL_INT,	BUS_ISA_MEM_BASE,	"membase" },
	{ CTL_INT,	BUS_ISA_PORT_BASE,	"portbase" },
	{ CTL_INT,	BUS_ISA_PORT_SHIFT,	"portshift" },
	{}
};

static const struct bin_table bin_bus_table[] = {
	{ CTL_DIR,	CTL_BUS_ISA,	"isa",	bin_bus_isa_table },
	{}
};


static const struct bin_table bin_s390dbf_table[] = {
	{ CTL_INT,	5678 , "debug_stoppable" },
	{ CTL_INT,	5679 ,	  "debug_active" },
	{}
};

static const struct bin_table bin_sunrpc_table[] = {
	
	
	
	

	{ CTL_INT,	CTL_SLOTTABLE_UDP,	"udp_slot_table_entries" },
	{ CTL_INT,	CTL_SLOTTABLE_TCP,	"tcp_slot_table_entries" },
	{ CTL_INT,	CTL_MIN_RESVPORT,	"min_resvport" },
	{ CTL_INT,	CTL_MAX_RESVPORT,	"max_resvport" },
	{}
};

static const struct bin_table bin_pm_table[] = {
	
	
	{ CTL_INT,	2 ,		"cmode" },
	{ CTL_INT,	3 ,		"p0" },
	{ CTL_INT,	4 ,		"cm" },
	{}
};

static const struct bin_table bin_root_table[] = {
	{ CTL_DIR,	CTL_KERN,	"kernel",	bin_kern_table },
	{ CTL_DIR,	CTL_VM,		"vm",		bin_vm_table },
	{ CTL_DIR,	CTL_NET,	"net",		bin_net_table },
	
	{ CTL_DIR,	CTL_FS,		"fs",		bin_fs_table },
	
	{ CTL_DIR,	CTL_DEV,	"dev",		bin_dev_table },
	{ CTL_DIR,	CTL_BUS,	"bus",		bin_bus_table },
	{ CTL_DIR,	CTL_ABI,	"abi" },
	
	
	{ CTL_DIR,	CTL_S390DBF,	"s390dbf",	bin_s390dbf_table },
	{ CTL_DIR,	CTL_SUNRPC,	"sunrpc",	bin_sunrpc_table },
	{ CTL_DIR,	CTL_PM,		"pm",		bin_pm_table },
	{}
};

static ssize_t bin_dir(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	return -ENOTDIR;
}


static ssize_t bin_string(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	ssize_t result, copied = 0;

	if (oldval && oldlen) {
		char __user *lastp;
		loff_t pos = 0;
		int ch;

		result = vfs_read(file, oldval, oldlen, &pos);
		if (result < 0)
			goto out;

		copied = result;
		lastp = oldval + copied - 1;

		result = -EFAULT;
		if (get_user(ch, lastp))
			goto out;

		
		if (ch == '\n') {
			result = -EFAULT;
			if (put_user('\0', lastp))
				goto out;
			copied -= 1;
		}
	}

	if (newval && newlen) {
		loff_t pos = 0;

		result = vfs_write(file, newval, newlen, &pos);
		if (result < 0)
			goto out;
	}

	result = copied;
out:
	return result;
}

static ssize_t bin_intvec(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	mm_segment_t old_fs = get_fs();
	ssize_t copied = 0;
	char *buffer;
	ssize_t result;

	result = -ENOMEM;
	buffer = kmalloc(BUFSZ, GFP_KERNEL);
	if (!buffer)
		goto out;

	if (oldval && oldlen) {
		unsigned __user *vec = oldval;
		size_t length = oldlen / sizeof(*vec);
		loff_t pos = 0;
		char *str, *end;
		int i;

		set_fs(KERNEL_DS);
		result = vfs_read(file, buffer, BUFSZ - 1, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out_kfree;

		str = buffer;
		end = str + result;
		*end++ = '\0';
		for (i = 0; i < length; i++) {
			unsigned long value;

			value = simple_strtoul(str, &str, 10);
			while (isspace(*str))
				str++;
			
			result = -EFAULT;
			if (put_user(value, vec + i))
				goto out_kfree;

			copied += sizeof(*vec);
			if (!isdigit(*str))
				break;
		}
	}

	if (newval && newlen) {
		unsigned __user *vec = newval;
		size_t length = newlen / sizeof(*vec);
		loff_t pos = 0;
		char *str, *end;
		int i;

		str = buffer;
		end = str + BUFSZ;
		for (i = 0; i < length; i++) {
			unsigned long value;

			result = -EFAULT;
			if (get_user(value, vec + i))
				goto out_kfree;

			str += snprintf(str, end - str, "%lu\t", value);
		}

		set_fs(KERNEL_DS);
		result = vfs_write(file, buffer, str - buffer, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out_kfree;
	}
	result = copied;
out_kfree:
	kfree(buffer);
out:
	return result;
}

static ssize_t bin_ulongvec(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	mm_segment_t old_fs = get_fs();
	ssize_t copied = 0;
	char *buffer;
	ssize_t result;

	result = -ENOMEM;
	buffer = kmalloc(BUFSZ, GFP_KERNEL);
	if (!buffer)
		goto out;

	if (oldval && oldlen) {
		unsigned long __user *vec = oldval;
		size_t length = oldlen / sizeof(*vec);
		loff_t pos = 0;
		char *str, *end;
		int i;

		set_fs(KERNEL_DS);
		result = vfs_read(file, buffer, BUFSZ - 1, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out_kfree;

		str = buffer;
		end = str + result;
		*end++ = '\0';
		for (i = 0; i < length; i++) {
			unsigned long value;

			value = simple_strtoul(str, &str, 10);
			while (isspace(*str))
				str++;
			
			result = -EFAULT;
			if (put_user(value, vec + i))
				goto out_kfree;

			copied += sizeof(*vec);
			if (!isdigit(*str))
				break;
		}
	}

	if (newval && newlen) {
		unsigned long __user *vec = newval;
		size_t length = newlen / sizeof(*vec);
		loff_t pos = 0;
		char *str, *end;
		int i;

		str = buffer;
		end = str + BUFSZ;
		for (i = 0; i < length; i++) {
			unsigned long value;

			result = -EFAULT;
			if (get_user(value, vec + i))
				goto out_kfree;

			str += snprintf(str, end - str, "%lu\t", value);
		}

		set_fs(KERNEL_DS);
		result = vfs_write(file, buffer, str - buffer, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out_kfree;
	}
	result = copied;
out_kfree:
	kfree(buffer);
out:
	return result;
}

static ssize_t bin_uuid(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	mm_segment_t old_fs = get_fs();
	ssize_t result, copied = 0;

	
	if (oldval && oldlen) {
		loff_t pos = 0;
		char buf[40], *str = buf;
		unsigned char uuid[16];
		int i;

		set_fs(KERNEL_DS);
		result = vfs_read(file, buf, sizeof(buf) - 1, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out;

		buf[result] = '\0';

		
		for (i = 0; i < 16; i++) {
			result = -EIO;
			if (!isxdigit(str[0]) || !isxdigit(str[1]))
				goto out;

			uuid[i] = (hex_to_bin(str[0]) << 4) |
					hex_to_bin(str[1]);
			str += 2;
			if (*str == '-')
				str++;
		}

		if (oldlen > 16)
			oldlen = 16;

		result = -EFAULT;
		if (copy_to_user(oldval, uuid, oldlen))
			goto out;

		copied = oldlen;
	}
	result = copied;
out:
	return result;
}

static ssize_t bin_dn_node_address(struct file *file,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	mm_segment_t old_fs = get_fs();
	ssize_t result, copied = 0;

	if (oldval && oldlen) {
		loff_t pos = 0;
		char buf[15], *nodep;
		unsigned long area, node;
		__le16 dnaddr;

		set_fs(KERNEL_DS);
		result = vfs_read(file, buf, sizeof(buf) - 1, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out;

		buf[result] = '\0';

		
		result = -EIO;
		nodep = strchr(buf, '.') + 1;
		if (!nodep)
			goto out;

		area = simple_strtoul(buf, NULL, 10);
		node = simple_strtoul(nodep, NULL, 10);

		result = -EIO;
		if ((area > 63)||(node > 1023))
			goto out;

		dnaddr = cpu_to_le16((area << 10) | node);

		result = -EFAULT;
		if (put_user(dnaddr, (__le16 __user *)oldval))
			goto out;

		copied = sizeof(dnaddr);
	}

	if (newval && newlen) {
		loff_t pos = 0;
		__le16 dnaddr;
		char buf[15];
		int len;

		result = -EINVAL;
		if (newlen != sizeof(dnaddr))
			goto out;

		result = -EFAULT;
		if (get_user(dnaddr, (__le16 __user *)newval))
			goto out;

		len = snprintf(buf, sizeof(buf), "%hu.%hu",
				le16_to_cpu(dnaddr) >> 10,
				le16_to_cpu(dnaddr) & 0x3ff);

		set_fs(KERNEL_DS);
		result = vfs_write(file, buf, len, &pos);
		set_fs(old_fs);
		if (result < 0)
			goto out;
	}

	result = copied;
out:
	return result;
}

static const struct bin_table *get_sysctl(const int *name, int nlen, char *path)
{
	const struct bin_table *table = &bin_root_table[0];
	int ctl_name;

	memcpy(path, "sys/", 4);
	path += 4;

repeat:
	if (!nlen)
		return ERR_PTR(-ENOTDIR);
	ctl_name = *name;
	name++;
	nlen--;
	for ( ; table->convert; table++) {
		int len = 0;

		if (!table->ctl_name) {
#ifdef CONFIG_NET
			struct net *net = current->nsproxy->net_ns;
			struct net_device *dev;
			dev = dev_get_by_index(net, ctl_name);
			if (dev) {
				len = strlen(dev->name);
				memcpy(path, dev->name, len);
				dev_put(dev);
			}
#endif
		
		} else if (ctl_name == table->ctl_name) {
			len = strlen(table->procname);
			memcpy(path, table->procname, len);
		}
		if (len) {
			path += len;
			if (table->child) {
				*path++ = '/';
				table = table->child;
				goto repeat;
			}
			*path = '\0';
			return table;
		}
	}
	return ERR_PTR(-ENOTDIR);
}

static char *sysctl_getname(const int *name, int nlen, const struct bin_table **tablep)
{
	char *tmp, *result;

	result = ERR_PTR(-ENOMEM);
	tmp = __getname();
	if (tmp) {
		const struct bin_table *table = get_sysctl(name, nlen, tmp);
		result = tmp;
		*tablep = table;
		if (IS_ERR(table)) {
			__putname(tmp);
			result = ERR_CAST(table);
		}
	}
	return result;
}

static ssize_t binary_sysctl(const int *name, int nlen,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	const struct bin_table *table = NULL;
	struct vfsmount *mnt;
	struct file *file;
	ssize_t result;
	char *pathname;
	int flags;

	pathname = sysctl_getname(name, nlen, &table);
	result = PTR_ERR(pathname);
	if (IS_ERR(pathname))
		goto out;

	
	if (oldval && oldlen && newval && newlen) {
		flags = O_RDWR;
	} else if (newval && newlen) {
		flags = O_WRONLY;
	} else if (oldval && oldlen) {
		flags = O_RDONLY;
	} else {
		result = 0;
		goto out_putname;
	}

	mnt = current->nsproxy->pid_ns->proc_mnt;
	file = file_open_root(mnt->mnt_root, mnt, pathname, flags);
	result = PTR_ERR(file);
	if (IS_ERR(file))
		goto out_putname;

	result = table->convert(file, oldval, oldlen, newval, newlen);

	fput(file);
out_putname:
	__putname(pathname);
out:
	return result;
}


#else 

static ssize_t binary_sysctl(const int *name, int nlen,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	return -ENOSYS;
}

#endif 


static void deprecated_sysctl_warning(const int *name, int nlen)
{
	int i;

	if (name[0] == CTL_KERN && name[1] == KERN_VERSION)
		return;

	if (printk_ratelimit()) {
		printk(KERN_INFO
			"warning: process `%s' used the deprecated sysctl "
			"system call with ", current->comm);
		for (i = 0; i < nlen; i++)
			printk("%d.", name[i]);
		printk("\n");
	}
	return;
}

#define WARN_ONCE_HASH_BITS 8
#define WARN_ONCE_HASH_SIZE (1<<WARN_ONCE_HASH_BITS)

static DECLARE_BITMAP(warn_once_bitmap, WARN_ONCE_HASH_SIZE);

#define FNV32_OFFSET 2166136261U
#define FNV32_PRIME 0x01000193

static void warn_on_bintable(const int *name, int nlen)
{
	int i;
	u32 hash = FNV32_OFFSET;

	for (i = 0; i < nlen; i++)
		hash = (hash ^ name[i]) * FNV32_PRIME;
	hash %= WARN_ONCE_HASH_SIZE;
	if (__test_and_set_bit(hash, warn_once_bitmap))
		return;
	deprecated_sysctl_warning(name, nlen);
}

static ssize_t do_sysctl(int __user *args_name, int nlen,
	void __user *oldval, size_t oldlen, void __user *newval, size_t newlen)
{
	int name[CTL_MAXNAME];
	int i;

	
	if (nlen < 0 || nlen > CTL_MAXNAME)
		return -ENOTDIR;
	
	for (i = 0; i < nlen; i++)
		if (get_user(name[i], args_name + i))
			return -EFAULT;

	warn_on_bintable(name, nlen);

	return binary_sysctl(name, nlen, oldval, oldlen, newval, newlen);
}

SYSCALL_DEFINE1(sysctl, struct __sysctl_args __user *, args)
{
	struct __sysctl_args tmp;
	size_t oldlen = 0;
	ssize_t result;

	if (copy_from_user(&tmp, args, sizeof(tmp)))
		return -EFAULT;

	if (tmp.oldval && !tmp.oldlenp)
		return -EFAULT;

	if (tmp.oldlenp && get_user(oldlen, tmp.oldlenp))
		return -EFAULT;

	result = do_sysctl(tmp.name, tmp.nlen, tmp.oldval, oldlen,
			   tmp.newval, tmp.newlen);

	if (result >= 0) {
		oldlen = result;
		result = 0;
	}

	if (tmp.oldlenp && put_user(oldlen, tmp.oldlenp))
		return -EFAULT;

	return result;
}


#ifdef CONFIG_COMPAT
#include <asm/compat.h>

struct compat_sysctl_args {
	compat_uptr_t	name;
	int		nlen;
	compat_uptr_t	oldval;
	compat_uptr_t	oldlenp;
	compat_uptr_t	newval;
	compat_size_t	newlen;
	compat_ulong_t	__unused[4];
};

asmlinkage long compat_sys_sysctl(struct compat_sysctl_args __user *args)
{
	struct compat_sysctl_args tmp;
	compat_size_t __user *compat_oldlenp;
	size_t oldlen = 0;
	ssize_t result;

	if (copy_from_user(&tmp, args, sizeof(tmp)))
		return -EFAULT;

	if (tmp.oldval && !tmp.oldlenp)
		return -EFAULT;

	compat_oldlenp = compat_ptr(tmp.oldlenp);
	if (compat_oldlenp && get_user(oldlen, compat_oldlenp))
		return -EFAULT;

	result = do_sysctl(compat_ptr(tmp.name), tmp.nlen,
			   compat_ptr(tmp.oldval), oldlen,
			   compat_ptr(tmp.newval), tmp.newlen);

	if (result >= 0) {
		oldlen = result;
		result = 0;
	}

	if (compat_oldlenp && put_user(oldlen, compat_oldlenp))
		return -EFAULT;

	return result;
}

#endif 
