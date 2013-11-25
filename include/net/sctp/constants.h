/* SCTP kernel implementation
 * (C) Copyright IBM Corp. 2001, 2004
 * Copyright (c) 1999-2000 Cisco, Inc.
 * Copyright (c) 1999-2001 Motorola, Inc.
 * Copyright (c) 2001 Intel Corp.
 *
 * This file is part of the SCTP kernel implementation
 *
 * This SCTP implementation is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This SCTP implementation is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *                 ************************
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Please send any bug reports or fixes you make to the
 * email address(es):
 *    lksctp developers <lksctp-developers@lists.sourceforge.net>
 *
 * Or submit a bug report through the following website:
 *    http://www.sf.net/projects/lksctp
 *
 * Written or modified by:
 *   La Monte H.P. Yarroll <piggy@acm.org>
 *   Karl Knutson          <karl@athena.chicago.il.us>
 *   Randall Stewart       <randall@stewart.chicago.il.us>
 *   Ken Morneau           <kmorneau@cisco.com>
 *   Qiaobing Xie          <qxie1@motorola.com>
 *   Xingang Guo           <xingang.guo@intel.com>
 *   Sridhar Samudrala     <samudrala@us.ibm.com>
 *   Daisy Chang           <daisyc@us.ibm.com>
 *
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */

#ifndef __sctp_constants_h__
#define __sctp_constants_h__

#include <linux/sctp.h>
#include <linux/ipv6.h> 
#include <net/sctp/user.h>
#include <net/tcp_states.h>  

enum { SCTP_MAX_STREAM = 0xffff };
enum { SCTP_DEFAULT_OUTSTREAMS = 10 };
enum { SCTP_DEFAULT_INSTREAMS = SCTP_MAX_STREAM };

#define SCTP_CID_BASE_MAX		SCTP_CID_SHUTDOWN_COMPLETE

#define SCTP_NUM_BASE_CHUNK_TYPES	(SCTP_CID_BASE_MAX + 1)

#define SCTP_NUM_ADDIP_CHUNK_TYPES	2

#define SCTP_NUM_PRSCTP_CHUNK_TYPES	1

#define SCTP_NUM_AUTH_CHUNK_TYPES	1

#define SCTP_NUM_CHUNK_TYPES		(SCTP_NUM_BASE_CHUNK_TYPES + \
					 SCTP_NUM_ADDIP_CHUNK_TYPES +\
					 SCTP_NUM_PRSCTP_CHUNK_TYPES +\
					 SCTP_NUM_AUTH_CHUNK_TYPES)

typedef enum {

	SCTP_EVENT_T_CHUNK = 1,
	SCTP_EVENT_T_TIMEOUT,
	SCTP_EVENT_T_OTHER,
	SCTP_EVENT_T_PRIMITIVE

} sctp_event_t;


typedef enum {
	SCTP_EVENT_TIMEOUT_NONE = 0,
	SCTP_EVENT_TIMEOUT_T1_COOKIE,
	SCTP_EVENT_TIMEOUT_T1_INIT,
	SCTP_EVENT_TIMEOUT_T2_SHUTDOWN,
	SCTP_EVENT_TIMEOUT_T3_RTX,
	SCTP_EVENT_TIMEOUT_T4_RTO,
	SCTP_EVENT_TIMEOUT_T5_SHUTDOWN_GUARD,
	SCTP_EVENT_TIMEOUT_HEARTBEAT,
	SCTP_EVENT_TIMEOUT_SACK,
	SCTP_EVENT_TIMEOUT_AUTOCLOSE,
} sctp_event_timeout_t;

#define SCTP_EVENT_TIMEOUT_MAX		SCTP_EVENT_TIMEOUT_AUTOCLOSE
#define SCTP_NUM_TIMEOUT_TYPES		(SCTP_EVENT_TIMEOUT_MAX + 1)

typedef enum {
	SCTP_EVENT_NO_PENDING_TSN = 0,
	SCTP_EVENT_ICMP_PROTO_UNREACH,
} sctp_event_other_t;

#define SCTP_EVENT_OTHER_MAX		SCTP_EVENT_ICMP_PROTO_UNREACH
#define SCTP_NUM_OTHER_TYPES		(SCTP_EVENT_OTHER_MAX + 1)

typedef enum {
	SCTP_PRIMITIVE_ASSOCIATE = 0,
	SCTP_PRIMITIVE_SHUTDOWN,
	SCTP_PRIMITIVE_ABORT,
	SCTP_PRIMITIVE_SEND,
	SCTP_PRIMITIVE_REQUESTHEARTBEAT,
	SCTP_PRIMITIVE_ASCONF,
} sctp_event_primitive_t;

#define SCTP_EVENT_PRIMITIVE_MAX	SCTP_PRIMITIVE_ASCONF
#define SCTP_NUM_PRIMITIVE_TYPES	(SCTP_EVENT_PRIMITIVE_MAX + 1)


typedef union {
	sctp_cid_t chunk;
	sctp_event_timeout_t timeout;
	sctp_event_other_t other;
	sctp_event_primitive_t primitive;
} sctp_subtype_t;

#define SCTP_SUBTYPE_CONSTRUCTOR(_name, _type, _elt) \
static inline sctp_subtype_t	\
SCTP_ST_## _name (_type _arg)		\
{ sctp_subtype_t _retval; _retval._elt = _arg; return _retval; }

SCTP_SUBTYPE_CONSTRUCTOR(CHUNK,		sctp_cid_t,		chunk)
SCTP_SUBTYPE_CONSTRUCTOR(TIMEOUT,	sctp_event_timeout_t,	timeout)
SCTP_SUBTYPE_CONSTRUCTOR(OTHER,		sctp_event_other_t,	other)
SCTP_SUBTYPE_CONSTRUCTOR(PRIMITIVE,	sctp_event_primitive_t,	primitive)


#define sctp_chunk_is_data(a) (a->chunk_hdr->type == SCTP_CID_DATA)

#define SCTP_DATA_SNDSIZE(c) ((int)((unsigned long)(c->chunk_end)\
		       		- (unsigned long)(c->chunk_hdr)\
				- sizeof(sctp_data_chunk_t)))

typedef enum {

	SCTP_IERROR_NO_ERROR	        = 0,
	SCTP_IERROR_BASE		= 1000,
	SCTP_IERROR_NO_COOKIE,
	SCTP_IERROR_BAD_SIG,
	SCTP_IERROR_STALE_COOKIE,
	SCTP_IERROR_NOMEM,
	SCTP_IERROR_MALFORMED,
	SCTP_IERROR_BAD_TAG,
	SCTP_IERROR_BIG_GAP,
	SCTP_IERROR_DUP_TSN,
	SCTP_IERROR_HIGH_TSN,
	SCTP_IERROR_IGNORE_TSN,
	SCTP_IERROR_NO_DATA,
	SCTP_IERROR_BAD_STREAM,
	SCTP_IERROR_BAD_PORTS,
	SCTP_IERROR_AUTH_BAD_HMAC,
	SCTP_IERROR_AUTH_BAD_KEYID,
	SCTP_IERROR_PROTO_VIOLATION,
	SCTP_IERROR_ERROR,
	SCTP_IERROR_ABORT,
} sctp_ierror_t;



typedef enum {

	SCTP_STATE_CLOSED		= 0,
	SCTP_STATE_COOKIE_WAIT		= 1,
	SCTP_STATE_COOKIE_ECHOED	= 2,
	SCTP_STATE_ESTABLISHED		= 3,
	SCTP_STATE_SHUTDOWN_PENDING	= 4,
	SCTP_STATE_SHUTDOWN_SENT	= 5,
	SCTP_STATE_SHUTDOWN_RECEIVED	= 6,
	SCTP_STATE_SHUTDOWN_ACK_SENT	= 7,

} sctp_state_t;

#define SCTP_STATE_MAX			SCTP_STATE_SHUTDOWN_ACK_SENT
#define SCTP_STATE_NUM_STATES		(SCTP_STATE_MAX + 1)

typedef enum {
	SCTP_SS_CLOSED         = TCP_CLOSE,
	SCTP_SS_LISTENING      = TCP_LISTEN,
	SCTP_SS_ESTABLISHING   = TCP_SYN_SENT,
	SCTP_SS_ESTABLISHED    = TCP_ESTABLISHED,
	SCTP_SS_CLOSING        = TCP_CLOSING,
} sctp_sock_state_t;

const char *sctp_cname(const sctp_subtype_t);	
const char *sctp_oname(const sctp_subtype_t);	
const char *sctp_tname(const sctp_subtype_t);	
const char *sctp_pname(const sctp_subtype_t);	

extern const char *const sctp_state_tbl[];
extern const char *const sctp_evttype_tbl[];
extern const char *const sctp_status_tbl[];

enum { SCTP_MAX_CHUNK_LEN = ((1<<16) - sizeof(__u32)) };

enum { SCTP_ARBITRARY_COOKIE_ECHO_LEN = 200 };

#define SCTP_TSN_MAP_INITIAL BITS_PER_LONG
#define SCTP_TSN_MAP_INCREMENT SCTP_TSN_MAP_INITIAL
#define SCTP_TSN_MAP_SIZE 4096

enum { SCTP_MIN_PMTU = 576 };
enum { SCTP_MAX_DUP_TSNS = 16 };
enum { SCTP_MAX_GABS = 16 };

#define SCTP_DEFAULT_TIMEOUT_HEARTBEAT	(30*1000)

#define SCTP_DEFAULT_TIMEOUT_SACK	(200)

#define SCTP_RTO_INITIAL	(3 * 1000)
#define SCTP_RTO_MIN		(1 * 1000)
#define SCTP_RTO_MAX		(60 * 1000)

#define SCTP_RTO_ALPHA          3   
#define SCTP_RTO_BETA           2   

#define SCTP_DEFAULT_MAX_BURST		4

#define SCTP_CLOCK_GRANULARITY	1	

#define SCTP_DEFAULT_COOKIE_LIFE	(60 * 1000) 

#define SCTP_DEFAULT_MINWINDOW	1500	
#define SCTP_DEFAULT_MAXWINDOW	65535	
#define SCTP_DEFAULT_RWND_SHIFT  4	
#define SCTP_DEFAULT_MAXSEGMENT 1500	
#define SCTP_DEFAULT_MINSEGMENT 512	
#define SCTP_HOW_MANY_SECRETS 2		
#define SCTP_SECRET_SIZE 32		

#define SCTP_SIGNATURE_SIZE 20	        

#define SCTP_COOKIE_MULTIPLE 32 

#if defined (CONFIG_SCTP_HMAC_MD5)
#define SCTP_COOKIE_HMAC_ALG "hmac(md5)"
#elif defined (CONFIG_SCTP_HMAC_SHA1)
#define SCTP_COOKIE_HMAC_ALG "hmac(sha1)"
#else
#define SCTP_COOKIE_HMAC_ALG NULL
#endif

typedef enum {
	SCTP_XMIT_OK,
	SCTP_XMIT_PMTU_FULL,
	SCTP_XMIT_RWND_FULL,
	SCTP_XMIT_NAGLE_DELAY,
} sctp_xmit_t;

typedef enum {
	SCTP_TRANSPORT_UP,
	SCTP_TRANSPORT_DOWN,
} sctp_transport_cmd_t;

typedef enum {
	SCTP_SCOPE_GLOBAL,		
	SCTP_SCOPE_PRIVATE,		
	SCTP_SCOPE_LINK,		
	SCTP_SCOPE_LOOPBACK,		
	SCTP_SCOPE_UNUSABLE,		
} sctp_scope_t;

typedef enum {
	SCTP_SCOPE_POLICY_DISABLE,	
	SCTP_SCOPE_POLICY_ENABLE,	
	SCTP_SCOPE_POLICY_PRIVATE,	
	SCTP_SCOPE_POLICY_LINK,		
} sctp_scope_policy_t;

#define IS_IPV4_UNUSABLE_ADDRESS(a)	    \
	((htonl(INADDR_BROADCAST) == a) ||  \
	 ipv4_is_multicast(a) ||	    \
	 ipv4_is_zeronet(a) ||		    \
	 ipv4_is_test_198(a) ||		    \
	 ipv4_is_anycast_6to4(a))

#define SCTP_ADDR6_ALLOWED	0x00000001	
#define SCTP_ADDR4_PEERSUPP	0x00000002	
#define SCTP_ADDR6_PEERSUPP	0x00000004	

typedef enum {
	SCTP_RTXR_T3_RTX,
	SCTP_RTXR_FAST_RTX,
	SCTP_RTXR_PMTUD,
	SCTP_RTXR_T1_RTX,
} sctp_retransmit_reason_t;

typedef enum {
	SCTP_LOWER_CWND_T3_RTX,
	SCTP_LOWER_CWND_FAST_RTX,
	SCTP_LOWER_CWND_ECNE,
	SCTP_LOWER_CWND_INACTIVE,
} sctp_lower_cwnd_t;



enum {
	SCTP_AUTH_HMAC_ID_RESERVED_0,
	SCTP_AUTH_HMAC_ID_SHA1,
	SCTP_AUTH_HMAC_ID_RESERVED_2,
#if defined (CONFIG_CRYPTO_SHA256) || defined (CONFIG_CRYPTO_SHA256_MODULE)
	SCTP_AUTH_HMAC_ID_SHA256,
#endif
	__SCTP_AUTH_HMAC_MAX
};

#define SCTP_AUTH_HMAC_ID_MAX	__SCTP_AUTH_HMAC_MAX - 1
#define SCTP_AUTH_NUM_HMACS 	__SCTP_AUTH_HMAC_MAX
#define SCTP_SHA1_SIG_SIZE 20
#define SCTP_SHA256_SIG_SIZE 32

#define SCTP_NUM_NOAUTH_CHUNKS	4
#define SCTP_AUTH_MAX_CHUNKS	(SCTP_NUM_CHUNK_TYPES - SCTP_NUM_NOAUTH_CHUNKS)

#define SCTP_AUTH_RANDOM_LENGTH 32

#endif 
