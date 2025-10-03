#include "boot_id.h"
#include "uuid_lib.h"

#include <zephyr/drivers/hwinfo.h>
// #include <zephyr/logging/log.h>
// LOG_MODULE_REGISTER(boot, CONFIG_LOG_DEFAULT_LEVEL);
#include <zephyr/drivers/hwinfo.h>

uuid_t boot_uuid;
uuid_t gateway_uuid;

void init_id(void) {
	static bool id_has_been_generated = false;

	if (id_has_been_generated == false) {
		uuid_gen(&boot_uuid);
		uuid_generate_string(&boot_uuid);

		hwinfo_get_device_id(gateway_uuid.val, 8);
		/* gateway id should be a 128bit space to allow future forward compatibility
		 * if the nordic maintained individuality is not available. Extend to 128 to ensure
		 * the rest of the pipeline remains compatible*/
		for (uint8_t i = 0; i < 8; i++) {
			gateway_uuid.val[i + 8] = gateway_uuid.val[i] ^ 0xAA;
		}

		uuid_generate_string(&gateway_uuid);

		id_has_been_generated = true;
	}
}

uint8_t *get_boot_id(void) {
	return boot_uuid.as_string;
}

void get_boot_id_bytes(uint8_t *p) {
	get_uuid_bytes(p, boot_uuid);
}

uint8_t *get_gateway_id(void) {
	return gateway_uuid.as_string;
}

void get_gateway_id_bytes(uint8_t *p) {
	get_uuid_bytes(p, gateway_uuid);
}
