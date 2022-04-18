#ifndef VECTOR_INTERFACE_H
#define VECTOR_INTERFACE_H

#include <stddef.h>
#include <inttypes.h>
#include <allocator_i.h>

#define VEC_MIN_SIZE												 10
#define VEC_REALLOC_SCALE_FACTOR						 2

#define VEC_OK															 0
#define VEC_ERR__MALLOC											-1
#define VEC_ERR__REALLOC										-2
#define VEC_ERR__STATIC_MODE								-10
#define VEC_ERR__NULL_VEC										-11
#define VEC_ERR__NULL_ELEM									-12
#define VEC_ERR__NULL_POS										-13
#define VEC_ERR__NULL_CMP_FN								-14
#define VEC_ERR__INVALID_POS								-15
#define VEC_ERR__ELEM_NOT_FOUND							-16
#define VEC_ERR__SHRINK											-17
#define VEC_ERR__NULL_CALLBACK							-18
#define VEC_ERR__EMPTY_VEC									-19
#define VEC_ERR__NULL_DATA									-20
#define VEC_ERR__DIFFERENT_TYPES						-21
#define VEC_ERR__INVALID_INDEX							-22
#define VEC_ERR__OBSERVER_CONSTRUCT  				-23
#define VEC_ERR__VECTOR_CONSTRUCT  					-24
#define VEC_ERR__ORDERED_MODE		  					-25

//FLAGS
#define VEC_FLAG__STATIC										(1 << 0)
#define VEC_FLAG__OBSERVED	    						(1 << 1)
#define VEC_FLAG__RECURSIVE_DESTRUCTION			(1 << 2)
#define VEC_FLAG__ORDERED										(1 << 3)

//ACTIONS
#define VEC_ACTION__MAKE_ORDERED						(1 << 0)
#define VEC_ACTION__MAKE_STATIC							(1 << 1)
#define VEC_ACTION__RESIZE									(1 << 2)

#define VEC_ACTION__APPEND									(1 << 3)
#define VEC_ACTION__ADD											(1 << 4)
#define VEC_ACTION__INSERT									(1 << 5)

#define VEC_ACTION__DESTRUCT								(1 << 6)
#define VEC_ACTION__ERASE										(1 << 7)
#define VEC_ACTION__CLEAR										(1 << 8)
#define VEC_ACTION__REPLACE									(1 << 9)

#define VEC_ACTION__SORT										(1 << 10)
#define VEC_ACTION__COPY										(1 << 11)
#define VEC_ACTION__FILTER									(1 << 12)
#define VEC_ACTION__SLICE										(1 << 13)
#define VEC_ACTION__RELEASE_DATA						(1 << 14)

//ACTION GROUPS
#define VEC_ACTION__ADDITION				(VEC_ACTION__APPEND | VEC_ACTION__ADD | VEC_ACTION__INSERT)
#define VEC_ACTION__REMOVING				(VEC_ACTION__DESTRUCT | VEC_ACTION__ERASE | VEC_ACTION__CLEAR | VEC_ACTION__REPLACE)

typedef struct tagVector* Vec;

//ACTION DATA
typedef struct {
	Vec vector;
	Vec filtered;
} filter_action_extra_t;

typedef struct {
	Vec vector;
	Vec slice;
} slice_action_extra_t;

typedef struct {
	Vec vector;
	size_t new_capacity;
} resize_action_extra_t;

typedef struct {
	Vec vector;
	void* pos;
	void* elem;
} insert_action_extra_t;

typedef struct {
	Vec vector;
	void* elem;
} add_action_extra_t;

typedef struct {
	Vec vector;
	Vec other;
} append_action_extra_t;

typedef struct {
	Vec vector;
	void* pos;
} erase_action_extra_t;

typedef struct {
	Vec vector;
	void* pos;
	void* elem;
} replace_action_extra_t;

typedef struct {
	//live cycle Vec
	Vec 			(*construct)(size_t elem_size);
	Vec 			(*construct_from_data)(size_t elem_size, void* data, size_t data_size);
	Vec				(*construct_with_allocator)(size_t elem_size, const AllocatorInterface allocator);
	int32_t		(*destruct)(Vec v);
	
	//new vector from this
	Vec 			(*copy)(const Vec v);
	Vec				(*filter)(const Vec v, int (*cb)(const void* elem, size_t index, void* extra), void* extra);
	Vec				(*slice)(const Vec v, size_t begin_index, size_t end_index);

	//state
	size_t 		(*size)(const Vec v);
	size_t 		(*capacity)(const Vec v);
	int32_t		(*empty)(const Vec v, size_t index);
	uint32_t	(*get_flags)(const Vec v);
	int32_t		(*clear_flags)(Vec v, uint32_t flags);
	int32_t		(*set_flags)(Vec v, uint32_t flags);
	int32_t		(*make_static)(Vec v);
	int32_t		(*is_static)(const Vec v);
	int32_t		(*set_recursive_destruction)(Vec v);
	int32_t		(*make_ordered)(Vec v);
	size_t		(*elem_size)(const Vec v);
	uint32_t	(*error)(const Vec v);

	//other
	int32_t		(*set_compare_fn)(Vec v, int32_t (*cmp)(const void* first, const void* second));
	int32_t		(*set_elem_destructor)(Vec v, void (*cb)(void* elem));
	void*			(*release_data)(Vec v);
	void*			(*get_data_copy)(Vec v);

	//capacity
	int32_t 	(*reserve)(Vec v, size_t capacity);
	int32_t 	(*resize)(Vec v, size_t capacity);

	//addition
	int32_t 	(*add)(Vec v, void* elem);
	int32_t 	(*insert)(Vec v, void* pos, void* elem);
	int32_t		(*append)(Vec v, const Vec other);
	
	//removing
	int32_t		(*clear)(Vec v);
	int32_t		(*erase)(Vec v, void* pos);
	int32_t		(*erase_at)(Vec v, size_t index);

	//access
	void*			(*at)(const Vec v, size_t index);
	void*			(*begin)(const Vec v);
	void*			(*end)(const Vec v);
	void*			(*back)(const Vec v);
	void*			(*next)(const Vec v, void* elem);
	int32_t		(*for_each)(Vec v, void (*cb)(void* elem, size_t index, void* extra), void* extra);
	int32_t		(*find)(const Vec v, void* elem, int (*cmp)(void* first, void* second));

	//modification
	int32_t 	(*replace)(Vec v, void* pos, void* elem);
	int32_t 	(*replace_at)(Vec v, size_t index, void* elem);
	int32_t		(*sort)(Vec v);

	//notification
	int32_t		(*subscribe)(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra), void* cb_extra, int auto_free_extra);
	void*			(*unsubscribe)(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra));

} VectorInterface;

extern VectorInterface iVec;

#endif
