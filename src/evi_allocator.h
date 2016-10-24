#ifndef EVI_ALLOCATOR_H
#define EVI_ALLOCATOR_H

/**
 * \defgroup EVI_ALLOCATOR
 *
 * \{
 */
/**
 * \brief Initialize the allocator
 *
 * \param saddr virtual address start of the allocator
 * \param eaddr virtual address end of the allocator
 * \param psaddr virtual address start of the private memory
 * \param peaddr virtual address end of the private memory
 *
 * \return Return the error code of operation. (0 if everything is OK)
 */
int evi_allocator_init(uintptr_t saddr, uintptr_t eaddr,
			 uintptr_t psaddr, uintptr_t peaddr);

/**
 * \brief Allocate memory of sz bytes int the private region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *evi_private_malloc(size_t sz);

/**
 * \brief Allocate memory of sz bytes in the private region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *evi_private_malloc(size_t sz);

/**
 * \brief Allocate memory of sz bytes in the shared region
 *
 * \param sz  Size of the required memory
 *
 * \return Return the pointer of the allocated memory. NULL in case of error.
 */
void *evi_shared_malloc(size_t sz);

/**
 * \brief Free a previous allocated memory in the private region
 *
 * \param ptr  Pointer to the memory to free
 */
void evi_private_free(void *ptr);

/**
 * \brief Free a previous allocated memory in the shared region
 *
 * \param ptr  Pointer to the memory to free
 */
void evi_shared_free(void *ptr);

/** \} */

#endif /* EVI_ALLOCATOR_H*/