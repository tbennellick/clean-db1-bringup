#include "ff.h"
#include "process_storage.h"
#include "proto/BFP.pb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(process_storage, LOG_LEVEL_DBG);

#define BFP_LOG_FILE_MAGIC    0x42, 0x46, 0x50, 0x5f, 0x4C, 0x4F, 0x47, 0xE2, 0x80, 0xBD, 0x04
#define BFP_LOG_FILE_REVISION 0x04
uint8_t log_file_header[12] = {BFP_LOG_FILE_MAGIC, BFP_LOG_FILE_REVISION};

static struct k_poll_signal log_rotate_signal;
static struct k_poll_signal terminate_recording_signal;

extern void log_rotate_timer_expire(struct k_timer *timer_id) {
	k_poll_signal_raise(&log_rotate_signal, 0);
}

char *rotate_log_file(const char *session_path) {
	static uint32_t log_file_count = 0;
	static char current_file_path[30];
	static FIL log_file;

	int ret = snprintf(current_file_path, sizeof(current_file_path), "%s/%08d.binpb", session_path, log_file_count++);

	if (ret < 0 || ret >= sizeof(current_file_path)) {
		LOG_ERR("Log file path snprintf failed");
		return "\0";
	}

	FRESULT fres = f_open(&log_file, current_file_path, FA_CREATE_ALWAYS | FA_WRITE);
	if (fres != FR_OK) {
		LOG_ERR("Failed to create log file %s: %d", current_file_path, fres);
		return "\0";
	}

	UINT bytes_written;
	fres = f_write(&log_file, log_file_header, sizeof(log_file_header), &bytes_written);
	if (fres != FR_OK || bytes_written != sizeof(log_file_header)) {
		LOG_ERR("Failed to write log file header: %d", fres);
		f_close(&log_file);
		return "\0";
	}

	f_close(&log_file);

	LOG_INF("Rotated to log file: %s", current_file_path);
	return current_file_path;
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

	char *log_file_path = rotate_log_file(session_path);

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
			log_file_path = rotate_log_file(session_path);
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
