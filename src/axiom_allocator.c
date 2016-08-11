#include <pthread.h>
#include <stdlib.h>
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

#undef DEBUG
#include <debug.h>

#define BLOCK_REQ_SIZE (128 * 1024 * 1024)
#define DEFAULT_PRIO   (0)

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
	assert(err < 0);

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

        printf("request B:0x%lx S:%ld\n", request.base, request.size);

        mem = (void *)(start_addr + request.base);
        printf("protect B:%p S:%ld\n", mem, request.size);
        if (mprotect(mem, request.size, PROT_WRITE | PROT_READ)) {
                perror("mprotect");
                return NULL;
        }

        return mem;
}

void *axiom_private_malloc(size_t sz)
{
	void *ptr;
	size_t nsize = sz + sizeof(size_t);

	pthread_mutex_lock(&axiom_mem_hdlr.mutex);
	ptr = axiom_lmm_alloc(&axiom_mem_hdlr.almm, nsize, AXIOM_PRIVATE_MEM);
	if (ptr == NULL) {
		size_t bs;
		void *addr;

		bs = (nsize + BLOCK_REQ_SIZE - 1) / BLOCK_REQ_SIZE;
		addr = axiom_request_private_region(bs);
		if (addr != NULL) {
			axiom_lmm_add_reg(&axiom_mem_hdlr.almm, addr, bs,
					  AXIOM_PRIVATE_MEM,
					  DEFAULT_PRIO);
			ptr = axiom_lmm_alloc(&axiom_mem_hdlr.almm, nsize,
					      AXIOM_PRIVATE_MEM);
			*((size_t *) ptr) = nsize;
			ptr = (void *)((uintptr_t)ptr + sizeof(size_t));
		}

	}
	pthread_mutex_unlock(&axiom_mem_hdlr.mutex);

	return ptr;
}

