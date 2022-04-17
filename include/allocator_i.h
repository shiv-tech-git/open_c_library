#ifndef ALLOCATOR_INTERFACE_H
#define ALLOCATOR_INTERFACE_H

#include <stdlib.h>

typedef struct {
    void*		(*malloc)(size_t);
    void 		(*free)(void *);
    void*		(*realloc)(void *,size_t);
    void*		(*calloc)(size_t,size_t);
} AllocatorInterface;

extern AllocatorInterface DefaultAllocator;

#endif