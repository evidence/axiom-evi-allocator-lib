#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <freeidx_list.h>

int main()
{
	int i;
	int err;

	int vec_idx[100];
	freeidx_list_t fl;

	char buf[108];
	freeidx_list_t *block_fl;

	err = freeidx_list_init(&fl, vec_idx,
			    sizeof(vec_idx) / sizeof(vec_idx[0]));

	block_fl = freeidx_list_init_from_buffer(buf, sizeof(buf));

#define NTR 20
#define TMIN  7
#define TMAX  14
	for (i = 0; i < NTR; ++i) {
		printf("%d) a:%d\n", i, freeidx_list_alloc_idx(&fl));
		if (i > TMIN && i < TMAX) {
			err = freeidx_list_free_idx(&fl, i - TMIN);
			printf("%d) d:%d err=%d\n", i, i - TMIN, err);
		}
	}

	return 0;
}
