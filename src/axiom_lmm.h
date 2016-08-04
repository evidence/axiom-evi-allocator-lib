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

	uintptr_t min; /** Start address of the region */
	uintptr_t max; /** End address of the region */

	axiom_lmm_flags_t flags; /** region attribute */
	axiom_lmm_pri_t prio; /** allocation priority */

	size_t free; /** free space in this region */
} axiom_lmm_region_t;

typedef struct axiom_lmm {
	axiom_lmm_region_t *regions;
} axiom_lmm_t;


void axiom_lmm_init(axiom_lmm_t *lmm);

#endif /* AXIOM_LMM_H */
