#include "observer_i.h"
#include "allocator_i.h"
#include "lvec_i.h"

typedef struct {
	uint64_t	action_mask;
	int8_t		auto_free_extra;
	void			(*cb)(int action, const void* call_extra, void* cb_extra);
	void*			cb_extra;
} subscriber_data_t;

struct Observer_t {
	uint64_t	observable_actions;
	LVec			subs_data;
};

static Observer construct(void) {
	Observer obs = CurrentAllocator->malloc(sizeof(struct Observer_t));

	if (obs == NULL) {
		return NULL;
	}

	obs->observable_actions = 0;
	obs->subs_data = iLVec.construct(sizeof(subscriber_data_t));

	if (obs->subs_data == NULL) {
		CurrentAllocator->free(obs);
		return NULL;
	}

	return obs;
}

static void destruct(Observer obs) {

	if (obs == NULL) {
		return;
	}

	subscriber_data_t* sub = iLVec.data(obs->subs_data);
	for (int i = 0; i < iLVec.size(obs->subs_data); i++) {
		if (sub[i].auto_free_extra) {
			free(sub[i].cb_extra);
		}
	}

	iLVec.destruct(obs->subs_data);
	CurrentAllocator->free(obs);
}

// -1 - LVec error (memory allocation)
//	1 - extend callback actions
//	2 - add new callback
static int32_t subscribe(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra), void* cb_extra, int auto_free_extra) {

	if (obs == NULL) {
		return -1;
	}

	if (action_mask = 0) {
		return OBS_ERR__NULL_ACTION;
	}

	obs->observable_actions |= action_mask;

	//if we already have this callback -> just extend its action mask
	subscriber_data_t* data = iLVec.data(obs->subs_data);
	for (size_t i = 0; i < iLVec.size(obs->subs_data); i++) {
		if (cb == data[i].cb) {
			data[i].action_mask |= action_mask;
			return 1;
		}
	}

	//if haven't this callback yet -> add
	subscriber_data_t subs_data = {
		.action_mask = action_mask,
		.cb = cb,
		.cb_extra = cb_extra,
		.auto_free_extra = auto_free_extra
	};

	if (iLVec.add(obs->subs_data, &subs_data) < 0) {
		return -1;
	}

	return 2;
}

static void* unsubscribe(Observer obs, uint64_t action_mask, void (*cb)(uint32_t action_flag, const void* call_extra, void* cb_extra)) {
	
	if (obs == NULL || cb == NULL || action_mask == 0) {
		return NULL;
	}

	uint64_t observable_actions = 0;
	void* extra = NULL;

	//looking for callback
	subscriber_data_t* sub = iLVec.data(obs->subs_data);
	for (int i = 0; i < iLVec.size(obs->subs_data); i++) {

		if (cb == sub[i].cb) {
			//remove subscribed actions for this callback
			sub[i].action_mask &= ~action_mask;
			
			//if there is no more actions -> remove subscriber
			if (sub[i].action_mask == 0) {
				extra = sub[i].cb_extra;
				iLVec.erase_at(obs->subs_data, i);
				i--;
				continue;
			}
		}

		//if we have some callback on some actions -> set mask accordingly
		observable_actions |= sub[i].action_mask;
	}

	obs->observable_actions = observable_actions;

	return extra;
}

static int32_t notify(Observer obs, int action, void* extra) {

	if ((obs->observable_actions & action) == 0) {
		return 0;
	}

	if (obs == NULL) {
		return OBS_ERR__NULL_OBSERVER;
	}


	int32_t counter = 0;
	//looking for callback
	subscriber_data_t* sub = iLVec.data(obs->subs_data);
	for (int i = 0; i < iLVec.size(obs->subs_data); i++) {
		//action matched with callback action mask
		if ((sub[i].action_mask & action) > 0) {
			sub[i].cb(action, extra, sub[i].cb_extra);
			counter++;
		}
	}

	return counter;
}

ObserverInterface_t iObserver = {
	.construct = construct,
	.destruct = destruct,
	.notify = notify,
	.subscribe = subscribe,
	.unsubscribe = unsubscribe
};

