#pragma once
#include "stdint.h"
#include "stdbool.h"
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "math.h"
bool abp2s_check_status(uint8_t status);

#define UINT24_MAX 0x1000000
#define OUT_MAX_90 15099494.0f //round( UINT24_MAX* 0.9f))
#define OUT_MIN_10 1677722.0f //round( UINT24_MAX* 0.1f))
float abp2s_calculate_pressure_psi(uint32_t counts, float pmin, float pmax);
float psi_to_mbar(float psi);

#define ABP2S_TMAX 150
#define ABP2S_TMIN (-50)
float abp2s_calculate_temperature(uint32_t counts);
