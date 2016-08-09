#ifndef FREELIST_H
#define FREELIST_H

#define FREELIST_OK             0
#define FREELIST_INVALID_IDX   -1
#define FREELIST_INVALID_ARGS  -100

typedef struct freelist_s {
	int free_p;
#ifdef FREELIST_USED_LIST_EN
	int used_p;
#endif
	int n_elem;
	int *idx_vec;
} freelist_t;

int freelist_init(freelist_t *fl, int *idx_vec, int n_elem);
freelist_t *freelist_init_from_buffer(void *buf, size_t buf_size);
int freelist_alloc_idx(freelist_t *fl);
int freelist_free_idx(freelist_t *fl, int idx);

#define FREELIST_SPACE(n_elem) (sizeof(freelist_t) + n_elem * sizeof(int))

#endif /* FREELIST_H */
