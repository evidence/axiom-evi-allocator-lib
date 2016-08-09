#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>

#include <assert.h>

#include <axiom_lmm.h>
#include <axiom_err.h>

static void axiom_lmm_free_in_region(struct axiom_lmm_region *reg,
				     void *block, size_t size);

void axiom_lmm_init(axiom_lmm_t *lmm)
{
	lmm->regions = NULL;
}

static inline void dump_region(struct axiom_lmm_region *r)
{
	fprintf(stderr, "R:%p prio:%d m:0x%"PRIxPTR" M:0x%"PRIxPTR" sz:%zu"\
		" F:%zu diff:%zu nodes:%p\n",
		r, r->prio, r->start, r->end, r->end - r->start,
		r->free, (r->end - r->start), r->nodes);
}

int axiom_lmm_add_region(axiom_lmm_t *lmm, axiom_lmm_region_t *region,
			 void *addr, size_t size, axiom_lmm_flags_t flags,
			 axiom_lmm_pri_t prio)
{
	uintptr_t min = (uintptr_t)addr;
	uintptr_t max = min + size;
	struct axiom_lmm_region **rp, *r;

	min = (min + AXIOM_LMM_ALIGN_MASK) & ~AXIOM_LMM_ALIGN_MASK;
	max &= ~AXIOM_LMM_ALIGN_MASK;

	if (max <= min) {
		fprintf(stderr, "invalid region 0x%"PRIxPTR" 0x%"PRIxPTR"\n",
			min, max);
		return AXIOM_LMM_INVALID_REGION;
	}

	region->nodes = NULL;
	region->start = min;
	region->end = max;
	region->flags = flags;
	region->prio = prio;
	region->free = 0;

	rp = &(lmm->regions);

	if (*rp == region) {
		fprintf(stderr, "region already in the pool\n");
		return AXIOM_LMM_OK;
	}

	for (r = lmm->regions;
	     r && ((r->prio > prio)
		   || ((r->prio == prio)
			&& ((r->end - r->start) > (max - min))));
	     rp = &(r->next), r = r->next) {
		if (r == region) {
			fprintf(stderr, "region already in the pool\n");
			return AXIOM_LMM_OK;
		}
		/* assert((max <= r->start) || (min >= r->end)); */
	}
	region->next = r;
	*rp = region;

#ifndef REMOVED_API
	axiom_lmm_free_in_region(region, (void *)region->start,
				 region->end - region->start);
#endif

	return AXIOM_LMM_OK;
}

void axiom_lmm_dump_regions(axiom_lmm_t *lmm)
{
	int cnt = 0;
	struct axiom_lmm_region *r;

	for (r = lmm->regions; r; r = r->next) {
#if 0
		fprintf(stderr, "R:%p prio:%d m:0x%"PRIxPTR" M:0x%"PRIxPTR" sz:%zu\n",
			r, r->prio, r->start, r->end, r->end - r->start);
#else
		dump_region(r);
#endif
		++cnt;
	}
	fprintf(stderr, "Num regions: %d\n", cnt);
}

void axiom_lmm_dump(axiom_lmm_t *lmm)
{
	struct axiom_lmm_region *reg;

	fprintf(stderr, "%s(lmm=%p)\n", __func__, lmm);

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct axiom_lmm_node *node;
		size_t free_check;

		fprintf(stderr, " region %08lx-%08lx size=%zu flags=%08lx pri=%d free=%zu\n",
			reg->start, reg->end, reg->end - reg->start,
			(unsigned long)reg->flags, reg->prio, reg->free);

		/* CHECKREGPTR(reg); */

		free_check = 0;
		for (node = reg->nodes; node; node = node->next)
		{
			fprintf(stderr, "  node %p-0x%08"PRIxPTR" size=%zu next=%p\n",
				node, (uintptr_t)node + node->size, node->size, node->next);

			assert(((uintptr_t)node & AXIOM_LMM_ALIGN_MASK) == 0);
			assert((node->size & AXIOM_LMM_ALIGN_MASK) == 0);
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

static void axiom_lmm_free_in_region(struct axiom_lmm_region *reg,
				     void *block, size_t size)
{
	struct axiom_lmm_node *prevnode, *nextnode;
	struct axiom_lmm_node *node = (struct axiom_lmm_node*)
				((uintptr_t)block & ~AXIOM_LMM_ALIGN_MASK);

	size = (((uintptr_t)block & AXIOM_LMM_ALIGN_MASK) + size
		+ AXIOM_LMM_ALIGN_MASK)
		& ~AXIOM_LMM_ALIGN_MASK;

	/* Record the newly freed space in the region's free space counter.  */
	reg->free += size;
//dump_region(reg);
/*printf("F:%zu m:0x%"PRIxPTR" M:0x%"PRIxPTR" diff:%zu sz:%zu\n",
	reg->free, reg->start, reg->end, reg->end - reg->start, size);*/
	assert(reg->free <= reg->end - reg->start);

	/* Now find the location in that region's free list
	   at which to add the node.  */
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
			   just grow prevnode around newly freed memory.  */
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

int axiom_lmm_free(axiom_lmm_t *lmm, void *block, size_t size)
{
	struct axiom_lmm_region *reg;
	struct axiom_lmm_node *node = (struct axiom_lmm_node*)
				((uintptr_t)block & ~AXIOM_LMM_ALIGN_MASK);
	/*struct axiom_lmm_node *prevnode, *nextnode;*/

	assert(lmm != 0);
	if (lmm == NULL)
		return AXIOM_LMM_INVALID_MEM_DESC;
	if (block == NULL)
		return AXIOM_LMM_INVALID_BLOCK;
	if (size <= 0)
		return AXIOM_LMM_INVALID_SIZE;
#if 0
	size = (((uintptr_t)block & AXIOM_LMM_ALIGN_MASK) + size
		+ AXIOM_LMM_ALIGN_MASK)
		& ~AXIOM_LMM_ALIGN_MASK;
#endif
	/* First find the region to add this block to.  */
	for (reg = lmm->regions; ; reg = reg->next)
	{
		if (reg == 0)
			return AXIOM_LMM_INVALID_BLOCK;

		/* TODO: */
		/*CHECKREGPTR(reg);*/

		if (((uintptr_t)node >= reg->start)
		    && ((uintptr_t)node < reg->end))
			break;
	}
#if 1
	axiom_lmm_free_in_region(reg, block, size);
#else
	/* Record the newly freed space in the region's free space counter.  */
	reg->free += size;
	assert(reg->free <= reg->end - reg->start);

	/* Now find the location in that region's free list
	   at which to add the node.  */
	for (prevnode = 0, nextnode = reg->nodes;
	     (nextnode != 0) && (nextnode < node);
	     prevnode = nextnode, nextnode = nextnode->next);

	/* Coalesce the new free chunk into the previous chunk if possible.  */
	if ((prevnode) &&
	    ((uintptr_t)prevnode + prevnode->size >= (uintptr_t)node))
	{
		assert((uintptr_t)prevnode + prevnode->size
		       == (uintptr_t)node);

		/* Coalesce prevnode with nextnode if possible.  */
		if (((uintptr_t)nextnode)
		    && ((uintptr_t)node + size >= (uintptr_t)nextnode))
		{
			assert((uintptr_t)node + size
			       == (uintptr_t)nextnode);

			prevnode->size += size + nextnode->size;
			prevnode->next = nextnode->next;
		}
		else
		{
			/* Not possible -
			   just grow prevnode around newly freed memory.  */
			prevnode->size += size;
		}
	}
	else
	{
		/* Insert the new node into the free list.  */
		if (prevnode)
			prevnode->next = node;
		else
			reg->nodes = node;

		/* Try coalescing the new node with the nextnode.  */
		if ((nextnode) &&
		    (uintptr_t)node + size >= (uintptr_t)nextnode)
		{
			node->size = size + nextnode->size;
			node->next = nextnode->next;
		}
		else
		{
			node->size = size;
			node->next = nextnode;
		}
	}
#endif
	return AXIOM_LMM_OK;
}

static void *axiom_lmm_alloc_find_node(struct axiom_lmm_region *reg,
				       size_t size)
{
	struct axiom_lmm_node **nodep, *node;

	for (nodep = &reg->nodes; *nodep != 0; nodep = &node->next) {
		node = *nodep;

		assert(((uintptr_t)node & AXIOM_LMM_ALIGN_MASK) == 0);
		assert((node->size & AXIOM_LMM_ALIGN_MASK) == 0);
		assert((node->next == 0) || (node->next > node));
		assert((uintptr_t)node < reg->end);

		if (node->size < size)
			continue;

		if (node->size > size) {
			struct axiom_lmm_node *newnode;

			/* Split the node and return its head */
			newnode = (struct axiom_lmm_node*)
					((void*)node + size);
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

		return (void*)node;
	}

	return NULL;
}

void *axiom_lmm_alloc(axiom_lmm_t *lmm, size_t size, axiom_lmm_flags_t flags)
{
	struct axiom_lmm_region *reg;

	if (lmm == NULL || size <= 0)
		return NULL;

	size = (size + AXIOM_LMM_ALIGN_MASK) & ~AXIOM_LMM_ALIGN_MASK;

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		void *addr;

		/*TODO:*/
		/* CHECKREGPTR(reg); */

		if (flags & ~reg->flags)
			continue;

		addr = axiom_lmm_alloc_find_node(reg, size);
		if (addr != NULL)
			return addr;
	}

	return NULL;
}

size_t axiom_lmm_avail(axiom_lmm_t *lmm, axiom_lmm_flags_t flags)
{
	struct axiom_lmm_region *reg;
	size_t avail = 0;

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		/*TODO:*/
		/*CHECKREGPTR(reg);*/

		/* Don't count inapplicable regions.  */
		if (flags & ~reg->flags)
			continue;

		avail += reg->free;
	}

	return avail;
}

static inline uintptr_t axiom_lmm_adjust_align(uintptr_t addr, int align_bits,
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
    @lmm[in]     The memory pool from which to allocate.
    @size[in]    The number of contiguous bytes of memory needed.
    @flags[in]   The memory type required for this allocation. For each bit set
		 in the flags parameter, the corresponding bit in a region's
		 flags word must also be set in order for the region to be
		 considered for allocation. If the flags parameter is zero,
		 memory will be allocated from any region.
    @align_bits  The number of low bits of the returned memory block address
		 that must match the corresponding bits in align_ofs.
    @align_ofs   The required offset from natural power-of-two alignment.
		 If align_ofs is zero, then the returned memory block will be
		 naturally aligned on a 2^align_bits boundary.
    @in_min      Start address of the address range in which to search for a
		 free block. The returned memory block, if found, will have an
		 address no lower than in_min.
    @in_size     Size of the address range in which to search for the free
		 block. The returned memory block, if found, will fit entirely
		 within this address range, so that
		 mem_block+size <= in_min + in_size

    @return      Returns a pointer to the memory block allocated, or NULL
		 if no memory block satisfying all of the specified requirements
		 can be found.
*/
void *axiom_lmm_alloc_gen(axiom_lmm_t *lmm, size_t size,
			  axiom_lmm_flags_t flags, int align_bits,
			  uintptr_t align_ofs, uintptr_t in_min,
			  size_t in_size)
{
	uintptr_t in_max = in_min + in_size;
	struct axiom_lmm_region *reg;

	assert(lmm != 0);
	assert(size > 0);

	for (reg = lmm->regions; reg; reg = reg->next)
	{
		struct axiom_lmm_node **nodep, *node;

		/*TODO:*/
		/* CHECKREGPTR(reg); */

		/* First trivially reject the entire region if possible.  */
		if ((flags & ~reg->flags)
		    || (reg->start >= in_max)
		    || (reg->end <= in_min))
			continue;

		for (nodep = &reg->nodes; *nodep != 0; nodep = &node->next) {
			uintptr_t addr;
			struct axiom_lmm_node *anode;

			node = *nodep;
			assert(((uintptr_t)node & AXIOM_LMM_ALIGN_MASK) == 0);
			assert(((uintptr_t)node->size & AXIOM_LMM_ALIGN_MASK) == 0);
			assert((node->next == 0) || (node->next > node));
			assert((uintptr_t)node < reg->end);

			/* Now make a first-cut trivial elimination check
			   to skip chunks that are _definitely_ too small.  */
			if (node->size < size)
				continue;

			/* Now compute the address at which
			   the allocated chunk would have to start.  */
			addr = (uintptr_t)node;
			if (addr < in_min)
				addr = in_min;
			addr = axiom_lmm_adjust_align(addr, align_bits,
						      align_ofs);

			/* See if the block at the adjusted address
			   is still entirely within the node.  */
			if ((addr - (uintptr_t)node + size) > node->size)
				continue;

			/* If the block extends past the range constraint,
			   then all of the rest of the nodes in this region
			   will extend past it too, so stop here. */
			if (addr + size > in_max)
				break;

			/* OK, we can allocate the block from this node.  */

			/* If the allocation leaves at least AXIOM_LMM_ALIGN_SIZE
			   space before it, then split the node.  */
			anode = (struct axiom_lmm_node*)(addr & ~AXIOM_LMM_ALIGN_MASK);
			assert(anode >= node);
			if (anode > node) {
				size_t split_size = (uintptr_t)anode
				                  - (uintptr_t)node;
				assert((split_size & AXIOM_LMM_ALIGN_MASK) == 0);
				anode->next = node->next;
				anode->size = node->size - split_size;
				node->size = split_size;
				nodep = &node->next;
			}

			/* Now use the first part of the anode
			   to satisfy the allocation,
			   splitting off the tail end if necessary.  */
			size = ((addr & AXIOM_LMM_ALIGN_MASK) + size + AXIOM_LMM_ALIGN_MASK)
				& ~AXIOM_LMM_ALIGN_MASK;
			if (anode->size > size) {
				struct axiom_lmm_node *newnode;

				/* Split the node and return its head.  */
				newnode = (struct axiom_lmm_node*)
						((void*)anode + size);
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

			return ((void*)addr);
		}
	}

	return NULL;
}

void *axiom_lmm_alloc_aligned(axiom_lmm_t *lmm, size_t size,
			      axiom_lmm_flags_t flags, int align_bits,
			      uintptr_t align_ofs)
{
	return axiom_lmm_alloc_gen(lmm, size, flags, align_bits, align_ofs,
				   (uintptr_t)0, (size_t)-1);
}

#ifdef REMOVED_API
/* REMOVED API */
int axiom_lmm_add_free(axiom_lmm_t *lmm, void *block, size_t size)
{
	struct axiom_lmm_region *reg;
	uintptr_t min = (uintptr_t) block;
	uintptr_t max = min + size;
	int err;

	/* Restrict the min and max further to be properly aligned.
	   Note that this is the opposite of what lmm_free() does,
	   because lmm_free() assumes the block was allocated with lmm_alloc()
	   and thus would be a subset of a larger, already-aligned free block.
	   Here we can assume no such thing.  */
	min = (min + AXIOM_LMM_ALIGN_MASK) & ~AXIOM_LMM_ALIGN_MASK;
	max &= ~AXIOM_LMM_ALIGN_MASK;

	if (max < min)
		return AXIOM_LMM_INVALID_BLOCK;

	/* If after alignment we have nothing left, we're done.  */
	if (max == min)
		return AXIOM_LMM_OK;

	/* Add the block to the free list(s) of whatever region(s) it overlaps.
	   If some or all of the block doesn't fall into any existing region,
	   then that memory is simply dropped on the floor.  */
	for (reg = lmm->regions; reg; reg = reg->next)
	{
		if (reg->start >= reg->end
		    || (reg->start & AXIOM_LMM_ALIGN_MASK)
		    || (reg->end & AXIOM_LMM_ALIGN_MASK))
			return AXIOM_LMM_INVALID_REGION;

		if ((max > reg->start) && (min < reg->end))
		{
			uintptr_t new_min = min, new_max = max;

			/* Only add the part of the block
			   that actually falls within this region.  */
			if (new_min < reg->start)
				new_min = reg->start;
			if (new_max > reg->end)
				new_max = reg->end;

			if (new_max <= new_min)
				return AXIOM_LMM_INVALID_REGION;

			/* Add the block.  */
			err = axiom_lmm_free(lmm, (void*)new_min,
					     new_max - new_min);
			if (err)
				return err;
		}
	}

	return AXIOM_LMM_OK;
}
#endif
