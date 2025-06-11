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
    struct sensor_value val;
    float  f = abp2s_calculate_pressure(14260634, &val);
    int32_t kf = (int32_t)(f * 1000);
    zassert_equal(kf,875, "Example calculation failed, expected 875, got %d", kf);
}
//
//ZTEST(pressure_sensor, datasheet_example)
//{
//struct sensor_value val;
//abp2s_calculate_pressure(0x3FFFFF, &val);
//zassert_equal(abp2s_check_status(0xFF), false, "Check Status failed for MISO high");
//}




ZTEST_SUITE(pressure_sensor, NULL, NULL, NULL, NULL, NULL);
