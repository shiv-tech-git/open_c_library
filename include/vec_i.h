#ifndef VECTOR_INTERFACE_H
#define VECTOR_INTERFACE_H

#include <stddef.h>
#include <inttypes.h>
#include <allocator_i.h>

//FLAGS
#define VECTOR_FLAG__STATIC					(1 << 0)
#define VECTOR_FLAG__OBSERVED	    		(1 << 1)
#define VECTOR_FLAG__RECURSIVE_DESTRUCTION	(1 << 2)
#define VECTOR_FLAG__ORDERED				(1 << 3)
#define VECTOR_FLAG__SORTED					(1 << 4)

//ACTIONS
#define VECTOR_ACTION__MAKE_ORDERED			(1 << 0)
#define VECTOR_ACTION__MAKE_STATIC			(1 << 1)
#define VECTOR_ACTION__RESIZE				(1 << 2)

#define VECTOR_ACTION__APPEND				(1 << 3)
#define VECTOR_ACTION__ADD					(1 << 4)
#define VECTOR_ACTION__INSERT				(1 << 5)

#define VECTOR_ACTION__DESTRUCT				(1 << 6)
#define VECTOR_ACTION__ERASE				(1 << 7)
#define VECTOR_ACTION__CLEAR				(1 << 8)
#define VECTOR_ACTION__REPLACE				(1 << 9)

#define VECTOR_ACTION__SORT					(1 << 10)

//ACTION GROUPS
#define VECTOR_ACTION__ADDITION				(VECTOR_ACTION__APPEND | VECTOR_ACTION__ADD | VECTOR_ACTION__INSERT)
#define VECTOR_ACTION__REMOVING				(VECTOR_ACTION__DESTRUCT | VECTOR_ACTION__ERASE | VECTOR_ACTION__CLEAR | VECTOR_ACTION__REPLACE)


typedef struct tagVector Vector_t;
typedef Vector_t* Vec;

typedef struct {
	//live cycle Vec
	Vec 		(*construct)(size_t elem_size);
	Vec 		(*construct_from_array)(size_t elem_size, void* array, size_t array_size);
	Vec			(*construct_with_allocator)(size_t elem_size, const AllocatorInterface allocator);
	int32_t		(*destruct)(Vec v);
	
	//new vector from this
	Vec 		(*clone)(const Vec v);
	Vec			(*filter)(const Vec v, int (*cb)(const void* elem, size_t index, void* extra), void* extra);
	Vec			(*slice)(const Vec v, size_t begin_index, size_t end_index);

	//state
	size_t 		(*size)(const Vec v);
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
	int32_t		(*set_on_destruct_cb)(Vec v, void (*cb)(Vec v, void* cb_extra), void* cb_extra);
	int32_t		(*set_allocator)(Vec v, AllocatorInterface allocator);

	//capacity
	int32_t 	(*reserve)(Vec v, size_t size);
	int32_t 	(*resize)(Vec v, size_t size);

	//addition
	int32_t 	(*add)(Vec v, void* elem);
	int32_t 	(*insert)(Vec v, void* pos, void* elem);
	int32_t		(*append)(Vec v, const Vec other);
	
	//removing
	int32_t		(*clear)(Vec v);
	int32_t		(*erase)(Vec v, void* elem);
	int32_t		(*erase_at)(Vec v, size_t index);

	//access
	void*		(*at)(const Vec v, size_t index);
	void*		(*begin)(const Vec v);
	void*		(*end)(const Vec v);
	void*		(*back)(const Vec v);
	void*		(*next)(const Vec v, void* elem);
	int32_t		(*for_each)(Vec v, void (*cb)(void* elem, size_t index, void* extra), void* extra);
	int32_t		(*find)(const Vec v, void* elem);

	//modification
	int32_t 	(*replace)(Vec v, void* pos, void* elem);
	int32_t 	(*replace_at)(Vec v, size_t index, void* elem);
	int32_t		(*sort)(Vec v);

	//notification
	int32_t		(*subscribe)(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra), void* cb_extra);
	void*		(*unsubscribe)(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra));

} VectorInterface;

extern VectorInterface iVec;

#endif
