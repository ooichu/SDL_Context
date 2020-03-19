/*
 * File: dynarr.h
 * Autor: @ooichu
 * Description: very basic dynamic array implementation.
 * Only for internal using in SDL_Context.
 */

#include <stdlib.h> /* malloc, realloc, free */

// Note: this implementation do not check if memory is out and etc.

#define dynarr_t(t) \
	struct { t* pool; unsigned long length, allocated; size_t bytesPerBlock; }

#define dynarr_init(t, a) \
	do { \
		(a).pool = malloc(0); \
		(a).length = (a).allocated = 0; \
		(a).bytesPerBlock = sizeof(t); \
	} while (0)

#define dynarr_free(a) \
	do { \
		free((a).pool);	\
		(a).length = (a).allocated = 0; \
	} while (0)

#define dynarr_resize(a, sz) \
	(a).pool = realloc((a).pool, ((a).allocated = (sz)) * (a).bytesPerBlock)

#define dynarr_push(a, val) \
	(a).pool[(a).length++] = val

#define dynarr_pop(a) \
	(a).pool[--(a).length]

