#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "ads1298_utils.h"

LOG_MODULE_REGISTER(ADS1298_UTILS, LOG_LEVEL_DBG);

/* ADS1298 Settling Time Table (Table 12 from datasheet) */
static const ads1298_settling_time_t settling_table[] = {
    {0b000,   296,    584},   /* 32kSPS */
    {0b001,   584,   1160},   /* 16kSPS */
    {0b010,  1160,   2312},   /* 8kSPS */
    {0b011,  2312,   4616},   /* 4kSPS */
    {0b100,  4616,   9224},   /* 2kSPS */
    {0b101,  9224,  18440},   /* 1kSPS */
    {0b110, 18440,  36872}    /* 500SPS */
};

uint32_t ads1298_get_tsettle_us(uint8_t config1_reg)
{
    uint8_t dr_bits = config1_reg & ADS1298_CONFIG1_DR_MASK;
    uint16_t cycles;

    if (dr_bits > 6) {
        LOG_ERR("Invalid DR bits: %d", dr_bits);
        return 0;
    }
    
    if(config1_reg & ADS1298_CONFIG1_HR) {
        /* High-Resolution Mode */
        cycles = settling_table[dr_bits].hr_mode_cycles;
    } else {
        /* Low-Power Mode */
        cycles = settling_table[dr_bits].lp_mode_cycles;
    }
    
    uint32_t ps = cycles * ADS1298_TCLK_PS;
    uint32_t us = (ps / 1000000) + 1; /* Convert ps to us, round up */
    return us;
}