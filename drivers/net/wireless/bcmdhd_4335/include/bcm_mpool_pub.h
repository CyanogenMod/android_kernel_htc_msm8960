/*
 * Memory pools library, Public interface
 *
 * API Overview
 *
 * This package provides a memory allocation subsystem based on pools of
 * homogenous objects.
 *
 * Instrumentation is available for reporting memory utilization both
 * on a per-data-structure basis and system wide.
 *
 * There are two main types defined in this API.
 *
 *    pool manager: A singleton object that acts as a factory for
 *                  pool allocators. It also is used for global
 *                  instrumentation, such as reporting all blocks
 *                  in use across all data structures. The pool manager
 *                  creates and provides individual memory pools
 *                  upon request to application code.
 *
 *    memory pool:  An object for allocating homogenous memory blocks.
 *
 * Global identifiers in this module use the following prefixes:
 *    bcm_mpm_*     Memory pool manager
 *    bcm_mp_*      Memory pool
 *
 * There are two main types of memory pools:
 *
 *    prealloc: The contiguous memory block of objects can either be supplied
 *              by the client or malloc'ed by the memory manager. The objects are
 *              allocated out of a block of memory and freed back to the block.
 *
 *    heap:     The memory pool allocator uses the heap (malloc/free) for memory.
 *              In this case, the pool allocator is just providing statistics
 *              and instrumentation on top of the heap, without modifying the heap
 *              allocation implementation.
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id$
 */

#ifndef _BCM_MPOOL_PUB_H
#define _BCM_MPOOL_PUB_H 1

#include <typedefs.h> 



struct osl_info;

struct bcmstrbuf;

struct bcm_mpm_mgr;
typedef struct bcm_mpm_mgr *bcm_mpm_mgr_h;

struct bcm_mp_pool;
typedef struct bcm_mp_pool *bcm_mp_pool_h;


#define BCM_MP_NAMELEN 8


typedef struct bcm_mp_stats {
	char name[BCM_MP_NAMELEN];  
	unsigned int objsz;         
	uint16 nobj;                
	uint16 num_alloc;           
	uint16 high_water;          
	uint16 failed_alloc;        
} bcm_mp_stats_t;



/*
 * bcm_mpm_init() - initialize the whole memory pool system.
 *
 * Parameters:
 *    osh:       INPUT  Operating system handle. Needed for heap memory allocation.
 *    max_pools: INPUT Maximum number of mempools supported.
 *    mgr:       OUTPUT The handle is written with the new pools manager object/handle.
 *
 * Returns:
 *    BCME_OK     Object initialized successfully. May be used.
 *    BCME_NOMEM  Initialization failed due to no memory. Object must not be used.
 */
int bcm_mpm_init(struct osl_info *osh, int max_pools, bcm_mpm_mgr_h *mgrp);


int bcm_mpm_deinit(bcm_mpm_mgr_h *mgrp);

int bcm_mpm_create_prealloc_pool(bcm_mpm_mgr_h mgr,
                                 unsigned int obj_sz,
                                 int nobj,
                                 void *memstart,
                                 unsigned int memsize,
                                 char poolname[BCM_MP_NAMELEN],
                                 bcm_mp_pool_h *newp);


int bcm_mpm_delete_prealloc_pool(bcm_mpm_mgr_h mgr, bcm_mp_pool_h *poolp);

int bcm_mpm_create_heap_pool(bcm_mpm_mgr_h mgr, unsigned int obj_sz,
                             char poolname[BCM_MP_NAMELEN],
                             bcm_mp_pool_h *newp);


int bcm_mpm_delete_heap_pool(bcm_mpm_mgr_h mgr, bcm_mp_pool_h *poolp);


int bcm_mpm_stats(bcm_mpm_mgr_h mgr, bcm_mp_stats_t *stats, int *nentries);


int bcm_mpm_dump(bcm_mpm_mgr_h mgr, struct bcmstrbuf *b);


int bcm_mpm_get_obj_size(bcm_mpm_mgr_h mgr, unsigned int obj_sz, unsigned int *padded_obj_sz);




void* bcm_mp_alloc(bcm_mp_pool_h pool);

int bcm_mp_free(bcm_mp_pool_h pool, void *objp);

int bcm_mp_stats(bcm_mp_pool_h pool, bcm_mp_stats_t *stats);


int bcm_mp_dump(bcm_mp_pool_h pool, struct bcmstrbuf *b);


#endif 
