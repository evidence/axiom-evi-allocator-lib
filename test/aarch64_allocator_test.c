#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <axiom_allocator.h>

void hit_enter_string(char *s)
{
        printf("%s (hit enter)\n", s);
        getchar();
}

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

	axiom_allocator_init(vaddr_start, vaddr_end);
	hit_enter_string("After axiom_allocator_init");

	p = axiom_private_malloc(4096);
	printf("p: %p\n", p);
	for (i = 0; i < 4096; ++i)
		((char *)p)[i] = 0x42;
	hit_enter_string("After axiom_private_malloc");

	for (i = 0; i < ALLOC_TST_N; ++i) {
		char *pend = NULL;
		tst[i] = axiom_private_malloc(ALLOC_TST_SZ);
		if (tst[i] != NULL) {
			allocated_size += ALLOC_TST_SZ;
			memset(tst[i], 0x43 + i, ALLOC_TST_SZ);
			pend = ((char*)(tst[i]) + ALLOC_TST_SZ);
		}
		want_size += ALLOC_TST_SZ;
		printf("Alloc[%d]: %p %p\n", i, tst[i], pend);
	}
	printf("want size: %zu got size: %zu\n", want_size, allocated_size);
	hit_enter_string("After loop of axiom_private_malloc");

	for (i = 0; i < ALLOC_TST_N; ++i) {
		if (tst[i] != NULL) {
			printf("Free[%d]: %p\n", i, tst[i]);
			axiom_free(tst[i]);
		}
	}
	hit_enter_string("After loop of axiom_free");

	allp = axiom_private_malloc(allocated_size);
	printf("allp: %p\n", allp);
	hit_enter_string("After axiom_private_malloc");

	axiom_free(allp);
	{
		size_t ss = vaddr_end - vaddr_start - 2 * 4096;
		allp = axiom_private_malloc(ss);
		printf("start memset\n", allp);
		memset(allp, 0xAA , ss);
		printf("allp: %p\n", allp);
	}
	hit_enter_string("After axiom_private_malloc");

	return 0;
}
