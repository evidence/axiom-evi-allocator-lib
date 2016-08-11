#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <freeidx_list.h>

int main()
{
	int i;
	int err;

	int vec_idx[100];
	freelist_t fl;

	char buf[108];
	freelist_t *block_fl;

	err = freelist_init(&fl, vec_idx,
			    sizeof(vec_idx) / sizeof(vec_idx[0]));

	block_fl = freelist_init_from_buffer(buf, sizeof(buf));

#define NTR 20
#define TMIN  7
#define TMAX  14
	for (i = 0; i < NTR; ++i) {
		printf("%d) a:%d\n", i, freelist_alloc_idx(&fl));
		if (i > TMIN && i < TMAX) {
			err = freelist_free_idx(&fl, i - TMIN);
			printf("%d) d:%d err=%d\n", i, i - TMIN, err);
		}
	}

	return 0;
}
