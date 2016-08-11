/**
 * \file
 */

#include <pthread.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include <axiom_lmm.h>
#include <axiom_err.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <axiom_mem_dev_user.h>

#include <axiom_allocator.h>

#include <assert.h>

/**
 * \cond INTERNAL_MACRO
 */
#undef DEBUG
#define DEBUG
/**
 * \endcond
 */

#include <debug.h>

/**
 * \cond INTERNAL_MACRO
 */
#define BLOCK_REQ_SIZE (128 * 1024 * 1024)
#define DEFAULT_PRIO   (0)
/**
 * \endcond
 */

static struct axiom_allocator_s {
	pthread_mutex_t mutex;
	uintptr_t vaddr_start;
	uintptr_t vaddr_end;
	int mem_dev_fd;
	axiom_lmm_t almm;
} axiom_mem_hdlr = { PTHREAD_MUTEX_INITIALIZER, };

/*
extern unsigned long __ld_shm_info_start_addr;
extern unsigned long __ld_shm_info_end_addr;
*/

void axiom_allocator_init(uintptr_t saddr, uintptr_t eaddr)
{
	int err;
/*
        unsigned long *data_start_addr = (unsigned long *)&__ld_shm_info_start_addr;
        unsigned long *data_end_addr = (unsigned long *)&__ld_shm_info_end_addr;
        size_t size = *data_end_addr - *data_start_addr;
*/
        size_t size = eaddr - saddr;

	err = axiom_lmm_init(&axiom_mem_hdlr.almm);
	assert(err == AXIOM_LMM_OK);

	err = open("/dev/axiom_dev_mem0", O_RDWR);
	DBG("open return %d\n", err);
	assert(err >= 0);

	axiom_mem_hdlr.mem_dev_fd = err;

        DBG("addr = 0x%"PRIxPTR"\n", saddr);
        DBG("size = %zd\n", size);

	axiom_mem_hdlr.vaddr_start = saddr;
	axiom_mem_hdlr.vaddr_end = eaddr;

	err = mprotect((void *)(saddr), size, PROT_NONE);
}

static void *axiom_request_private_region(unsigned long size)
{
        struct axiom_mem_dev_info request;
        unsigned long start_addr = (unsigned long)(axiom_mem_hdlr.vaddr_start);
        void *mem;
        int err;
	int fd = axiom_mem_hdlr.mem_dev_fd;

        request.size = size;

        err = ioctl(fd, AXIOM_MEM_DEV_PRIVATE_ALLOC, &request);
        if (err) {
                perror("ioctl");
                return NULL;
        }

        DBG("request B:0x%lx S:%ld\n", request.base, request.size);

        mem = (void *)(start_addr + request.base);
        DBG("protect B:%p S:%ld\n", mem, request.size);
        if (mprotect(mem, request.size, PROT_WRITE | PROT_READ)) {
                perror("mprotect");
                return NULL;
        }

        return mem;
}

static void *private_alloc_with_region(size_t nsize)
{
	size_t bs;
	void *addr;
	void *ptr = NULL;
	int err;

	bs = ((nsize + BLOCK_REQ_SIZE - 1) / BLOCK_REQ_SIZE)
	     * BLOCK_REQ_SIZE;
	DBG("Request region of size: %zu\n", bs);
	addr = axiom_request_private_region(bs);
	DBG("private region address: %p\n", addr);

	if (addr == NULL)
		return ptr;

	err = axiom_lmm_add_reg(&axiom_mem_hdlr.almm, addr, bs,
			  AXIOM_PRIVATE_MEM,
			  DEFAULT_PRIO);
	if (err != AXIOM_LMM_OK)
		return ptr;

	ptr = axiom_lmm_alloc(&axiom_mem_hdlr.almm, nsize,
			      AXIOM_PRIVATE_MEM);

	return ptr;
}

void *axiom_private_malloc(size_t sz)
{
	void *ptr;
	size_t nsize = sz + sizeof(size_t);

	pthread_mutex_lock(&axiom_mem_hdlr.mutex);
	ptr = axiom_lmm_alloc(&axiom_mem_hdlr.almm, nsize, AXIOM_PRIVATE_MEM);
	if (ptr == NULL)
		ptr = private_alloc_with_region(nsize);

	if (ptr != NULL) {
		*((size_t *) ptr) = nsize;
		ptr = (void *)((uintptr_t)ptr + sizeof(size_t));
	}

	pthread_mutex_unlock(&axiom_mem_hdlr.mutex);

	return ptr;
}

void axiom_free(void *ptr)
{
	char *p;

	if (ptr == NULL) {
		DBG("Nothing to free: pointer is null!\n");
		return;
	}

	p = ((char *)ptr - sizeof(size_t));

	pthread_mutex_lock(&axiom_mem_hdlr.mutex);

	if (*(size_t *)p > 0) {
		DBG("Freeing %zu bytes\n", *(size_t *)p);
		axiom_lmm_free(&axiom_mem_hdlr.almm, p, *(size_t *)p);
	} else {
		DBG("Invalid size: %p %zu \n", p, *(size_t *)p);
	}

	pthread_mutex_unlock(&axiom_mem_hdlr.mutex);
}
