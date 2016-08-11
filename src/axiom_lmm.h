#ifndef AXIOM_LMM_H
#define AXIOM_LMM_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t axiom_lmm_flags_t;
typedef int      axiom_lmm_pri_t;

/** \brief Free list node */
struct axiom_lmm_node {
	/** Next free list node */
	struct axiom_lmm_node *next;
	/** Size of the free area */
	size_t size;
};

/** \brief Region descriptor */
typedef struct axiom_lmm_region {
	/** Pointer to the following region */
	struct axiom_lmm_region *next;
	
	/** List of free memory blocks in this region. */
	struct axiom_lmm_node *nodes;

	/** Start address of the region */
	uintptr_t start;
	/** End address of the region */
	uintptr_t end;

	/** Region attribute */
	axiom_lmm_flags_t flags;
	/** Region allocation priority */
	axiom_lmm_pri_t prio;

	/** Actual region free space */
	size_t free;
} axiom_lmm_region_t;

typedef struct axiom_lmm {
	/** List of registered regions */
	axiom_lmm_region_t *regions;
	/** Private buffer used to store "unnamed" regions */
	char workspace[4096 - sizeof(axiom_lmm_region_t *)];
} axiom_lmm_t;

/* TODO: check power of 2???*/
#define AXIOM_LMM_ALIGN_SIZE      sizeof(struct axiom_lmm_node)
#define AXIOM_LMM_ALIGN_MASK      (AXIOM_LMM_ALIGN_SIZE - 1)

#define AXIOM_PRIVATE_MEM         1
#define AXIOM_SHARE_MEM           2

int axiom_lmm_init(axiom_lmm_t *lmm);
int axiom_lmm_add_region(axiom_lmm_t *lmm, axiom_lmm_region_t *region,
			 void *addr, size_t size, axiom_lmm_flags_t flags,
			 axiom_lmm_pri_t prio);

int axiom_lmm_free(axiom_lmm_t *lmm, void *block, size_t size);
void *axiom_lmm_alloc(axiom_lmm_t *lmm, size_t size, axiom_lmm_flags_t flags);
size_t axiom_lmm_avail(axiom_lmm_t *lmm, axiom_lmm_flags_t flags);
void *axiom_lmm_alloc_gen(axiom_lmm_t *lmm, size_t size,
			  axiom_lmm_flags_t flags, int align_bits,
			  uintptr_t align_ofs, uintptr_t in_min,
			  size_t in_size);

void axiom_lmm_dump_regions(axiom_lmm_t *lmm);
void axiom_lmm_dump(axiom_lmm_t *lmm);

int axiom_lmm_add_reg(axiom_lmm_t *lmm, void *addr, size_t size,
		      axiom_lmm_flags_t flags, axiom_lmm_pri_t prio);

#define REMOVED_API
#undef REMOVED_API

#ifdef REMOVED_API
/*REMOVED API*/
int axiom_lmm_add_free(axiom_lmm_t *lmm, void *block, size_t size);
#endif

#endif /* AXIOM_LMM_H */
