#include <limits.h>
#include <zephyr/ztest.h>
#include <check_status.h>
#include "stdbool.h"

ZTEST(pressure_status, test_no_responce)
{
    zassert_equal(abp2s_check_status(0), false, "Check Status failed for device response");
}

ZTEST(pressure_status, test_power_only)
{
    zassert_equal(abp2s_check_status(1<<6), true, "Check Status failed for power only");
}

ZTEST(pressure_status, test_powerandbusy)
{
    zassert_equal(abp2s_check_status(0x3<<5), true, "Check Status failed for power and busy");
}

ZTEST(pressure_status, test_maxhigh)
{
    zassert_equal(abp2s_check_status(0x61), true, "Check Status failed for max high");
}

ZTEST(pressure_status, integrity)
{
    zassert_equal(abp2s_check_status(1<<2), false, "Check Status failed for power only");
}

ZTEST(pressure_status, MISO_HIGH)
{
    zassert_equal(abp2s_check_status(0xFF), false, "Check Status failed for MISO high");
}

ZTEST_SUITE(pressure_status, NULL, NULL, NULL, NULL, NULL);
