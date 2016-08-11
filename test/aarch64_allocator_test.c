#include <stdio.h>
#include <axiom_allocator.h>

int main()
{
	uintptr_t vaddr_start = (uintptr_t)0x4000000000;
	uintptr_t vaddr_end = (uintptr_t)0x4040000000;

	axiom_allocator_init(vaddr_start, vaddr_end);

	return 0;
}
