#include <stddef.h>
#include <inttypes.h>

#include <stdio.h>

#include <freelist.h>

int freelist_init(freelist_t *fl, int *idx_vec, int n_elem)
{
	int i;

	if (n_elem < 0 || fl == NULL || idx_vec == NULL)
		return FREELIST_INVALID_ARGS;

	fl->free_p = 0;
#ifdef FREELIST_USED_LIST_EN
	fl->used_p = FREELIST_INVALID_IDX;
#endif
	fl->n_elem = n_elem;
	fl->idx_vec = idx_vec;

	for (i = 0; i < n_elem - 1; ++i)
		fl->idx_vec[i] = i + 1;

	fl->idx_vec[n_elem - 1] = FREELIST_INVALID_IDX;

	return FREELIST_OK;
}

freelist_t *freelist_init_from_buffer(void *buf, size_t buf_size)
{
	freelist_t *res = NULL;
	int *idx_vec;
	int n_elem;

	if (buf_size < sizeof(*res) + sizeof(*res->idx_vec)) {
		fprintf(stderr, "Not enough room for freelist "
			"(freelist_t=%zu)\n", sizeof(*res));
		return NULL;
	}

	res = (freelist_t *)buf;
	idx_vec = (int *)((uintptr_t)buf + sizeof(*res));
	n_elem = (int)((uintptr_t)buf + buf_size - (uintptr_t)idx_vec)
		      / sizeof(*res->idx_vec);

	fprintf(stderr, "buf=%p s:%zu (n_elem=%d, estimate=%zu, wasted=%zu) \n", buf,
		buf_size, n_elem, FREELIST_SPACE(n_elem), buf_size - FREELIST_SPACE(n_elem));
	fprintf(stderr, "res=%p idx_vec[%d] -> %p\n", res, n_elem, idx_vec);

	if (freelist_init(res, idx_vec, n_elem) != FREELIST_OK)
		return NULL;

	return res;
}

int freelist_alloc_idx(freelist_t *fl)
{
	int idx;

	if (fl->free_p == FREELIST_INVALID_IDX)
		return FREELIST_INVALID_IDX;

	idx = fl->free_p;
	fl->free_p = fl->idx_vec[fl->free_p];
#ifdef FREELIST_USED_LIST_EN
	fl->idx_vec[idx] = fl->used_p;
	fl->used_p = idx;
#else
	fl->idx_vec[idx] = FREELIST_INVALID_IDX;
#endif
	fprintf(stderr, "%s] idx = %d\n", __func__, idx);
	
	return idx;
}

int freelist_free_idx(freelist_t *fl, int idx)
{
	if (idx < 0 || idx >= fl->n_elem) {
		fprintf(stderr, "%s] idx = %d\n", __func__, FREELIST_INVALID_IDX);
		return FREELIST_INVALID_IDX;
	}

#ifdef FREELIST_USED_LIST_EN
	if (fl->used_p == idx) {
		fl->used_p = fl->idx_vec[idx];
	} else {
		int i, n;

		for (i = fl->used_p; i != FREELIST_INVALID_IDX;
		     i = fl->vec_idx[i]) {

			n = fl->vec_idx[i];
			if (n == idx) {
				fl->vec_idx[i] = fl->vec_idx[n];
				break;
			}
		}
		if (i == FREELIST_INVALID_IDX)
			return FREELIST_INVALID_IDX;
	}
#endif
	fl->idx_vec[idx] = fl->free_p;
	fl->free_p = idx;
	DBG("idx = %d\n", idx);
	
	return FREELIST_OK;
}

int freelist_elem_for_memblock(size_t mem_size, size_t elem_size)
{
	freelist_t *fl;

	return (mem_size - sizeof(freelist_t))
		/ (elem_size + sizeof(fl->free_p));
}
