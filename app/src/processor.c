#include "processor.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <BFP.pb.h>

LOG_MODULE_REGISTER(processor, LOG_LEVEL_DBG);

K_MSGQ_DEFINE(processor_input_msgq, sizeof(BaseEvent), CONFIG_PROCESSOR_Q_LEN, 4);

static int processor_thread(void *arg1, void *arg2, void *arg3) {
	struct k_msgq *storage_queue = (struct k_msgq *)arg1;
	BaseEvent event;

	LOG_INF("Processor thread started");

	while (true) {
		int ret = k_msgq_get(&processor_input_msgq, &event, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to get event from input queue: %d", ret);
			continue;
		}

		ret = k_msgq_put(storage_queue, &event, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("Failed to put event to storage queue: %d", ret);
		}
		printk(">%d ", event.event_type);

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

struct k_msgq *start_processing(struct k_msgq *storage_queue) {
	static K_THREAD_STACK_DEFINE(processor_thread_stack, 2048);
	static struct k_thread processor_thread_data;

	k_thread_create(&processor_thread_data,
	                processor_thread_stack,
	                K_THREAD_STACK_SIZEOF(processor_thread_stack),
	                (k_thread_entry_t)processor_thread,
	                storage_queue,
	                NULL,
	                NULL,
	                K_PRIO_PREEMPT(6),
	                0,
	                K_NO_WAIT);

	return &processor_input_msgq;
}
