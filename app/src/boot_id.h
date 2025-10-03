#pragma once
#include "stdint.h"

#include <zephyr/bluetooth/uuid.h>

void init_id(void);
void get_boot_id_bytes(uint8_t *p);
uint8_t *get_boot_id(void);
#define BOOT_ID_LEN BT_UUID_STR_LEN

uint8_t *get_gateway_id(void);
void get_gateway_id_bytes(uint8_t *p);