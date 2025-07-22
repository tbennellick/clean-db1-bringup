#pragma once

#include <zephyr/types.h>

/* ADS1298 Settling Time Table (Table 12 from datasheet) */
typedef struct {
    uint8_t dr_bits;         // DR[2:0] value (000 to 110)
    uint16_t hr_mode_cycles; // High-Resolution Mode Settling Time (tCLK)
    uint16_t lp_mode_cycles; // Low-Power Mode Settling Time (tCLK)
} ads1298_settling_time_t;

/* tCLK period in picoseconds (488ps) */
#define ADS1298_TCLK_PS 488

/* CONFIG1 register bits */
#define ADS1298_CONFIG1_DR_MASK       0x07
#define ADS1298_CONFIG1_HR            BIT(7)

/**
 * @brief Calculate settling time in microseconds based on CONFIG1 register
 * 
 * @param config1_reg CONFIG1 register value containing data rate and mode bits
 * @return uint32_t Settling time in microseconds (0 if invalid)
 */
uint32_t ads1298_get_tsettle_us(uint8_t config1_reg);