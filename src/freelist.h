#ifndef FREELIST_H
#define FREELIST_H

#define FREELIST_OK             0
#define FREELIST_INVALID_IDX   -1
#define FREELIST_INVALID_ARGS  -100

typedef struct freelist_s {
	/** First free index */
	int free_p;
#ifdef FREELIST_USED_LIST_EN
	int used_p;
#endif
	/** Number of indexes in this free index list */
	int n_elem;
	/** Vector of indexes */
	int *idx_vec;
} freelist_t;

int freelist_init(freelist_t *fl, int *idx_vec, int n_elem);
freelist_t *freelist_init_from_buffer(void *buf, size_t buf_size);
int freelist_alloc_idx(freelist_t *fl);
int freelist_free_idx(freelist_t *fl, int idx);
int freelist_elem_for_memblock(size_t mem_size, size_t elem_size);

#define FREELIST_SPACE(n_elem) (sizeof(freelist_t) + n_elem * sizeof(int))

#endif /* FREELIST_H */
