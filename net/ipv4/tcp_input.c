

#define pr_fmt(fmt) "TCP: " fmt

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/kernel.h>
#include <net/dst.h>
#include <net/tcp.h>
#include <net/inet_common.h>
#include <linux/ipsec.h>
#include <asm/unaligned.h>
#include <net/netdma.h>

int sysctl_tcp_timestamps __read_mostly = 1;
int sysctl_tcp_window_scaling __read_mostly = 1;
int sysctl_tcp_sack __read_mostly = 1;
int sysctl_tcp_fack __read_mostly = 1;
int sysctl_tcp_reordering __read_mostly = TCP_FASTRETRANS_THRESH;
EXPORT_SYMBOL(sysctl_tcp_reordering);
int sysctl_tcp_ecn __read_mostly = 2;
EXPORT_SYMBOL(sysctl_tcp_ecn);
int sysctl_tcp_dsack __read_mostly = 1;
int sysctl_tcp_app_win __read_mostly = 31;
int sysctl_tcp_adv_win_scale __read_mostly = 1;
EXPORT_SYMBOL(sysctl_tcp_adv_win_scale);

int sysctl_tcp_stdurg __read_mostly;
int sysctl_tcp_rfc1337 __read_mostly;
int sysctl_tcp_max_orphans __read_mostly = NR_FILE;
int sysctl_tcp_frto __read_mostly = 2;
int sysctl_tcp_frto_response __read_mostly;
int sysctl_tcp_nometrics_save __read_mostly;

int sysctl_tcp_thin_dupack __read_mostly;

int sysctl_tcp_moderate_rcvbuf __read_mostly = 1;
int sysctl_tcp_abc __read_mostly;
int sysctl_tcp_default_init_rwnd __read_mostly = TCP_DEFAULT_INIT_RCVWND;

#define FLAG_DATA		0x01 
#define FLAG_WIN_UPDATE		0x02 
#define FLAG_DATA_ACKED		0x04 
#define FLAG_RETRANS_DATA_ACKED	0x08 
#define FLAG_SYN_ACKED		0x10 
#define FLAG_DATA_SACKED	0x20 
#define FLAG_ECE		0x40 
#define FLAG_SLOWPATH		0x100 
#define FLAG_ONLY_ORIG_SACKED	0x200 
#define FLAG_SND_UNA_ADVANCED	0x400 
#define FLAG_DSACKING_ACK	0x800 
#define FLAG_NONHEAD_RETRANS_ACKED	0x1000 
#define FLAG_SACK_RENEGING	0x2000 

#define FLAG_ACKED		(FLAG_DATA_ACKED|FLAG_SYN_ACKED)
#define FLAG_NOT_DUP		(FLAG_DATA|FLAG_WIN_UPDATE|FLAG_ACKED)
#define FLAG_CA_ALERT		(FLAG_DATA_SACKED|FLAG_ECE)
#define FLAG_FORWARD_PROGRESS	(FLAG_ACKED|FLAG_DATA_SACKED)
#define FLAG_ANY_PROGRESS	(FLAG_FORWARD_PROGRESS|FLAG_SND_UNA_ADVANCED)

#define TCP_REMNANT (TCP_FLAG_FIN|TCP_FLAG_URG|TCP_FLAG_SYN|TCP_FLAG_PSH)
#define TCP_HP_BITS (~(TCP_RESERVED_BITS|TCP_FLAG_PSH))

static void tcp_measure_rcv_mss(struct sock *sk, const struct sk_buff *skb)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	const unsigned int lss = icsk->icsk_ack.last_seg_size;
	unsigned int len;

	icsk->icsk_ack.last_seg_size = 0;

	len = skb_shinfo(skb)->gso_size ? : skb->len;
	if (len >= icsk->icsk_ack.rcv_mss) {
		icsk->icsk_ack.rcv_mss = len;
	} else {
		len += skb->data - skb_transport_header(skb);
		if (len >= TCP_MSS_DEFAULT + sizeof(struct tcphdr) ||
		    (len >= TCP_MIN_MSS + sizeof(struct tcphdr) &&
		     !(tcp_flag_word(tcp_hdr(skb)) & TCP_REMNANT))) {
			len -= tcp_sk(sk)->tcp_header_len;
			icsk->icsk_ack.last_seg_size = len;
			if (len == lss) {
				icsk->icsk_ack.rcv_mss = len;
				return;
			}
		}
		if (icsk->icsk_ack.pending & ICSK_ACK_PUSHED)
			icsk->icsk_ack.pending |= ICSK_ACK_PUSHED2;
		icsk->icsk_ack.pending |= ICSK_ACK_PUSHED;
	}
}

static void tcp_incr_quickack(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	unsigned quickacks = tcp_sk(sk)->rcv_wnd / (2 * icsk->icsk_ack.rcv_mss);

	if (quickacks == 0)
		quickacks = 2;
	if (quickacks > icsk->icsk_ack.quick)
		icsk->icsk_ack.quick = min(quickacks, TCP_MAX_QUICKACKS);
}

static void tcp_enter_quickack_mode(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	tcp_incr_quickack(sk);
	icsk->icsk_ack.pingpong = 0;
	icsk->icsk_ack.ato = TCP_ATO_MIN;
}


static inline int tcp_in_quickack_mode(const struct sock *sk)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	return icsk->icsk_ack.quick && !icsk->icsk_ack.pingpong;
}

static inline void TCP_ECN_queue_cwr(struct tcp_sock *tp)
{
	if (tp->ecn_flags & TCP_ECN_OK)
		tp->ecn_flags |= TCP_ECN_QUEUE_CWR;
}

static inline void TCP_ECN_accept_cwr(struct tcp_sock *tp, const struct sk_buff *skb)
{
	if (tcp_hdr(skb)->cwr)
		tp->ecn_flags &= ~TCP_ECN_DEMAND_CWR;
}

static inline void TCP_ECN_withdraw_cwr(struct tcp_sock *tp)
{
	tp->ecn_flags &= ~TCP_ECN_DEMAND_CWR;
}

static inline void TCP_ECN_check_ce(struct tcp_sock *tp, const struct sk_buff *skb)
{
	if (!(tp->ecn_flags & TCP_ECN_OK))
		return;

	switch (TCP_SKB_CB(skb)->ip_dsfield & INET_ECN_MASK) {
	case INET_ECN_NOT_ECT:
		if (tp->ecn_flags & TCP_ECN_SEEN)
			tcp_enter_quickack_mode((struct sock *)tp);
		break;
	case INET_ECN_CE:
		tp->ecn_flags |= TCP_ECN_DEMAND_CWR;
		
	default:
		tp->ecn_flags |= TCP_ECN_SEEN;
	}
}

static inline void TCP_ECN_rcv_synack(struct tcp_sock *tp, const struct tcphdr *th)
{
	if ((tp->ecn_flags & TCP_ECN_OK) && (!th->ece || th->cwr))
		tp->ecn_flags &= ~TCP_ECN_OK;
}

static inline void TCP_ECN_rcv_syn(struct tcp_sock *tp, const struct tcphdr *th)
{
	if ((tp->ecn_flags & TCP_ECN_OK) && (!th->ece || !th->cwr))
		tp->ecn_flags &= ~TCP_ECN_OK;
}

static inline int TCP_ECN_rcv_ecn_echo(const struct tcp_sock *tp, const struct tcphdr *th)
{
	if (th->ece && !th->syn && (tp->ecn_flags & TCP_ECN_OK))
		return 1;
	return 0;
}


static void tcp_fixup_sndbuf(struct sock *sk)
{
	int sndmem = SKB_TRUESIZE(tcp_sk(sk)->rx_opt.mss_clamp + MAX_TCP_HEADER);

	sndmem *= TCP_INIT_CWND;
	if (sk->sk_sndbuf < sndmem)
		sk->sk_sndbuf = min(sndmem, sysctl_tcp_wmem[2]);
}


static int __tcp_grow_window(const struct sock *sk, const struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	
	int truesize = tcp_win_from_space(skb->truesize) >> 1;
	int window = tcp_win_from_space(sysctl_tcp_rmem[2]) >> 1;

	while (tp->rcv_ssthresh <= window) {
		if (truesize <= skb->len)
			return 2 * inet_csk(sk)->icsk_ack.rcv_mss;

		truesize >>= 1;
		window >>= 1;
	}
	return 0;
}

static void tcp_grow_window(struct sock *sk, const struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);

	
	if (tp->rcv_ssthresh < tp->window_clamp &&
	    (int)tp->rcv_ssthresh < tcp_space(sk) &&
	    !sk_under_memory_pressure(sk)) {
		int incr;

		if (tcp_win_from_space(skb->truesize) <= skb->len)
			incr = 2 * tp->advmss;
		else
			incr = __tcp_grow_window(sk, skb);

		if (incr) {
			incr = max_t(int, incr, 2 * skb->len);
			tp->rcv_ssthresh = min(tp->rcv_ssthresh + incr,
					       tp->window_clamp);
			inet_csk(sk)->icsk_ack.quick |= 1;
		}
	}
}


static void tcp_fixup_rcvbuf(struct sock *sk)
{
	u32 mss = tcp_sk(sk)->advmss;
	u32 icwnd = sysctl_tcp_default_init_rwnd;
	int rcvmem;

	if (mss > 1460)
		icwnd = max_t(u32, (1460 * icwnd) / mss, 2);

	rcvmem = SKB_TRUESIZE(mss + MAX_TCP_HEADER);
	while (tcp_win_from_space(rcvmem) < mss)
		rcvmem += 128;

	rcvmem *= icwnd;

	if (sk->sk_rcvbuf < rcvmem)
		sk->sk_rcvbuf = min(rcvmem, sysctl_tcp_rmem[2]);
}

static void tcp_init_buffer_space(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int maxwin;

	if (!(sk->sk_userlocks & SOCK_RCVBUF_LOCK))
		tcp_fixup_rcvbuf(sk);
	if (!(sk->sk_userlocks & SOCK_SNDBUF_LOCK))
		tcp_fixup_sndbuf(sk);

	tp->rcvq_space.space = tp->rcv_wnd;

	maxwin = tcp_full_space(sk);

	if (tp->window_clamp >= maxwin) {
		tp->window_clamp = maxwin;

		if (sysctl_tcp_app_win && maxwin > 4 * tp->advmss)
			tp->window_clamp = max(maxwin -
					       (maxwin >> sysctl_tcp_app_win),
					       4 * tp->advmss);
	}

	
	if (sysctl_tcp_app_win &&
	    tp->window_clamp > 2 * tp->advmss &&
	    tp->window_clamp + tp->advmss > maxwin)
		tp->window_clamp = max(2 * tp->advmss, maxwin - tp->advmss);

	tp->rcv_ssthresh = min(tp->rcv_ssthresh, tp->window_clamp);
	tp->snd_cwnd_stamp = tcp_time_stamp;
}

static void tcp_clamp_window(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);

	icsk->icsk_ack.quick = 0;

	if (sk->sk_rcvbuf < sysctl_tcp_rmem[2] &&
	    !(sk->sk_userlocks & SOCK_RCVBUF_LOCK) &&
	    !sk_under_memory_pressure(sk) &&
	    sk_memory_allocated(sk) < sk_prot_mem_limits(sk, 0)) {
		sk->sk_rcvbuf = min(atomic_read(&sk->sk_rmem_alloc),
				    sysctl_tcp_rmem[2]);
	}
	if (atomic_read(&sk->sk_rmem_alloc) > sk->sk_rcvbuf)
		tp->rcv_ssthresh = min(tp->window_clamp, 2U * tp->advmss);
}

void tcp_initialize_rcv_mss(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	unsigned int hint = min_t(unsigned int, tp->advmss, tp->mss_cache);

	hint = min(hint, tp->rcv_wnd / 2);
	hint = min(hint, TCP_MSS_DEFAULT);
	hint = max(hint, TCP_MIN_MSS);

	inet_csk(sk)->icsk_ack.rcv_mss = hint;
}
EXPORT_SYMBOL(tcp_initialize_rcv_mss);

static void tcp_rcv_rtt_update(struct tcp_sock *tp, u32 sample, int win_dep)
{
	u32 new_sample = tp->rcv_rtt_est.rtt;
	long m = sample;

	if (m == 0)
		m = 1;

	if (new_sample != 0) {
		if (!win_dep) {
			m -= (new_sample >> 3);
			new_sample += m;
		} else {
			m <<= 3;
			if (m < new_sample)
				new_sample = m;
		}
	} else {
		
		new_sample = m << 3;
	}

	if (tp->rcv_rtt_est.rtt != new_sample)
		tp->rcv_rtt_est.rtt = new_sample;
}

static inline void tcp_rcv_rtt_measure(struct tcp_sock *tp)
{
	if (tp->rcv_rtt_est.time == 0)
		goto new_measure;
	if (before(tp->rcv_nxt, tp->rcv_rtt_est.seq))
		return;
	tcp_rcv_rtt_update(tp, tcp_time_stamp - tp->rcv_rtt_est.time, 1);

new_measure:
	tp->rcv_rtt_est.seq = tp->rcv_nxt + tp->rcv_wnd;
	tp->rcv_rtt_est.time = tcp_time_stamp;
}

static inline void tcp_rcv_rtt_measure_ts(struct sock *sk,
					  const struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	if (tp->rx_opt.rcv_tsecr &&
	    (TCP_SKB_CB(skb)->end_seq -
	     TCP_SKB_CB(skb)->seq >= inet_csk(sk)->icsk_ack.rcv_mss))
		tcp_rcv_rtt_update(tp, tcp_time_stamp - tp->rx_opt.rcv_tsecr, 0);
}

void tcp_rcv_space_adjust(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int time;
	int space;

	if (tp->rcvq_space.time == 0)
		goto new_measure;

	time = tcp_time_stamp - tp->rcvq_space.time;
	if (time < (tp->rcv_rtt_est.rtt >> 3) || tp->rcv_rtt_est.rtt == 0)
		return;

	space = 2 * (tp->copied_seq - tp->rcvq_space.seq);

	space = max(tp->rcvq_space.space, space);

	if (tp->rcvq_space.space != space) {
		int rcvmem;

		tp->rcvq_space.space = space;

		if (sysctl_tcp_moderate_rcvbuf &&
		    !(sk->sk_userlocks & SOCK_RCVBUF_LOCK)) {
			int new_clamp = space;

			space /= tp->advmss;
			if (!space)
				space = 1;
			rcvmem = SKB_TRUESIZE(tp->advmss + MAX_TCP_HEADER);
			while (tcp_win_from_space(rcvmem) < tp->advmss)
				rcvmem += 128;
			space *= rcvmem;
			space = min(space, sysctl_tcp_rmem[2]);
			if (space > sk->sk_rcvbuf) {
				sk->sk_rcvbuf = space;

				
				tp->window_clamp = new_clamp;
			}
		}
	}

new_measure:
	tp->rcvq_space.seq = tp->copied_seq;
	tp->rcvq_space.time = tcp_time_stamp;
}

static void tcp_event_data_recv(struct sock *sk, struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);
	u32 now;

	inet_csk_schedule_ack(sk);

	tcp_measure_rcv_mss(sk, skb);

	tcp_rcv_rtt_measure(tp);

	now = tcp_time_stamp;

	if (!icsk->icsk_ack.ato) {
		tcp_incr_quickack(sk);
		icsk->icsk_ack.ato = TCP_ATO_MIN;
	} else {
		int m = now - icsk->icsk_ack.lrcvtime;

		if (m <= TCP_ATO_MIN / 2) {
			
			icsk->icsk_ack.ato = (icsk->icsk_ack.ato >> 1) + TCP_ATO_MIN / 2;
		} else if (m < icsk->icsk_ack.ato) {
			icsk->icsk_ack.ato = (icsk->icsk_ack.ato >> 1) + m;
			if (icsk->icsk_ack.ato > icsk->icsk_rto)
				icsk->icsk_ack.ato = icsk->icsk_rto;
		} else if (m > icsk->icsk_rto) {
			tcp_incr_quickack(sk);
			sk_mem_reclaim(sk);
		}
	}
	icsk->icsk_ack.lrcvtime = now;

	TCP_ECN_check_ce(tp, skb);

	if (skb->len >= 128)
		tcp_grow_window(sk, skb);
}

static void tcp_rtt_estimator(struct sock *sk, const __u32 mrtt)
{
	struct tcp_sock *tp = tcp_sk(sk);
	long m = mrtt; 

	if (m == 0)
		m = 1;
	if (tp->srtt != 0) {
		m -= (tp->srtt >> 3);	
		tp->srtt += m;		
		if (m < 0) {
			m = -m;		
			m -= (tp->mdev >> 2);   
			if (m > 0)
				m >>= 3;
		} else {
			m -= (tp->mdev >> 2);   
		}
		tp->mdev += m;	    	
		if (tp->mdev > tp->mdev_max) {
			tp->mdev_max = tp->mdev;
			if (tp->mdev_max > tp->rttvar)
				tp->rttvar = tp->mdev_max;
		}
		if (after(tp->snd_una, tp->rtt_seq)) {
			if (tp->mdev_max < tp->rttvar)
				tp->rttvar -= (tp->rttvar - tp->mdev_max) >> 2;
			tp->rtt_seq = tp->snd_nxt;
			tp->mdev_max = tcp_rto_min(sk);
		}
	} else {
		
		tp->srtt = m << 3;	
		tp->mdev = m << 1;	
		tp->mdev_max = tp->rttvar = max(tp->mdev, tcp_rto_min(sk));
		tp->rtt_seq = tp->snd_nxt;
	}
}

static inline void tcp_set_rto(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	inet_csk(sk)->icsk_rto = __tcp_set_rto(tp);


	tcp_bound_rto(sk);
}

void tcp_update_metrics(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct dst_entry *dst = __sk_dst_get(sk);

	if (sysctl_tcp_nometrics_save)
		return;

	dst_confirm(dst);

	if (dst && (dst->flags & DST_HOST)) {
		const struct inet_connection_sock *icsk = inet_csk(sk);
		int m;
		unsigned long rtt;

		if (icsk->icsk_backoff || !tp->srtt) {
			if (!(dst_metric_locked(dst, RTAX_RTT)))
				dst_metric_set(dst, RTAX_RTT, 0);
			return;
		}

		rtt = dst_metric_rtt(dst, RTAX_RTT);
		m = rtt - tp->srtt;

		if (!(dst_metric_locked(dst, RTAX_RTT))) {
			if (m <= 0)
				set_dst_metric_rtt(dst, RTAX_RTT, tp->srtt);
			else
				set_dst_metric_rtt(dst, RTAX_RTT, rtt - (m >> 3));
		}

		if (!(dst_metric_locked(dst, RTAX_RTTVAR))) {
			unsigned long var;
			if (m < 0)
				m = -m;

			
			m >>= 1;
			if (m < tp->mdev)
				m = tp->mdev;

			var = dst_metric_rtt(dst, RTAX_RTTVAR);
			if (m >= var)
				var = m;
			else
				var -= (var - m) >> 2;

			set_dst_metric_rtt(dst, RTAX_RTTVAR, var);
		}

		if (tcp_in_initial_slowstart(tp)) {
			
			if (dst_metric(dst, RTAX_SSTHRESH) &&
			    !dst_metric_locked(dst, RTAX_SSTHRESH) &&
			    (tp->snd_cwnd >> 1) > dst_metric(dst, RTAX_SSTHRESH))
				dst_metric_set(dst, RTAX_SSTHRESH, tp->snd_cwnd >> 1);
			if (!dst_metric_locked(dst, RTAX_CWND) &&
			    tp->snd_cwnd > dst_metric(dst, RTAX_CWND))
				dst_metric_set(dst, RTAX_CWND, tp->snd_cwnd);
		} else if (tp->snd_cwnd > tp->snd_ssthresh &&
			   icsk->icsk_ca_state == TCP_CA_Open) {
			
			if (!dst_metric_locked(dst, RTAX_SSTHRESH))
				dst_metric_set(dst, RTAX_SSTHRESH,
					       max(tp->snd_cwnd >> 1, tp->snd_ssthresh));
			if (!dst_metric_locked(dst, RTAX_CWND))
				dst_metric_set(dst, RTAX_CWND,
					       (dst_metric(dst, RTAX_CWND) +
						tp->snd_cwnd) >> 1);
		} else {
			if (!dst_metric_locked(dst, RTAX_CWND))
				dst_metric_set(dst, RTAX_CWND,
					       (dst_metric(dst, RTAX_CWND) +
						tp->snd_ssthresh) >> 1);
			if (dst_metric(dst, RTAX_SSTHRESH) &&
			    !dst_metric_locked(dst, RTAX_SSTHRESH) &&
			    tp->snd_ssthresh > dst_metric(dst, RTAX_SSTHRESH))
				dst_metric_set(dst, RTAX_SSTHRESH, tp->snd_ssthresh);
		}

		if (!dst_metric_locked(dst, RTAX_REORDERING)) {
			if (dst_metric(dst, RTAX_REORDERING) < tp->reordering &&
			    tp->reordering != sysctl_tcp_reordering)
				dst_metric_set(dst, RTAX_REORDERING, tp->reordering);
		}
	}
}

__u32 tcp_init_cwnd(const struct tcp_sock *tp, const struct dst_entry *dst)
{
	__u32 cwnd = (dst ? dst_metric(dst, RTAX_INITCWND) : 0);

	if (!cwnd)
		cwnd = TCP_INIT_CWND;
	return min_t(__u32, cwnd, tp->snd_cwnd_clamp);
}

void tcp_enter_cwr(struct sock *sk, const int set_ssthresh)
{
	struct tcp_sock *tp = tcp_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);

	tp->prior_ssthresh = 0;
	tp->bytes_acked = 0;
	if (icsk->icsk_ca_state < TCP_CA_CWR) {
		tp->undo_marker = 0;
		if (set_ssthresh)
			tp->snd_ssthresh = icsk->icsk_ca_ops->ssthresh(sk);
		tp->snd_cwnd = min(tp->snd_cwnd,
				   tcp_packets_in_flight(tp) + 1U);
		tp->snd_cwnd_cnt = 0;
		tp->high_seq = tp->snd_nxt;
		tp->snd_cwnd_stamp = tcp_time_stamp;
		TCP_ECN_queue_cwr(tp);

		tcp_set_ca_state(sk, TCP_CA_CWR);
	}
}

static void tcp_disable_fack(struct tcp_sock *tp)
{
	
	if (tcp_is_fack(tp))
		tp->lost_skb_hint = NULL;
	tp->rx_opt.sack_ok &= ~TCP_FACK_ENABLED;
}

static void tcp_dsack_seen(struct tcp_sock *tp)
{
	tp->rx_opt.sack_ok |= TCP_DSACK_SEEN;
}


static void tcp_init_metrics(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct dst_entry *dst = __sk_dst_get(sk);

	if (dst == NULL)
		goto reset;

	dst_confirm(dst);

	if (dst_metric_locked(dst, RTAX_CWND))
		tp->snd_cwnd_clamp = dst_metric(dst, RTAX_CWND);
	if (dst_metric(dst, RTAX_SSTHRESH)) {
		tp->snd_ssthresh = dst_metric(dst, RTAX_SSTHRESH);
		if (tp->snd_ssthresh > tp->snd_cwnd_clamp)
			tp->snd_ssthresh = tp->snd_cwnd_clamp;
	} else {
		tp->snd_ssthresh = TCP_INFINITE_SSTHRESH;
	}
	if (dst_metric(dst, RTAX_REORDERING) &&
	    tp->reordering != dst_metric(dst, RTAX_REORDERING)) {
		tcp_disable_fack(tp);
		tp->reordering = dst_metric(dst, RTAX_REORDERING);
	}

	if (dst_metric(dst, RTAX_RTT) == 0 || tp->srtt == 0)
		goto reset;

	if (dst_metric_rtt(dst, RTAX_RTT) > tp->srtt) {
		tp->srtt = dst_metric_rtt(dst, RTAX_RTT);
		tp->rtt_seq = tp->snd_nxt;
	}
	if (dst_metric_rtt(dst, RTAX_RTTVAR) > tp->mdev) {
		tp->mdev = dst_metric_rtt(dst, RTAX_RTTVAR);
		tp->mdev_max = tp->rttvar = max(tp->mdev, tcp_rto_min(sk));
	}
	tcp_set_rto(sk);
reset:
	if (tp->srtt == 0) {
		tp->mdev = tp->mdev_max = tp->rttvar = TCP_TIMEOUT_FALLBACK;
		inet_csk(sk)->icsk_rto = TCP_TIMEOUT_FALLBACK;
	}
	if (tp->total_retrans > 1)
		tp->snd_cwnd = 1;
	else
		tp->snd_cwnd = tcp_init_cwnd(tp, dst);
	tp->snd_cwnd_stamp = tcp_time_stamp;
}

static void tcp_update_reordering(struct sock *sk, const int metric,
				  const int ts)
{
	struct tcp_sock *tp = tcp_sk(sk);
	if (metric > tp->reordering) {
		int mib_idx;

		tp->reordering = min(TCP_MAX_REORDERING, metric);

		
		if (ts)
			mib_idx = LINUX_MIB_TCPTSREORDER;
		else if (tcp_is_reno(tp))
			mib_idx = LINUX_MIB_TCPRENOREORDER;
		else if (tcp_is_fack(tp))
			mib_idx = LINUX_MIB_TCPFACKREORDER;
		else
			mib_idx = LINUX_MIB_TCPSACKREORDER;

		NET_INC_STATS_BH(sock_net(sk), mib_idx);
#if FASTRETRANS_DEBUG > 1
		printk(KERN_DEBUG "Disorder%d %d %u f%u s%u rr%d\n",
		       tp->rx_opt.sack_ok, inet_csk(sk)->icsk_ca_state,
		       tp->reordering,
		       tp->fackets_out,
		       tp->sacked_out,
		       tp->undo_marker ? tp->undo_retrans : 0);
#endif
		tcp_disable_fack(tp);
	}
}

static void tcp_verify_retransmit_hint(struct tcp_sock *tp, struct sk_buff *skb)
{
	if ((tp->retransmit_skb_hint == NULL) ||
	    before(TCP_SKB_CB(skb)->seq,
		   TCP_SKB_CB(tp->retransmit_skb_hint)->seq))
		tp->retransmit_skb_hint = skb;

	if (!tp->lost_out ||
	    after(TCP_SKB_CB(skb)->end_seq, tp->retransmit_high))
		tp->retransmit_high = TCP_SKB_CB(skb)->end_seq;
}

static void tcp_skb_mark_lost(struct tcp_sock *tp, struct sk_buff *skb)
{
	if (!(TCP_SKB_CB(skb)->sacked & (TCPCB_LOST|TCPCB_SACKED_ACKED))) {
		tcp_verify_retransmit_hint(tp, skb);

		tp->lost_out += tcp_skb_pcount(skb);
		TCP_SKB_CB(skb)->sacked |= TCPCB_LOST;
	}
}

static void tcp_skb_mark_lost_uncond_verify(struct tcp_sock *tp,
					    struct sk_buff *skb)
{
	tcp_verify_retransmit_hint(tp, skb);

	if (!(TCP_SKB_CB(skb)->sacked & (TCPCB_LOST|TCPCB_SACKED_ACKED))) {
		tp->lost_out += tcp_skb_pcount(skb);
		TCP_SKB_CB(skb)->sacked |= TCPCB_LOST;
	}
}

static int tcp_is_sackblock_valid(struct tcp_sock *tp, int is_dsack,
				  u32 start_seq, u32 end_seq)
{
	
	if (after(end_seq, tp->snd_nxt) || !before(start_seq, end_seq))
		return 0;

	
	if (!before(start_seq, tp->snd_nxt))
		return 0;

	if (after(start_seq, tp->snd_una))
		return 1;

	if (!is_dsack || !tp->undo_marker)
		return 0;

	
	if (after(end_seq, tp->snd_una))
		return 0;

	if (!before(start_seq, tp->undo_marker))
		return 1;

	
	if (!after(end_seq, tp->undo_marker))
		return 0;

	return !before(start_seq, end_seq - tp->max_window);
}

static void tcp_mark_lost_retrans(struct sock *sk)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;
	int cnt = 0;
	u32 new_low_seq = tp->snd_nxt;
	u32 received_upto = tcp_highest_sack_seq(tp);

	if (!tcp_is_fack(tp) || !tp->retrans_out ||
	    !after(received_upto, tp->lost_retrans_low) ||
	    icsk->icsk_ca_state != TCP_CA_Recovery)
		return;

	tcp_for_write_queue(skb, sk) {
		u32 ack_seq = TCP_SKB_CB(skb)->ack_seq;

		if (skb == tcp_send_head(sk))
			break;
		if (cnt == tp->retrans_out)
			break;
		if (!after(TCP_SKB_CB(skb)->end_seq, tp->snd_una))
			continue;

		if (!(TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_RETRANS))
			continue;

		if (after(received_upto, ack_seq)) {
			TCP_SKB_CB(skb)->sacked &= ~TCPCB_SACKED_RETRANS;
			tp->retrans_out -= tcp_skb_pcount(skb);

			tcp_skb_mark_lost_uncond_verify(tp, skb);
			NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPLOSTRETRANSMIT);
		} else {
			if (before(ack_seq, new_low_seq))
				new_low_seq = ack_seq;
			cnt += tcp_skb_pcount(skb);
		}
	}

	if (tp->retrans_out)
		tp->lost_retrans_low = new_low_seq;
}

static int tcp_check_dsack(struct sock *sk, const struct sk_buff *ack_skb,
			   struct tcp_sack_block_wire *sp, int num_sacks,
			   u32 prior_snd_una)
{
	struct tcp_sock *tp = tcp_sk(sk);
	u32 start_seq_0 = get_unaligned_be32(&sp[0].start_seq);
	u32 end_seq_0 = get_unaligned_be32(&sp[0].end_seq);
	int dup_sack = 0;

	if (before(start_seq_0, TCP_SKB_CB(ack_skb)->ack_seq)) {
		dup_sack = 1;
		tcp_dsack_seen(tp);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPDSACKRECV);
	} else if (num_sacks > 1) {
		u32 end_seq_1 = get_unaligned_be32(&sp[1].end_seq);
		u32 start_seq_1 = get_unaligned_be32(&sp[1].start_seq);

		if (!after(end_seq_0, end_seq_1) &&
		    !before(start_seq_0, start_seq_1)) {
			dup_sack = 1;
			tcp_dsack_seen(tp);
			NET_INC_STATS_BH(sock_net(sk),
					LINUX_MIB_TCPDSACKOFORECV);
		}
	}

	
	if (dup_sack && tp->undo_marker && tp->undo_retrans &&
	    !after(end_seq_0, prior_snd_una) &&
	    after(end_seq_0, tp->undo_marker))
		tp->undo_retrans--;

	return dup_sack;
}

struct tcp_sacktag_state {
	int reord;
	int fack_count;
	int flag;
};

static int tcp_match_skb_to_sack(struct sock *sk, struct sk_buff *skb,
				 u32 start_seq, u32 end_seq)
{
	int in_sack, err;
	unsigned int pkt_len;
	unsigned int mss;

	in_sack = !after(start_seq, TCP_SKB_CB(skb)->seq) &&
		  !before(end_seq, TCP_SKB_CB(skb)->end_seq);

	if (tcp_skb_pcount(skb) > 1 && !in_sack &&
	    after(TCP_SKB_CB(skb)->end_seq, start_seq)) {
		mss = tcp_skb_mss(skb);
		in_sack = !after(start_seq, TCP_SKB_CB(skb)->seq);

		if (!in_sack) {
			pkt_len = start_seq - TCP_SKB_CB(skb)->seq;
			if (pkt_len < mss)
				pkt_len = mss;
		} else {
			pkt_len = end_seq - TCP_SKB_CB(skb)->seq;
			if (pkt_len < mss)
				return -EINVAL;
		}

		if (pkt_len > mss) {
			unsigned int new_len = (pkt_len / mss) * mss;
			if (!in_sack && new_len < pkt_len) {
				new_len += mss;
				if (new_len > skb->len)
					return 0;
			}
			pkt_len = new_len;
		}
		err = tcp_fragment(sk, skb, pkt_len, mss);
		if (err < 0)
			return err;
	}

	return in_sack;
}

static u8 tcp_sacktag_one(struct sock *sk,
			  struct tcp_sacktag_state *state, u8 sacked,
			  u32 start_seq, u32 end_seq,
			  int dup_sack, int pcount)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int fack_count = state->fack_count;

	
	if (dup_sack && (sacked & TCPCB_RETRANS)) {
		if (tp->undo_marker && tp->undo_retrans &&
		    after(end_seq, tp->undo_marker))
			tp->undo_retrans--;
		if (sacked & TCPCB_SACKED_ACKED)
			state->reord = min(fack_count, state->reord);
	}

	
	if (!after(end_seq, tp->snd_una))
		return sacked;

	if (!(sacked & TCPCB_SACKED_ACKED)) {
		if (sacked & TCPCB_SACKED_RETRANS) {
			if (sacked & TCPCB_LOST) {
				sacked &= ~(TCPCB_LOST|TCPCB_SACKED_RETRANS);
				tp->lost_out -= pcount;
				tp->retrans_out -= pcount;
			}
		} else {
			if (!(sacked & TCPCB_RETRANS)) {
				if (before(start_seq,
					   tcp_highest_sack_seq(tp)))
					state->reord = min(fack_count,
							   state->reord);

				
				if (!after(end_seq, tp->frto_highmark))
					state->flag |= FLAG_ONLY_ORIG_SACKED;
			}

			if (sacked & TCPCB_LOST) {
				sacked &= ~TCPCB_LOST;
				tp->lost_out -= pcount;
			}
		}

		sacked |= TCPCB_SACKED_ACKED;
		state->flag |= FLAG_DATA_SACKED;
		tp->sacked_out += pcount;

		fack_count += pcount;

		
		if (!tcp_is_fack(tp) && (tp->lost_skb_hint != NULL) &&
		    before(start_seq, TCP_SKB_CB(tp->lost_skb_hint)->seq))
			tp->lost_cnt_hint += pcount;

		if (fack_count > tp->fackets_out)
			tp->fackets_out = fack_count;
	}

	if (dup_sack && (sacked & TCPCB_SACKED_RETRANS)) {
		sacked &= ~TCPCB_SACKED_RETRANS;
		tp->retrans_out -= pcount;
	}

	return sacked;
}

static int tcp_shifted_skb(struct sock *sk, struct sk_buff *skb,
			   struct tcp_sacktag_state *state,
			   unsigned int pcount, int shifted, int mss,
			   int dup_sack)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *prev = tcp_write_queue_prev(sk, skb);
	u32 start_seq = TCP_SKB_CB(skb)->seq;	
	u32 end_seq = start_seq + shifted;	

	BUG_ON(!pcount);

	tcp_sacktag_one(sk, state, TCP_SKB_CB(skb)->sacked,
			start_seq, end_seq, dup_sack, pcount);

	if (skb == tp->lost_skb_hint)
		tp->lost_cnt_hint += pcount;

	TCP_SKB_CB(prev)->end_seq += shifted;
	TCP_SKB_CB(skb)->seq += shifted;

	skb_shinfo(prev)->gso_segs += pcount;
	BUG_ON(skb_shinfo(skb)->gso_segs < pcount);
	skb_shinfo(skb)->gso_segs -= pcount;

	if (!skb_shinfo(prev)->gso_size) {
		skb_shinfo(prev)->gso_size = mss;
		skb_shinfo(prev)->gso_type = sk->sk_gso_type;
	}

	
	if (skb_shinfo(skb)->gso_segs <= 1) {
		skb_shinfo(skb)->gso_size = 0;
		skb_shinfo(skb)->gso_type = 0;
	}

	
	TCP_SKB_CB(prev)->sacked |= (TCP_SKB_CB(skb)->sacked & TCPCB_EVER_RETRANS);

	if (skb->len > 0) {
		BUG_ON(!tcp_skb_pcount(skb));
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_SACKSHIFTED);
		return 0;
	}

	

	if (skb == tp->retransmit_skb_hint)
		tp->retransmit_skb_hint = prev;
	if (skb == tp->scoreboard_skb_hint)
		tp->scoreboard_skb_hint = prev;
	if (skb == tp->lost_skb_hint) {
		tp->lost_skb_hint = prev;
		tp->lost_cnt_hint -= tcp_skb_pcount(prev);
	}

	TCP_SKB_CB(skb)->tcp_flags |= TCP_SKB_CB(prev)->tcp_flags;
	if (skb == tcp_highest_sack(sk))
		tcp_advance_highest_sack(sk, skb);

	tcp_unlink_write_queue(skb, sk);
	sk_wmem_free_skb(sk, skb);

	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_SACKMERGED);

	return 1;
}

static int tcp_skb_seglen(const struct sk_buff *skb)
{
	return tcp_skb_pcount(skb) == 1 ? skb->len : tcp_skb_mss(skb);
}

static int skb_can_shift(const struct sk_buff *skb)
{
	return !skb_headlen(skb) && skb_is_nonlinear(skb);
}

static struct sk_buff *tcp_shift_skb_data(struct sock *sk, struct sk_buff *skb,
					  struct tcp_sacktag_state *state,
					  u32 start_seq, u32 end_seq,
					  int dup_sack)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *prev;
	int mss;
	int pcount = 0;
	int len;
	int in_sack;

	if (!sk_can_gso(sk))
		goto fallback;

	
	if (!dup_sack &&
	    (TCP_SKB_CB(skb)->sacked & (TCPCB_LOST|TCPCB_SACKED_RETRANS)) == TCPCB_SACKED_RETRANS)
		goto fallback;
	if (!skb_can_shift(skb))
		goto fallback;
	
	if (!after(TCP_SKB_CB(skb)->end_seq, tp->snd_una))
		goto fallback;

	
	if (unlikely(skb == tcp_write_queue_head(sk)))
		goto fallback;
	prev = tcp_write_queue_prev(sk, skb);

	if ((TCP_SKB_CB(prev)->sacked & TCPCB_TAGBITS) != TCPCB_SACKED_ACKED)
		goto fallback;

	in_sack = !after(start_seq, TCP_SKB_CB(skb)->seq) &&
		  !before(end_seq, TCP_SKB_CB(skb)->end_seq);

	if (in_sack) {
		len = skb->len;
		pcount = tcp_skb_pcount(skb);
		mss = tcp_skb_seglen(skb);

		if (mss != tcp_skb_seglen(prev))
			goto fallback;
	} else {
		if (!after(TCP_SKB_CB(skb)->end_seq, start_seq))
			goto noop;
		if (tcp_skb_pcount(skb) <= 1)
			goto noop;

		in_sack = !after(start_seq, TCP_SKB_CB(skb)->seq);
		if (!in_sack) {
			goto fallback;
		}

		len = end_seq - TCP_SKB_CB(skb)->seq;
		BUG_ON(len < 0);
		BUG_ON(len > skb->len);

		mss = tcp_skb_mss(skb);

		if (mss != tcp_skb_seglen(prev))
			goto fallback;

		if (len == mss) {
			pcount = 1;
		} else if (len < mss) {
			goto noop;
		} else {
			pcount = len / mss;
			len = pcount * mss;
		}
	}

	
	if (!after(TCP_SKB_CB(skb)->seq + len, tp->snd_una))
		goto fallback;

	if (!skb_shift(prev, skb, len))
		goto fallback;
	if (!tcp_shifted_skb(sk, skb, state, pcount, len, mss, dup_sack))
		goto out;

	if (prev == tcp_write_queue_tail(sk))
		goto out;
	skb = tcp_write_queue_next(sk, prev);

	if (!skb_can_shift(skb) ||
	    (skb == tcp_send_head(sk)) ||
	    ((TCP_SKB_CB(skb)->sacked & TCPCB_TAGBITS) != TCPCB_SACKED_ACKED) ||
	    (mss != tcp_skb_seglen(skb)))
		goto out;

	len = skb->len;
	if (skb_shift(prev, skb, len)) {
		pcount += tcp_skb_pcount(skb);
		tcp_shifted_skb(sk, skb, state, tcp_skb_pcount(skb), len, mss, 0);
	}

out:
	state->fack_count += pcount;
	return prev;

noop:
	return skb;

fallback:
	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_SACKSHIFTFALLBACK);
	return NULL;
}

static struct sk_buff *tcp_sacktag_walk(struct sk_buff *skb, struct sock *sk,
					struct tcp_sack_block *next_dup,
					struct tcp_sacktag_state *state,
					u32 start_seq, u32 end_seq,
					int dup_sack_in)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *tmp;

	tcp_for_write_queue_from(skb, sk) {
		int in_sack = 0;
		int dup_sack = dup_sack_in;

		if (skb == tcp_send_head(sk))
			break;

		
		if (!before(TCP_SKB_CB(skb)->seq, end_seq))
			break;

		if ((next_dup != NULL) &&
		    before(TCP_SKB_CB(skb)->seq, next_dup->end_seq)) {
			in_sack = tcp_match_skb_to_sack(sk, skb,
							next_dup->start_seq,
							next_dup->end_seq);
			if (in_sack > 0)
				dup_sack = 1;
		}

		if (in_sack <= 0) {
			tmp = tcp_shift_skb_data(sk, skb, state,
						 start_seq, end_seq, dup_sack);
			if (tmp != NULL) {
				if (tmp != skb) {
					skb = tmp;
					continue;
				}

				in_sack = 0;
			} else {
				in_sack = tcp_match_skb_to_sack(sk, skb,
								start_seq,
								end_seq);
			}
		}

		if (unlikely(in_sack < 0))
			break;

		if (in_sack) {
			TCP_SKB_CB(skb)->sacked =
				tcp_sacktag_one(sk,
						state,
						TCP_SKB_CB(skb)->sacked,
						TCP_SKB_CB(skb)->seq,
						TCP_SKB_CB(skb)->end_seq,
						dup_sack,
						tcp_skb_pcount(skb));

			if (!before(TCP_SKB_CB(skb)->seq,
				    tcp_highest_sack_seq(tp)))
				tcp_advance_highest_sack(sk, skb);
		}

		state->fack_count += tcp_skb_pcount(skb);
	}
	return skb;
}

static struct sk_buff *tcp_sacktag_skip(struct sk_buff *skb, struct sock *sk,
					struct tcp_sacktag_state *state,
					u32 skip_to_seq)
{
	tcp_for_write_queue_from(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;

		if (after(TCP_SKB_CB(skb)->end_seq, skip_to_seq))
			break;

		state->fack_count += tcp_skb_pcount(skb);
	}
	return skb;
}

static struct sk_buff *tcp_maybe_skipping_dsack(struct sk_buff *skb,
						struct sock *sk,
						struct tcp_sack_block *next_dup,
						struct tcp_sacktag_state *state,
						u32 skip_to_seq)
{
	if (next_dup == NULL)
		return skb;

	if (before(next_dup->start_seq, skip_to_seq)) {
		skb = tcp_sacktag_skip(skb, sk, state, next_dup->start_seq);
		skb = tcp_sacktag_walk(skb, sk, NULL, state,
				       next_dup->start_seq, next_dup->end_seq,
				       1);
	}

	return skb;
}

static int tcp_sack_cache_ok(const struct tcp_sock *tp, const struct tcp_sack_block *cache)
{
	return cache < tp->recv_sack_cache + ARRAY_SIZE(tp->recv_sack_cache);
}

static int
tcp_sacktag_write_queue(struct sock *sk, const struct sk_buff *ack_skb,
			u32 prior_snd_una)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	const unsigned char *ptr = (skb_transport_header(ack_skb) +
				    TCP_SKB_CB(ack_skb)->sacked);
	struct tcp_sack_block_wire *sp_wire = (struct tcp_sack_block_wire *)(ptr+2);
	struct tcp_sack_block sp[TCP_NUM_SACKS];
	struct tcp_sack_block *cache;
	struct tcp_sacktag_state state;
	struct sk_buff *skb;
	int num_sacks = min(TCP_NUM_SACKS, (ptr[1] - TCPOLEN_SACK_BASE) >> 3);
	int used_sacks;
	int found_dup_sack = 0;
	int i, j;
	int first_sack_index;

	state.flag = 0;
	state.reord = tp->packets_out;

	if (!tp->sacked_out) {
		if (WARN_ON(tp->fackets_out))
			tp->fackets_out = 0;
		tcp_highest_sack_reset(sk);
	}

	found_dup_sack = tcp_check_dsack(sk, ack_skb, sp_wire,
					 num_sacks, prior_snd_una);
	if (found_dup_sack)
		state.flag |= FLAG_DSACKING_ACK;

	if (before(TCP_SKB_CB(ack_skb)->ack_seq, prior_snd_una - tp->max_window))
		return 0;

	if (!tp->packets_out)
		goto out;

	used_sacks = 0;
	first_sack_index = 0;
	for (i = 0; i < num_sacks; i++) {
		int dup_sack = !i && found_dup_sack;

		sp[used_sacks].start_seq = get_unaligned_be32(&sp_wire[i].start_seq);
		sp[used_sacks].end_seq = get_unaligned_be32(&sp_wire[i].end_seq);

		if (!tcp_is_sackblock_valid(tp, dup_sack,
					    sp[used_sacks].start_seq,
					    sp[used_sacks].end_seq)) {
			int mib_idx;

			if (dup_sack) {
				if (!tp->undo_marker)
					mib_idx = LINUX_MIB_TCPDSACKIGNOREDNOUNDO;
				else
					mib_idx = LINUX_MIB_TCPDSACKIGNOREDOLD;
			} else {
				
				if ((TCP_SKB_CB(ack_skb)->ack_seq != tp->snd_una) &&
				    !after(sp[used_sacks].end_seq, tp->snd_una))
					continue;
				mib_idx = LINUX_MIB_TCPSACKDISCARD;
			}

			NET_INC_STATS_BH(sock_net(sk), mib_idx);
			if (i == 0)
				first_sack_index = -1;
			continue;
		}

		
		if (!after(sp[used_sacks].end_seq, prior_snd_una))
			continue;

		used_sacks++;
	}

	
	for (i = used_sacks - 1; i > 0; i--) {
		for (j = 0; j < i; j++) {
			if (after(sp[j].start_seq, sp[j + 1].start_seq)) {
				swap(sp[j], sp[j + 1]);

				
				if (j == first_sack_index)
					first_sack_index = j + 1;
			}
		}
	}

	skb = tcp_write_queue_head(sk);
	state.fack_count = 0;
	i = 0;

	if (!tp->sacked_out) {
		
		cache = tp->recv_sack_cache + ARRAY_SIZE(tp->recv_sack_cache);
	} else {
		cache = tp->recv_sack_cache;
		
		while (tcp_sack_cache_ok(tp, cache) && !cache->start_seq &&
		       !cache->end_seq)
			cache++;
	}

	while (i < used_sacks) {
		u32 start_seq = sp[i].start_seq;
		u32 end_seq = sp[i].end_seq;
		int dup_sack = (found_dup_sack && (i == first_sack_index));
		struct tcp_sack_block *next_dup = NULL;

		if (found_dup_sack && ((i + 1) == first_sack_index))
			next_dup = &sp[i + 1];

		
		while (tcp_sack_cache_ok(tp, cache) &&
		       !before(start_seq, cache->end_seq))
			cache++;

		
		if (tcp_sack_cache_ok(tp, cache) && !dup_sack &&
		    after(end_seq, cache->start_seq)) {

			
			if (before(start_seq, cache->start_seq)) {
				skb = tcp_sacktag_skip(skb, sk, &state,
						       start_seq);
				skb = tcp_sacktag_walk(skb, sk, next_dup,
						       &state,
						       start_seq,
						       cache->start_seq,
						       dup_sack);
			}

			
			if (!after(end_seq, cache->end_seq))
				goto advance_sp;

			skb = tcp_maybe_skipping_dsack(skb, sk, next_dup,
						       &state,
						       cache->end_seq);

			
			if (tcp_highest_sack_seq(tp) == cache->end_seq) {
				
				skb = tcp_highest_sack(sk);
				if (skb == NULL)
					break;
				state.fack_count = tp->fackets_out;
				cache++;
				goto walk;
			}

			skb = tcp_sacktag_skip(skb, sk, &state, cache->end_seq);
			
			cache++;
			continue;
		}

		if (!before(start_seq, tcp_highest_sack_seq(tp))) {
			skb = tcp_highest_sack(sk);
			if (skb == NULL)
				break;
			state.fack_count = tp->fackets_out;
		}
		skb = tcp_sacktag_skip(skb, sk, &state, start_seq);

walk:
		skb = tcp_sacktag_walk(skb, sk, next_dup, &state,
				       start_seq, end_seq, dup_sack);

advance_sp:
		if (after(end_seq, tp->frto_highmark))
			state.flag &= ~FLAG_ONLY_ORIG_SACKED;

		i++;
	}

	
	for (i = 0; i < ARRAY_SIZE(tp->recv_sack_cache) - used_sacks; i++) {
		tp->recv_sack_cache[i].start_seq = 0;
		tp->recv_sack_cache[i].end_seq = 0;
	}
	for (j = 0; j < used_sacks; j++)
		tp->recv_sack_cache[i++] = sp[j];

	tcp_mark_lost_retrans(sk);

	tcp_verify_left_out(tp);

	if ((state.reord < tp->fackets_out) &&
	    ((icsk->icsk_ca_state != TCP_CA_Loss) || tp->undo_marker) &&
	    (!tp->frto_highmark || after(tp->snd_una, tp->frto_highmark)))
		tcp_update_reordering(sk, tp->fackets_out - state.reord, 0);

out:

#if FASTRETRANS_DEBUG > 0
	WARN_ON((int)tp->sacked_out < 0);
	WARN_ON((int)tp->lost_out < 0);
	WARN_ON((int)tp->retrans_out < 0);
	WARN_ON((int)tcp_packets_in_flight(tp) < 0);
#endif
	return state.flag;
}

static int tcp_limit_reno_sacked(struct tcp_sock *tp)
{
	u32 holes;

	holes = max(tp->lost_out, 1U);
	holes = min(holes, tp->packets_out);

	if ((tp->sacked_out + holes) > tp->packets_out) {
		tp->sacked_out = tp->packets_out - holes;
		return 1;
	}
	return 0;
}

static void tcp_check_reno_reordering(struct sock *sk, const int addend)
{
	struct tcp_sock *tp = tcp_sk(sk);
	if (tcp_limit_reno_sacked(tp))
		tcp_update_reordering(sk, tp->packets_out + addend, 0);
}


static void tcp_add_reno_sack(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	tp->sacked_out++;
	tcp_check_reno_reordering(sk, 0);
	tcp_verify_left_out(tp);
}


static void tcp_remove_reno_sacks(struct sock *sk, int acked)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (acked > 0) {
		
		if (acked - 1 >= tp->sacked_out)
			tp->sacked_out = 0;
		else
			tp->sacked_out -= acked - 1;
	}
	tcp_check_reno_reordering(sk, acked);
	tcp_verify_left_out(tp);
}

static inline void tcp_reset_reno_sack(struct tcp_sock *tp)
{
	tp->sacked_out = 0;
}

static int tcp_is_sackfrto(const struct tcp_sock *tp)
{
	return (sysctl_tcp_frto == 0x2) && !tcp_is_reno(tp);
}

int tcp_use_frto(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct sk_buff *skb;

	if (!sysctl_tcp_frto)
		return 0;

	
	if (icsk->icsk_mtup.probe_size)
		return 0;

	if (tcp_is_sackfrto(tp))
		return 1;

	
	if (tp->retrans_out > 1)
		return 0;

	skb = tcp_write_queue_head(sk);

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(skb) || (!skb))
		printk(KERN_ERR "[NET] skb is NULL in %s!\n", __func__);
#endif

	if (tcp_skb_is_last(sk, skb))
		return 1;
	skb = tcp_write_queue_next(sk, skb);	
	tcp_for_write_queue_from(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;
		if (TCP_SKB_CB(skb)->sacked & TCPCB_RETRANS)
			return 0;
		
		if (!(TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED))
			break;
	}
	return 1;
}

void tcp_enter_frto(struct sock *sk)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;

	if ((!tp->frto_counter && icsk->icsk_ca_state <= TCP_CA_Disorder) ||
	    tp->snd_una == tp->high_seq ||
	    ((icsk->icsk_ca_state == TCP_CA_Loss || tp->frto_counter) &&
	     !icsk->icsk_retransmits)) {
		tp->prior_ssthresh = tcp_current_ssthresh(sk);
		if (tp->frto_counter) {
			u32 stored_cwnd;
			stored_cwnd = tp->snd_cwnd;
			tp->snd_cwnd = 2;
			tp->snd_ssthresh = icsk->icsk_ca_ops->ssthresh(sk);
			tp->snd_cwnd = stored_cwnd;
		} else {
			tp->snd_ssthresh = icsk->icsk_ca_ops->ssthresh(sk);
		}
		tcp_ca_event(sk, CA_EVENT_FRTO);
	}

	tp->undo_marker = tp->snd_una;
	tp->undo_retrans = 0;

	skb = tcp_write_queue_head(sk);

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(skb) || (!skb))
		printk(KERN_ERR "[NET] skb is NULL in %s!\n", __func__);
#endif

	if (TCP_SKB_CB(skb)->sacked & TCPCB_RETRANS)
		tp->undo_marker = 0;
	if (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_RETRANS) {
		TCP_SKB_CB(skb)->sacked &= ~TCPCB_SACKED_RETRANS;
		tp->retrans_out -= tcp_skb_pcount(skb);
	}
	tcp_verify_left_out(tp);

	
	tp->snd_cwnd = min(tp->snd_cwnd, tcp_packets_in_flight(tp) + 1);

	if (tcp_is_sackfrto(tp) && (tp->frto_counter ||
	    ((1 << icsk->icsk_ca_state) & (TCPF_CA_Recovery|TCPF_CA_Loss))) &&
	    after(tp->high_seq, tp->snd_una)) {
		tp->frto_highmark = tp->high_seq;
	} else {
		tp->frto_highmark = tp->snd_nxt;
	}
	tcp_set_ca_state(sk, TCP_CA_Disorder);
	tp->high_seq = tp->snd_nxt;
	tp->frto_counter = 1;
}

static void tcp_enter_frto_loss(struct sock *sk, int allowed_segments, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;

	tp->lost_out = 0;
	tp->retrans_out = 0;
	if (tcp_is_reno(tp))
		tcp_reset_reno_sack(tp);

	tcp_for_write_queue(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;

		TCP_SKB_CB(skb)->sacked &= ~TCPCB_LOST;
		if ((tp->frto_counter == 1) && !(flag & FLAG_DATA_ACKED)) {
			
			if (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_RETRANS)
				tp->retrans_out += tcp_skb_pcount(skb);
			
			flag |= FLAG_DATA_ACKED;
		} else {
			if (TCP_SKB_CB(skb)->sacked & TCPCB_RETRANS)
				tp->undo_marker = 0;
			TCP_SKB_CB(skb)->sacked &= ~TCPCB_SACKED_RETRANS;
		}

		if (!(TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED)) {
			TCP_SKB_CB(skb)->sacked |= TCPCB_LOST;
			tp->lost_out += tcp_skb_pcount(skb);
			tp->retransmit_high = TCP_SKB_CB(skb)->end_seq;
		}
	}
	tcp_verify_left_out(tp);

	tp->snd_cwnd = tcp_packets_in_flight(tp) + allowed_segments;
	tp->snd_cwnd_cnt = 0;
	tp->snd_cwnd_stamp = tcp_time_stamp;
	tp->frto_counter = 0;
	tp->bytes_acked = 0;

	tp->reordering = min_t(unsigned int, tp->reordering,
			       sysctl_tcp_reordering);
	tcp_set_ca_state(sk, TCP_CA_Loss);
	tp->high_seq = tp->snd_nxt;
	TCP_ECN_queue_cwr(tp);

	tcp_clear_all_retrans_hints(tp);
}

static void tcp_clear_retrans_partial(struct tcp_sock *tp)
{
	tp->retrans_out = 0;
	tp->lost_out = 0;

	tp->undo_marker = 0;
	tp->undo_retrans = 0;
}

void tcp_clear_retrans(struct tcp_sock *tp)
{
	tcp_clear_retrans_partial(tp);

	tp->fackets_out = 0;
	tp->sacked_out = 0;
}

void tcp_enter_loss(struct sock *sk, int how)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;

	
	if (icsk->icsk_ca_state <= TCP_CA_Disorder || tp->snd_una == tp->high_seq ||
	    (icsk->icsk_ca_state == TCP_CA_Loss && !icsk->icsk_retransmits)) {
		tp->prior_ssthresh = tcp_current_ssthresh(sk);
		tp->snd_ssthresh = icsk->icsk_ca_ops->ssthresh(sk);
		tcp_ca_event(sk, CA_EVENT_LOSS);
	}
	tp->snd_cwnd	   = 1;
	tp->snd_cwnd_cnt   = 0;
	tp->snd_cwnd_stamp = tcp_time_stamp;

	tp->bytes_acked = 0;
	tcp_clear_retrans_partial(tp);

	if (tcp_is_reno(tp))
		tcp_reset_reno_sack(tp);

	if (!how) {
		tp->undo_marker = tp->snd_una;
	} else {
		tp->sacked_out = 0;
		tp->fackets_out = 0;
	}
	tcp_clear_all_retrans_hints(tp);

	tcp_for_write_queue(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;

		if (TCP_SKB_CB(skb)->sacked & TCPCB_RETRANS)
			tp->undo_marker = 0;
		TCP_SKB_CB(skb)->sacked &= (~TCPCB_TAGBITS)|TCPCB_SACKED_ACKED;
		if (!(TCP_SKB_CB(skb)->sacked&TCPCB_SACKED_ACKED) || how) {
			TCP_SKB_CB(skb)->sacked &= ~TCPCB_SACKED_ACKED;
			TCP_SKB_CB(skb)->sacked |= TCPCB_LOST;
			tp->lost_out += tcp_skb_pcount(skb);
			tp->retransmit_high = TCP_SKB_CB(skb)->end_seq;
		}
	}
	tcp_verify_left_out(tp);

	tp->reordering = min_t(unsigned int, tp->reordering,
			       sysctl_tcp_reordering);
	tcp_set_ca_state(sk, TCP_CA_Loss);
	tp->high_seq = tp->snd_nxt;
	TCP_ECN_queue_cwr(tp);
	
	tp->frto_counter = 0;
}

static int tcp_check_sack_reneging(struct sock *sk, int flag)
{
	if (flag & FLAG_SACK_RENEGING) {
		struct inet_connection_sock *icsk = inet_csk(sk);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPSACKRENEGING);

		tcp_enter_loss(sk, 1);
		icsk->icsk_retransmits++;
		tcp_retransmit_skb(sk, tcp_write_queue_head(sk));
		inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS,
					  icsk->icsk_rto, TCP_RTO_MAX);
		return 1;
	}
	return 0;
}

static inline int tcp_fackets_out(const struct tcp_sock *tp)
{
	return tcp_is_reno(tp) ? tp->sacked_out + 1 : tp->fackets_out;
}

static inline int tcp_dupack_heuristics(const struct tcp_sock *tp)
{
	return tcp_is_fack(tp) ? tp->fackets_out : tp->sacked_out + 1;
}

static inline int tcp_skb_timedout(const struct sock *sk,
				   const struct sk_buff *skb)
{
	return tcp_time_stamp - TCP_SKB_CB(skb)->when > inet_csk(sk)->icsk_rto;
}

static inline int tcp_head_timedout(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	return tp->packets_out &&
	       tcp_skb_timedout(sk, tcp_write_queue_head(sk));
}


static int tcp_time_to_recover(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	__u32 packets_out;

	
	if (tp->frto_counter)
		return 0;

	
	if (tp->lost_out)
		return 1;

	
	if (tcp_dupack_heuristics(tp) > tp->reordering)
		return 1;

	if (tcp_is_fack(tp) && tcp_head_timedout(sk))
		return 1;

	packets_out = tp->packets_out;
	if (packets_out <= tp->reordering &&
	    tp->sacked_out >= max_t(__u32, packets_out/2, sysctl_tcp_reordering) &&
	    !tcp_may_send_now(sk)) {
		return 1;
	}

	if ((tp->thin_dupack || sysctl_tcp_thin_dupack) &&
	    tcp_stream_is_thin(tp) && tcp_dupack_heuristics(tp) > 1 &&
	    tcp_is_sack(tp) && !tcp_send_head(sk))
		return 1;

	return 0;
}

static void tcp_timeout_skbs(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;

	if (!tcp_is_fack(tp) || !tcp_head_timedout(sk))
		return;

	skb = tp->scoreboard_skb_hint;
	if (tp->scoreboard_skb_hint == NULL)
		skb = tcp_write_queue_head(sk);

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(skb) || (!skb))
		printk(KERN_ERR "[NET] skb is NULL in %s!\n", __func__);
#endif

	tcp_for_write_queue_from(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;
		if (!tcp_skb_timedout(sk, skb))
			break;

		tcp_skb_mark_lost(tp, skb);
	}

	tp->scoreboard_skb_hint = skb;

	tcp_verify_left_out(tp);
}

static void tcp_mark_head_lost(struct sock *sk, int packets, int mark_head)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;
	int cnt, oldcnt;
	int err;
	unsigned int mss;
	
	const u32 loss_high = tcp_is_sack(tp) ?  tp->snd_nxt : tp->high_seq;

	WARN_ON(packets > tp->packets_out);
	if (tp->lost_skb_hint) {
		skb = tp->lost_skb_hint;
		cnt = tp->lost_cnt_hint;
		
		if (mark_head && skb != tcp_write_queue_head(sk))
			return;
	} else {
		skb = tcp_write_queue_head(sk);
		cnt = 0;
	}

#ifdef CONFIG_HTC_NETWORK_MODIFY
	if (IS_ERR(skb) || (!skb))
		printk(KERN_ERR "[NET] skb is NULL in %s!\n", __func__);
#endif

	tcp_for_write_queue_from(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;
		
		
		tp->lost_skb_hint = skb;
		tp->lost_cnt_hint = cnt;

		if (after(TCP_SKB_CB(skb)->end_seq, loss_high))
			break;

		oldcnt = cnt;
		if (tcp_is_fack(tp) || tcp_is_reno(tp) ||
		    (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED))
			cnt += tcp_skb_pcount(skb);

		if (cnt > packets) {
			if ((tcp_is_sack(tp) && !tcp_is_fack(tp)) ||
			    (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED) ||
			    (oldcnt >= packets))
				break;

			mss = skb_shinfo(skb)->gso_size;
			err = tcp_fragment(sk, skb, (packets - oldcnt) * mss, mss);
			if (err < 0)
				break;
			cnt = packets;
		}

		tcp_skb_mark_lost(tp, skb);

		if (mark_head)
			break;
	}
	tcp_verify_left_out(tp);
}


static void tcp_update_scoreboard(struct sock *sk, int fast_rexmit)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_is_reno(tp)) {
		tcp_mark_head_lost(sk, 1, 1);
	} else if (tcp_is_fack(tp)) {
		int lost = tp->fackets_out - tp->reordering;
		if (lost <= 0)
			lost = 1;
		tcp_mark_head_lost(sk, lost, 0);
	} else {
		int sacked_upto = tp->sacked_out - tp->reordering;
		if (sacked_upto >= 0)
			tcp_mark_head_lost(sk, sacked_upto, 0);
		else if (fast_rexmit)
			tcp_mark_head_lost(sk, 1, 1);
	}

	tcp_timeout_skbs(sk);
}

static inline void tcp_moderate_cwnd(struct tcp_sock *tp)
{
	tp->snd_cwnd = min(tp->snd_cwnd,
			   tcp_packets_in_flight(tp) + tcp_max_burst(tp));
	tp->snd_cwnd_stamp = tcp_time_stamp;
}

static inline u32 tcp_cwnd_min(const struct sock *sk)
{
	const struct tcp_congestion_ops *ca_ops = inet_csk(sk)->icsk_ca_ops;

	return ca_ops->min_cwnd ? ca_ops->min_cwnd(sk) : tcp_sk(sk)->snd_ssthresh;
}

static void tcp_cwnd_down(struct sock *sk, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int decr = tp->snd_cwnd_cnt + 1;

	if ((flag & (FLAG_ANY_PROGRESS | FLAG_DSACKING_ACK)) ||
	    (tcp_is_reno(tp) && !(flag & FLAG_NOT_DUP))) {
		tp->snd_cwnd_cnt = decr & 1;
		decr >>= 1;

		if (decr && tp->snd_cwnd > tcp_cwnd_min(sk))
			tp->snd_cwnd -= decr;

		tp->snd_cwnd = min(tp->snd_cwnd, tcp_packets_in_flight(tp) + 1);
		tp->snd_cwnd_stamp = tcp_time_stamp;
	}
}

static inline int tcp_packet_delayed(const struct tcp_sock *tp)
{
	return !tp->retrans_stamp ||
		(tp->rx_opt.saw_tstamp && tp->rx_opt.rcv_tsecr &&
		 before(tp->rx_opt.rcv_tsecr, tp->retrans_stamp));
}


#if FASTRETRANS_DEBUG > 1
static void DBGUNDO(struct sock *sk, const char *msg)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_sock *inet = inet_sk(sk);

	if (sk->sk_family == AF_INET) {
		printk(KERN_DEBUG "Undo %s %pI4/%u c%u l%u ss%u/%u p%u\n",
		       msg,
		       &inet->inet_daddr, ntohs(inet->inet_dport),
		       tp->snd_cwnd, tcp_left_out(tp),
		       tp->snd_ssthresh, tp->prior_ssthresh,
		       tp->packets_out);
	}
#if IS_ENABLED(CONFIG_IPV6)
	else if (sk->sk_family == AF_INET6) {
		struct ipv6_pinfo *np = inet6_sk(sk);
		printk(KERN_DEBUG "Undo %s %pI6/%u c%u l%u ss%u/%u p%u\n",
		       msg,
		       &np->daddr, ntohs(inet->inet_dport),
		       tp->snd_cwnd, tcp_left_out(tp),
		       tp->snd_ssthresh, tp->prior_ssthresh,
		       tp->packets_out);
	}
#endif
}
#else
#define DBGUNDO(x...) do { } while (0)
#endif

static void tcp_undo_cwr(struct sock *sk, const bool undo_ssthresh)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tp->prior_ssthresh) {
		const struct inet_connection_sock *icsk = inet_csk(sk);

		if (icsk->icsk_ca_ops->undo_cwnd)
			tp->snd_cwnd = icsk->icsk_ca_ops->undo_cwnd(sk);
		else
			tp->snd_cwnd = max(tp->snd_cwnd, tp->snd_ssthresh << 1);

		if (undo_ssthresh && tp->prior_ssthresh > tp->snd_ssthresh) {
			tp->snd_ssthresh = tp->prior_ssthresh;
			TCP_ECN_withdraw_cwr(tp);
		}
	} else {
		tp->snd_cwnd = max(tp->snd_cwnd, tp->snd_ssthresh);
	}
	tp->snd_cwnd_stamp = tcp_time_stamp;
}

static inline int tcp_may_undo(const struct tcp_sock *tp)
{
	return tp->undo_marker && (!tp->undo_retrans || tcp_packet_delayed(tp));
}

static int tcp_try_undo_recovery(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_may_undo(tp)) {
		int mib_idx;

		DBGUNDO(sk, inet_csk(sk)->icsk_ca_state == TCP_CA_Loss ? "loss" : "retrans");
		tcp_undo_cwr(sk, true);
		if (inet_csk(sk)->icsk_ca_state == TCP_CA_Loss)
			mib_idx = LINUX_MIB_TCPLOSSUNDO;
		else
			mib_idx = LINUX_MIB_TCPFULLUNDO;

		NET_INC_STATS_BH(sock_net(sk), mib_idx);
		tp->undo_marker = 0;
	}
	if (tp->snd_una == tp->high_seq && tcp_is_reno(tp)) {
		tcp_moderate_cwnd(tp);
		return 1;
	}
	tcp_set_ca_state(sk, TCP_CA_Open);
	return 0;
}

static void tcp_try_undo_dsack(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tp->undo_marker && !tp->undo_retrans) {
		DBGUNDO(sk, "D-SACK");
		tcp_undo_cwr(sk, true);
		tp->undo_marker = 0;
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPDSACKUNDO);
	}
}

static int tcp_any_retrans_done(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;

	if (tp->retrans_out)
		return 1;

	skb = tcp_write_queue_head(sk);
	if (unlikely(skb && TCP_SKB_CB(skb)->sacked & TCPCB_EVER_RETRANS))
		return 1;

	return 0;
}


static int tcp_try_undo_partial(struct sock *sk, int acked)
{
	struct tcp_sock *tp = tcp_sk(sk);
	
	int failed = tcp_is_reno(tp) || (tcp_fackets_out(tp) > tp->reordering);

	if (tcp_may_undo(tp)) {
		if (!tcp_any_retrans_done(sk))
			tp->retrans_stamp = 0;

		tcp_update_reordering(sk, tcp_fackets_out(tp) + acked, 1);

		DBGUNDO(sk, "Hoe");
		tcp_undo_cwr(sk, false);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPPARTIALUNDO);

		failed = 0;
	}
	return failed;
}

static int tcp_try_undo_loss(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_may_undo(tp)) {
		struct sk_buff *skb;
		tcp_for_write_queue(skb, sk) {
			if (skb == tcp_send_head(sk))
				break;
			TCP_SKB_CB(skb)->sacked &= ~TCPCB_LOST;
		}

		tcp_clear_all_retrans_hints(tp);

		DBGUNDO(sk, "partial loss");
		tp->lost_out = 0;
		tcp_undo_cwr(sk, true);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPLOSSUNDO);
		inet_csk(sk)->icsk_retransmits = 0;
		tp->undo_marker = 0;
		if (tcp_is_sack(tp))
			tcp_set_ca_state(sk, TCP_CA_Open);
		return 1;
	}
	return 0;
}

static inline void tcp_complete_cwr(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	
	if (tp->undo_marker) {
		if (inet_csk(sk)->icsk_ca_state == TCP_CA_CWR) {
			tp->snd_cwnd = min(tp->snd_cwnd, tp->snd_ssthresh);
			tp->snd_cwnd_stamp = tcp_time_stamp;
		} else if (tp->snd_ssthresh < TCP_INFINITE_SSTHRESH) {
			
			tp->snd_cwnd = tp->snd_ssthresh;
			tp->snd_cwnd_stamp = tcp_time_stamp;
		}
	}
	tcp_ca_event(sk, CA_EVENT_COMPLETE_CWR);
}

static void tcp_try_keep_open(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int state = TCP_CA_Open;

	if (tcp_left_out(tp) || tcp_any_retrans_done(sk))
		state = TCP_CA_Disorder;

	if (inet_csk(sk)->icsk_ca_state != state) {
		tcp_set_ca_state(sk, state);
		tp->high_seq = tp->snd_nxt;
	}
}

static void tcp_try_to_open(struct sock *sk, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);

	tcp_verify_left_out(tp);

	if (!tp->frto_counter && !tcp_any_retrans_done(sk))
		tp->retrans_stamp = 0;

	if (flag & FLAG_ECE)
		tcp_enter_cwr(sk, 1);

	if (inet_csk(sk)->icsk_ca_state != TCP_CA_CWR) {
		tcp_try_keep_open(sk);
		if (inet_csk(sk)->icsk_ca_state != TCP_CA_Open)
			tcp_moderate_cwnd(tp);
	} else {
		tcp_cwnd_down(sk, flag);
	}
}

static void tcp_mtup_probe_failed(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);

	icsk->icsk_mtup.search_high = icsk->icsk_mtup.probe_size - 1;
	icsk->icsk_mtup.probe_size = 0;
}

static void tcp_mtup_probe_success(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);

	
	tp->prior_ssthresh = tcp_current_ssthresh(sk);
	tp->snd_cwnd = tp->snd_cwnd *
		       tcp_mss_to_mtu(sk, tp->mss_cache) /
		       icsk->icsk_mtup.probe_size;
	tp->snd_cwnd_cnt = 0;
	tp->snd_cwnd_stamp = tcp_time_stamp;
	tp->snd_ssthresh = tcp_current_ssthresh(sk);

	icsk->icsk_mtup.search_low = icsk->icsk_mtup.probe_size;
	icsk->icsk_mtup.probe_size = 0;
	tcp_sync_mss(sk, icsk->icsk_pmtu_cookie);
}

void tcp_simple_retransmit(struct sock *sk)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb;
	unsigned int mss = tcp_current_mss(sk);
	u32 prior_lost = tp->lost_out;

	tcp_for_write_queue(skb, sk) {
		if (skb == tcp_send_head(sk))
			break;
		if (tcp_skb_seglen(skb) > mss &&
		    !(TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED)) {
			if (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_RETRANS) {
				TCP_SKB_CB(skb)->sacked &= ~TCPCB_SACKED_RETRANS;
				tp->retrans_out -= tcp_skb_pcount(skb);
			}
			tcp_skb_mark_lost_uncond_verify(tp, skb);
		}
	}

	tcp_clear_retrans_hints_partial(tp);

	if (prior_lost == tp->lost_out)
		return;

	if (tcp_is_reno(tp))
		tcp_limit_reno_sacked(tp);

	tcp_verify_left_out(tp);

	if (icsk->icsk_ca_state != TCP_CA_Loss) {
		tp->high_seq = tp->snd_nxt;
		tp->snd_ssthresh = tcp_current_ssthresh(sk);
		tp->prior_ssthresh = 0;
		tp->undo_marker = 0;
		tcp_set_ca_state(sk, TCP_CA_Loss);
	}
	tcp_xmit_retransmit_queue(sk);
}
EXPORT_SYMBOL(tcp_simple_retransmit);

static void tcp_update_cwnd_in_recovery(struct sock *sk, int newly_acked_sacked,
					int fast_rexmit, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int sndcnt = 0;
	int delta = tp->snd_ssthresh - tcp_packets_in_flight(tp);

	if (tcp_packets_in_flight(tp) > tp->snd_ssthresh) {
		u64 dividend = (u64)tp->snd_ssthresh * tp->prr_delivered +
			       tp->prior_cwnd - 1;
		sndcnt = div_u64(dividend, tp->prior_cwnd) - tp->prr_out;
	} else {
		sndcnt = min_t(int, delta,
			       max_t(int, tp->prr_delivered - tp->prr_out,
				     newly_acked_sacked) + 1);
	}

	sndcnt = max(sndcnt, (fast_rexmit ? 1 : 0));
	tp->snd_cwnd = tcp_packets_in_flight(tp) + sndcnt;
}

static void tcp_fastretrans_alert(struct sock *sk, int pkts_acked,
				  int newly_acked_sacked, bool is_dupack,
				  int flag)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	int do_lost = is_dupack || ((flag & FLAG_DATA_SACKED) &&
				    (tcp_fackets_out(tp) > tp->reordering));
	int fast_rexmit = 0, mib_idx;

	if (WARN_ON(!tp->packets_out && tp->sacked_out))
		tp->sacked_out = 0;
	if (WARN_ON(!tp->sacked_out && tp->fackets_out))
		tp->fackets_out = 0;

	if (flag & FLAG_ECE)
		tp->prior_ssthresh = 0;

	
	if (tcp_check_sack_reneging(sk, flag))
		return;

	
	tcp_verify_left_out(tp);

	if (icsk->icsk_ca_state == TCP_CA_Open) {
		WARN_ON(tp->retrans_out != 0);
		tp->retrans_stamp = 0;
	} else if (!before(tp->snd_una, tp->high_seq)) {
		switch (icsk->icsk_ca_state) {
		case TCP_CA_Loss:
			icsk->icsk_retransmits = 0;
			if (tcp_try_undo_recovery(sk))
				return;
			break;

		case TCP_CA_CWR:
			if (tp->snd_una != tp->high_seq) {
				tcp_complete_cwr(sk);
				tcp_set_ca_state(sk, TCP_CA_Open);
			}
			break;

		case TCP_CA_Recovery:
			if (tcp_is_reno(tp))
				tcp_reset_reno_sack(tp);
			if (tcp_try_undo_recovery(sk))
				return;
			tcp_complete_cwr(sk);
			break;
		}
	}

	
	switch (icsk->icsk_ca_state) {
	case TCP_CA_Recovery:
		if (!(flag & FLAG_SND_UNA_ADVANCED)) {
			if (tcp_is_reno(tp) && is_dupack)
				tcp_add_reno_sack(sk);
		} else
			do_lost = tcp_try_undo_partial(sk, pkts_acked);
		break;
	case TCP_CA_Loss:
		if (flag & FLAG_DATA_ACKED)
			icsk->icsk_retransmits = 0;
		if (tcp_is_reno(tp) && flag & FLAG_SND_UNA_ADVANCED)
			tcp_reset_reno_sack(tp);
		if (!tcp_try_undo_loss(sk)) {
			tcp_moderate_cwnd(tp);
			tcp_xmit_retransmit_queue(sk);
			return;
		}
		if (icsk->icsk_ca_state != TCP_CA_Open)
			return;
		
	default:
		if (tcp_is_reno(tp)) {
			if (flag & FLAG_SND_UNA_ADVANCED)
				tcp_reset_reno_sack(tp);
			if (is_dupack)
				tcp_add_reno_sack(sk);
		}

		if (icsk->icsk_ca_state <= TCP_CA_Disorder)
			tcp_try_undo_dsack(sk);

		if (!tcp_time_to_recover(sk)) {
			tcp_try_to_open(sk, flag);
			return;
		}

		
		if (icsk->icsk_ca_state < TCP_CA_CWR &&
		    icsk->icsk_mtup.probe_size &&
		    tp->snd_una == tp->mtu_probe.probe_seq_start) {
			tcp_mtup_probe_failed(sk);
			
			tp->snd_cwnd++;
			tcp_simple_retransmit(sk);
			return;
		}

		

		if (tcp_is_reno(tp))
			mib_idx = LINUX_MIB_TCPRENORECOVERY;
		else
			mib_idx = LINUX_MIB_TCPSACKRECOVERY;

		NET_INC_STATS_BH(sock_net(sk), mib_idx);

		tp->high_seq = tp->snd_nxt;
		tp->prior_ssthresh = 0;
		tp->undo_marker = tp->snd_una;
		tp->undo_retrans = tp->retrans_out;

		if (icsk->icsk_ca_state < TCP_CA_CWR) {
			if (!(flag & FLAG_ECE))
				tp->prior_ssthresh = tcp_current_ssthresh(sk);
			tp->snd_ssthresh = icsk->icsk_ca_ops->ssthresh(sk);
			TCP_ECN_queue_cwr(tp);
		}

		tp->bytes_acked = 0;
		tp->snd_cwnd_cnt = 0;
		tp->prior_cwnd = tp->snd_cwnd;
		tp->prr_delivered = 0;
		tp->prr_out = 0;
		tcp_set_ca_state(sk, TCP_CA_Recovery);
		fast_rexmit = 1;
	}

	if (do_lost || (tcp_is_fack(tp) && tcp_head_timedout(sk)))
		tcp_update_scoreboard(sk, fast_rexmit);
	tp->prr_delivered += newly_acked_sacked;
	tcp_update_cwnd_in_recovery(sk, newly_acked_sacked, fast_rexmit, flag);
	tcp_xmit_retransmit_queue(sk);
}

void tcp_valid_rtt_meas(struct sock *sk, u32 seq_rtt)
{
	tcp_rtt_estimator(sk, seq_rtt);
	tcp_set_rto(sk);
	inet_csk(sk)->icsk_backoff = 0;
}
EXPORT_SYMBOL(tcp_valid_rtt_meas);

static void tcp_ack_saw_tstamp(struct sock *sk, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);

	tcp_valid_rtt_meas(sk, tcp_time_stamp - tp->rx_opt.rcv_tsecr);
}

static void tcp_ack_no_tstamp(struct sock *sk, u32 seq_rtt, int flag)
{

	if (flag & FLAG_RETRANS_DATA_ACKED)
		return;

	tcp_valid_rtt_meas(sk, seq_rtt);
}

static inline void tcp_ack_update_rtt(struct sock *sk, const int flag,
				      const s32 seq_rtt)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	
	if (tp->rx_opt.saw_tstamp && tp->rx_opt.rcv_tsecr)
		tcp_ack_saw_tstamp(sk, flag);
	else if (seq_rtt >= 0)
		tcp_ack_no_tstamp(sk, seq_rtt, flag);
}

static void tcp_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
{
	const struct inet_connection_sock *icsk = inet_csk(sk);
	icsk->icsk_ca_ops->cong_avoid(sk, ack, in_flight);
	tcp_sk(sk)->snd_cwnd_stamp = tcp_time_stamp;
}

static void tcp_rearm_rto(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	if (!tp->packets_out) {
		inet_csk_clear_xmit_timer(sk, ICSK_TIME_RETRANS);
	} else {
		inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS,
					  inet_csk(sk)->icsk_rto, TCP_RTO_MAX);
	}
}

static u32 tcp_tso_acked(struct sock *sk, struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	u32 packets_acked;

	BUG_ON(!after(TCP_SKB_CB(skb)->end_seq, tp->snd_una));

	packets_acked = tcp_skb_pcount(skb);
	if (tcp_trim_head(sk, skb, tp->snd_una - TCP_SKB_CB(skb)->seq))
		return 0;
	packets_acked -= tcp_skb_pcount(skb);

	if (packets_acked) {
		BUG_ON(tcp_skb_pcount(skb) == 0);
		BUG_ON(!before(TCP_SKB_CB(skb)->seq, TCP_SKB_CB(skb)->end_seq));
	}

	return packets_acked;
}

static int tcp_clean_rtx_queue(struct sock *sk, int prior_fackets,
			       u32 prior_snd_una)
{
	struct tcp_sock *tp = tcp_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct sk_buff *skb;
	u32 now = tcp_time_stamp;
	int fully_acked = 1;
	int flag = 0;
	u32 pkts_acked = 0;
	u32 reord = tp->packets_out;
	u32 prior_sacked = tp->sacked_out;
	s32 seq_rtt = -1;
	s32 ca_seq_rtt = -1;
	ktime_t last_ackt = net_invalid_timestamp();

	while ((skb = tcp_write_queue_head(sk)) && skb != tcp_send_head(sk)) {
		struct tcp_skb_cb *scb = TCP_SKB_CB(skb);
		u32 acked_pcount;
		u8 sacked = scb->sacked;

		
		if (after(scb->end_seq, tp->snd_una)) {
			if (tcp_skb_pcount(skb) == 1 ||
			    !after(tp->snd_una, scb->seq))
				break;

			acked_pcount = tcp_tso_acked(sk, skb);
			if (!acked_pcount)
				break;

			fully_acked = 0;
		} else {
			acked_pcount = tcp_skb_pcount(skb);
		}

		if (sacked & TCPCB_RETRANS) {
			if (sacked & TCPCB_SACKED_RETRANS)
				tp->retrans_out -= acked_pcount;
			flag |= FLAG_RETRANS_DATA_ACKED;
			ca_seq_rtt = -1;
			seq_rtt = -1;
			if ((flag & FLAG_DATA_ACKED) || (acked_pcount > 1))
				flag |= FLAG_NONHEAD_RETRANS_ACKED;
		} else {
			ca_seq_rtt = now - scb->when;
			last_ackt = skb->tstamp;
			if (seq_rtt < 0) {
				seq_rtt = ca_seq_rtt;
			}
			if (!(sacked & TCPCB_SACKED_ACKED))
				reord = min(pkts_acked, reord);
		}

		if (sacked & TCPCB_SACKED_ACKED)
			tp->sacked_out -= acked_pcount;
		if (sacked & TCPCB_LOST)
			tp->lost_out -= acked_pcount;

		tp->packets_out -= acked_pcount;
		pkts_acked += acked_pcount;

		if (!(scb->tcp_flags & TCPHDR_SYN)) {
			flag |= FLAG_DATA_ACKED;
		} else {
			flag |= FLAG_SYN_ACKED;
			tp->retrans_stamp = 0;
		}

		if (!fully_acked)
			break;

		tcp_unlink_write_queue(skb, sk);
		sk_wmem_free_skb(sk, skb);
		tp->scoreboard_skb_hint = NULL;
		if (skb == tp->retransmit_skb_hint)
			tp->retransmit_skb_hint = NULL;
		if (skb == tp->lost_skb_hint)
			tp->lost_skb_hint = NULL;
	}

	if (likely(between(tp->snd_up, prior_snd_una, tp->snd_una)))
		tp->snd_up = tp->snd_una;

	if (skb && (TCP_SKB_CB(skb)->sacked & TCPCB_SACKED_ACKED))
		flag |= FLAG_SACK_RENEGING;

	if (flag & FLAG_ACKED) {
		const struct tcp_congestion_ops *ca_ops
			= inet_csk(sk)->icsk_ca_ops;

		if (unlikely(icsk->icsk_mtup.probe_size &&
			     !after(tp->mtu_probe.probe_seq_end, tp->snd_una))) {
			tcp_mtup_probe_success(sk);
		}

		tcp_ack_update_rtt(sk, flag, seq_rtt);
		tcp_rearm_rto(sk);

		if (tcp_is_reno(tp)) {
			tcp_remove_reno_sacks(sk, pkts_acked);
		} else {
			int delta;

			
			if (reord < prior_fackets)
				tcp_update_reordering(sk, tp->fackets_out - reord, 0);

			delta = tcp_is_fack(tp) ? pkts_acked :
						  prior_sacked - tp->sacked_out;
			tp->lost_cnt_hint -= min(tp->lost_cnt_hint, delta);
		}

		tp->fackets_out -= min(pkts_acked, tp->fackets_out);

		if (ca_ops->pkts_acked) {
			s32 rtt_us = -1;

			
			if (!(flag & FLAG_RETRANS_DATA_ACKED)) {
				
				if (ca_ops->flags & TCP_CONG_RTT_STAMP &&
				    !ktime_equal(last_ackt,
						 net_invalid_timestamp()))
					rtt_us = ktime_us_delta(ktime_get_real(),
								last_ackt);
				else if (ca_seq_rtt >= 0)
					rtt_us = jiffies_to_usecs(ca_seq_rtt);
			}

			ca_ops->pkts_acked(sk, pkts_acked, rtt_us);
		}
	}

#if FASTRETRANS_DEBUG > 0
	WARN_ON((int)tp->sacked_out < 0);
	WARN_ON((int)tp->lost_out < 0);
	WARN_ON((int)tp->retrans_out < 0);
	if (!tp->packets_out && tcp_is_sack(tp)) {
		icsk = inet_csk(sk);
		if (tp->lost_out) {
			printk(KERN_DEBUG "Leak l=%u %d\n",
			       tp->lost_out, icsk->icsk_ca_state);
			tp->lost_out = 0;
		}
		if (tp->sacked_out) {
			printk(KERN_DEBUG "Leak s=%u %d\n",
			       tp->sacked_out, icsk->icsk_ca_state);
			tp->sacked_out = 0;
		}
		if (tp->retrans_out) {
			printk(KERN_DEBUG "Leak r=%u %d\n",
			       tp->retrans_out, icsk->icsk_ca_state);
			tp->retrans_out = 0;
		}
	}
#endif
	return flag;
}

static void tcp_ack_probe(struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);

	

	if (!after(TCP_SKB_CB(tcp_send_head(sk))->end_seq, tcp_wnd_end(tp))) {
		icsk->icsk_backoff = 0;
		inet_csk_clear_xmit_timer(sk, ICSK_TIME_PROBE0);
	} else {
		inet_csk_reset_xmit_timer(sk, ICSK_TIME_PROBE0,
					  min(icsk->icsk_rto << icsk->icsk_backoff, TCP_RTO_MAX),
					  TCP_RTO_MAX);
	}
}

static inline int tcp_ack_is_dubious(const struct sock *sk, const int flag)
{
	return !(flag & FLAG_NOT_DUP) || (flag & FLAG_CA_ALERT) ||
		inet_csk(sk)->icsk_ca_state != TCP_CA_Open;
}

static inline int tcp_may_raise_cwnd(const struct sock *sk, const int flag)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	return (!(flag & FLAG_ECE) || tp->snd_cwnd < tp->snd_ssthresh) &&
		!((1 << inet_csk(sk)->icsk_ca_state) & (TCPF_CA_Recovery | TCPF_CA_CWR));
}

static inline int tcp_may_update_window(const struct tcp_sock *tp,
					const u32 ack, const u32 ack_seq,
					const u32 nwin)
{
	return	after(ack, tp->snd_una) ||
		after(ack_seq, tp->snd_wl1) ||
		(ack_seq == tp->snd_wl1 && nwin > tp->snd_wnd);
}

static int tcp_ack_update_window(struct sock *sk, const struct sk_buff *skb, u32 ack,
				 u32 ack_seq)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int flag = 0;
	u32 nwin = ntohs(tcp_hdr(skb)->window);

	if (likely(!tcp_hdr(skb)->syn))
		nwin <<= tp->rx_opt.snd_wscale;

	if (tcp_may_update_window(tp, ack, ack_seq, nwin)) {
		flag |= FLAG_WIN_UPDATE;
		tcp_update_wl(tp, ack_seq);

		if (tp->snd_wnd != nwin) {
			tp->snd_wnd = nwin;

			tp->pred_flags = 0;
			tcp_fast_path_check(sk);

			if (nwin > tp->max_window) {
				tp->max_window = nwin;
				tcp_sync_mss(sk, inet_csk(sk)->icsk_pmtu_cookie);
			}
		}
	}

	tp->snd_una = ack;

	return flag;
}

static void tcp_conservative_spur_to_response(struct tcp_sock *tp)
{
	tp->snd_cwnd = min(tp->snd_cwnd, tp->snd_ssthresh);
	tp->snd_cwnd_cnt = 0;
	tp->bytes_acked = 0;
	TCP_ECN_queue_cwr(tp);
	tcp_moderate_cwnd(tp);
}

static void tcp_ratehalving_spur_to_response(struct sock *sk)
{
	tcp_enter_cwr(sk, 0);
}

static void tcp_undo_spur_to_response(struct sock *sk, int flag)
{
	if (flag & FLAG_ECE)
		tcp_ratehalving_spur_to_response(sk);
	else
		tcp_undo_cwr(sk, true);
}

static int tcp_process_frto(struct sock *sk, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);

	tcp_verify_left_out(tp);

	
	if (flag & FLAG_DATA_ACKED)
		inet_csk(sk)->icsk_retransmits = 0;

	if ((flag & FLAG_NONHEAD_RETRANS_ACKED) ||
	    ((tp->frto_counter >= 2) && (flag & FLAG_RETRANS_DATA_ACKED)))
		tp->undo_marker = 0;

	if (!before(tp->snd_una, tp->frto_highmark)) {
		tcp_enter_frto_loss(sk, (tp->frto_counter == 1 ? 2 : 3), flag);
		return 1;
	}

	if (!tcp_is_sackfrto(tp)) {
		if (!(flag & FLAG_ANY_PROGRESS) && (flag & FLAG_NOT_DUP))
			return 1;

		if (!(flag & FLAG_DATA_ACKED)) {
			tcp_enter_frto_loss(sk, (tp->frto_counter == 1 ? 0 : 3),
					    flag);
			return 1;
		}
	} else {
		if (!(flag & FLAG_DATA_ACKED) && (tp->frto_counter == 1)) {
			
			tp->snd_cwnd = min(tp->snd_cwnd,
					   tcp_packets_in_flight(tp));
			return 1;
		}

		if ((tp->frto_counter >= 2) &&
		    (!(flag & FLAG_FORWARD_PROGRESS) ||
		     ((flag & FLAG_DATA_SACKED) &&
		      !(flag & FLAG_ONLY_ORIG_SACKED)))) {
			
			if (!(flag & FLAG_FORWARD_PROGRESS) &&
			    (flag & FLAG_NOT_DUP))
				return 1;

			tcp_enter_frto_loss(sk, 3, flag);
			return 1;
		}
	}

	if (tp->frto_counter == 1) {
		
		tp->snd_cwnd = tcp_packets_in_flight(tp) + 2;
		tp->frto_counter = 2;

		if (!tcp_may_send_now(sk))
			tcp_enter_frto_loss(sk, 2, flag);

		return 1;
	} else {
		switch (sysctl_tcp_frto_response) {
		case 2:
			tcp_undo_spur_to_response(sk, flag);
			break;
		case 1:
			tcp_conservative_spur_to_response(tp);
			break;
		default:
			tcp_ratehalving_spur_to_response(sk);
			break;
		}
		tp->frto_counter = 0;
		tp->undo_marker = 0;
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPSPURIOUSRTOS);
	}
	return 0;
}

static int tcp_ack(struct sock *sk, const struct sk_buff *skb, int flag)
{
	struct inet_connection_sock *icsk = NULL;
	struct tcp_sock *tp = NULL;
	u32 prior_snd_una = 0;
	u32 ack_seq = 0;
	u32 ack = 0;
	bool is_dupack = false;
	u32 prior_in_flight = 0;
	u32 prior_fackets = 0;
	int prior_packets = 0;
	int prior_sacked = 0;
	int pkts_acked = 0;
	int newly_acked_sacked = 0;
	int frto_cwnd = 0;

    if ((!sk) || IS_ERR(sk))
        goto invalid_ack;

    if ((!skb) || IS_ERR(skb))
        goto invalid_ack;

    icsk = inet_csk(sk);
    tp = tcp_sk(sk);
    prior_snd_una = tp->snd_una;
    ack_seq = TCP_SKB_CB(skb)->seq;
    ack = TCP_SKB_CB(skb)->ack_seq;
    prior_sacked = tp->sacked_out;

	if (before(ack, prior_snd_una))
		goto old_ack;

	if (after(ack, tp->snd_nxt))
		goto invalid_ack;

	if (after(ack, prior_snd_una))
		flag |= FLAG_SND_UNA_ADVANCED;

	if (sysctl_tcp_abc) {
		if (icsk->icsk_ca_state < TCP_CA_CWR)
			tp->bytes_acked += ack - prior_snd_una;
		else if (icsk->icsk_ca_state == TCP_CA_Loss)
			
			tp->bytes_acked += min(ack - prior_snd_una,
					       tp->mss_cache);
	}

	prior_fackets = tp->fackets_out;
	prior_in_flight = tcp_packets_in_flight(tp);

	if (!(flag & FLAG_SLOWPATH) && after(ack, prior_snd_una)) {
		tcp_update_wl(tp, ack_seq);
		tp->snd_una = ack;
		flag |= FLAG_WIN_UPDATE;

		tcp_ca_event(sk, CA_EVENT_FAST_ACK);

		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPHPACKS);
	} else {
		if (ack_seq != TCP_SKB_CB(skb)->end_seq)
			flag |= FLAG_DATA;
		else
			NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPPUREACKS);

		flag |= tcp_ack_update_window(sk, skb, ack, ack_seq);

		if (TCP_SKB_CB(skb)->sacked)
			flag |= tcp_sacktag_write_queue(sk, skb, prior_snd_una);

		if (TCP_ECN_rcv_ecn_echo(tp, tcp_hdr(skb)))
			flag |= FLAG_ECE;

		tcp_ca_event(sk, CA_EVENT_SLOW_ACK);
	}

	sk->sk_err_soft = 0;
	icsk->icsk_probes_out = 0;
	tp->rcv_tstamp = tcp_time_stamp;
	prior_packets = tp->packets_out;
	if (!prior_packets)
		goto no_queue;

	
	flag |= tcp_clean_rtx_queue(sk, prior_fackets, prior_snd_una);

	pkts_acked = prior_packets - tp->packets_out;
	newly_acked_sacked = (prior_packets - prior_sacked) -
			     (tp->packets_out - tp->sacked_out);

	if (tp->frto_counter)
		frto_cwnd = tcp_process_frto(sk, flag);
	
	if (before(tp->frto_highmark, tp->snd_una))
		tp->frto_highmark = 0;

	if (tcp_ack_is_dubious(sk, flag)) {
		
		if ((flag & FLAG_DATA_ACKED) && !frto_cwnd &&
		    tcp_may_raise_cwnd(sk, flag))
			tcp_cong_avoid(sk, ack, prior_in_flight);
		is_dupack = !(flag & (FLAG_SND_UNA_ADVANCED | FLAG_NOT_DUP));
		tcp_fastretrans_alert(sk, pkts_acked, newly_acked_sacked,
				      is_dupack, flag);
	} else {
		if ((flag & FLAG_DATA_ACKED) && !frto_cwnd)
			tcp_cong_avoid(sk, ack, prior_in_flight);
	}

	if ((flag & FLAG_FORWARD_PROGRESS) || !(flag & FLAG_NOT_DUP))
		dst_confirm(__sk_dst_get(sk));

	return 1;

no_queue:
	
	if (flag & FLAG_DSACKING_ACK)
		tcp_fastretrans_alert(sk, pkts_acked, newly_acked_sacked,
				      is_dupack, flag);
	if (tcp_send_head(sk))
		tcp_ack_probe(sk);
	return 1;

invalid_ack:
	SOCK_DEBUG(sk, "Ack %u after %u:%u\n", ack, tp->snd_una, tp->snd_nxt);
	return -1;

old_ack:
	if (TCP_SKB_CB(skb)->sacked) {
		flag |= tcp_sacktag_write_queue(sk, skb, prior_snd_una);
		newly_acked_sacked = tp->sacked_out - prior_sacked;
		tcp_fastretrans_alert(sk, pkts_acked, newly_acked_sacked,
				      is_dupack, flag);
	}

	SOCK_DEBUG(sk, "Ack %u before %u:%u\n", ack, tp->snd_una, tp->snd_nxt);
	return 0;
}

void tcp_parse_options(const struct sk_buff *skb, struct tcp_options_received *opt_rx,
		       const u8 **hvpp, int estab)
{
	const unsigned char *ptr;
	const struct tcphdr *th = tcp_hdr(skb);
	int length = (th->doff * 4) - sizeof(struct tcphdr);

	ptr = (const unsigned char *)(th + 1);
	opt_rx->saw_tstamp = 0;

	while (length > 0) {
		int opcode = *ptr++;
		int opsize;

		switch (opcode) {
		case TCPOPT_EOL:
			return;
		case TCPOPT_NOP:	
			length--;
			continue;
		default:
			opsize = *ptr++;
			if (opsize < 2) 
				return;
			if (opsize > length)
				return;	
			switch (opcode) {
			case TCPOPT_MSS:
				if (opsize == TCPOLEN_MSS && th->syn && !estab) {
					u16 in_mss = get_unaligned_be16(ptr);
					if (in_mss) {
						if (opt_rx->user_mss &&
						    opt_rx->user_mss < in_mss)
							in_mss = opt_rx->user_mss;
						opt_rx->mss_clamp = in_mss;
					}
				}
				break;
			case TCPOPT_WINDOW:
				if (opsize == TCPOLEN_WINDOW && th->syn &&
				    !estab && sysctl_tcp_window_scaling) {
					__u8 snd_wscale = *(__u8 *)ptr;
					opt_rx->wscale_ok = 1;
					if (snd_wscale > 14) {
						if (net_ratelimit())
							pr_info("%s: Illegal window scaling value %d >14 received\n",
								__func__,
								snd_wscale);
						snd_wscale = 14;
					}
					opt_rx->snd_wscale = snd_wscale;
				}
				break;
			case TCPOPT_TIMESTAMP:
				if ((opsize == TCPOLEN_TIMESTAMP) &&
				    ((estab && opt_rx->tstamp_ok) ||
				     (!estab && sysctl_tcp_timestamps))) {
					opt_rx->saw_tstamp = 1;
					opt_rx->rcv_tsval = get_unaligned_be32(ptr);
					opt_rx->rcv_tsecr = get_unaligned_be32(ptr + 4);
				}
				break;
			case TCPOPT_SACK_PERM:
				if (opsize == TCPOLEN_SACK_PERM && th->syn &&
				    !estab && sysctl_tcp_sack) {
					opt_rx->sack_ok = TCP_SACK_SEEN;
					tcp_sack_reset(opt_rx);
				}
				break;

			case TCPOPT_SACK:
				if ((opsize >= (TCPOLEN_SACK_BASE + TCPOLEN_SACK_PERBLOCK)) &&
				   !((opsize - TCPOLEN_SACK_BASE) % TCPOLEN_SACK_PERBLOCK) &&
				   opt_rx->sack_ok) {
					TCP_SKB_CB(skb)->sacked = (ptr - 2) - (unsigned char *)th;
				}
				break;
#ifdef CONFIG_TCP_MD5SIG
			case TCPOPT_MD5SIG:
				break;
#endif
			case TCPOPT_COOKIE:
				switch (opsize) {
				case TCPOLEN_COOKIE_BASE:
					
					break;
				case TCPOLEN_COOKIE_PAIR:
					
					break;
				case TCPOLEN_COOKIE_MIN+0:
				case TCPOLEN_COOKIE_MIN+2:
				case TCPOLEN_COOKIE_MIN+4:
				case TCPOLEN_COOKIE_MIN+6:
				case TCPOLEN_COOKIE_MAX:
					
					opt_rx->cookie_plus = opsize;
					*hvpp = ptr;
					break;
				default:
					
					break;
				}
				break;
			}

			ptr += opsize-2;
			length -= opsize;
		}
	}
}
EXPORT_SYMBOL(tcp_parse_options);

static int tcp_parse_aligned_timestamp(struct tcp_sock *tp, const struct tcphdr *th)
{
	const __be32 *ptr = (const __be32 *)(th + 1);

	if (*ptr == htonl((TCPOPT_NOP << 24) | (TCPOPT_NOP << 16)
			  | (TCPOPT_TIMESTAMP << 8) | TCPOLEN_TIMESTAMP)) {
		tp->rx_opt.saw_tstamp = 1;
		++ptr;
		tp->rx_opt.rcv_tsval = ntohl(*ptr);
		++ptr;
		tp->rx_opt.rcv_tsecr = ntohl(*ptr);
		return 1;
	}
	return 0;
}

static int tcp_fast_parse_options(const struct sk_buff *skb,
				  const struct tcphdr *th,
				  struct tcp_sock *tp, const u8 **hvpp)
{
	if (th->doff == (sizeof(*th) / 4)) {
		tp->rx_opt.saw_tstamp = 0;
		return 0;
	} else if (tp->rx_opt.tstamp_ok &&
		   th->doff == ((sizeof(*th) + TCPOLEN_TSTAMP_ALIGNED) / 4)) {
		if (tcp_parse_aligned_timestamp(tp, th))
			return 1;
	}
	tcp_parse_options(skb, &tp->rx_opt, hvpp, 1);
	return 1;
}

#ifdef CONFIG_TCP_MD5SIG
const u8 *tcp_parse_md5sig_option(const struct tcphdr *th)
{
	int length = (th->doff << 2) - sizeof(*th);
	const u8 *ptr = (const u8 *)(th + 1);

	
	if (length < TCPOLEN_MD5SIG)
		return NULL;

	while (length > 0) {
		int opcode = *ptr++;
		int opsize;

		switch(opcode) {
		case TCPOPT_EOL:
			return NULL;
		case TCPOPT_NOP:
			length--;
			continue;
		default:
			opsize = *ptr++;
			if (opsize < 2 || opsize > length)
				return NULL;
			if (opcode == TCPOPT_MD5SIG)
				return opsize == TCPOLEN_MD5SIG ? ptr : NULL;
		}
		ptr += opsize - 2;
		length -= opsize;
	}
	return NULL;
}
EXPORT_SYMBOL(tcp_parse_md5sig_option);
#endif

static inline void tcp_store_ts_recent(struct tcp_sock *tp)
{
	tp->rx_opt.ts_recent = tp->rx_opt.rcv_tsval;
	tp->rx_opt.ts_recent_stamp = get_seconds();
}

static inline void tcp_replace_ts_recent(struct tcp_sock *tp, u32 seq)
{
	if (tp->rx_opt.saw_tstamp && !after(seq, tp->rcv_wup)) {

		if (tcp_paws_check(&tp->rx_opt, 0))
			tcp_store_ts_recent(tp);
	}
}


static int tcp_disordered_ack(const struct sock *sk, const struct sk_buff *skb)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	const struct tcphdr *th = tcp_hdr(skb);
	u32 seq = TCP_SKB_CB(skb)->seq;
	u32 ack = TCP_SKB_CB(skb)->ack_seq;

	return (
		(th->ack && seq == TCP_SKB_CB(skb)->end_seq && seq == tp->rcv_nxt) &&

		
		ack == tp->snd_una &&

		
		!tcp_may_update_window(tp, ack, seq, ntohs(th->window) << tp->rx_opt.snd_wscale) &&

		
		(s32)(tp->rx_opt.ts_recent - tp->rx_opt.rcv_tsval) <= (inet_csk(sk)->icsk_rto * 1024) / HZ);
}

static inline int tcp_paws_discard(const struct sock *sk,
				   const struct sk_buff *skb)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	return !tcp_paws_check(&tp->rx_opt, TCP_PAWS_WINDOW) &&
	       !tcp_disordered_ack(sk, skb);
}


static inline int tcp_sequence(const struct tcp_sock *tp, u32 seq, u32 end_seq)
{
	return	!before(end_seq, tp->rcv_wup) &&
		!after(seq, tp->rcv_nxt + tcp_receive_window(tp));
}

static void tcp_reset(struct sock *sk)
{
	
	switch (sk->sk_state) {
	case TCP_SYN_SENT:
		sk->sk_err = ECONNREFUSED;
		break;
	case TCP_CLOSE_WAIT:
		sk->sk_err = EPIPE;
		break;
	case TCP_CLOSE:
		return;
	default:
		sk->sk_err = ECONNRESET;
	}
	
	smp_wmb();

	if (!sock_flag(sk, SOCK_DEAD))
		sk->sk_error_report(sk);

	tcp_done(sk);
}

static void tcp_fin(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	inet_csk_schedule_ack(sk);

	sk->sk_shutdown |= RCV_SHUTDOWN;
	sock_set_flag(sk, SOCK_DONE);

	switch (sk->sk_state) {
	case TCP_SYN_RECV:
	case TCP_ESTABLISHED:
		
		tcp_set_state(sk, TCP_CLOSE_WAIT);
		inet_csk(sk)->icsk_ack.pingpong = 1;
		break;

	case TCP_CLOSE_WAIT:
	case TCP_CLOSING:
		break;
	case TCP_LAST_ACK:
		
		break;

	case TCP_FIN_WAIT1:
		tcp_send_ack(sk);
		tcp_set_state(sk, TCP_CLOSING);
		break;
	case TCP_FIN_WAIT2:
		
		tcp_send_ack(sk);
		tcp_time_wait(sk, TCP_TIME_WAIT, 0);
		break;
	default:
		pr_err("%s: Impossible, sk->sk_state=%d\n",
		       __func__, sk->sk_state);
		break;
	}

	__skb_queue_purge(&tp->out_of_order_queue);
	if (tcp_is_sack(tp))
		tcp_sack_reset(&tp->rx_opt);
	sk_mem_reclaim(sk);

	if (!sock_flag(sk, SOCK_DEAD)) {
		sk->sk_state_change(sk);

		
		if (sk->sk_shutdown == SHUTDOWN_MASK ||
		    sk->sk_state == TCP_CLOSE)
			sk_wake_async(sk, SOCK_WAKE_WAITD, POLL_HUP);
		else
			sk_wake_async(sk, SOCK_WAKE_WAITD, POLL_IN);
	}
}

static inline int tcp_sack_extend(struct tcp_sack_block *sp, u32 seq,
				  u32 end_seq)
{
	if (!after(seq, sp->end_seq) && !after(sp->start_seq, end_seq)) {
		if (before(seq, sp->start_seq))
			sp->start_seq = seq;
		if (after(end_seq, sp->end_seq))
			sp->end_seq = end_seq;
		return 1;
	}
	return 0;
}

static void tcp_dsack_set(struct sock *sk, u32 seq, u32 end_seq)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_is_sack(tp) && sysctl_tcp_dsack) {
		int mib_idx;

		if (before(seq, tp->rcv_nxt))
			mib_idx = LINUX_MIB_TCPDSACKOLDSENT;
		else
			mib_idx = LINUX_MIB_TCPDSACKOFOSENT;

		NET_INC_STATS_BH(sock_net(sk), mib_idx);

		tp->rx_opt.dsack = 1;
		tp->duplicate_sack[0].start_seq = seq;
		tp->duplicate_sack[0].end_seq = end_seq;
	}
}

static void tcp_dsack_extend(struct sock *sk, u32 seq, u32 end_seq)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (!tp->rx_opt.dsack)
		tcp_dsack_set(sk, seq, end_seq);
	else
		tcp_sack_extend(tp->duplicate_sack, seq, end_seq);
}

static void tcp_send_dupack(struct sock *sk, const struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (TCP_SKB_CB(skb)->end_seq != TCP_SKB_CB(skb)->seq &&
	    before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt)) {
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_DELAYEDACKLOST);
		tcp_enter_quickack_mode(sk);

		if (tcp_is_sack(tp) && sysctl_tcp_dsack) {
			u32 end_seq = TCP_SKB_CB(skb)->end_seq;

			if (after(TCP_SKB_CB(skb)->end_seq, tp->rcv_nxt))
				end_seq = tp->rcv_nxt;
			tcp_dsack_set(sk, TCP_SKB_CB(skb)->seq, end_seq);
		}
	}

	tcp_send_ack(sk);
}

static void tcp_sack_maybe_coalesce(struct tcp_sock *tp)
{
	int this_sack;
	struct tcp_sack_block *sp = &tp->selective_acks[0];
	struct tcp_sack_block *swalk = sp + 1;

	for (this_sack = 1; this_sack < tp->rx_opt.num_sacks;) {
		if (tcp_sack_extend(sp, swalk->start_seq, swalk->end_seq)) {
			int i;

			tp->rx_opt.num_sacks--;
			for (i = this_sack; i < tp->rx_opt.num_sacks; i++)
				sp[i] = sp[i + 1];
			continue;
		}
		this_sack++, swalk++;
	}
}

static void tcp_sack_new_ofo_skb(struct sock *sk, u32 seq, u32 end_seq)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct tcp_sack_block *sp = &tp->selective_acks[0];
	int cur_sacks = tp->rx_opt.num_sacks;
	int this_sack;

	if (!cur_sacks)
		goto new_sack;

	for (this_sack = 0; this_sack < cur_sacks; this_sack++, sp++) {
		if (tcp_sack_extend(sp, seq, end_seq)) {
			
			for (; this_sack > 0; this_sack--, sp--)
				swap(*sp, *(sp - 1));
			if (cur_sacks > 1)
				tcp_sack_maybe_coalesce(tp);
			return;
		}
	}

	if (this_sack >= TCP_NUM_SACKS) {
		this_sack--;
		tp->rx_opt.num_sacks--;
		sp--;
	}
	for (; this_sack > 0; this_sack--, sp--)
		*sp = *(sp - 1);

new_sack:
	
	sp->start_seq = seq;
	sp->end_seq = end_seq;
	tp->rx_opt.num_sacks++;
}


static void tcp_sack_remove(struct tcp_sock *tp)
{
	struct tcp_sack_block *sp = &tp->selective_acks[0];
	int num_sacks = tp->rx_opt.num_sacks;
	int this_sack;

	
	if (skb_queue_empty(&tp->out_of_order_queue)) {
		tp->rx_opt.num_sacks = 0;
		return;
	}

	for (this_sack = 0; this_sack < num_sacks;) {
		
		if (!before(tp->rcv_nxt, sp->start_seq)) {
			int i;

			
			WARN_ON(before(tp->rcv_nxt, sp->end_seq));

			
			for (i=this_sack+1; i < num_sacks; i++)
				tp->selective_acks[i-1] = tp->selective_acks[i];
			num_sacks--;
			continue;
		}
		this_sack++;
		sp++;
	}
	tp->rx_opt.num_sacks = num_sacks;
}

static void tcp_ofo_queue(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	__u32 dsack_high = tp->rcv_nxt;
	struct sk_buff *skb;

	while ((skb = skb_peek(&tp->out_of_order_queue)) != NULL) {
		if (after(TCP_SKB_CB(skb)->seq, tp->rcv_nxt))
			break;

		if (before(TCP_SKB_CB(skb)->seq, dsack_high)) {
			__u32 dsack = dsack_high;
			if (before(TCP_SKB_CB(skb)->end_seq, dsack_high))
				dsack_high = TCP_SKB_CB(skb)->end_seq;
			tcp_dsack_extend(sk, TCP_SKB_CB(skb)->seq, dsack);
		}

		if (!after(TCP_SKB_CB(skb)->end_seq, tp->rcv_nxt)) {
			SOCK_DEBUG(sk, "ofo packet was already received\n");
			__skb_unlink(skb, &tp->out_of_order_queue);
			__kfree_skb(skb);
			continue;
		}
		SOCK_DEBUG(sk, "ofo requeuing : rcv_next %X seq %X - %X\n",
			   tp->rcv_nxt, TCP_SKB_CB(skb)->seq,
			   TCP_SKB_CB(skb)->end_seq);

		__skb_unlink(skb, &tp->out_of_order_queue);
		__skb_queue_tail(&sk->sk_receive_queue, skb);
		tp->rcv_nxt = TCP_SKB_CB(skb)->end_seq;
		if (tcp_hdr(skb)->fin)
			tcp_fin(sk);
	}
}

static int tcp_prune_ofo_queue(struct sock *sk);
static int tcp_prune_queue(struct sock *sk);

static inline int tcp_try_rmem_schedule(struct sock *sk, unsigned int size)
{
	if (atomic_read(&sk->sk_rmem_alloc) > sk->sk_rcvbuf ||
	    !sk_rmem_schedule(sk, size)) {

		if (tcp_prune_queue(sk) < 0)
			return -1;

		if (!sk_rmem_schedule(sk, size)) {
			if (!tcp_prune_ofo_queue(sk))
				return -1;

			if (!sk_rmem_schedule(sk, size))
				return -1;
		}
	}
	return 0;
}

static void tcp_data_queue_ofo(struct sock *sk, struct sk_buff *skb)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb1;
	u32 seq, end_seq;

	TCP_ECN_check_ce(tp, skb);

	if (tcp_try_rmem_schedule(sk, skb->truesize)) {
		
		__kfree_skb(skb);
		return;
	}

	
	tp->pred_flags = 0;
	inet_csk_schedule_ack(sk);

	SOCK_DEBUG(sk, "out of order segment: rcv_next %X seq %X - %X\n",
		   tp->rcv_nxt, TCP_SKB_CB(skb)->seq, TCP_SKB_CB(skb)->end_seq);

	skb1 = skb_peek_tail(&tp->out_of_order_queue);
	if (!skb1) {
		
		if (tcp_is_sack(tp)) {
			tp->rx_opt.num_sacks = 1;
			tp->selective_acks[0].start_seq = TCP_SKB_CB(skb)->seq;
			tp->selective_acks[0].end_seq =
						TCP_SKB_CB(skb)->end_seq;
		}
		__skb_queue_head(&tp->out_of_order_queue, skb);
		goto end;
	}

	seq = TCP_SKB_CB(skb)->seq;
	end_seq = TCP_SKB_CB(skb)->end_seq;

	if (seq == TCP_SKB_CB(skb1)->end_seq) {
		if (skb->len <= skb_tailroom(skb1) && !tcp_hdr(skb)->fin) {
			NET_INC_STATS_BH(sock_net(sk),
					 LINUX_MIB_TCPRCVCOALESCE);
			BUG_ON(skb_copy_bits(skb, 0,
					     skb_put(skb1, skb->len),
					     skb->len));
			TCP_SKB_CB(skb1)->end_seq = end_seq;
			TCP_SKB_CB(skb1)->ack_seq = TCP_SKB_CB(skb)->ack_seq;
			__kfree_skb(skb);
			skb = NULL;
		} else {
			__skb_queue_after(&tp->out_of_order_queue, skb1, skb);
		}

		if (!tp->rx_opt.num_sacks ||
		    tp->selective_acks[0].end_seq != seq)
			goto add_sack;

		
		tp->selective_acks[0].end_seq = end_seq;
		goto end;
	}

	
	while (1) {
		if (!after(TCP_SKB_CB(skb1)->seq, seq))
			break;
		if (skb_queue_is_first(&tp->out_of_order_queue, skb1)) {
			skb1 = NULL;
			break;
		}
		skb1 = skb_queue_prev(&tp->out_of_order_queue, skb1);
	}

	
	if (skb1 && before(seq, TCP_SKB_CB(skb1)->end_seq)) {
		if (!after(end_seq, TCP_SKB_CB(skb1)->end_seq)) {
			
			__kfree_skb(skb);
			skb = NULL;
			tcp_dsack_set(sk, seq, end_seq);
			goto add_sack;
		}
		if (after(seq, TCP_SKB_CB(skb1)->seq)) {
			
			tcp_dsack_set(sk, seq,
				      TCP_SKB_CB(skb1)->end_seq);
		} else {
			if (skb_queue_is_first(&tp->out_of_order_queue,
					       skb1))
				skb1 = NULL;
			else
				skb1 = skb_queue_prev(
					&tp->out_of_order_queue,
					skb1);
		}
	}
	if (!skb1)
		__skb_queue_head(&tp->out_of_order_queue, skb);
	else
		__skb_queue_after(&tp->out_of_order_queue, skb1, skb);

	
	while (!skb_queue_is_last(&tp->out_of_order_queue, skb)) {
		skb1 = skb_queue_next(&tp->out_of_order_queue, skb);

		if (!after(end_seq, TCP_SKB_CB(skb1)->seq))
			break;
		if (before(end_seq, TCP_SKB_CB(skb1)->end_seq)) {
			tcp_dsack_extend(sk, TCP_SKB_CB(skb1)->seq,
					 end_seq);
			break;
		}
		__skb_unlink(skb1, &tp->out_of_order_queue);
		tcp_dsack_extend(sk, TCP_SKB_CB(skb1)->seq,
				 TCP_SKB_CB(skb1)->end_seq);
		__kfree_skb(skb1);
	}

add_sack:
	if (tcp_is_sack(tp))
		tcp_sack_new_ofo_skb(sk, seq, end_seq);
end:
	if (skb)
		skb_set_owner_r(skb, sk);
}


static void tcp_data_queue(struct sock *sk, struct sk_buff *skb)
{
	const struct tcphdr *th = tcp_hdr(skb);
	struct tcp_sock *tp = tcp_sk(sk);
	int eaten = -1;

	if (TCP_SKB_CB(skb)->seq == TCP_SKB_CB(skb)->end_seq)
		goto drop;

	skb_dst_drop(skb);
	__skb_pull(skb, th->doff * 4);

	TCP_ECN_accept_cwr(tp, skb);

	tp->rx_opt.dsack = 0;

	if (TCP_SKB_CB(skb)->seq == tp->rcv_nxt) {
		if (tcp_receive_window(tp) == 0)
			goto out_of_window;

		
		if (tp->ucopy.task == current &&
		    tp->copied_seq == tp->rcv_nxt && tp->ucopy.len &&
		    sock_owned_by_user(sk) && !tp->urg_data) {
			int chunk = min_t(unsigned int, skb->len,
					  tp->ucopy.len);

			__set_current_state(TASK_RUNNING);

			local_bh_enable();
			if (!skb_copy_datagram_iovec(skb, 0, tp->ucopy.iov, chunk)) {
				tp->ucopy.len -= chunk;
				tp->copied_seq += chunk;
				eaten = (chunk == skb->len);
				tcp_rcv_space_adjust(sk);
			}
			local_bh_disable();
		}

		if (eaten <= 0) {
queue_and_out:
			if (eaten < 0 &&
			    tcp_try_rmem_schedule(sk, skb->truesize))
				goto drop;

			skb_set_owner_r(skb, sk);
			__skb_queue_tail(&sk->sk_receive_queue, skb);
		}
		tp->rcv_nxt = TCP_SKB_CB(skb)->end_seq;
		if (skb->len)
			tcp_event_data_recv(sk, skb);
		if (th->fin)
			tcp_fin(sk);

		if (!skb_queue_empty(&tp->out_of_order_queue)) {
			tcp_ofo_queue(sk);

			if (skb_queue_empty(&tp->out_of_order_queue))
				inet_csk(sk)->icsk_ack.pingpong = 0;
		}

		if (tp->rx_opt.num_sacks)
			tcp_sack_remove(tp);

		tcp_fast_path_check(sk);

		if (eaten > 0)
			__kfree_skb(skb);
		else if (!sock_flag(sk, SOCK_DEAD))
			sk->sk_data_ready(sk, 0);
		return;
	}

	if (!after(TCP_SKB_CB(skb)->end_seq, tp->rcv_nxt)) {
		
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_DELAYEDACKLOST);
		tcp_dsack_set(sk, TCP_SKB_CB(skb)->seq, TCP_SKB_CB(skb)->end_seq);

out_of_window:
		tcp_enter_quickack_mode(sk);
		inet_csk_schedule_ack(sk);
drop:
		__kfree_skb(skb);
		return;
	}

	
	if (!before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt + tcp_receive_window(tp)))
		goto out_of_window;

	tcp_enter_quickack_mode(sk);

	if (before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt)) {
		
		SOCK_DEBUG(sk, "partial packet: rcv_next %X seq %X - %X\n",
			   tp->rcv_nxt, TCP_SKB_CB(skb)->seq,
			   TCP_SKB_CB(skb)->end_seq);

		tcp_dsack_set(sk, TCP_SKB_CB(skb)->seq, tp->rcv_nxt);

		if (!tcp_receive_window(tp))
			goto out_of_window;
		goto queue_and_out;
	}

	tcp_data_queue_ofo(sk, skb);
}

static struct sk_buff *tcp_collapse_one(struct sock *sk, struct sk_buff *skb,
					struct sk_buff_head *list)
{
	struct sk_buff *next = NULL;

	if (!skb_queue_is_last(list, skb))
		next = skb_queue_next(list, skb);

	__skb_unlink(skb, list);
	__kfree_skb(skb);
	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPRCVCOLLAPSED);

	return next;
}

static void
tcp_collapse(struct sock *sk, struct sk_buff_head *list,
	     struct sk_buff *head, struct sk_buff *tail,
	     u32 start, u32 end)
{
	struct sk_buff *skb, *n;
	bool end_of_skbs;

	skb = head;
restart:
	end_of_skbs = true;
	skb_queue_walk_from_safe(list, skb, n) {
		if (skb == tail)
			break;
		
		if (!before(start, TCP_SKB_CB(skb)->end_seq)) {
			skb = tcp_collapse_one(sk, skb, list);
			if (!skb)
				break;
			goto restart;
		}

		if (!tcp_hdr(skb)->syn && !tcp_hdr(skb)->fin &&
		    (tcp_win_from_space(skb->truesize) > skb->len ||
		     before(TCP_SKB_CB(skb)->seq, start))) {
			end_of_skbs = false;
			break;
		}

		if (!skb_queue_is_last(list, skb)) {
			struct sk_buff *next = skb_queue_next(list, skb);
			if (next != tail &&
			    TCP_SKB_CB(skb)->end_seq != TCP_SKB_CB(next)->seq) {
				end_of_skbs = false;
				break;
			}
		}

		
		start = TCP_SKB_CB(skb)->end_seq;
	}
	if (end_of_skbs || tcp_hdr(skb)->syn || tcp_hdr(skb)->fin)
		return;

	while (before(start, end)) {
		struct sk_buff *nskb;
		unsigned int header = skb_headroom(skb);
		int copy = SKB_MAX_ORDER(header, 0);

		
		if (copy < 0)
			return;
		if (end - start < copy)
			copy = end - start;
		nskb = alloc_skb(copy + header, GFP_ATOMIC);
		if (!nskb)
			return;

		skb_set_mac_header(nskb, skb_mac_header(skb) - skb->head);
		skb_set_network_header(nskb, (skb_network_header(skb) -
					      skb->head));
		skb_set_transport_header(nskb, (skb_transport_header(skb) -
						skb->head));
		skb_reserve(nskb, header);
		memcpy(nskb->head, skb->head, header);
		memcpy(nskb->cb, skb->cb, sizeof(skb->cb));
		TCP_SKB_CB(nskb)->seq = TCP_SKB_CB(nskb)->end_seq = start;
		__skb_queue_before(list, skb, nskb);
		skb_set_owner_r(nskb, sk);

		
		while (copy > 0) {
			int offset = start - TCP_SKB_CB(skb)->seq;
			int size = TCP_SKB_CB(skb)->end_seq - start;

			BUG_ON(offset < 0);
			if (size > 0) {
				size = min(copy, size);
				if (skb_copy_bits(skb, offset, skb_put(nskb, size), size))
					BUG();
				TCP_SKB_CB(nskb)->end_seq += size;
				copy -= size;
				start += size;
			}
			if (!before(start, TCP_SKB_CB(skb)->end_seq)) {
				skb = tcp_collapse_one(sk, skb, list);
				if (!skb ||
				    skb == tail ||
				    tcp_hdr(skb)->syn ||
				    tcp_hdr(skb)->fin)
					return;
			}
		}
	}
}

static void tcp_collapse_ofo_queue(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct sk_buff *skb = skb_peek(&tp->out_of_order_queue);
	struct sk_buff *head;
	u32 start, end;

	if (skb == NULL)
		return;

	start = TCP_SKB_CB(skb)->seq;
	end = TCP_SKB_CB(skb)->end_seq;
	head = skb;

	for (;;) {
		struct sk_buff *next = NULL;

		if (!skb_queue_is_last(&tp->out_of_order_queue, skb))
			next = skb_queue_next(&tp->out_of_order_queue, skb);
		skb = next;

		if (!skb ||
		    after(TCP_SKB_CB(skb)->seq, end) ||
		    before(TCP_SKB_CB(skb)->end_seq, start)) {
			tcp_collapse(sk, &tp->out_of_order_queue,
				     head, skb, start, end);
			head = skb;
			if (!skb)
				break;
			
			start = TCP_SKB_CB(skb)->seq;
			end = TCP_SKB_CB(skb)->end_seq;
		} else {
			if (before(TCP_SKB_CB(skb)->seq, start))
				start = TCP_SKB_CB(skb)->seq;
			if (after(TCP_SKB_CB(skb)->end_seq, end))
				end = TCP_SKB_CB(skb)->end_seq;
		}
	}
}

static int tcp_prune_ofo_queue(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int res = 0;

	if (!skb_queue_empty(&tp->out_of_order_queue)) {
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_OFOPRUNED);
		__skb_queue_purge(&tp->out_of_order_queue);

		if (tp->rx_opt.sack_ok)
			tcp_sack_reset(&tp->rx_opt);
		sk_mem_reclaim(sk);
		res = 1;
	}
	return res;
}

static int tcp_prune_queue(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	SOCK_DEBUG(sk, "prune_queue: c=%x\n", tp->copied_seq);

	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_PRUNECALLED);

	if (atomic_read(&sk->sk_rmem_alloc) >= sk->sk_rcvbuf)
		tcp_clamp_window(sk);
	else if (sk_under_memory_pressure(sk))
		tp->rcv_ssthresh = min(tp->rcv_ssthresh, 4U * tp->advmss);

	tcp_collapse_ofo_queue(sk);
	if (!skb_queue_empty(&sk->sk_receive_queue))
		tcp_collapse(sk, &sk->sk_receive_queue,
			     skb_peek(&sk->sk_receive_queue),
			     NULL,
			     tp->copied_seq, tp->rcv_nxt);
	sk_mem_reclaim(sk);

	if (atomic_read(&sk->sk_rmem_alloc) <= sk->sk_rcvbuf)
		return 0;


	tcp_prune_ofo_queue(sk);

	if (atomic_read(&sk->sk_rmem_alloc) <= sk->sk_rcvbuf)
		return 0;

	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_RCVPRUNED);

	
	tp->pred_flags = 0;
	return -1;
}

void tcp_cwnd_application_limited(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (inet_csk(sk)->icsk_ca_state == TCP_CA_Open &&
	    sk->sk_socket && !test_bit(SOCK_NOSPACE, &sk->sk_socket->flags)) {
		
		u32 init_win = tcp_init_cwnd(tp, __sk_dst_get(sk));
		u32 win_used = max(tp->snd_cwnd_used, init_win);
		if (win_used < tp->snd_cwnd) {
			tp->snd_ssthresh = tcp_current_ssthresh(sk);
			tp->snd_cwnd = (tp->snd_cwnd + win_used) >> 1;
		}
		tp->snd_cwnd_used = 0;
	}
	tp->snd_cwnd_stamp = tcp_time_stamp;
}

static int tcp_should_expand_sndbuf(const struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	if (sk->sk_userlocks & SOCK_SNDBUF_LOCK)
		return 0;

	
	if (sk_under_memory_pressure(sk))
		return 0;

	
	if (sk_memory_allocated(sk) >= sk_prot_mem_limits(sk, 0))
		return 0;

	
	if (tp->packets_out >= tp->snd_cwnd)
		return 0;

	return 1;
}

static void tcp_new_space(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (tcp_should_expand_sndbuf(sk)) {
		int sndmem = SKB_TRUESIZE(max_t(u32,
						tp->rx_opt.mss_clamp,
						tp->mss_cache) +
					  MAX_TCP_HEADER);
		int demanded = max_t(unsigned int, tp->snd_cwnd,
				     tp->reordering + 1);
		sndmem *= 2 * demanded;
		if (sndmem > sk->sk_sndbuf)
			sk->sk_sndbuf = min(sndmem, sysctl_tcp_wmem[2]);
		tp->snd_cwnd_stamp = tcp_time_stamp;
	}

	sk->sk_write_space(sk);
}

static void tcp_check_space(struct sock *sk)
{
	if (sock_flag(sk, SOCK_QUEUE_SHRUNK)) {
		sock_reset_flag(sk, SOCK_QUEUE_SHRUNK);
		if (sk->sk_socket &&
		    test_bit(SOCK_NOSPACE, &sk->sk_socket->flags))
			tcp_new_space(sk);
	}
}

static inline void tcp_data_snd_check(struct sock *sk)
{
	tcp_push_pending_frames(sk);
	tcp_check_space(sk);
}

static void __tcp_ack_snd_check(struct sock *sk, int ofo_possible)
{
	struct tcp_sock *tp = tcp_sk(sk);

	    
	if (((tp->rcv_nxt - tp->rcv_wup) > inet_csk(sk)->icsk_ack.rcv_mss &&
	     __tcp_select_window(sk) >= tp->rcv_wnd) ||
	    
	    tcp_in_quickack_mode(sk) ||
	    
	    (ofo_possible && skb_peek(&tp->out_of_order_queue))) {
		
		tcp_send_ack(sk);
	} else {
		
		tcp_send_delayed_ack(sk);
	}
}

static inline void tcp_ack_snd_check(struct sock *sk)
{
	if (!inet_csk_ack_scheduled(sk)) {
		
		return;
	}
	__tcp_ack_snd_check(sk, 1);
}


static void tcp_check_urg(struct sock *sk, const struct tcphdr *th)
{
	struct tcp_sock *tp = tcp_sk(sk);
	u32 ptr = ntohs(th->urg_ptr);

	if (ptr && !sysctl_tcp_stdurg)
		ptr--;
	ptr += ntohl(th->seq);

	
	if (after(tp->copied_seq, ptr))
		return;

	if (before(ptr, tp->rcv_nxt))
		return;

	
	if (tp->urg_data && !after(ptr, tp->urg_seq))
		return;

	
	sk_send_sigurg(sk);

	if (tp->urg_seq == tp->copied_seq && tp->urg_data &&
	    !sock_flag(sk, SOCK_URGINLINE) && tp->copied_seq != tp->rcv_nxt) {
		struct sk_buff *skb = skb_peek(&sk->sk_receive_queue);
		tp->copied_seq++;
		if (skb && !before(tp->copied_seq, TCP_SKB_CB(skb)->end_seq)) {
			__skb_unlink(skb, &sk->sk_receive_queue);
			__kfree_skb(skb);
		}
	}

	tp->urg_data = TCP_URG_NOTYET;
	tp->urg_seq = ptr;

	
	tp->pred_flags = 0;
}

static void tcp_urg(struct sock *sk, struct sk_buff *skb, const struct tcphdr *th)
{
	struct tcp_sock *tp = tcp_sk(sk);

	
	if (th->urg)
		tcp_check_urg(sk, th);

	
	if (tp->urg_data == TCP_URG_NOTYET) {
		u32 ptr = tp->urg_seq - ntohl(th->seq) + (th->doff * 4) -
			  th->syn;

		
		if (ptr < skb->len) {
			u8 tmp;
			if (skb_copy_bits(skb, ptr, &tmp, 1))
				BUG();
			tp->urg_data = TCP_URG_VALID | tmp;
			if (!sock_flag(sk, SOCK_DEAD))
				sk->sk_data_ready(sk, 0);
		}
	}
}

static int tcp_copy_to_iovec(struct sock *sk, struct sk_buff *skb, int hlen)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int chunk = skb->len - hlen;
	int err;

	local_bh_enable();
	if (skb_csum_unnecessary(skb))
		err = skb_copy_datagram_iovec(skb, hlen, tp->ucopy.iov, chunk);
	else
		err = skb_copy_and_csum_datagram_iovec(skb, hlen,
						       tp->ucopy.iov);

	if (!err) {
		tp->ucopy.len -= chunk;
		tp->copied_seq += chunk;
		tcp_rcv_space_adjust(sk);
	}

	local_bh_disable();
	return err;
}

static __sum16 __tcp_checksum_complete_user(struct sock *sk,
					    struct sk_buff *skb)
{
	__sum16 result;

	if (sock_owned_by_user(sk)) {
		local_bh_enable();
		result = __tcp_checksum_complete(skb);
		local_bh_disable();
	} else {
		result = __tcp_checksum_complete(skb);
	}
	return result;
}

static inline int tcp_checksum_complete_user(struct sock *sk,
					     struct sk_buff *skb)
{
	return !skb_csum_unnecessary(skb) &&
	       __tcp_checksum_complete_user(sk, skb);
}

#ifdef CONFIG_NET_DMA
static int tcp_dma_try_early_copy(struct sock *sk, struct sk_buff *skb,
				  int hlen)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int chunk = skb->len - hlen;
	int dma_cookie;
	int copied_early = 0;

	if (tp->ucopy.wakeup)
		return 0;

	if (!tp->ucopy.dma_chan && tp->ucopy.pinned_list)
		tp->ucopy.dma_chan = net_dma_find_channel();

	if (tp->ucopy.dma_chan && skb_csum_unnecessary(skb)) {

		dma_cookie = dma_skb_copy_datagram_iovec(tp->ucopy.dma_chan,
							 skb, hlen,
							 tp->ucopy.iov, chunk,
							 tp->ucopy.pinned_list);

		if (dma_cookie < 0)
			goto out;

		tp->ucopy.dma_cookie = dma_cookie;
		copied_early = 1;

		tp->ucopy.len -= chunk;
		tp->copied_seq += chunk;
		tcp_rcv_space_adjust(sk);

		if ((tp->ucopy.len == 0) ||
		    (tcp_flag_word(tcp_hdr(skb)) & TCP_FLAG_PSH) ||
		    (atomic_read(&sk->sk_rmem_alloc) > (sk->sk_rcvbuf >> 1))) {
			tp->ucopy.wakeup = 1;
			sk->sk_data_ready(sk, 0);
		}
	} else if (chunk > 0) {
		tp->ucopy.wakeup = 1;
		sk->sk_data_ready(sk, 0);
	}
out:
	return copied_early;
}
#endif 

static int tcp_validate_incoming(struct sock *sk, struct sk_buff *skb,
			      const struct tcphdr *th, int syn_inerr)
{
	const u8 *hash_location;
	struct tcp_sock *tp = tcp_sk(sk);

	
	if (tcp_fast_parse_options(skb, th, tp, &hash_location) &&
	    tp->rx_opt.saw_tstamp &&
	    tcp_paws_discard(sk, skb)) {
		if (!th->rst) {
			NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_PAWSESTABREJECTED);
			tcp_send_dupack(sk, skb);
			goto discard;
		}
		
	}

	
	if (!tcp_sequence(tp, TCP_SKB_CB(skb)->seq, TCP_SKB_CB(skb)->end_seq)) {
		if (!th->rst)
			tcp_send_dupack(sk, skb);
		goto discard;
	}

	
	if (th->rst) {
		tcp_reset(sk);
		goto discard;
	}

	tcp_replace_ts_recent(tp, TCP_SKB_CB(skb)->seq);

	

	
	if (th->syn && !before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt)) {
		if (syn_inerr)
			TCP_INC_STATS_BH(sock_net(sk), TCP_MIB_INERRS);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPABORTONSYN);
		tcp_reset(sk);
		return -1;
	}

	return 1;

discard:
	__kfree_skb(skb);
	return 0;
}

int tcp_rcv_established(struct sock *sk, struct sk_buff *skb,
			const struct tcphdr *th, unsigned int len)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int res;


	tp->rx_opt.saw_tstamp = 0;


	if ((tcp_flag_word(th) & TCP_HP_BITS) == tp->pred_flags &&
	    TCP_SKB_CB(skb)->seq == tp->rcv_nxt &&
	    !after(TCP_SKB_CB(skb)->ack_seq, tp->snd_nxt)) {
		int tcp_header_len = tp->tcp_header_len;


		
		if (tcp_header_len == sizeof(struct tcphdr) + TCPOLEN_TSTAMP_ALIGNED) {
			
			if (!tcp_parse_aligned_timestamp(tp, th))
				goto slow_path;

			
			if ((s32)(tp->rx_opt.rcv_tsval - tp->rx_opt.ts_recent) < 0)
				goto slow_path;

		}

		if (len <= tcp_header_len) {
			
			if (len == tcp_header_len) {
				if (tcp_header_len ==
				    (sizeof(struct tcphdr) + TCPOLEN_TSTAMP_ALIGNED) &&
				    tp->rcv_nxt == tp->rcv_wup)
					tcp_store_ts_recent(tp);

				tcp_ack(sk, skb, 0);
				__kfree_skb(skb);
				tcp_data_snd_check(sk);
				return 0;
			} else { 
				TCP_INC_STATS_BH(sock_net(sk), TCP_MIB_INERRS);
				goto discard;
			}
		} else {
			int eaten = 0;
			int copied_early = 0;

			if (tp->copied_seq == tp->rcv_nxt &&
			    len - tcp_header_len <= tp->ucopy.len) {
#ifdef CONFIG_NET_DMA
				if (tp->ucopy.task == current &&
				    sock_owned_by_user(sk) &&
				    tcp_dma_try_early_copy(sk, skb, tcp_header_len)) {
					copied_early = 1;
					eaten = 1;
				}
#endif
				if (tp->ucopy.task == current &&
				    sock_owned_by_user(sk) && !copied_early) {
					__set_current_state(TASK_RUNNING);

					if (!tcp_copy_to_iovec(sk, skb, tcp_header_len))
						eaten = 1;
				}
				if (eaten) {
					if (tcp_header_len ==
					    (sizeof(struct tcphdr) +
					     TCPOLEN_TSTAMP_ALIGNED) &&
					    tp->rcv_nxt == tp->rcv_wup)
						tcp_store_ts_recent(tp);

					tcp_rcv_rtt_measure_ts(sk, skb);

					__skb_pull(skb, tcp_header_len);
					tp->rcv_nxt = TCP_SKB_CB(skb)->end_seq;
					NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPHPHITSTOUSER);
				}
				if (copied_early)
					tcp_cleanup_rbuf(sk, skb->len);
			}
			if (!eaten) {
				if (tcp_checksum_complete_user(sk, skb))
					goto csum_error;

				if (tcp_header_len ==
				    (sizeof(struct tcphdr) + TCPOLEN_TSTAMP_ALIGNED) &&
				    tp->rcv_nxt == tp->rcv_wup)
					tcp_store_ts_recent(tp);

				tcp_rcv_rtt_measure_ts(sk, skb);

				if ((int)skb->truesize > sk->sk_forward_alloc)
					goto step5;

				NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPHPHITS);

				
				__skb_pull(skb, tcp_header_len);
				__skb_queue_tail(&sk->sk_receive_queue, skb);
				skb_set_owner_r(skb, sk);
				tp->rcv_nxt = TCP_SKB_CB(skb)->end_seq;
			}

			tcp_event_data_recv(sk, skb);

			if (TCP_SKB_CB(skb)->ack_seq != tp->snd_una) {
				
				tcp_ack(sk, skb, FLAG_DATA);
				tcp_data_snd_check(sk);
				if (!inet_csk_ack_scheduled(sk))
					goto no_ack;
			}

			if (!copied_early || tp->rcv_nxt != tp->rcv_wup)
				__tcp_ack_snd_check(sk, 0);
no_ack:
#ifdef CONFIG_NET_DMA
			if (copied_early)
				__skb_queue_tail(&sk->sk_async_wait_queue, skb);
			else
#endif
			if (eaten)
				__kfree_skb(skb);
			else
				sk->sk_data_ready(sk, 0);
			return 0;
		}
	}

slow_path:
	if (len < (th->doff << 2) || tcp_checksum_complete_user(sk, skb))
		goto csum_error;


	res = tcp_validate_incoming(sk, skb, th, 1);
	if (res <= 0)
		return -res;

step5:
	if (th->ack && tcp_ack(sk, skb, FLAG_SLOWPATH) < 0)
		goto discard;

	tcp_rcv_rtt_measure_ts(sk, skb);

	
	tcp_urg(sk, skb, th);

	
	tcp_data_queue(sk, skb);

	tcp_data_snd_check(sk);
	tcp_ack_snd_check(sk);
	return 0;

csum_error:
	TCP_INC_STATS_BH(sock_net(sk), TCP_MIB_INERRS);

discard:
	__kfree_skb(skb);
	return 0;
}
EXPORT_SYMBOL(tcp_rcv_established);

static int tcp_rcv_synsent_state_process(struct sock *sk, struct sk_buff *skb,
					 const struct tcphdr *th, unsigned int len)
{
	const u8 *hash_location;
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct tcp_cookie_values *cvp = tp->cookie_values;
	int saved_clamp = tp->rx_opt.mss_clamp;

	tcp_parse_options(skb, &tp->rx_opt, &hash_location, 0);

	if (th->ack) {
		if (TCP_SKB_CB(skb)->ack_seq != tp->snd_nxt)
			goto reset_and_undo;

		if (tp->rx_opt.saw_tstamp && tp->rx_opt.rcv_tsecr &&
		    !between(tp->rx_opt.rcv_tsecr, tp->retrans_stamp,
			     tcp_time_stamp)) {
			NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_PAWSACTIVEREJECTED);
			goto reset_and_undo;
		}


		if (th->rst) {
			tcp_reset(sk);
			goto discard;
		}

		if (!th->syn)
			goto discard_and_undo;


		TCP_ECN_rcv_synack(tp, th);

		tp->snd_wl1 = TCP_SKB_CB(skb)->seq;
		tcp_ack(sk, skb, FLAG_SLOWPATH);

		tp->rcv_nxt = TCP_SKB_CB(skb)->seq + 1;
		tp->rcv_wup = TCP_SKB_CB(skb)->seq + 1;

		tp->snd_wnd = ntohs(th->window);
		tcp_init_wl(tp, TCP_SKB_CB(skb)->seq);

		if (!tp->rx_opt.wscale_ok) {
			tp->rx_opt.snd_wscale = tp->rx_opt.rcv_wscale = 0;
			tp->window_clamp = min(tp->window_clamp, 65535U);
		}

		if (tp->rx_opt.saw_tstamp) {
			tp->rx_opt.tstamp_ok	   = 1;
			tp->tcp_header_len =
				sizeof(struct tcphdr) + TCPOLEN_TSTAMP_ALIGNED;
			tp->advmss	    -= TCPOLEN_TSTAMP_ALIGNED;
			tcp_store_ts_recent(tp);
		} else {
			tp->tcp_header_len = sizeof(struct tcphdr);
		}

		if (tcp_is_sack(tp) && sysctl_tcp_fack)
			tcp_enable_fack(tp);

		tcp_mtup_init(sk);
		tcp_sync_mss(sk, icsk->icsk_pmtu_cookie);
		tcp_initialize_rcv_mss(sk);

		tp->copied_seq = tp->rcv_nxt;

		if (cvp != NULL &&
		    cvp->cookie_pair_size > 0 &&
		    tp->rx_opt.cookie_plus > 0) {
			int cookie_size = tp->rx_opt.cookie_plus
					- TCPOLEN_COOKIE_BASE;
			int cookie_pair_size = cookie_size
					     + cvp->cookie_desired;

			if (sizeof(cvp->cookie_pair) >= cookie_pair_size) {
				memcpy(&cvp->cookie_pair[cvp->cookie_desired],
				       hash_location, cookie_size);
				cvp->cookie_pair_size = cookie_pair_size;
			}
		}

		smp_mb();
		tcp_set_state(sk, TCP_ESTABLISHED);

		security_inet_conn_established(sk, skb);

		
		icsk->icsk_af_ops->rebuild_header(sk);

		tcp_init_metrics(sk);

		tcp_init_congestion_control(sk);

		tp->lsndtime = tcp_time_stamp;

		tcp_init_buffer_space(sk);

		if (sock_flag(sk, SOCK_KEEPOPEN))
			inet_csk_reset_keepalive_timer(sk, keepalive_time_when(tp));

		if (!tp->rx_opt.snd_wscale)
			__tcp_fast_path_on(tp, tp->snd_wnd);
		else
			tp->pred_flags = 0;

		if (!sock_flag(sk, SOCK_DEAD)) {
			sk->sk_state_change(sk);
			sk_wake_async(sk, SOCK_WAKE_IO, POLL_OUT);
		}

		if (sk->sk_write_pending ||
		    icsk->icsk_accept_queue.rskq_defer_accept ||
		    icsk->icsk_ack.pingpong) {
			inet_csk_schedule_ack(sk);
			icsk->icsk_ack.lrcvtime = tcp_time_stamp;
			icsk->icsk_ack.ato	 = TCP_ATO_MIN;
			tcp_incr_quickack(sk);
			tcp_enter_quickack_mode(sk);
			inet_csk_reset_xmit_timer(sk, ICSK_TIME_DACK,
						  TCP_DELACK_MAX, TCP_RTO_MAX);

discard:
			__kfree_skb(skb);
			return 0;
		} else {
			tcp_send_ack(sk);
		}
		return -1;
	}

	

	if (th->rst) {

		goto discard_and_undo;
	}

	
	if (tp->rx_opt.ts_recent_stamp && tp->rx_opt.saw_tstamp &&
	    tcp_paws_reject(&tp->rx_opt, 0))
		goto discard_and_undo;

	if (th->syn) {
		tcp_set_state(sk, TCP_SYN_RECV);

		if (tp->rx_opt.saw_tstamp) {
			tp->rx_opt.tstamp_ok = 1;
			tcp_store_ts_recent(tp);
			tp->tcp_header_len =
				sizeof(struct tcphdr) + TCPOLEN_TSTAMP_ALIGNED;
		} else {
			tp->tcp_header_len = sizeof(struct tcphdr);
		}

		tp->rcv_nxt = TCP_SKB_CB(skb)->seq + 1;
		tp->rcv_wup = TCP_SKB_CB(skb)->seq + 1;

		tp->snd_wnd    = ntohs(th->window);
		tp->snd_wl1    = TCP_SKB_CB(skb)->seq;
		tp->max_window = tp->snd_wnd;

		TCP_ECN_rcv_syn(tp, th);

		tcp_mtup_init(sk);
		tcp_sync_mss(sk, icsk->icsk_pmtu_cookie);
		tcp_initialize_rcv_mss(sk);

		tcp_send_synack(sk);
#if 0
		return -1;
#else
		goto discard;
#endif
	}

discard_and_undo:
	tcp_clear_options(&tp->rx_opt);
	tp->rx_opt.mss_clamp = saved_clamp;
	goto discard;

reset_and_undo:
	tcp_clear_options(&tp->rx_opt);
	tp->rx_opt.mss_clamp = saved_clamp;
	return 1;
}


int tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb,
			  const struct tcphdr *th, unsigned int len)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);
	int queued = 0;
	int res;

	tp->rx_opt.saw_tstamp = 0;

	switch (sk->sk_state) {
	case TCP_CLOSE:
		goto discard;

	case TCP_LISTEN:
		if (th->ack)
			return 1;

		if (th->rst)
			goto discard;

		if (th->syn) {
			if (th->fin)
				goto discard;
			if (icsk->icsk_af_ops->conn_request(sk, skb) < 0)
				return 1;

			kfree_skb(skb);
			return 0;
		}
		goto discard;

	case TCP_SYN_SENT:
		queued = tcp_rcv_synsent_state_process(sk, skb, th, len);
		if (queued >= 0)
			return queued;

		
		tcp_urg(sk, skb, th);
		__kfree_skb(skb);
		tcp_data_snd_check(sk);
		return 0;
	}

	res = tcp_validate_incoming(sk, skb, th, 0);
	if (res <= 0)
		return -res;

	
	if (th->ack) {
		int acceptable = tcp_ack(sk, skb, FLAG_SLOWPATH) > 0;

		switch (sk->sk_state) {
		case TCP_SYN_RECV:
			if (acceptable) {
				tp->copied_seq = tp->rcv_nxt;
				smp_mb();
				tcp_set_state(sk, TCP_ESTABLISHED);
				sk->sk_state_change(sk);

				if (sk->sk_socket)
					sk_wake_async(sk,
						      SOCK_WAKE_IO, POLL_OUT);

				tp->snd_una = TCP_SKB_CB(skb)->ack_seq;
				tp->snd_wnd = ntohs(th->window) <<
					      tp->rx_opt.snd_wscale;
				tcp_init_wl(tp, TCP_SKB_CB(skb)->seq);

				if (tp->rx_opt.tstamp_ok)
					tp->advmss -= TCPOLEN_TSTAMP_ALIGNED;

				icsk->icsk_af_ops->rebuild_header(sk);

				tcp_init_metrics(sk);

				tcp_init_congestion_control(sk);

				tp->lsndtime = tcp_time_stamp;

				tcp_mtup_init(sk);
				tcp_initialize_rcv_mss(sk);
				tcp_init_buffer_space(sk);
				tcp_fast_path_on(tp);
			} else {
				return 1;
			}
			break;

		case TCP_FIN_WAIT1:
			if (tp->snd_una == tp->write_seq) {
				tcp_set_state(sk, TCP_FIN_WAIT2);
				sk->sk_shutdown |= SEND_SHUTDOWN;
				dst_confirm(__sk_dst_get(sk));

				if (!sock_flag(sk, SOCK_DEAD))
					
					sk->sk_state_change(sk);
				else {
					int tmo;

					if (tp->linger2 < 0 ||
					    (TCP_SKB_CB(skb)->end_seq != TCP_SKB_CB(skb)->seq &&
					     after(TCP_SKB_CB(skb)->end_seq - th->fin, tp->rcv_nxt))) {
						tcp_done(sk);
						NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPABORTONDATA);
						return 1;
					}

					tmo = tcp_fin_time(sk);
					if (tmo > TCP_TIMEWAIT_LEN) {
						inet_csk_reset_keepalive_timer(sk, tmo - TCP_TIMEWAIT_LEN);
					} else if (th->fin || sock_owned_by_user(sk)) {
						inet_csk_reset_keepalive_timer(sk, tmo);
					} else {
						tcp_time_wait(sk, TCP_FIN_WAIT2, tmo);
						goto discard;
					}
				}
			}
			break;

		case TCP_CLOSING:
			if (tp->snd_una == tp->write_seq) {
				tcp_time_wait(sk, TCP_TIME_WAIT, 0);
				goto discard;
			}
			break;

		case TCP_LAST_ACK:
			if (tp->snd_una == tp->write_seq) {
				tcp_update_metrics(sk);
				tcp_done(sk);
				goto discard;
			}
			break;
		}
	} else
		goto discard;

	
	tcp_urg(sk, skb, th);

	
	switch (sk->sk_state) {
	case TCP_CLOSE_WAIT:
	case TCP_CLOSING:
	case TCP_LAST_ACK:
		if (!before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt))
			break;
	case TCP_FIN_WAIT1:
	case TCP_FIN_WAIT2:
		if (sk->sk_shutdown & RCV_SHUTDOWN) {
			if (TCP_SKB_CB(skb)->end_seq != TCP_SKB_CB(skb)->seq &&
			    after(TCP_SKB_CB(skb)->end_seq - th->fin, tp->rcv_nxt)) {
				NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPABORTONDATA);
				tcp_reset(sk);
				return 1;
			}
		}
		
	case TCP_ESTABLISHED:
		tcp_data_queue(sk, skb);
		queued = 1;
		break;
	}

	
	if (sk->sk_state != TCP_CLOSE) {
		tcp_data_snd_check(sk);
		tcp_ack_snd_check(sk);
	}

	if (!queued) {
discard:
		__kfree_skb(skb);
	}
	return 0;
}
EXPORT_SYMBOL(tcp_rcv_state_process);
