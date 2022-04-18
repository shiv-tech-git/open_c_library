#include <stddef.h>
#include <stdlib.h>

#include "vec_i.h"
#include "allocator_i.h"
#include "observer_i.h"


struct tagVector {
	size_t 		size;
  size_t    elem_size;
	size_t 		capacity;
	int32_t 	flags;
	char* 		data;
	uint32_t	error;
	int32_t 	(*cmp_fn)(const void* first, const void* second);
	void 			(*elem_destructor)(void* cb_extra);
	Observer	observer;
	const AllocatorInterface* allocator;
};

static Vec construct_with_allocator_and_data(size_t elem_size, AllocatorInterface* allocator, void* data, size_t data_size) {

  if (allocator == NULL) {
    allocator = &CurrentAllocator;
  }

  Vec vec = allocator->malloc(sizeof(struct tagVector));

  if (vec == NULL) {
    return NULL;
  }

  if (data == NULL) {
    vec->capacity = data_size;
    vec->size = data_size;
    vec->data = (char*) data;
  }
  else {
    vec->capacity = 0;
    vec->size = 0;
    vec->data = NULL;
  }

  vec->elem_size = elem_size;
  vec->allocator = allocator;
  vec->observer = NULL;
  vec->elem_destructor = NULL;
  vec->cmp_fn = NULL;
  vec->error = 0;
  vec->flags = 0;

  return vec;
}

//live cycle Vec
static Vec construct(size_t elem_size) {
  if (elem_size == 0) {
    return NULL;
  }
  return construct_with_allocator_and_data(elem_size, CurrentAllocator, NULL, 0);
}

static Vec construct_from_data(size_t elem_size, void* data, size_t data_size) {
  if (elem_size == 0 || data == NULL || data_size == NULL) {
    return NULL;
  }
  return construct_with_allocator_and_data(elem_size, CurrentAllocator, data, data_size);
}

static Vec construct_with_allocator(size_t elem_size, AllocatorInterface* allocator) {
  if (elem_size == 0 || allocator == NULL) {
    return NULL;
  }
  return construct_with_allocator_and_data(elem_size, allocator, NULL, 0);
}

static int32_t destruct(Vec v) {

  iObserver.notify(v->observer, VEC_ACTION__DESTRUCT, v);

  if (v->elem_destructor != NULL) {
    for (size_t i = 0; i < v->size; i++) {
      v->elem_destructor(v->data + (i * v->elem_size));
    }
  }

  //destruct observer
  if (v->observer != NULL) {
    iObserver.destruct(v->observer);
  }

  //destruct data
  if (v->data != NULL) {
    v->allocator->free(v->data);
  }

  //destruct vec
  AllocatorInterface* allocator = v->allocator;
  allocator->free(v);
}

//new vector from this
Vec copy(const Vec v) {
  char* data = v->allocator->malloc(v->size * v->elem_size);
  if (data == NULL) {
    v->error = VEC_ERR__MALLOC;
    return VEC_ERR__MALLOC;
  }

  iObserver.notify(v->observer, VEC_ACTION__COPY, v);
  memcpy(data, v->data, v->size * v->elem_size);
  return construct_from_data(v->elem_size, data, v->size);
}

Vec filter(const Vec v, int (*cb)(const void* elem, size_t index, void* extra), void* extra) {
  Vec filtered = construct(v->elem_size);

  if (filtered == NULL) {
    v->error = VEC_ERR__VECTOR_CONSTRUCT;
    return NULL;
  }

  for (size_t i = 0; i < v->size; i++) {
    char* elem = v->data + (i * v->elem_size);
    if (cb(elem, i, extra) == 0) {
      add(filtered, elem);
    }
  }

  filter_action_extra_t fd = { v, filtered };
  iObserver.notify(v->observer, VEC_ACTION__FILTER, &fd);

  return filtered;
}

Vec slice(const Vec v, size_t begin_index, size_t end_index) {

  if (begin_index >= end_index) {
    v->error = VEC_ERR__INVALID_INDEX;
    return NULL;
  }

  size_t size = end_index - begin_index;

  char* data = v->allocator->malloc(size * v->elem_size);
  if (data == NULL) {
    v->error = VEC_ERR__MALLOC;
    return VEC_ERR__MALLOC;
  }

  memcpy(data, v->data + (begin_index * v->elem_size), size * v->elem_size);
  Vec slice = construct_from_data(v->elem_size, data, size);

  slice_action_extra_t sd = { v, slice };
  iObserver.notify(v->observer, VEC_ACTION__SLICE, &sd);

  return slice;
}


//state
size_t size(const Vec v) {
  return v->size;
}

size_t capacity(const Vec v) {
  return v->capacity;
}

int32_t empty(const Vec v, size_t index) {
  return v->size == 0;
}

uint32_t get_flags(const Vec v) {
  return v->flags;
}

int32_t clear_flags(Vec v, uint32_t flags) {
  return v->flags &= ~flags;
}

int32_t set_flags(Vec v, uint32_t flags) {
  return v->flags |= flags;
}

int32_t make_static(Vec v) {
  iObserver.notify(v->observer, VEC_ACTION__MAKE_STATIC, v);
  v->flags |= VEC_FLAG__STATIC;
}

int32_t is_static(const Vec v) {
  return v->flags & VEC_FLAG__STATIC > 0;
}

int32_t set_recursive_destruction(Vec v) {
  v->flags |= VEC_FLAG__RECURSIVE_DESTRUCTION;
  return v->flags;
}

int32_t make_ordered(Vec v) {
  if (v->cmp_fn == NULL) {
    v->error = VEC_ERR__NULL_CMP_FN;
    return VEC_ERR__NULL_CMP_FN;
  }

  v->flags |= VEC_FLAG__ORDERED;
  sort(v);
  iObserver.notify(v->observer, VEC_ACTION__MAKE_ORDERED, v);
}

size_t elem_size(const Vec v) {
  return v->elem_size;
}

uint32_t error(const Vec v) {
  return v->error;
}


//other
int32_t set_compare_fn(Vec v, int32_t(*cmp)(const void* first, const void* second)) {
  v->cmp_fn = cmp;
}

int32_t set_elem_destructor(Vec v, void (*cb)(void* elem)) {
  v->elem_destructor = cb;
}

void* release_data(Vec v) {
  iObserver.notify(v->observer, VEC_ACTION__RELEASE_DATA, v);
  //destruct observer
  iObserver.destruct(v->observer);

  void* data = v->data;

  //destruct vec
  AllocatorInterface* allocator = v->allocator;
  allocator->free(v);

  return data;
}

void* get_data_copy(Vec v) {
  size_t size = v->size * v->elem_size;
  //use malloc coz returning data is not part of vetcor anymore
  char* data = malloc(size);
  if (data == NULL) {
    return NULL;
  }

  memcpy(data, v->data, size);
  return data;
}

//capacity
int32_t reserve(Vec v, size_t capacity) {
  if (size < VEC_MIN_SIZE) {
    capacity = VEC_MIN_SIZE;
  }

  if (v->data == NULL) {
    v->data = v->allocator->malloc(capacity * v->elem_size);
    if (v->data == NULL) {
      v->error = VEC_ERR__MALLOC;
      return VEC_ERR__MALLOC;
    }
  }

  return VEC_OK;
}

int32_t resize(Vec v, size_t capacity) {
  if (v == NULL) {
    return VEC_ERR__NULL_VEC;
  }

  if (v->data == NULL) {
    v->error = VEC_ERR__NULL_DATA;
    return VEC_ERR__NULL_DATA;
  }

  if (v->flags & VEC_FLAG__STATIC) {
    v->error = VEC_ERR__STATIC_MODE;
    return VEC_ERR__STATIC_MODE;
  }

  size_t new_capacity = capacity < VEC_MIN_SIZE ? VEC_MIN_SIZE : capacity;

  resize_action_extra_t rd = { v, new_capacity };
  iObserver.notify(v->observer, VEC_ACTION__RESIZE, &rd);

  if (new_capacity < v->size) {
    for (size_t i = new_capacity; i < v->size; i++) {
      v->elem_destructor(v->data + (i * v->elem_size));
    }
  }

  void* tmp = v->allocator->realloc(v->data, (new_capacity * v->elem_size));
  if (tmp == NULL) {
    v->error = VEC_ERR__REALLOC;
    return VEC_ERR__REALLOC;
  }

  v->data = tmp;
  v->capacity = new_capacity;
  v->size = new_capacity < v->size ? new_capacity : v->size;

  return 0;
}

int32_t _is_correct_pos(Vec v, void* _pos) {
  char* pos = _pos;
  char* last_elem_pos = v->data + ((v->size - 1) * v->elem_size);

  if (pos < v->data || pos > last_elem_pos) {
    return 0;
  }

  ptrdiff_t shift = pos - v->data;
  if ((shift % v->elem_size) != 0) {
    return 0;
  }

  return 1;
}

int32_t insert(Vec v, void* pos, void* elem) {
  int32_t res = 0;

  if (v == NULL) {
    return VEC_ERR__NULL_VEC;
  }

  if (v->flags & VEC_FLAG__ORDERED) {
    v->error = VEC_ERR__ORDERED_MODE;
    return VEC_ERR__ORDERED_MODE;
  }

  if (elem == NULL) {
    v->error = VEC_ERR__NULL_ELEM;
    return VEC_ERR__NULL_ELEM;
  }

  if (pos == NULL) {
    v->error = VEC_ERR__NULL_POS;
    return VEC_ERR__NULL_POS;
  }

  if (!_is_correct_pos(v, pos)) {
    v->error = VEC_ERR__INVALID_POS;
    return VEC_ERR__INVALID_POS;
  }

  //if data not allocated yet -> allocate it
  if (v->data == NULL) {
    res = reserve(v, VEC_MIN_SIZE);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  //if capacity is full -> realloc
  if (v->size == v->capacity) {
    res = resize(v, v->capacity * VEC_REALLOC_SCALE_FACTOR);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  insert_action_extra_t id = { v, pos, elem };
  iObserver.notify(v->observer, VEC_ACTION__INSERT, &id);

  char* _pos = (char*)pos;
  char* last_elem_last_byte = v->data + (v->size * v->elem_size) - 1;
  for (char* ptr = last_elem_last_byte; ptr >= _pos; ptr--) {
    ptr[v->elem_size] = *ptr;
  }

  memcpy(pos, elem, v->elem_size);
  v->size++;

  return VEC_OK;
}


static int32_t _ordered_insert(Vec v, void* elem) {
  if (v->cmp_fn == NULL) {
    return VEC_ERR__NULL_CMP_FN;
  }

  char* it = v->data;
  char* end = v->data + (v->size * v->elem_size);

  for (; it < end; it += v->elem_size) {
    if (v->cmp_fn(elem, it) > 0) {
      //insert
      char* last_elem_last_byte = v->data + (v->size * v->elem_size) - 1;
      for (char* ptr = last_elem_last_byte; ptr >= it; ptr--) {
        ptr[v->elem_size] = *ptr;
      }
      memcpy(it, elem, v->elem_size);
      v->size++;

      return VEC_OK;
    }
  }

  //push back
  memcpy(v->data + (v->size * v->elem_size), elem, v->elem_size);
  v->size++;

  return VEC_OK;
}

//addition
int32_t add(Vec v, void* elem) {
  int32_t res = 0;

  //if data not allocated yet -> allocate it
  if (v->data == NULL) {
    res = reserve(v, VEC_MIN_SIZE);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  //if capacity is full -> realloc
  if (v->size == v->capacity) {
    res = resize(v, v->capacity * VEC_REALLOC_SCALE_FACTOR);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  add_action_extra_t ad = { v, elem };
  iObserver.notify(v->observer, VEC_ACTION__ADD, &ad);

  if (v->flags & VEC_FLAG__ORDERED) {
    return _ordered_insert(v, elem);
  }

  memcpy(v->data + (v->size * v->elem_size), elem, v->elem_size);
  v->size++;
  return VEC_OK;
}

int32_t append(Vec v, const Vec other) {
  int32_t res = 0;

  if (v == NULL || other == NULL) {
    return VEC_ERR__NULL_VEC;
  }

  //differen data type size 
  if (v->elem_size != other->elem_size) {
    v->error = VEC_ERR__DIFFERENT_TYPES;
    return VEC_ERR__DIFFERENT_TYPES;
  }

  //if data not allocated yet -> allocate it
  if (v->data == NULL) {
    res = reserve(v, other->size);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  //aquire needed capasity
  size_t needed_capacity = v->size + other->size;
  if (needed_capacity > v->capacity) {
    res = resize(v, needed_capacity);
    if (res < 0) {
      v->error = res;
      return res;
    }
  }

  append_action_extra_t ad = { v, other };
  iObserver.notify(v->observer, VEC_ACTION__APPEND, &ad);

  memcpy(&v->data + (v->size * v->elem_size), &other->data, other->elem_size * other->size);

  return VEC_OK;
}


//removing
int32_t clear(Vec v) {
  iObserver.notify(v->observer, VEC_ACTION__CLEAR, &v);

  v->size = 0;
  return VEC_OK;
}

int32_t erase(Vec v, void* pos) {
  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (pos == NULL) {
    v->error = VEC_ERR__NULL_POS;
    return VEC_ERR__NULL_POS;
  }

  if (!_is_correct_pos(v, pos)) {
    v->error = VEC_ERR__INVALID_POS;
    return VEC_ERR__INVALID_POS;
  }

  erase_action_extra_t ed = { v, pos };
  iObserver.notify(v->observer, VEC_ACTION__ERASE, &ed);

  char* _pos = pos;
  char* last_elem = v->data + ((v->size - 1) * v->elem_size);

  v->elem_destructor(pos);

  for (char* ptr = _pos; ptr < last_elem; ptr++) {
    *ptr = ptr[v->elem_size];
  }

  v->size--;

  return VEC_OK;
}

int32_t erase_at(Vec v, size_t index) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (index >= v->size) {
    v->error = VEC_ERR__INVALID_INDEX;
    return VEC_ERR__INVALID_INDEX;
  }

  char* pos = v->data + (index * v->elem_size);

  v->elem_destructor(pos);
  return erase(v, pos);
}


//access
void* at(const Vec v, size_t index) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (index >= v->size) {
    v->error = VEC_ERR__INVALID_INDEX;
    return VEC_ERR__INVALID_INDEX;
  }

  return v->data + (index * v->elem_size);
}

void* begin(const Vec v) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  return v->data;
}

void* end(const Vec v) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  return v->data + (v->size * v->elem_size);
}

void* back(const Vec v) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  return v->data + (v->size * (v->elem_size - 1));
}

void* next(const Vec v, void* elem) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (elem == NULL) {
    return v->data;
  }

  if (!_is_correct_pos(v, elem)) {
    v->error = VEC_ERR__INVALID_POS;
    return VEC_ERR__INVALID_POS;
  }

  char* _elem = (char*)elem;

  char* next = _elem + v->elem_size;
  char* end = v->data + (v->size * v->elem_size);
  if (next == end) {
    return NULL;
  }

  return next;
}

int32_t for_each(Vec v, void (*cb)(void* elem, size_t index, void* extra), void* extra) {
  if (v == NULL) {
    return VEC_ERR__NULL_VEC;
  }

  if (cb == NULL) {
    v->error = VEC_ERR__NULL_CALLBACK;
    return VEC_ERR__NULL_CALLBACK;
  }

  if (v->size == 0) {
    v->error = VEC_ERR__EMPTY_VEC;
    return VEC_ERR__EMPTY_VEC;
  }

  char* ptr = NULL;
  for (size_t i = 0; i < v->size; i++) {
    ptr = v->data + (i * v->elem_size);
    cb(ptr, i, extra);
  }

  return VEC_OK;
}

int32_t find(const Vec v, void* elem, int (*cmp)(void* first, void* second)) {
  if (v == NULL) {
    return VEC_ERR__NULL_VEC;
  }

  if (cmp == NULL) {
    cmp = v->cmp_fn;

    if (cmp == NULL) {
      v->error = VEC_ERR__NULL_CALLBACK;
      return VEC_ERR__NULL_CALLBACK;
    }
  }

  if (v->size == 0) {
    v->error = VEC_ERR__EMPTY_VEC;
    return VEC_ERR__EMPTY_VEC;
  }

  char* ptr = NULL;
  for (size_t i = 0; i < v->size; i++) {
    ptr = v->data + (i * v->elem_size);
    if (cmp(ptr, elem) == 0) {
      return ptr;
    }
  }

  return NULL;
}


//modification
int32_t replace(Vec v, void* pos, void* elem) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (pos == NULL) {
    v->error = VEC_ERR__NULL_POS;
    return VEC_ERR__NULL_POS;
  }

  if (!_is_correct_pos(v, pos)) {
    v->error = VEC_ERR__INVALID_POS;
    return VEC_ERR__INVALID_POS;
  }

  replace_action_extra_t rd = { v, pos, elem};
  iObserver.notify(v->observer, VEC_ACTION__REPLACE, &rd);

  v->elem_destructor(pos);
  memcpy(pos, elem, v->elem_size);
  return VEC_OK;
}

int32_t replace_at(Vec v, size_t index, void* elem) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (index >= v->size) {
    v->error = VEC_ERR__INVALID_INDEX;
    return VEC_ERR__INVALID_INDEX;
  }

  char* pos = v->data + (index * v->elem_size);
  return replace(v, pos, elem);
}

int32_t sort(Vec v) {

  if (v == NULL) {
    return VEC_ERR__NULL_POS;
  }

  if (v->cmp_fn == NULL) {
    v->error = VEC_ERR__NULL_CMP_FN;
    return VEC_ERR__NULL_CMP_FN;
  }

  iObserver.notify(v->observer, VEC_ACTION__SORT, v);

  qsort(v->data, v->size, v->elem_size, v->cmp_fn);
}

//notification
int32_t subscribe(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra), void* cb_extra, int auto_free_extra) {
  if (v->observer == NULL) {
    v->observer = iObserver.construct();
    if (v->observer) {
      v->error = VEC_ERR__OBSERVER_CONSTRUCT;
      return VEC_ERR__OBSERVER_CONSTRUCT;
    }
  }

  return iObserver.subscribe(v->observer, action_mask, cb, cb_extra, auto_free_extra);
}

void*  unsubscribe(Vec v, uint64_t action_mask, void (*cb)(uint64_t action_flag, const void* calling_extra, void* cb_extra)) {
  return iObserver.unsubscribe(v->observer, action_mask, cb);
}

VectorInterface iVec = {
  .construct = construct,
  .construct_from_data = construct_from_data,
  .construct_with_allocator = construct_with_allocator,
  .destruct = destruct,

  .copy = copy,
  .filter = filter,
  .slice = slice,

  .size = size,
  .capacity = capacity,
  .empty = empty,
  .get_flags = get_flags,
  .clear_flags = clear_flags,
  .set_flags = set_flags,
  .make_static = make_static,
  .is_static = is_static,
  .set_recursive_destruction = set_recursive_destruction,
  .make_ordered = make_ordered,
  .elem_size = elem_size,
  .error = error,

  .set_compare_fn = set_compare_fn,
  .release_data = release_data,
  .get_data_copy = get_data_copy,

  .reserve = reserve,
  .resize = resize,

  .add = add,
  .insert = insert,
  .append = append,

  .clear = clear,
  .erase = erase,
  .erase_at = erase_at,

  .at = at,
  .begin = begin,
  .end = end,
  .back = back,
  .next = next,
  .for_each = for_each,
  .find = find
};

