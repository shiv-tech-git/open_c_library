#ifndef LIGHT_VECTOR_INTERFACE_H
#define LIGHT_VECTOR_INTERFACE_H

#include <stdlib.h>
#include <inttypes.h>

typedef struct tagLightVector LightVector_t;
typedef struct LightVector_t* LVec;

typedef struct {
	LVec 		(*construct)(size_t elem_size);
	int32_t		(*destruct)(LVec v);
	size_t 		(*size)(const LVec v);
	size_t		(*elem_size)(const LVec v);
	int32_t		(*add)(LVec v, void* elem);
	int32_t		(*erase_at)(LVec v, size_t index);
	void*		(*data)(const LVec v);
} LightVectorInterface;

extern LightVectorInterface iLVec;

#endif