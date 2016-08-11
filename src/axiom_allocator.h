#ifndef AXIOM_ALLOCATOR_H
#define AXIOM_ALLOCATOR_H

void axiom_allocator_init(uintptr_t saddr, uintptr_t eaddr);
void *axiom_private_malloc(size_t sz);
void axiom_free(void *ptr);

#endif /* AXIOM_ALLOCATOR_H*/
