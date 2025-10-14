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
	} else {
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
		init_storage();
	}

	StoredEventLog log = StoredEventLog_init_default;
	log.has_version = true;
	log.version = BFP_PROTO_REV;

	LOG_INF("Logging format version %d", log.version);

	LOG_INF("Init complete");

	while (1) {
		k_sleep(K_SECONDS(1));
		printk(".");
		debug_led_toggle(0);
		if (temperature_read_block(&temp_block, K_NO_WAIT) == 0) {
			printk(":");
		}
	}
	return 0;
}
