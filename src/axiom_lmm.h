#ifndef AXIOM_LMM_H
#define AXIOM_LMM_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t axiom_lmm_flags_t;
typedef int      axiom_lmm_pri_t;

struct axiom_lmm_node {
	struct axiom_lmm_node *next;
	size_t size;
};

typedef struct axiom_lmm_region {
	struct axiom_lmm_region *next;
	
	/** List of free memory blocks in this region. */
	struct axiom_lmm_node *nodes;

	uintptr_t start; /** Start address of the region */
	uintptr_t end;   /** End address of the region */

	axiom_lmm_flags_t flags; /** region attribute */
	axiom_lmm_pri_t prio; /** allocation priority */

	size_t free; /** free space in this region */
} axiom_lmm_region_t;

typedef struct axiom_lmm {
	axiom_lmm_region_t *regions;
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
