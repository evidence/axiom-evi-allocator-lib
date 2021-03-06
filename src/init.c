/*!
 * \file init.c
 *
 * \version     v1.2
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

#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>

#include <assert.h>

#include <evi_lmm.h>
#include <evi_err.h>

#include <freeidx_list.h>

/**
 * \cond INTERNAL_MACRO
 */
/*
#define DEBUG_NO_TIME
*/
/**
 * \endcond
 */
#include <debug.h>

/**
 * \defgroup EVI_LMM
 *
 * \{
 */

static void evi_lmm_free_in_region(struct evi_region_desc *reg,
				   void *block, size_t size);

static inline evi_region_desc_t *get_region_pool(evi_lmm_t *lmm);
static inline freeidx_list_t *get_freeidx_list(evi_lmm_t *lmm);

static inline int is_auto_region(evi_lmm_t *lmm, evi_region_desc_t *r)
{
	int res;
	freeidx_list_t *fl = get_freeidx_list(lmm);
	uintptr_t start = (uintptr_t)get_region_pool(lmm);
	uintptr_t end = start + fl->n_elem * sizeof(evi_region_desc_t);
	uintptr_t ra = (uintptr_t)r;

	res = (start <= ra) && (ra < end);

	return !!res;
}

/**
 * \brief Initialize axiom list memory manager handler
 *
 * \param lmm   Handler to initialize
 */
int evi_lmm_init(evi_lmm_t *lmm)
{
	int n_elem;
	evi_region_desc_t *region_pool;
	freeidx_list_t *fl;

	lmm->regions = NULL;

	n_elem = freeidx_list_elem_for_memblock(sizeof(lmm->workspace),
						sizeof(evi_region_desc_t));
	if (n_elem <= 0) {
		DBG("No room for region descriptors (%d <= 0!)\n", n_elem);
		return EVI_LMM_INVALID_MEM_DESC;
	}

	DBG("elem=%d\n", n_elem);
	fl = freeidx_list_init_from_buffer(lmm->workspace,
					   FREELIST_SPACE(n_elem));
	if (fl == NULL) {
		DBG("No room for %d region descriptors\n", n_elem);
		return EVI_LMM_INVALID_MEM_DESC;
	}

	region_pool = get_region_pool(lmm);
	fl = get_freeidx_list(lmm);
	DBG("WS:%p fl:%p rp:%p\n", lmm->workspace, fl, region_pool);

	return EVI_LMM_OK;
}

static inline void dump_region(struct evi_region_desc *r)
{
	fprintf(stderr, "R:%p prio:%d m:0x%"PRIxPTR" M:0x%"PRIxPTR" sz:%zu"
		" F:%zu diff:%zu nodes:%p\n",
		r, r->prio, r->start, r->end, r->end - r->start,
		r->free, (r->end - r->start), r->nodes);
}

/**
 * \cond INTERNAL_MACRO
 */
#define container_of(ptr, type, member) ({                        \
	const typeof(((type *)0)->member) *member_ptr = (ptr);  \
	(type *)((char *)member_ptr - offsetof(type, member)); })
/**
 * \endcond
 */

static int evi_lmm_merge_region(evi_lmm_t *lmm,
				evi_region_desc_t *tokeep,
				evi_region_desc_t *tomerge)
{
	DBG("Comparing K:%p M:%p\n", tokeep, tomerge);
	DBG("K:%p s:0x%"PRIxPTR" - e:0x%"PRIxPTR"\n",
	    tokeep, tokeep->start, tokeep->end);
	DBG("M:%p s:0x%"PRIxPTR" - e:0x%"PRIxPTR"\n",
	    tomerge, tomerge->start, tomerge->end);

	if (tokeep->flags != tomerge->flags) {
		DBG("Different flags\n");
		return -1;
	}

	if (tokeep->prio != tomerge->prio) {
		DBG("Different priorities\n");
		return -1;
	}

	if (!is_auto_region(lmm, tokeep)
	    || !is_auto_region(lmm, tomerge)) {
		DBG("No merge-able regions T:%d M:%d\n",
		    is_auto_region(lmm, tokeep),
		    is_auto_region(lmm, tomerge));
		return 0;
	}

	if (tokeep->end == tomerge->start) {
		struct evi_freelist_node *n;

		DBG("Can be merged\n");
		tokeep->end = tomerge->end;
		tokeep->free += tomerge->free;
		tokeep->next = tomerge->next;

		for (n = tokeep->nodes; n->next != NULL; n = n->next)
			;
		assert(n != NULL);

		if ((uintptr_t)n + n->size == (uintptr_t)tomerge->nodes) {
			int idx;
			evi_region_desc_t *region_pool = get_region_pool(lmm);
			freeidx_list_t *fl = get_freeidx_list(lmm);

			/* last free zone of tokeep is contiguous with the
			 * first free zone of tomerge.
			 */
			n->size += tomerge->nodes->size;
			n->next = tomerge->nodes->next;
			DBG("Merged NODES (%zu)\n", tomerge->nodes->size);
			DBG("T:%p B:%p\n", tomerge, region_pool);
			idx = ((uintptr_t)tomerge -
			       (uintptr_t)region_pool)
			      / sizeof(*tomerge);
			freeidx_list_free_idx(fl, idx);
		} else {
			n->next = tomerge->nodes;
			DBG("NO Merged NODES\n");
		}

		DBG("n=%p n->next=%p n+size=%"PRIxPTR"\n", n, n->next,
		    (uintptr_t)n + n->size);
		DBG("tomerge->nodes=%p\n", tomerge->nodes);

	} else {
		DBG("NO merge\n");
	}

	return 0;
}

static inline freeidx_list_t *get_freeidx_list(evi_lmm_t *lmm)
{
	freeidx_list_t *fl = (freeidx_list_t *)(lmm->workspace);
	return fl;
}

static inline evi_region_desc_t *get_region_pool(evi_lmm_t *lmm)
{
	evi_region_desc_t *rp;
	freeidx_list_t *fl = get_freeidx_list(lmm);

	rp = (evi_region_desc_t *)((uintptr_t)&(lmm->workspace[0])
				   + FREELIST_SPACE(fl->n_elem));

	return rp;
}

/**
 * \brief Adds a memory region to the pool of the passed hanlder
 *
 * \param lmm     Memory region pool handler
 * \param addr    Start virtual address of the memory region
 * \param size    Size of the memory region
 * \param flags   Memory region bitmap flags
 * \param prio    Priority of the memory region inside the memory region pool
 *
 * \return        EVI_LMM_OK if the memory region is successfully added,
 *                otherwise an EVI_LMM_* error \see evi_err.h
 */
int evi_lmm_add_reg(evi_lmm_t *lmm, void *addr, size_t size,
		    evi_region_flags_t flags, evi_region_prio_t prio)
{
	evi_region_desc_t *region;
	freeidx_list_t *fl = get_freeidx_list(lmm);
	evi_region_desc_t *region_pool = get_region_pool(lmm);
	int idx = freeidx_list_alloc_idx(fl);

	if (idx == FREELIST_INVALID_IDX)
		return EVI_LMM_INVALID_MEM_DESC;

	region = &(region_pool[idx]);

	return evi_lmm_add_region(lmm, region, addr, size, flags, prio);
}

void evi_lmm_dump_regions(evi_lmm_t *lmm)
{
	int cnt = 0;
	struct evi_region_desc *r;

	for (r = lmm->regions; r; r = r->next) {
		dump_region(r);
		++cnt;
	}
	fprintf(stderr, "Num regions: %d\n", cnt);
}

void evi_lmm_dump(evi_lmm_t *lmm)
{
	struct evi_region_desc *reg;

	fprintf(stderr, "%s(lmm=%p)\n", __func__, lmm);

	for (reg = lmm->regions; reg; reg = reg->next) {
		struct evi_freelist_node *node;
		size_t free_check;

		fprintf(stderr,
		" region %08lx-%08lx size=%zu flags=%08lx pri=%d free=%zu\n",
			reg->start, reg->end, reg->end - reg->start,
			(unsigned long)reg->flags, reg->prio, reg->free);

		/* CHECKREGPTR(reg); */

		free_check = 0;
		for (node = reg->nodes; node; node = node->next) {
			fprintf(stderr,
				"  node %p-0x%08"PRIxPTR" size=%zu next=%p\n",
				node, (uintptr_t)node + node->size, node->size,
				node->next);

			assert(((uintptr_t)node & EVI_LMM_ALIGN_MASK) == 0);
			assert((node->size & EVI_LMM_ALIGN_MASK) == 0);
			assert(node->size >= sizeof(*node));
			assert((node->next == 0) || (node->next > node));
			assert((uintptr_t)node < reg->end);

			free_check += node->size;
		}

		fprintf(stderr, " free_check=%zu\n", free_check);
		assert(reg->free == free_check);
	}

	fprintf(stderr, "%s done\n", __func__);
}

static void evi_lmm_free_in_region(struct evi_region_desc *reg,
				     void *block, size_t size)
{
	struct evi_freelist_node *prevnode, *nextnode;
	struct evi_freelist_node *node = (struct evi_freelist_node *)
				((uintptr_t)block & ~EVI_LMM_ALIGN_MASK);

	size = (((uintptr_t)block & EVI_LMM_ALIGN_MASK) + size
		+ EVI_LMM_ALIGN_MASK)
		& ~EVI_LMM_ALIGN_MASK;

	/* Record the newly freed space in the region's free space counter.  */
	reg->free += size;
	assert(reg->free <= reg->end - reg->start);

	/* Now find the location in that region's free list
	 * at which to add the node.
	 */
	for (prevnode = 0, nextnode = reg->nodes;
	     (nextnode != 0) && (nextnode < node);
	     prevnode = nextnode, nextnode = nextnode->next);

	/* Coalesce the new free chunk into the previous chunk if possible.  */
	if ((prevnode) &&
	    ((uintptr_t)prevnode + prevnode->size >= (uintptr_t)node)) {
		assert((uintptr_t)prevnode + prevnode->size == (uintptr_t)node);

		/* Coalesce prevnode with nextnode if possible.  */
		if (((uintptr_t)nextnode)
		    && ((uintptr_t)node + size >= (uintptr_t)nextnode)) {
			assert((uintptr_t)node + size == (uintptr_t)nextnode);

			prevnode->size += size + nextnode->size;
			prevnode->next = nextnode->next;
		} else {
			/* Not possible -
			 * just grow prevnode around newly freed memory.
			 */
			prevnode->size += size;
		}
	} else {
		/* Insert the new node into the free list.  */
		if (prevnode)
			prevnode->next = node;
		else
			reg->nodes = node;

		/* Try coalescing the new node with the nextnode.  */
		if ((nextnode) &&
		    (uintptr_t)node + size >= (uintptr_t)nextnode) {
			node->size = size + nextnode->size;
			node->next = nextnode->next;
		} else {
			node->size = size;
			node->next = nextnode;
		}
	}
}

/**
 * \brief Free a memory block
 *
 * \param lmm     Memory region pool handler
 * \param block   Virtual address of the memory block
 * \param size    Size of the memory block to free
 *
 * \return        EVI_LMM_OK if the memory region is successfully added,
 *                otherwise an EVI_LMM_* error \see evi_err.h
 */
int evi_lmm_free(evi_lmm_t *lmm, void *block, size_t size)
{
	struct evi_region_desc *reg;
	struct evi_freelist_node *node = (struct evi_freelist_node *)
				((uintptr_t)block & ~EVI_LMM_ALIGN_MASK);
	/*struct evi_freelist_node *prevnode, *nextnode;*/

	assert(lmm != 0);
	if (lmm == NULL)
		return EVI_LMM_INVALID_MEM_DESC;
	if (block == NULL)
		return EVI_LMM_INVALID_BLOCK;
	if (size <= 0)
		return EVI_LMM_INVALID_SIZE;

	/* First find the region to add this block to.  */
	for (reg = lmm->regions; ; reg = reg->next) {
		if (reg == 0)
			return EVI_LMM_INVALID_BLOCK;

		/* TODO: */
		/*CHECKREGPTR(reg);*/

		if (((uintptr_t)node >= reg->start)
		    && ((uintptr_t)node < reg->end))
			break;
	}

	evi_lmm_free_in_region(reg, block, size);

	return EVI_LMM_OK;
}

static void *evi_lmm_alloc_find_node(struct evi_region_desc *reg,
				       size_t size)
{
	struct evi_freelist_node **nodep, *node;

	for (nodep = &reg->nodes; *nodep != 0; nodep = &node->next) {
		node = *nodep;

		assert(((uintptr_t)node & EVI_LMM_ALIGN_MASK) == 0);
		assert((node->size & EVI_LMM_ALIGN_MASK) == 0);
		assert((node->next == 0) || (node->next > node));
		assert((uintptr_t)node < reg->end);

		if (node->size < size)
			continue;

		if (node->size > size) {
			struct evi_freelist_node *newnode;

			/* Split the node and return its head */
			newnode = (struct evi_freelist_node *)
					((void *)node + size);
			newnode->next = node->next;
			newnode->size = node->size - size;
			*nodep = newnode;
		} else {
			/* Remove and return the entire node. */
			*nodep = node->next;
		}

		/* Adjust the region's free memory counter.  */
		assert(reg->free >= size);
		reg->free -= size;

		return (void *)node;
	}

	return NULL;
}

/**
 * \brief Allocates memory
 *
 * \param lmm     The memory pool where to allocate.
 * \param size    Size of needed memory.
 * \param flags   The memory type required for this allocation. For each bit set
 *                in the flags parameter, the corresponding bit in a region's
 *                flags word must also be set in order for the region to be
 *                considered for allocation. If the flags parameter is zero,
 *                memory will be allocated from any region.
 *
 * \return        Returns a pointer to the allocated memory or NULL
 *                if there is no avalilable memory block.
 */
void *evi_lmm_alloc(evi_lmm_t *lmm, size_t size, evi_region_flags_t flags)
{
	struct evi_region_desc *reg;

	if (lmm == NULL || size <= 0)
		return NULL;

	size = (size + EVI_LMM_ALIGN_MASK) & ~EVI_LMM_ALIGN_MASK;

	for (reg = lmm->regions; reg; reg = reg->next) {
		void *addr;

		/*TODO:*/
		/* CHECKREGPTR(reg); */

		if (flags & ~reg->flags)
			continue;

		addr = evi_lmm_alloc_find_node(reg, size);
		if (addr != NULL)
			return addr;
	}

	return NULL;
}

size_t evi_lmm_avail(evi_lmm_t *lmm, evi_region_flags_t flags)
{
	struct evi_region_desc *reg;
	size_t avail = 0;

	for (reg = lmm->regions; reg; reg = reg->next) {
		/*TODO:*/
		/*CHECKREGPTR(reg);*/

		/* Don't count inapplicable regions.  */
		if (flags & ~reg->flags)
			continue;

		avail += reg->free;
	}

	return avail;
}

static inline uintptr_t evi_lmm_adjust_align(uintptr_t addr, int align_bits,
					     uintptr_t align_ofs)
{
	int i;

	for (i = 0; i < align_bits; i++) {
		uintptr_t bit = (uintptr_t)1 << i;

		if ((addr ^ align_ofs) & bit)
			addr += bit;
	}

	return addr;
}

/**
 * \brief Allocates memory using the specified constraints
 *
 * \param lmm     The memory pool where to allocate.
 * \param size    Size of needed memory.
 * \param flags   The memory type required for this allocation. For each bit set
 *                in the flags parameter, the corresponding bit in a region's
 *                flags word must also be set in order for the region to be
 *                considered for allocation. If the flags parameter is zero,
 *                memory will be allocated from any region.
 * \param align_bits The number of low bits of the returned memory block address
 *                   that must match the corresponding bits in align_ofs.
 * \param align_ofs  The required offset from natural power-of-two alignment.
 *                   The returned memory block will be aligned on a
 *                   2^align_bits + align_ofs boundary.
 * \param in_min  Start address of the address range in which to search for a
 *                free block. The returned memory block, if found, will have an
 *                address no lower than in_min.
 * \param in_size Size of the address range in which to search for the free
 *                block. The returned memory block, if found, will fit entirely
 *                within this address range, so that
 *                mem_block + size <= in_min + in_size
 *
 * \return        Returns a pointer to the allocated memory or NULL
 *                if there is no memory block that satisfy all of the specified
 *                constraints.
 */
void *evi_lmm_alloc_gen(evi_lmm_t *lmm, size_t size,
			evi_region_flags_t flags, int align_bits,
			uintptr_t align_ofs, uintptr_t in_min,
			size_t in_size)
{
	uintptr_t in_max = in_min + in_size;
	struct evi_region_desc *reg;

	assert(lmm != 0);
	assert(size > 0);

	for (reg = lmm->regions; reg; reg = reg->next) {
		struct evi_freelist_node **nodep, *node;

		/*TODO:*/
		/* CHECKREGPTR(reg); */

		/* First trivially reject the entire region if possible. */
		if ((flags & ~reg->flags)
		    || (reg->start >= in_max)
		    || (reg->end <= in_min))
			continue;

		for (nodep = &reg->nodes; *nodep != 0; nodep = &node->next) {
			uintptr_t addr;
			struct evi_freelist_node *anode;

			node = *nodep;
			assert(((uintptr_t)node & EVI_LMM_ALIGN_MASK) == 0);
			assert(((uintptr_t)node->size
			       & EVI_LMM_ALIGN_MASK) == 0);
			assert((node->next == 0) || (node->next > node));
			assert((uintptr_t)node < reg->end);

			/* Now make a first-cut trivial elimination check
			 * to skip chunks that are _definitely_ too small.
			 */
			if (node->size < size)
				continue;

			/* Now compute the address at which
			 * the allocated chunk would have to start.
			 */
			addr = (uintptr_t)node;
			if (addr < in_min)
				addr = in_min;
			addr = evi_lmm_adjust_align(addr, align_bits,
						      align_ofs);

			/* See if the block at the adjusted address
			 * is still entirely within the node.
			 */
			if ((addr - (uintptr_t)node + size) > node->size)
				continue;

			/* If the block extends past the range constraint,
			 * then all of the rest of the nodes in this region
			 * will extend past it too, so stop here.
			 */
			if (addr + size > in_max)
				break;

			/* OK, we can allocate the block from this node.  */

			/* If the allocation leaves at least EVI_LMM_ALIGN_SIZE
			 * space before it, then split the node.
			 */
			anode = (struct evi_freelist_node *)
				 (addr & ~EVI_LMM_ALIGN_MASK);
			assert(anode >= node);
			if (anode > node) {
				size_t split_size = (uintptr_t)anode
						    - (uintptr_t)node;
				assert((split_size & EVI_LMM_ALIGN_MASK) == 0);
				anode->next = node->next;
				anode->size = node->size - split_size;
				node->size = split_size;
				nodep = &node->next;
			}

			/* Now use the first part of the anode
			 * to satisfy the allocation,
			 * splitting off the tail end if necessary.
			 */
			size = ((addr & EVI_LMM_ALIGN_MASK) + size
			       + EVI_LMM_ALIGN_MASK)
				& ~EVI_LMM_ALIGN_MASK;
			if (anode->size > size) {
				struct evi_freelist_node *newnode;

				/* Split the node and return its head.  */
				newnode = (struct evi_freelist_node *)
						((void *)anode + size);
				newnode->next = anode->next;
				newnode->size = anode->size - size;
				*nodep = newnode;
			} else {
				/* Remove and return the entire node.  */
				*nodep = anode->next;
			}

			/* Adjust the region's free memory counter.  */
			assert(reg->free >= size);
			reg->free -= size;

			return ((void *)addr);
		}
	}

	return NULL;
}

/**
 * \brief Allocates aligned memory block
 *
 * \param lmm     The memory pool where to allocate.
 * \param size    Size of needed memory.
 * \param flags   The memory type required for this allocation. For each bit set
 *                in the flags parameter, the corresponding bit in a region's
 *                flags word must also be set in order for the region to be
 *                considered for allocation. If the flags parameter is zero,
 *                memory will be allocated from any region.
 * \param align_bits The number of low bits of the returned memory block address
 *                   that must match the corresponding bits in align_ofs.
 * \param align_ofs  The required offset from natural power-of-two alignment.
 *                   The returned memory block will be aligned on a
 *                   2^align_bits + align_ofs boundary.
 *
 * \return        Returns a pointer to the allocated memory or NULL
 *                if there is no available memory.
 */
void *evi_lmm_alloc_aligned(evi_lmm_t *lmm, size_t size,
			      evi_region_flags_t flags, int align_bits,
			      uintptr_t align_ofs)
{
	return evi_lmm_alloc_gen(lmm, size, flags, align_bits, align_ofs,
				   (uintptr_t)0, (size_t)-1);
}




#ifdef REMOVED_API
/* REMOVED API */
int evi_lmm_add_free(evi_lmm_t *lmm, void *block, size_t size)
{
	struct evi_region_desc *reg;
	uintptr_t min = (uintptr_t) block;
	uintptr_t max = min + size;
	int err;

	/* Restrict the min and max further to be properly aligned.
	 * Note that this is the opposite of what lmm_free() does,
	 * because lmm_free() assumes the block was allocated with lmm_alloc()
	 * and thus would be a subset of a larger, already-aligned free block.
	 * Here we can assume no such thing.
	 */
	min = (min + EVI_LMM_ALIGN_MASK) & ~EVI_LMM_ALIGN_MASK;
	max &= ~EVI_LMM_ALIGN_MASK;

	if (max < min)
		return EVI_LMM_INVALID_BLOCK;

	/* If after alignment we have nothing left, we're done.  */
	if (max == min)
		return EVI_LMM_OK;

	/* Add the block to the free list(s) of whatever region(s) it overlaps.
	 * If some or all of the block doesn't fall into any existing region,
	 * then that memory is simply dropped on the floor.
	 */
	for (reg = lmm->regions; reg; reg = reg->next) {
		if (reg->start >= reg->end
		    || (reg->start & EVI_LMM_ALIGN_MASK)
		    || (reg->end & EVI_LMM_ALIGN_MASK))
			return EVI_LMM_INVALID_REGION;

		if ((max > reg->start) && (min < reg->end)) {
			uintptr_t new_min = min, new_max = max;

			/* Only add the part of the block
			 * that actually falls within this region.
			 */
			if (new_min < reg->start)
				new_min = reg->start;
			if (new_max > reg->end)
				new_max = reg->end;

			if (new_max <= new_min)
				return EVI_LMM_INVALID_REGION;

			/* Add the block.  */
			err = evi_lmm_free(lmm, (void *)new_min,
					     new_max - new_min);
			if (err)
				return err;
		}
	}

	return EVI_LMM_OK;
}
#endif

/**
 * \brief Adds a memory region to the pool of the passed hanlder.
 *        \see evi_lmm_add_reg
 *
 * \param lmm     Memory region pool handler
 * \param region  Pointer to a user region memory descriptor
 * \param addr    Start virtual address of the memory region
 * \param size    Size of the memory region
 * \param flags   Memory region bitmap flags
 * \param prio    Priority of the memory region inside the memory region pool
 *
 * \return        EVI_LMM_OK if the memory region is successfully added,
 *                otherwise an EVI_LMM_* error \see evi_err.h
 */
int evi_lmm_add_region(evi_lmm_t *lmm, evi_region_desc_t *region,
			  void *addr, size_t size, evi_region_flags_t flags,
			  evi_region_prio_t prio)
{
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end = start + size;
	struct evi_region_desc **rp, *r;

	start = (start + EVI_LMM_ALIGN_MASK) & ~EVI_LMM_ALIGN_MASK;
	end &= ~EVI_LMM_ALIGN_MASK;

	DBG("size=%zu\n", size);
	DBG("start = 0x%"PRIxPTR"\n", start);
	DBG("end = 0x%"PRIxPTR"\n", end);
	DBG("region = %p\n", region);

	if (end <= start) {
		DBG("invalid region 0x%"PRIxPTR" 0x%"PRIxPTR"\n",
		    start, end);
		return EVI_LMM_INVALID_REGION;
	}

	region->nodes = NULL;
	region->start = start;
	region->end = end;
	region->flags = flags;
	region->prio = prio;
	region->free = 0;

	rp = &(lmm->regions);

	if (*rp == region) {
		DBG("region already in the pool\n");
		return EVI_LMM_OK;
	}

	for (r = lmm->regions;
	     r && ((r->prio > prio)
		   || ((r->prio == prio)
			&& (r->start < start)));
	     rp = &(r->next), r = r->next) {
		if (r == region) {
			DBG("region already in the pool\n");
			return EVI_LMM_OK;
		}
		/* assert((max <= r->start) || (min >= r->end)); */
	}
	region->next = r;
	*rp = region;

#ifndef REMOVED_API
	evi_lmm_free_in_region(region, (void *)region->start,
				 region->end - region->start);
#endif

	DBG("+++++++++++++++++++++++++\n");
	DBG("Region %p\n", region);
	if (rp == &(lmm->regions)) {
		DBG("Added on TOP\n");
	} else {
		struct evi_region_desc *reg;

		reg = container_of(rp, struct evi_region_desc, next);
		DBG("Added after %p %p\n", rp, reg);
		evi_lmm_merge_region(lmm, reg, region);
	}
	if (region->next == NULL) {
		DBG("Added as LAST element\n");
	} else {
		struct evi_region_desc *reg = region->next;

		DBG("Added before %p\n", reg);
		evi_lmm_merge_region(lmm, region, reg);
	}
	DBG("+++++++++++++++++++++++++\n");

	DBG("Added prio:%d min:0x%"PRIxPTR" max:0x%"PRIxPTR"\n",
		region->prio, region->start, region->end);

	return EVI_LMM_OK;
}

/** \} */
