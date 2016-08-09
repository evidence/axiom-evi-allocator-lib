#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <axiom_lmm.h>
#include <axiom_err.h>

void *print_alloc(char *vn, axiom_lmm_t *lmm, size_t size, axiom_lmm_flags_t flags)
{
	void *p;

	p = axiom_lmm_alloc(lmm, size, flags);
	printf("%s = 0x%"PRIxPTR" - 0x%"PRIxPTR" %zu (ALLOC)\n", vn, (uintptr_t)p,
	       (uintptr_t)p + size, size);

	return p;
}
	
#define PRINT_ALLOC(vn, pl, sz, fl) \
	do { \
		vn = print_alloc(#vn, pl, sz, fl); \
	} while(0)

void print_free(char *vn, void *p, axiom_lmm_t *lmm, size_t size)
{
	printf("%s = 0x%"PRIxPTR" - 0x%"PRIxPTR" %zu (FREE)\n", vn, (uintptr_t)p,
	       (uintptr_t)p + size, size);
	axiom_lmm_free(lmm, p, size);
}

#define PRINT_FREE(vn, pl, sz) \
	do { \
		print_free(#vn, vn, pl, sz); \
	} while(0)

#define PRINT_ALLOC_GEN(vn, pl, sz, fl, ab, aof, in_min, in_size) \
	do { \
		vn = axiom_lmm_alloc_gen(pl, sz, fl, ab, aof, in_min, in_size); \
		printf(#vn " = %p (%zu) (ALLOC GEN)\n", vn, sz); \
		vn; \
	} while(0)

int main()
{
	axiom_lmm_t almm;
	axiom_lmm_region_t areg1;
	axiom_lmm_region_t areg2;
	axiom_lmm_region_t areg3;
	axiom_lmm_region_t areg4;
	int i;
	int err;

#define REGION_NUMS   6
#define REGION_SIZE   (1 * 1024 * 1024)
	void *p[REGION_NUMS];

	uintptr_t min, max;
	size_t sz;

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

	err = axiom_lmm_add_reg(&almm, p[3] + REGION_SIZE/2,
				REGION_SIZE/2, AXIOM_PRIVATE_MEM, 5);
	
	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}

	axiom_lmm_dump(&almm);
	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
	{
		void *z1, *z2, *z3, *zT;
#if 0
		z1 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z1 = %p\n", z1);
		z2 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z2 = %p\n", z2);
		z3 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z3 = %p\n", z3);
		axiom_lmm_free(&almm, z2, 4096);
		z2 = axiom_lmm_alloc(&almm, 4096, 0);
		printf("z2 = %p\n", z2);
		zT = axiom_lmm_alloc(&almm, 500 * 1024, 0);
		printf("zT = %p\n", zT);
		axiom_lmm_free(&almm, z1, 4096);
#else
		PRINT_ALLOC(z1, &almm, 4096, 0);
		PRINT_ALLOC(z2, &almm, 4096, 0);
		PRINT_ALLOC(z3, &almm, 4096, 0);
		PRINT_FREE(z2, &almm, 4096);
		PRINT_ALLOC(z2, &almm, 4096, 0);
		PRINT_ALLOC(zT, &almm, 500 * 1024, 0);
		PRINT_FREE(z1, &almm, 4096);
#endif

	}
	axiom_lmm_dump(&almm);
	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));


	err = axiom_lmm_add_reg(&almm, p[3],
				REGION_SIZE/2, AXIOM_PRIVATE_MEM, 5);
	
	if (err != AXIOM_LMM_OK) {
		printf("unable to add region\n");
		exit(EXIT_FAILURE);
	}

	axiom_lmm_dump(&almm);
	printf("---------------------\n");
	axiom_lmm_dump_regions(&almm);

	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
	{
		void *f1;
		PRINT_ALLOC(f1, &almm, 4096, 0);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
		PRINT_FREE(f1, &almm, 4096);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));

	}

	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
	{
		void *x1, *x2, *x3, *xT;
#if 0
		x1 = axiom_lmm_alloc_gen(&almm, 4096, 0, 12, 0, 0, (size_t)-1);
		printf("x1 = %p 0x%"PRIxPTR"\n", x1, (uintptr_t)x1);
		x2 = axiom_lmm_alloc_gen(&almm, 508 * 1024, 0, 12, 0, 0, (size_t)-1);
		printf("x2 = %p\n", x2);
		x3 = axiom_lmm_alloc_gen(&almm, 1024, 0, 12, 0, 0, (size_t)-1);
		printf("x3 = %p\n", x3);
#endif
		PRINT_ALLOC_GEN(x1, &almm, (size_t)4096, 0, 12, 0, 0, (size_t)-1);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
		PRINT_ALLOC_GEN(x2, &almm, (size_t)(508 * 1024), 0, 12, 0,
				0, (size_t)-1);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
		//PRINT_ALLOC_GEN(x3, &almm, 3 * 1024, 0, /*12*/ 0, 0, 0, (size_t)-1);
		PRINT_ALLOC(x3, &almm, 3 * 1024, 0);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
		PRINT_FREE(x1, &almm, 4096);
		printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));
		PRINT_FREE(x2, &almm, 508 * 1024);
		PRINT_FREE(x3, &almm, 3 * 1024);
	}
	printf("tot free space:%zu\n", axiom_lmm_avail(&almm, 0));

	axiom_lmm_dump(&almm);


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


	return 0;
}
