#include <stdlib.h>

#include "allocator_i.h"
#include "lvec_i.h"

struct tagLightVector {
	size_t 		size;
	size_t 		elem_size;
	size_t 		capacity;
	char* 		data;
};

static LVec construct(size_t elem_size) {
  if (elem_size == 0) {
    return NULL;
  }

  LVec lvec = CurrentAllocator->malloc(sizeof(struct tagLightVector));

  if (lvec == NULL) {
    return NULL;
  }

  lvec->capacity = LVEC_START_CAPACITY;
  lvec->size = 0;
  lvec->elem_size = elem_size;

  lvec->data = CurrentAllocator->malloc(elem_size * LVEC_START_CAPACITY);
  if (lvec->data == NULL) {
    CurrentAllocator->free(lvec);
    return NULL;
  }

  return lvec;
}

static void destruct(LVec lvec) {
  free(lvec->data);
  free(lvec);
}

static size_t size(const LVec lvec) {
  return lvec->size;
}

static size_t elem_size(const LVec lvec) {
  return lvec->elem_size;
}

static _lvec_resize(LVec lvec) {
  if (lvec == NULL) {
    return -1;
  }

  size_t new_capacity = lvec->capacity * LVEC_REALLOC_SCALE_FACTOR;
  void* tmp = realloc(lvec->data, (new_capacity * lvec->elem_size));
  if (tmp == NULL) {
    return -1;
  }

  lvec->data = tmp;
  lvec->capacity = new_capacity;

  return 0;
}

static int32_t add(LVec lvec, void* elem) {
  if (lvec == NULL) {
    return -1;
  }

  if (lvec->size == lvec->capacity) {
    if (_lvec_resize(lvec) < 0) {
      return -1;
    }
  }

  memcpy(lvec->data + (lvec->size * lvec->elem_size), elem, lvec->elem_size);
  lvec->size++;

  return 0;
}

static int32_t erase_at(LVec lvec, size_t index) {
  if (lvec == NULL) {
    return -1;
  }

  if (index >= lvec->size) {
    return -1;
  }

  char* ptr = lvec->data + (index * lvec->elem_size);
  char* last_elem = lvec->data + ((lvec->size - 1) * lvec->elem_size);

  for (; ptr < last_elem; ptr++) {
    *ptr = ptr[lvec->elem_size];
  }

  lvec->size--;

  return 0;
}

static void* data(const LVec lvec) {
  return lvec->data;
}

LightVectorInterface iLVec = {
  .construct = construct,
  .destruct = destruct,
  .size = size,
  .elem_size = elem_size,
  .data = data,
  .add = add,
  .erase_at = erase_at,
};
