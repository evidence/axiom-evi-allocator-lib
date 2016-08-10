#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <time.h>

#ifdef DEBUG_NO_TIME
#define _dbg_printf(_fmt, ... )                                             \
	do {                                                                \
		fprintf(stderr, "%s: " _fmt "%s", __func__, __VA_ARGS__);   \
		fflush(stderr);                                             \
	} while (0);
#else
#define _dbg_printf(_fmt, ... )                                               \
	do {                                                                  \
		struct timespec tp;                                           \
		clock_gettime(CLOCK_REALTIME, &tp);                           \
		fprintf(stderr, "[%d.%ld] %s: " _fmt "%s",                    \
			(int)tp.tv_sec, tp.tv_nsec, __func__, __VA_ARGS__);   \
		fflush(stderr);                                               \
	} while (0);
#endif

#ifdef DEBUG
#define DBG(...) _dbg_printf(__VA_ARGS__, "")
#else
#define DBG(...) do {} while(0);
#endif

#define DBG_F(...) _dbg_printf(__VA_ARGS__, "")

#endif /* DEBUG_H */
