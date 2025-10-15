#include "process_storage.h"
#include "proto/BFP.pb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(process_storage, LOG_LEVEL_DBG);

// #define BFP_FILENAME_DIGITS_COUNT    8
// #define BFP_EVENT_FILE_EXTENSION     ".binpb"
// #define BFP_FILENAME_LEN             (BFP_FILENAME_DIGITS_COUNT + sizeof(BFP_EVENT_FILE_EXTENSION) - 1)

static struct k_poll_signal log_rotate_signal;
static struct k_poll_signal terminate_recording_signal;

extern void log_rotate_timer_expire(struct k_timer *timer_id) {
	k_poll_signal_raise(&log_rotate_signal, 0);
}

void process_storage_queue(struct k_msgq *msgq, char *session_path) {

	struct k_poll_event storage_events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, msgq),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &log_rotate_signal),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &terminate_recording_signal),
	};

	k_poll_signal_init(&log_rotate_signal);
	k_poll_signal_reset(&log_rotate_signal);

	k_poll_signal_init(&terminate_recording_signal);
	k_poll_signal_reset(&terminate_recording_signal);

	struct k_timer log_rotate_timer;
	log_rotate_timer.user_data = (void *)&log_rotate_signal;
	k_timer_init(&log_rotate_timer, log_rotate_timer_expire, NULL);
	k_timer_start(&log_rotate_timer, K_SECONDS(CONFIG_LOG_ROTATE_INTERVAL), K_SECONDS(CONFIG_LOG_ROTATE_INTERVAL));

	uint32_t log_file_count = 0;

	char current_file_path[30];
	snprintf(current_file_path, sizeof current_file_path, "%s/%08d.binpb", session_path, 0);
	BaseEvent event;

	while (1) {
		k_poll(storage_events, ARRAY_SIZE(storage_events), K_FOREVER);

		if (storage_events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
			k_msgq_get(msgq, &event, K_FOREVER);
			LOG_DBG("Storage got event type %d", event.event_type);
			storage_events[0].state = K_POLL_STATE_NOT_READY;
		}
		if (storage_events[1].state == K_POLL_STATE_SIGNALED) {
			LOG_DBG("Log rotate timer fired");
			k_poll_signal_reset(storage_events[1].signal);
			storage_events[1].state = K_POLL_STATE_NOT_READY;
		}
		if (storage_events[2].state == K_POLL_STATE_SIGNALED) {
			LOG_INF("Terminating recording");
			k_poll_signal_reset(storage_events[2].signal);
			break;
		}
	}

	k_timer_stop(&log_rotate_timer);
}
