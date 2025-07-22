#pragma once
#include <stdint.h>
#include <zephyr/kernel.h>

struct exg_config {
    /** Number of channels in use */
    uint8_t channels;
    /** Sample Rate */
    uint32_t sample_clk_freq;
    /** Memory slab to store RX data. */
    struct k_mem_slab *mem_slab;
    /** Read/Write timeout. Number of milliseconds to wait in case TX queue
     * is full or RX queue is empty, or 0, or SYS_FOREVER_MS. */
    int32_t timeout;
};
