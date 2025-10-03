#pragma once
#include "stdint.h"

void init_id(void);
void get_boot_id_bytes(uint8_t *p);
uint8_t *get_boot_id(void);

uint8_t *get_device_id(void);
void get_device_id_bytes(uint8_t *p);