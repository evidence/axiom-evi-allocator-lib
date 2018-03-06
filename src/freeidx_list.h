/*!
 * \file freeidx_list.h
 *
 * \version     v1.1
 * \date        2016-09-23
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef FREELIST_H
#define FREELIST_H

/**
 * \addtogroup EVI_LMM
 *
 * \{
 */

#define FREELIST_OK             0
#define FREELIST_INVALID_IDX   -1
#define FREELIST_INVALID_ARGS  -100

typedef struct freeidx_list_s {
	/** First free index */
	int free_p;
#ifdef FREELIST_USED_LIST_EN
	int used_p;
#endif
	/** Number of indexes in this free index list */
	int n_elem;
	/** Vector of indexes */
	int *idx_vec;
} freeidx_list_t;

int freeidx_list_init(freeidx_list_t *fl, int *idx_vec, int n_elem);
freeidx_list_t *freeidx_list_init_from_buffer(void *buf, size_t buf_size);
int freeidx_list_alloc_idx(freeidx_list_t *fl);
int freeidx_list_free_idx(freeidx_list_t *fl, int idx);
int freeidx_list_elem_for_memblock(size_t mem_size, size_t elem_size);

#define FREELIST_SPACE(n_elem) (sizeof(freeidx_list_t) + n_elem * sizeof(int))

/** \} */

#endif /* FREELIST_H */
