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
 *
 * \return Return the error code of operation. (0 if everything is OK)
 */
int axiom_allocator_init(uintptr_t saddr, uintptr_t eaddr,
			 uintptr_t priv_start, uintptr_t priv_end);

/**
 * \brief Allocate memory of sz bytes int the private region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *axiom_private_malloc(size_t sz);


/**
 * \brief Allocate memory of sz bytes in the private region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *axiom_private_malloc(size_t sz);

/**
 * \brief Allocate memory of sz bytes in the shared region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *axiom_shared_malloc(size_t sz);

/**
 * \brief Free a previous allocated memory in the private region
 *
 * \param ptr  Pointer to the memory to free
 */
void axiom_private_free(void *ptr);

/**
 * \brief Free a previous allocated memory in the shared region
 *
 * \param ptr  Pointer to the memory to free
 */
void axiom_shared_free(void *ptr);

/** \} */

#endif /* AXIOM_ALLOCATOR_H*/
