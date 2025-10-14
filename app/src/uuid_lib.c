#include "uuid_lib.h"

#include <zephyr/random/random.h>

void uuid_gen(uuid_t *uuid) {
	sys_rand_get(uuid->val, UUID_NBYTES);

	//    Set bits Adjust as RFC 4122 section 4.4. for version 4 UUID
	// (a) set the high nibble of the 7th byte equal to 4 and
	// (b) set the two most significant bits of the 9th byte to 10'B,
	//     so the high nibble will be one of {8,9,A,B}.
	uuid->val[6] = 0x40 | (uuid->val[6] & 0xf);
	uuid->val[8] = 0x80 | (uuid->val[8] & 0x3f);
}

void uuid_generate_string(uuid_t *uuid) {
	uint8_t temp[15];
	uint8_t remain;
	snprintf(uuid->as_string,
	         sizeof uuid->as_string,
	         "%02x%02x%02x%02x-",
	         uuid->val[0],
	         uuid->val[1],
	         uuid->val[2],
	         uuid->val[3]);
	snprintf(temp, sizeof temp, "%02x%02x-", uuid->val[4], uuid->val[5]);
	remain = sizeof uuid->as_string - strlen(uuid->as_string);
	strncat(uuid->as_string, temp, remain);

	snprintf(temp, sizeof temp, "%02x%02x-", uuid->val[6], uuid->val[7]);
	remain = sizeof uuid->as_string - strlen(uuid->as_string);
	strncat(uuid->as_string, temp, remain);

	snprintf(temp, sizeof temp, "%02x%02x-", uuid->val[8], uuid->val[9]);
	remain = sizeof uuid->as_string - strlen(uuid->as_string);
	strncat(uuid->as_string, temp, remain);

	snprintf(temp,
	         sizeof temp,
	         "%02x%02x%02x%02x%02x%02x",
	         uuid->val[10],
	         uuid->val[11],
	         uuid->val[12],
	         uuid->val[13],
	         uuid->val[14],
	         uuid->val[15]);
	remain = sizeof uuid->as_string - strlen(uuid->as_string);
	strncat(uuid->as_string, temp, remain);
}

void get_uuid_bytes(uint8_t *p, uuid_t u) {
	memcpy(p, &u.val, UUID_NBYTES);
}
