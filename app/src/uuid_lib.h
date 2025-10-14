#pragma once
#include "stdint.h"

#define UUID_NBYTES      16
#define UUID_STRING_SIZE 37
typedef struct _uuid_t {
	uint8_t val[16];
	char as_string[UUID_STRING_SIZE];
} uuid_t;

void uuid_gen(uuid_t *uuid);
void uuid_generate_string(uuid_t *uuid);
void get_uuid_bytes(uint8_t *p, uuid_t u);
