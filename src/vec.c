#include "vec_i.h"

typedef struct {
	size_t 		size;
	size_t 		capacity;
	int32_t 	flags;
	char* 		data;
	uint32_t	error;
	int32_t 	(*cmp_fn)(const void* first, const void* second);
	void 		(*on_destruct_cb)(Vec v, void* cb_extra);
	const AllocatorInterface allocator;
} Vector_t;