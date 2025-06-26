#include <limits.h>
#include <zephyr/ztest.h>
#include "max9867_utils.h"
#include "stdbool.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>


struct mic_gain_fixture{
    audio_property_value_t val;
    uint8_t preamp_gain;
    uint8_t mic_gain;
};

static void *suite_setup(void)
{
    struct mic_gain_fixture *fixture = malloc(sizeof(struct mic_gain_fixture));
    return fixture;
}

static void suite_before(void *f)
{
    struct mic_gain_fixture *fixture = (struct mic_gain_fixture *)f;
    zassume_not_null(fixture, NULL);

    fixture->preamp_gain = 200;
    fixture->mic_gain = 199;
    fixture->val.vol = 255;
}
static void suite_teardown(void *f)
{
    free(f);
}

ZTEST_SUITE(mic_gain, NULL, suite_setup, suite_before, NULL, suite_teardown);

ZTEST_F(mic_gain, 0)
{
    fixture->val.vol = 0;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,0, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x14, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 1)
{
    fixture->val.vol = 1;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,0, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x13, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}


ZTEST_F(mic_gain, 20)
{
    fixture->val.vol = 20;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,0, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x0, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 21)
{
    fixture->val.vol = 21;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,3, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x13, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 39)
{
    fixture->val.vol = 39;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,3, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x01, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 40)
{
    fixture->val.vol = 40;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,3, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x00, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 41)
{
    fixture->val.vol = 41;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,4, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x09, "Mic gain %d wrong at vol=%d should be %d", fixture->mic_gain, fixture->val.vol,0x0A);
    /* NB Table 12 in DS rev 4 possibly has 0x09 and 0x0a swapped, support request outstanding */
}
ZTEST_F(mic_gain, 42)
{
    fixture->val.vol = 42;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,4, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x08, "Mic gain %d wrong at vol=%d should be %d", fixture->mic_gain, fixture->val.vol,0x0A);
}


ZTEST_F(mic_gain, 50)
{
    fixture->val.vol = 50;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,4, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x00, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}

ZTEST_F(mic_gain, 70)
{
    fixture->val.vol = 70;
    split_mic_gain(fixture->val, &fixture->preamp_gain, &fixture->mic_gain);
    zassert_equal(fixture->preamp_gain,4, "Preamp %d wrong at vol=%d", fixture->preamp_gain, fixture->val.vol);
    zassert_equal(fixture->mic_gain,0x00, "Mic gain %d wrong at vol=%d", fixture->mic_gain, fixture->val.vol);
}
