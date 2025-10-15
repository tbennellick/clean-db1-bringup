#include "debug_leds.h"
#include "imu.h"
#include "led_manager.h"
#include "modem.h"
#include "power.h"
#include "pressure.h"
#include "rip.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <BFP_proto_rev.h>
#include <app_version.h>
#include <proto/BFP.pb.h>

// #include "exg.h"
#include "als.h"
#include "audio.h"
#include "boot_id.h"
#include "display.h"
#include "fuel_gauge.h"
#include "storage.h"
#include "temperature.h"
#include "ui.h"
#include "usb.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
temp_block_t temp_block;

int main(void) {
	LOG_INF("BFP2 Main core %s\n", APP_VERSION_STRING);
	init_debug_leds();
	debug_led_on();
	init_id();
	LOG_INF("Boot ID    \t%s", get_boot_id());
	LOG_INF("Device  ID \t%s", get_device_id());

	led_manager_init();
	led_manager_set(LED_MANAGER_COLOUR_OFF, LED_MANAGER_MODE_CONT);
	//    init_power(); Called early in kernel startup

	init_ui();
	LOG_INF("Buttons: Left %d Right %d", left_button(), right_button());

	if (left_button()) {
		LOG_WRN("Left button pressed on boot - entering extract mode");
		init_usb();
		while (1) {
			k_sleep(K_SECONDS(1));
			printk("*");
			debug_led_toggle(0);
		}
	}
	LOG_INF("Continuing in Record mode");
	// init_imu();
	// init_rip();
	// init_pressure();
	// //    init_exg();
	// init_fuel_gauge();
	// init_modem();
	// init_als();
	// init_audio();
	// init_display();
	// init_temperature();

	struct k_msgq *storage_queue = init_storage();

	LOG_INF("Init complete");

	BaseEvent base_event = BaseEvent_init_default;
	base_event.has_timestamp_us = true;
	base_event.has_event_type = true;
	base_event.has_priority = true;

	base_event.timestamp_us = k_uptime_get() * 1000;
	base_event.event_type = EventType_EVENT_TYPE_EXG_DATA;
	base_event.priority = Priority_PRIORITY_HIGH;

	base_event.which_event_data = BaseEvent_exg_data_event_tag;
	base_event.event_data.exg_data_event.has_status = true;
	base_event.event_data.exg_data_event.has_readings = true;

	base_event.event_data.exg_data_event.status = 0;
	uint8_t loop_count = 0;
	while (1) {
		memset(base_event.event_data.exg_data_event.readings,
		       loop_count++,
		       sizeof(base_event.event_data.exg_data_event.readings));
		const int ret = k_msgq_put(storage_queue, &base_event, K_NO_WAIT);
		if (ret != 0) {
			printk("!");
		}
		printk(".");
		k_sleep(K_SECONDS(1));

		debug_led_toggle(0);
		if (temperature_read_block(&temp_block, K_NO_WAIT) == 0) {
			printk(":");
		}
	}
	return 0;
}
