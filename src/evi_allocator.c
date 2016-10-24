/**
 * \file
 *
 * \brief Implements libc malloc/free style functions
 */

#include <pthread.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include <evi_lmm.h>
#include <evi_err.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <axiom_mem_dev_user.h>

#include <evi_allocator.h>

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

static struct evi_allocator_s {
	pthread_mutex_t mutex;
	uintptr_t vaddr_start;
	uintptr_t vaddr_end;
	int mem_dev_fd;
	evi_lmm_t almm;
} evi_mem_hdlr = { PTHREAD_MUTEX_INITIALIZER, };

/*
extern unsigned long __ld_shm_info_start_addr;
extern unsigned long __ld_shm_info_end_addr;
*/

static int add_region(uintptr_t saddr, uintptr_t eaddr,
		      evi_region_flags_t flags, evi_region_prio_t prio)
{
        struct axiom_mem_dev_info request;
	int err;
	int fd = evi_mem_hdlr.mem_dev_fd;
        void *mem = (void *)saddr;
	/* void *mapped_addr;*/

        DBG("Request region [0x%"PRIxPTR"] [0x%"PRIxPTR"]\n", saddr, eaddr);
	request.base = saddr;
        request.size = eaddr - saddr;
        DBG("Request region b:%ld s:%ld\n", request.base, request.size);

	err = ioctl(fd, AXIOM_MEM_DEV_RESERVE_MEM, &request);
	if (err != 0) {
		perror("ioctl");
		return err;
	}
#if 0
#if 1
	err = mprotect(mem, request.size, PROT_WRITE | PROT_READ);
	if (err) {
		perror("mprotect");
		return err;
	}
#else
	mapped_addr = mmap((void *)saddr, request.size, PROT_WRITE | PROT_READ,
			   MAP_SHARED | MAP_LOCKED | MAP_FIXED, fd, 0);
	if (mapped_addr == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
#endif
#endif
	err = evi_lmm_add_reg(&evi_mem_hdlr.almm, (void *)saddr,
				request.size, flags, prio);

	DBG("evi_lmm_add_reg err = %d\n", err);

	return err;
}

/* Dangerous do not use*/
static int rem_region(uintptr_t saddr, uintptr_t eaddr)
{
        struct axiom_mem_dev_info request;
	int err;
	int fd = evi_mem_hdlr.mem_dev_fd;
        void *mem = (void *)saddr;

	DBG("Request region [0x%"PRIxPTR"] [0x%"PRIxPTR"]\n", saddr, eaddr);
	request.base = saddr;
	request.size = eaddr - saddr;
	DBG("Request region b:%ld s:%ld\n", request.base, request.size);

	/*TODO: remove region inside lmm */

	err = ioctl(fd, AXIOM_MEM_DEV_REVOKE_MEM, &request);
	if (err != 0) {
		perror("ioctl");
		return err;
	}

	err = mprotect(mem, request.size, PROT_NONE);
	if (err) {
		perror("mprotect");
		return err;
	}

	return err;
}

static int get_app_id()
{
	int app_id;
	char *app_id_str;

	app_id_str = getenv("AXIOM_APP_ID");
	if (app_id_str == NULL)
		return -1;

	app_id = atoi(app_id_str);

	return ((app_id < 0) ? -1 : app_id);
}

int evi_allocator_init(uintptr_t saddr, uintptr_t eaddr,
			 uintptr_t psaddr, uintptr_t peaddr)
{
	int err;
	struct axiom_mem_dev_info request;
	int app_id;

/*
        unsigned long *data_start_addr = (unsigned long *)&__ld_shm_info_start_addr;
        unsigned long *data_end_addr = (unsigned long *)&__ld_shm_info_end_addr;
        size_t size = *data_end_addr - *data_start_addr;
*/
        size_t size = eaddr - saddr;

	err = evi_lmm_init(&evi_mem_hdlr.almm);
	DBG("evi_lmm_init returns %d\n", err);
	assert(err == EVI_LMM_OK);

	app_id = get_app_id();
	if (app_id < 0) {
		DBG("Invalid AXIOM_APP_ID\n");
		return -1;
	}

	err = open("/dev/axiom_dev_mem0", O_RDWR);
	DBG("open /dev/axiom_dev_mem0 returns %d\n", err);
	assert(err >= 0);

	evi_mem_hdlr.mem_dev_fd = err;

        DBG("addr = 0x%"PRIxPTR"\n", saddr);
        DBG("size = %zd\n", size);

	evi_mem_hdlr.vaddr_start = saddr;
	evi_mem_hdlr.vaddr_end = eaddr;

	request.base = saddr;
	request.size = eaddr - saddr;
#if 0
	/* TODO: re-enable when using new linker script */
	err = mprotect((void *)(saddr), size, PROT_NONE);
	if (err)
		return err;
#endif
	err = ioctl(evi_mem_hdlr.mem_dev_fd, AXIOM_MEM_DEV_CONFIG_VMEM, &request);
	if (err) {
		perror("ioctl");
		return err;
	}
	DBG("Config B:0x%lx S:%ld\n", request.base, request.size);

	err = ioctl(evi_mem_hdlr.mem_dev_fd, AXIOM_MEM_DEV_SET_APP_ID, &app_id);
	if (err) {
		perror("ioctl");
		return err;
	}

	err = add_region(psaddr, peaddr, EVI_PRIVATE_MEM, DEFAULT_PRIO);
	DBG("add_private_region err = %d\n", err);

	return err;
}

#if 0
static void *evi_request_private_region(unsigned long size)
{
        struct axiom_mem_dev_info request;
        unsigned long start_addr = (unsigned long)(evi_mem_hdlr.vaddr_start);
        void *mem;
        int err;
	int fd = evi_mem_hdlr.mem_dev_fd;

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
#endif

#if 0
static void *private_alloc_with_region(size_t nsize)
{
	size_t bs;
	void *addr;
	void *ptr = NULL;
	int err;

	bs = ((nsize + BLOCK_REQ_SIZE - 1) / BLOCK_REQ_SIZE)
	     * BLOCK_REQ_SIZE;
	DBG("Request region of size: %zu\n", bs);
	addr = evi_request_private_region(bs);
	DBG("private region address: %p\n", addr);

	if (addr == NULL)
		return ptr;

	err = evi_lmm_add_reg(&evi_mem_hdlr.almm, addr, bs,
				EVI_PRIVATE_MEM, DEFAULT_PRIO);
	if (err != EVI_LMM_OK)
		return ptr;

	ptr = evi_lmm_alloc(&evi_mem_hdlr.almm, nsize,
			    EVI_PRIVATE_MEM);

	return ptr;
}
#endif

void *evi_private_malloc(size_t sz)
{
	void *ptr;
	size_t nsize = sz + sizeof(size_t);

	pthread_mutex_lock(&evi_mem_hdlr.mutex);
	ptr = evi_lmm_alloc(&evi_mem_hdlr.almm, nsize, EVI_PRIVATE_MEM);

	if (ptr != NULL) {
		*((size_t *) ptr) = nsize;
		ptr = (void *)((uintptr_t)ptr + sizeof(size_t));
	}

	pthread_mutex_unlock(&evi_mem_hdlr.mutex);

	return ptr;
}

static void *request_shared_memory(size_t sz)
{
	void *ptr;

	/* TODO: to ask shared block to the server */
	ptr = evi_lmm_alloc(&evi_mem_hdlr.almm, sz, EVI_SHARE_MEM);

	return ptr;
}

void *evi_shared_malloc(size_t sz)
{
	void *ptr;
	size_t nsize = sz + sizeof(size_t);

	pthread_mutex_lock(&evi_mem_hdlr.mutex);
	ptr = evi_lmm_alloc(&evi_mem_hdlr.almm, nsize, EVI_SHARE_MEM);

	if (ptr == NULL)
		ptr = request_shared_memory(nsize);

	if (ptr != NULL) {
		*((size_t *) ptr) = nsize;
		ptr = (void *)((uintptr_t)ptr + sizeof(size_t));
	}

	pthread_mutex_unlock(&evi_mem_hdlr.mutex);

	return ptr;
}

static void evi_free(void *ptr)
{
	char *p;

	if (ptr == NULL) {
		DBG("Nothing to free: pointer is null!\n");
		return;
	}

	p = ((char *)ptr - sizeof(size_t));

	pthread_mutex_lock(&evi_mem_hdlr.mutex);

	if (*(size_t *)p > 0) {
		DBG("Freeing %zu bytes\n", *(size_t *)p);
		evi_lmm_free(&evi_mem_hdlr.almm, p, *(size_t *)p);
	} else {
		DBG("Invalid size: %p %zu \n", p, *(size_t *)p);
	}

	pthread_mutex_unlock(&evi_mem_hdlr.mutex);
}

void evi_private_free(void *ptr)
{
	evi_free(ptr);
}

void evi_shared_free(void *ptr)
{
	evi_free(ptr);
}
