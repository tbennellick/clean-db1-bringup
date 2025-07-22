#include <zephyr/ztest.h>
#include "ads1298_utils.h"

ZTEST_SUITE(exg_settling, NULL, NULL, NULL, NULL, NULL);

ZTEST(exg_settling, test_lp_16ksps)
{
    uint8_t config1 = 0x01; /* LP + DR=001 (16k SPS) */
    uint32_t result = ads1298_get_tsettle_us(config1);
    uint32_t expected = 2; /* 1160 cycles * 488ps = 566ns ≈ 1µs (but rounds up to 2) */
    zassert_equal(result, expected, "16k SPS LP mode: expected %d µs, got %d µs", expected, result);
}

ZTEST(exg_settling, test_lp_32ksps)
{
    uint8_t config1 = 0x00; /* LP + DR=000 (32k SPS) */
    uint32_t result = ads1298_get_tsettle_us(config1);
    uint32_t expected = 1; /* 584 cycles * 488ps = 285ns ≈ 1µs */
    zassert_equal(result, expected, "32k SPS LP mode: expected %d µs, got %d µs", expected, result);
}

/* Edge Case Tests */
ZTEST(exg_settling, test_invalid_dr_bits)
{
    uint8_t config1 = ADS1298_CONFIG1_HR | 0x07; /* Invalid DR=111 */
    uint32_t result = ads1298_get_tsettle_us(config1);
    uint32_t expected = 0; /* Should return 0 for invalid DR bits */
    zassert_equal(result, expected, "Invalid DR bits: expected %d µs, got %d µs", expected, result);
}