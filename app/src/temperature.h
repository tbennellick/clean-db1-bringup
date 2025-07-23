#pragma once
#include <zephyr/kernel.h>

#define TEMP_BLOCK_SIZE 16  /* Number of samples per block */

typedef struct {
    int16_t samples[TEMP_BLOCK_SIZE];
    uint32_t timestamp_ms;
    uint16_t count;
} temp_block_t;

int init_temperature(void);

/**
 * @brief Read a temperature block from the queue
 * @param block Pointer to store the temperature block
 * @param timeout Timeout for the read operation
 * @return 0 on success, -ENOMSG if no data available, other negative errno on failure
 */
int temperature_read_block(temp_block_t *block, k_timeout_t timeout);
