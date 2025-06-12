#include <limits.h>
#include <zephyr/ztest.h>
#include <abp2s_utils.h>
#include "stdbool.h"

ZTEST(pressure_sensor, test_no_responce)
{
    zassert_equal(abp2s_check_status(0), false, "Check Status failed for device response");
}

ZTEST(pressure_sensor, test_power_only)
{
    zassert_equal(abp2s_check_status(1<<6), true, "Check Status failed for power only");
}

ZTEST(pressure_sensor, test_powerandbusy)
{
    zassert_equal(abp2s_check_status(0x3<<5), true, "Check Status failed for power and busy");
}

ZTEST(pressure_sensor, test_maxhigh)
{
    zassert_equal(abp2s_check_status(0x61), true, "Check Status failed for max high");
}

ZTEST(pressure_sensor, integrity)
{
    zassert_equal(abp2s_check_status(1<<2), false, "Check Status failed for power only");
}

ZTEST(pressure_sensor, MISO_HIGH)
{
    zassert_equal(abp2s_check_status(0xFF), false, "Check Status failed for MISO high");
}


ZTEST(pressure_sensor, outmax)
{
    int32_t val = OUT_MAX_90;
    zassert_equal(OUT_MAX_90, 15099494, "OUT_MAX_90 should be 15099494, it is %d", val);
    zassert_equal(OUT_MAX_90, 15099494, "OUT_MAX_90 should be 15099494, it is %d", OUT_MAX_90);
}

ZTEST(pressure_sensor, outmin)
{
    int32_t val = OUT_MIN_10;
    zassert_equal(OUT_MIN_10, 1677722 , "OUT_Min should be 1677722, it is %d", val);
}


ZTEST(pressure_sensor, datasheet_example)
{
    float  f = abp2s_calculate_pressure(14260634);
    int32_t kf = (int32_t)(f * 1000);
    zassert_equal(kf,875, "Example calculation failed, expected 875, got %d", kf);
}

ZTEST(pressure_sensor, datasheet_example_temp)
{
    float  f = abp2s_calculate_temperature(6291456);
    int32_t kf = (int32_t)(f * 1000);
    zassert_equal(kf, 25000, "Example temperature calculation failed, expected 25000, got %d", kf);
}

ZTEST(pressure_sensor, i24_to_i32)
{
    uint8_t a[3] = {0,0,0};
    int32_t res = int24_to_int32(a);
    zassert_equal(res, 0, "int24_to_int32 failed, expected 0, got %d", res);

    uint8_t b[3] = {0,0x0B,0xB8};
    res = int24_to_int32(b);
    zassert_equal(res, 3000, "int24_to_int32 failed, expected 0, got %d", res);

    uint8_t c[3] = {0xff,0xf4,0x48};
    res = int24_to_int32(c);
    zassert_equal(res, -3000, "int24_to_int32 failed, expected 0, got %d", res);

    uint8_t d[3] = {0xff,0xff,0xff};
    res = int24_to_int32(d);
    zassert_equal(res, -1, "int24_to_int32 failed, expected 0, got %d", res);

    uint8_t e[3] = {0xff,0xff,0xfe};
    res = int24_to_int32(e);
    zassert_equal(res, -2, "int24_to_int32 failed, expected 0, got %d", res);

    uint8_t f[3] = {0x7f,0xff,0xff};
    res = int24_to_int32(f);
    zassert_equal(res, 838807, "int24_to_int32 failed, expected 0, got %d", res);
}



ZTEST_SUITE(pressure_sensor, NULL, NULL, NULL, NULL, NULL);
