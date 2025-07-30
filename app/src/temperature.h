#pragma once
#include <zephyr/kernel.h>

#define TEMP_BLOCK_SIZE 16  /* Number of samples per block */

typedef struct {
    int16_t samples[TEMP_BLOCK_SIZE];
    uint32_t timestamp_ms;
    uint16_t count;
} temp_block_t;

int init_temperature(void);
