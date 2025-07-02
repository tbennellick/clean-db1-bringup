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

/* Part is ABP2DANT001PGSA3XX */
/* That is 1psi gauge */
/* MCX FPU is single precision */
float abp2s_calculate_pressure(int32_t counts)
{


    //calculation of pressure value according to equation 2 of datasheet
    double pressure = ((counts - OUT_MIN_10) * (PMAX - PMIN)) / (OUT_MAX_90 - OUT_MIN_10) + PMIN;


//    float pressure = ((((float)counts - (float)OUT_MIN_10) * (PMAX - PMIN) ) /
//                        (OUT_MAX_90 - OUT_MIN_10) )
//                        + PMIN;
    return (float)pressure;
}

double datasheet_calculate_pressure(int32_t counts)
{


    double outputmax = 15099494; // output at maximum pressure [counts]
    double outputmin = 1677722; // output at minimum pressure [counts]
    double pmax = 1; // maximum value of pressure range [bar, psi, kPa, etc.]
    double pmin = 0; // minimum value of pressure range [bar, psi, kP

    double pressure = ((counts - outputmin) * (pmax - pmin)) / (outputmax - outputmin) + pmin;

    return pressure;
}



float abp2s_calculate_temperature(int32_t counts) {
    float temp = (((float)counts * (ABP2S_TMAX - ABP2S_TMIN)) /
             (UINT24_MAX - 1))
            + ABP2S_TMIN;
    return  temp;
}

//
//int32_t int24_to_int32(uint8_t b[3]) {
//    int32_t result = ( (b[0] << 16) | (b[1] << 8) | b[2] );
//    if (b[0] & 0x80)
//        result |= 0xFF000000;
//    return result;
//}
