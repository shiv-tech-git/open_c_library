#include "observer_i.h"

typedef struct {
	uint64_t 	action;
	void 		(*cb)(int action, const void* call_extra, void* cb_extra);
	void* 		cb_extra;
} observer_callback_data_t;

typedef struct {
	uint64_t	observers;
	LVec		obs_cb_data;
} Observer_t;