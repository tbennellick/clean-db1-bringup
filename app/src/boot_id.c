#include "boot_id.h"
#include "uuid_lib.h"

#include <zephyr/drivers/hwinfo.h>
// #include <zephyr/logging/log.h>
// LOG_MODULE_REGISTER(boot, CONFIG_LOG_DEFAULT_LEVEL);

uuid_t boot_uuid;
uuid_t device_uuid;

void init_id(void) {
	static bool id_has_been_generated = false;

	if (id_has_been_generated == false) {
		uuid_gen(&boot_uuid);
		uuid_generate_string(&boot_uuid);

		hwinfo_get_device_id(device_uuid.val, 16);
		uuid_generate_string(&device_uuid);

		id_has_been_generated = true;
	}
}

uint8_t *get_boot_id(void) {
	return boot_uuid.as_string;
}

void get_boot_id_bytes(uint8_t *p) {
	get_uuid_bytes(p, boot_uuid);
}

uint8_t *get_device_id(void) {
	return device_uuid.as_string;
}

void get_device_id_bytes(uint8_t *p) {
	get_uuid_bytes(p, device_uuid);
}
