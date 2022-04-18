#ifndef OBSERVER_INTERFACE_H
#define OBSERVER_INTERFACE_H

#include <inttypes.h>
#include <stdlib.h>

#include "lvec_i.h"

#define OBS_OK																 0
#define OBS_ERR__NULL_OBSERVER								-1
#define OBS_ERR__NULL_ACTION									-2
#define OBS_ERR__NULL_CALLBACK								-3
#define OBS_ERR__NULL_VEC											-4

typedef struct Observer_t* Observer;

typedef struct {
	Observer	(*construct)(void);
	void 			(*destruct)(Observer obs);
	int32_t		(*subscribe)(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra), void* cb_extra, int auto_free_extra);
	void*			(*unsubscribe)(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra));
	int32_t		(*notify)(Observer obs, int action, void* extra);
} ObserverInterface_t;

extern ObserverInterface_t iObserver;

#endif