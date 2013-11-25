/* SCTP kernel implementation
 * (C) Copyright IBM Corp. 2001, 2004
 * Copyright (c) 1999-2000 Cisco, Inc.
 * Copyright (c) 1999-2001 Motorola, Inc.
 * Copyright (c) 2001 Intel Corp.
 *
 * This file is part of the SCTP kernel implementation
 *
 * These are the definitions needed for the tsnmap type.  The tsnmap is used
 * to track out of order TSNs received.
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
 *   Jon Grimm             <jgrimm@us.ibm.com>
 *   La Monte H.P. Yarroll <piggy@acm.org>
 *   Karl Knutson          <karl@athena.chicago.il.us>
 *   Sridhar Samudrala     <sri@us.ibm.com>
 *
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */
#include <net/sctp/constants.h>

#ifndef __sctp_tsnmap_h__
#define __sctp_tsnmap_h__

struct sctp_tsnmap {
	unsigned long *tsn_map;

	
	__u32 base_tsn;

	__u32 cumulative_tsn_ack_point;

	
	__u32 max_tsn_seen;

	__u16 len;

	
	__u16 pending_data;

	__u16 num_dup_tsns;
	__be32 dup_tsns[SCTP_MAX_DUP_TSNS];
};

struct sctp_tsnmap_iter {
	__u32 start;
};

struct sctp_tsnmap *sctp_tsnmap_init(struct sctp_tsnmap *, __u16 len,
				     __u32 initial_tsn, gfp_t gfp);

void sctp_tsnmap_free(struct sctp_tsnmap *map);

int sctp_tsnmap_check(const struct sctp_tsnmap *, __u32 tsn);

int sctp_tsnmap_mark(struct sctp_tsnmap *, __u32 tsn);

void sctp_tsnmap_skip(struct sctp_tsnmap *map, __u32 tsn);

static inline __u32 sctp_tsnmap_get_ctsn(const struct sctp_tsnmap *map)
{
	return map->cumulative_tsn_ack_point;
}

static inline __u32 sctp_tsnmap_get_max_tsn_seen(const struct sctp_tsnmap *map)
{
	return map->max_tsn_seen;
}

static inline __u16 sctp_tsnmap_num_dups(struct sctp_tsnmap *map)
{
	return map->num_dup_tsns;
}

static inline __be32 *sctp_tsnmap_get_dups(struct sctp_tsnmap *map)
{
	map->num_dup_tsns = 0;
	return map->dup_tsns;
}

__u16 sctp_tsnmap_num_gabs(struct sctp_tsnmap *map,
			   struct sctp_gap_ack_block *gabs);

__u16 sctp_tsnmap_pending(struct sctp_tsnmap *map);

static inline int sctp_tsnmap_has_gap(const struct sctp_tsnmap *map)
{
	return map->cumulative_tsn_ack_point != map->max_tsn_seen;
}

static inline void sctp_tsnmap_mark_dup(struct sctp_tsnmap *map, __u32 tsn)
{
	if (map->num_dup_tsns < SCTP_MAX_DUP_TSNS)
		map->dup_tsns[map->num_dup_tsns++] = htonl(tsn);
}

void sctp_tsnmap_renege(struct sctp_tsnmap *, __u32 tsn);

int sctp_tsnmap_has_gap(const struct sctp_tsnmap *);

#endif 
