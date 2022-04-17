#ifndef OBSERVER_INTERFACE_H
#define OBSERVER_INTERFACE_H

#include <inttypes.h>
#include <stdlib.h>

#include "lvec_i.h"

typedef struct tagObserver Observer_t;
typedef Observer_t* Observer;

typedef struct {
	Observer	(*construct)(void);
	void 		(*destruct)(Observer obs);
	int32_t		(*subscribe)(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra), void* cb_extra);
	void*		(*unsubscribe)(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra));
	int32_t		(*notify)(Observer obs, int action);
} ObserverInterface_t;

extern ObserverInterface_t iObserver;

#endif