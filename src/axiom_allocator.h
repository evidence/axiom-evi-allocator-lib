#ifndef AXIOM_ALLOCATOR_H
#define AXIOM_ALLOCATOR_H

/**
 * \defgroup AXIOM_ALLOCATOR
 *
 * \{
 */
/**
 * \brief Initialize the allocator
 *
 * \param saddr virtual address start of the allocator
 * \param eaddr virtual address end of the allocator
 */
void axiom_allocator_init(uintptr_t saddr, uintptr_t eaddr);

/**
 * \brief Allocate memory of sz bytes
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *axiom_private_malloc(size_t sz);

/**
 * \brief Free a previous allocated memory
 *
 * \param ptr  Pointer to the memory to free
 */
void axiom_free(void *ptr);

/** \} */

#endif /* AXIOM_ALLOCATOR_H*/
