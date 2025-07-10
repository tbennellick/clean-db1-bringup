#include "abp2s_utils.h"
#include "abp2_bits.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ABP2S_STAT, LOG_LEVEL_WRN);

bool abp2s_check_status(uint8_t status)
{
    /* Device valid if Status == 01X0000X */
    if (status & ABP2S_ALWAYS_LOW)
    {
        LOG_ERR("Invalid status byte 0x%02x, want 01#0000#", status);
        return false;
    }
    if(!(status& ABP2S_POWER_BIT))
    {
        LOG_ERR("No responce, status byte 0x%02x", status);
        return false;
    }
    LOG_DBG("Device status byte 0x%02x", status);
    return true;
}

/* Part is currently ABP2DANT001PGSA3XX */
/* That is 0 - 1psi gauge */
/* MCX FPU is single precision */
float abp2s_calculate_pressure_psi(uint32_t counts, float pmin, float pmax)
{
    /* Eqn 2 section 8.11 of abp2 datasheet */
    float counts_float = (float)counts;
    float pressure = ((counts_float - OUT_MIN_10) * (pmax - pmin)) / (OUT_MAX_90 - OUT_MIN_10) + pmin;
    return pressure;
}

float psi_to_mbar(float psi) {
    return psi * 68.9476f;
}

float abp2s_calculate_temperature(uint32_t counts) {
    float temp = (((float)counts * (ABP2S_TMAX - ABP2S_TMIN)) /
             (UINT24_MAX - 1))
            + ABP2S_TMIN;
    return  temp;
}