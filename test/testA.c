#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <axiom_lmm.h>
#include <axiom_err.h>

int main()
{
	axiom_lmm_t almm;
	axiom_region_desc_t areg1;
	axiom_region_desc_t areg2;
	axiom_region_desc_t areg3;
	axiom_region_desc_t areg4;
	int i;
	int err;

#define REGION_NUMS   6
#define REGION_SIZE   (1 * 1024 * 1024)
	void *p[REGION_NUMS];

	uintptr_t min, max;
	size_t sz;

	printf("Hello world!\n");
	axiom_lmm_init(&almm);

	for (i = 0; i < REGION_NUMS; ++i) {
		uintptr_t t;

		p[i] = malloc(REGION_SIZE);
		t = (uintptr_t)p[i];
		if (p[i] == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		} else {
			printf("p[%d] %p P:0x%"PRIxPTR"\n", i, p[i], t);
		}

		if (i == 0) {
			min = max = t;
		} else {
			if (min > t)
				min = t;
			if (max < t)
				max = t;
		}
	}

	printf("sizeof axiom_freelist_node %zu\n",
	       sizeof(struct axiom_freelist_node));
	err = axiom_lmm_add_region(&almm, &areg1, p[1],
				   REGION_SIZE, AXIOM_PRIVATE_MEM, 5);
	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}
#if 0
	printf("---------------------\n");
	axiom_lmm_dump_regions(&almm);
#endif
	err = axiom_lmm_add_region(&almm, &areg2, p[2],
				   REGION_SIZE, AXIOM_PRIVATE_MEM, 5);

	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}

	err = axiom_lmm_add_region(&almm, &areg3, p[3],
				   REGION_SIZE/2, AXIOM_PRIVATE_MEM, 5);

	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}
	err = axiom_lmm_add_region(&almm, &areg4, p[3] + REGION_SIZE/2,
				   REGION_SIZE/2, AXIOM_PRIVATE_MEM, 5);

	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}

	printf("---------------------\n");
	axiom_lmm_dump_regions(&almm);
return 0;
	sz = max - min;

	printf("m:0x%"PRIxPTR" M:0x%"PRIxPTR" sz:%zu\n", min, max, sz);
#ifdef REMOVED_API
/* REMOVED API */
	printf("call to axiom_lmm_add_free\n");
	err = axiom_lmm_add_free(&almm, (void*)min, sz);
	if (err != AXIOM_LMM_OK) {
		printf("unable to add free memory (%d)\n", err);
		exit(EXIT_FAILURE);
	}
#endif

	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
	{
		void *z1, *z2, *z3;
		z1 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z1 = %p\n", z1);

		z2 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z2 = %p\n", z2);

		z3 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z3 = %p\n", z3);

		axiom_lmm_free(&almm, z2, 4096);
		z2 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z2 = %p\n", z2);
	}
	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
	{
		void *x1, *x2, *x3;
		x1 = axiom_lmm_alloc_gen(&almm, 4096, 0, 12, 0, 0, (size_t)-1);
		x2 = axiom_lmm_alloc_gen(&almm, 512 * 1024, 0, 12, 0, 0, (size_t)-1);
		x3 = axiom_lmm_alloc_gen(&almm, 512 * 1024, 0, 12, 0, 0, (size_t)-1);
		printf("x1 = %p 0x%"PRIxPTR"\n", x1, (uintptr_t)x1);
		printf("x2 = %p\n", x2);
		printf("x3 = %p\n", x3);
	}
	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));

	return 0;
}
