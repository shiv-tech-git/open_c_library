#ifndef LIGHT_VECTOR_INTERFACE_H
#define LIGHT_VECTOR_INTERFACE_H

#include <stdlib.h>
#include <inttypes.h>

#define LVEC_START_CAPACITY					8
#define	LVEC_REALLOC_SCALE_FACTOR		2

typedef struct tagLightVector* LVec;

typedef struct {
	LVec 			(*construct)(size_t elem_size);
	void			(*destruct)(LVec lvec);
	size_t 		(*size)(const LVec lvec);
	size_t		(*elem_size)(const LVec lvec);
	void*			(*data)(const LVec lvec);
	int32_t		(*add)(LVec lvec, void* elem);
	int32_t		(*erase_at)(LVec lvec, size_t index);
} LightVectorInterface;

extern LightVectorInterface iLVec;

#endif