#pragma once
#include "stdint.h"
#include "stdbool.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "math.h"
bool abp2s_check_status(uint8_t status);

#define UINT24_MAX 0x1000000
#define OUT_MAX_90 15099494 //round( UINT24_MAX* 0.9f))
#define OUT_MIN_10 1677722 //round( UINT24_MAX* 0.1f))
#define PMAX 1
#define PMIN -PMAX
float abp2s_calculate_pressure(uint32_t counts);
#define ABP2S_TMAX 150
#define ABP2S_TMIN (-50)
float abp2s_calculate_temperature(uint32_t counts);

int32_t int24_to_int32(uint8_t b[3]);
