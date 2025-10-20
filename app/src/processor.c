#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <BFP.pb.h>

#include "processor.h"

LOG_MODULE_REGISTER(processor, LOG_LEVEL_DBG);

static int processor_thread(void *arg1, void *arg2, void *arg3) {
	struct k_msgq *main_queue = (struct k_msgq *)arg1;
	struct k_msgq *storage_queue = (struct k_msgq *)arg2;
	BaseEvent event;

	LOG_INF("Processor thread started");

	while (true) {
		int ret = k_msgq_get(main_queue, &event, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to get event from main queue: %d", ret);
			continue;
		}

		ret = k_msgq_put(storage_queue, &event, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("Failed to put event to storage queue: %d", ret);
		}

		switch (event.event_type) {
		case EventType_EVENT_TYPE_EXG_DATA:
			LOG_DBG("Processing EXG data event");
			break;
		case EventType_EVENT_TYPE_UNKNOWN:
			LOG_WRN("Received unknown event type");
			break;
		default:
			LOG_DBG("Processing unhandled event type %d", event.event_type);
			break;
		}
	}

	return 0;
}

void start_processing(struct k_msgq *main_queue, struct k_msgq *storage_queue) {
	static K_THREAD_STACK_DEFINE(processor_thread_stack, 2048);
	static struct k_thread processor_thread_data;

	k_thread_create(&processor_thread_data,
	                processor_thread_stack,
	                K_THREAD_STACK_SIZEOF(processor_thread_stack),
	                (k_thread_entry_t)processor_thread,
	                main_queue,
	                storage_queue,
	                NULL,
	                K_PRIO_PREEMPT(6),
	                0,
	                K_NO_WAIT);
}
