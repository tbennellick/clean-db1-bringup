#pragma once
#include <zephyr/kernel.h>

typedef struct {
	int16_t samples[CONFIG_NASAL_TEMP_BLOCK_SIZE];
	uint32_t timestamp_ms;
	uint16_t count;
} temp_block_t;

int init_temperature(void);
int temperature_read_block(temp_block_t *block, k_timeout_t timeout);