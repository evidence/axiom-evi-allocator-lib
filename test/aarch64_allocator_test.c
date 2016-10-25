#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <evi_lmm.h>


#include <evi_allocator.h>


#include <inttypes.h>

void hit_enter_string(char *s)
{
        printf("%s (hit enter)\n", s);
        getchar();
}

int get_app_id()
{
	int app_id;
	char *app_id_str;

	app_id_str = getenv("AXIOM_APP_ID");
	if (app_id_str == NULL)
		return -1;

	app_id = atoi(app_id_str);

	return ((app_id < 0) ? -1 : app_id);
}

#define MB_1 (1024 * 1024)

int main()
{
#define ALLOC_TST_N  10
#define ALLOC_TST_SZ (120 * 1024 * 1024)
	uintptr_t vaddr_start = (uintptr_t)0x4000000000;
	uintptr_t vaddr_end = (uintptr_t)0x4040000000;
	void *p;
	void *tst[ALLOC_TST_N];
	void *allp;
	int i;
	size_t allocated_size = 0;
	size_t want_size = 0;
	int err;
	int app_id;

	app_id = get_app_id();
	if (app_id < 0) {
		printf("Invalid AXIOM_APP_ID (=%d)\n", app_id);
		exit(EXIT_FAILURE);
	}

	hit_enter_string("Before evi_allocator_init");

	err = evi_allocator_init(app_id,
				 vaddr_start, vaddr_end,
				 vaddr_start + app_id * MB_1,
				 vaddr_start + (app_id + 1) * MB_1);
	if (err) {
		printf("Error in evi_allocator_init\n");
		exit(EXIT_FAILURE);
	}
	hit_enter_string("After evi_allocator_init");

	p = evi_private_malloc(4096);
	printf("p: %p\n", p);
	for (i = 0; i < 4096; ++i)
		((char *)p)[i] = (char)app_id; /* 0x42; */
	hit_enter_string("After evi_private_malloc");

	for (i = 0; i < ALLOC_TST_N; ++i) {
		char *pend = NULL;
		tst[i] = evi_private_malloc(ALLOC_TST_SZ);
		if (tst[i] != NULL) {
			allocated_size += ALLOC_TST_SZ;
			memset(tst[i], 0x43 + i, ALLOC_TST_SZ);
			pend = ((char*)(tst[i]) + ALLOC_TST_SZ);
		}
		want_size += ALLOC_TST_SZ;
		printf("Alloc[%d]: %p %p\n", i, tst[i], pend);
	}
	printf("want size: %zu got size: %zu\n", want_size, allocated_size);
	hit_enter_string("After loop of evi_private_malloc");

	for (i = 0; i < ALLOC_TST_N; ++i) {
		if (tst[i] != NULL) {
			printf("Free[%d]: %p\n", i, tst[i]);
			evi_private_free(tst[i]);
		}
	}
	hit_enter_string("After loop of evi_private_free");

	allp = evi_private_malloc(allocated_size);
	printf("allp: %p\n", allp);
	hit_enter_string("After evi_private_malloc");

	evi_private_free(allp);
	{
		size_t ss = vaddr_end - vaddr_start - 2 * 4096;
		allp = evi_private_malloc(ss);
		printf("start memset\n");
		if (allp)
			memset(allp, 0xAA , ss);
		printf("allp: %p\n", allp);
	}
	hit_enter_string("After evi_private_malloc");

	return 0;
}
