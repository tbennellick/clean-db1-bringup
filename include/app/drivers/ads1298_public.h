#pragma once
#include <stdint.h>
typedef struct ads1298_sample {
    uint8_t status[3+ (3*8)];
    uint64_t timestamp;
    uint32_t sequence_number;
} ads1298_sample;