#include <stdlib.h>

#include "allocator_i.h"

AllocatorInterface DefaultAllocator = { malloc, free, realloc, calloc };
AllocatorInterface* CurrentAllocator = &DefaultAllocator;