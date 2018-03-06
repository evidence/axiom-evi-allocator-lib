/*!
 * \file evi_lmm.h
 *
 * \version     v1.1
 * \date        2016-09-23
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */

/*
 * Copyright (c) 1995, 1998 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 *
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#ifndef EVI_LMM_H
#define EVI_LMM_H

/**
 * \addtogroup EVI_LMM
 *
 * \{
 */

#include <stdint.h>
#include <stddef.h>

typedef uint32_t evi_region_flags_t;
typedef int      evi_region_prio_t;

/** \brief Free list node */
struct evi_freelist_node {
	/** Next free list node */
	struct evi_freelist_node *next;
	/** Size of the free area */
	size_t size;
};

/** \brief Region descriptor */
typedef struct evi_region_desc {
	/** Pointer to the following region */
	struct evi_region_desc *next;

	/** List of free memory blocks in this region. */
	struct evi_freelist_node *nodes;

	/** Start address of the region */
	uintptr_t start;
	/** End address of the region */
	uintptr_t end;

	/** Region attribute */
	evi_region_flags_t flags;
	/** Region allocation priority */
	evi_region_prio_t prio;

	/** Actual region free space */
	size_t free;
} evi_region_desc_t;

typedef struct evi_lmm {
	/** List of registered regions */
	evi_region_desc_t *regions;
	/** Private buffer used to store "unnamed" regions */
	char workspace[4096 - sizeof(evi_region_desc_t *)];
} evi_lmm_t;

/* TODO: check power of 2???*/
#define EVI_LMM_ALIGN_SIZE      sizeof(struct evi_freelist_node)
#define EVI_LMM_ALIGN_MASK      (EVI_LMM_ALIGN_SIZE - 1)

#define EVI_PRIVATE_MEM         1
#define EVI_SHARE_MEM           2

int evi_lmm_init(evi_lmm_t *lmm);
int evi_lmm_add_region(evi_lmm_t *lmm, evi_region_desc_t *region,
		       void *addr, size_t size, evi_region_flags_t flags,
		       evi_region_prio_t prio);

int evi_lmm_free(evi_lmm_t *lmm, void *block, size_t size);
void *evi_lmm_alloc(evi_lmm_t *lmm, size_t size, evi_region_flags_t flags);
size_t evi_lmm_avail(evi_lmm_t *lmm, evi_region_flags_t flags);
void *evi_lmm_alloc_gen(evi_lmm_t *lmm, size_t size,
			evi_region_flags_t flags, int align_bits,
			uintptr_t align_ofs, uintptr_t in_min,
			size_t in_size);

void evi_lmm_dump_regions(evi_lmm_t *lmm);
void evi_lmm_dump(evi_lmm_t *lmm);

int evi_lmm_add_reg(evi_lmm_t *lmm, void *addr, size_t size,
		    evi_region_flags_t flags, evi_region_prio_t prio);

#define REMOVED_API
#undef REMOVED_API

#ifdef REMOVED_API
/*REMOVED API*/
int evi_lmm_add_free(evi_lmm_t *lmm, void *block, size_t size);
#endif

/** \} */

#endif /* EVI_LMM_H */
