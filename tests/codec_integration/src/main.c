#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>


struct codec_integration_fixture{
    audio_property_value_t val;
    const struct device * codec_dev;
    struct audio_codec_cfg audio_cfg;
};

static void *suite_setup(void)
{
    struct codec_integration_fixture *fixture = malloc(sizeof(struct codec_integration_fixture));
    fixture->codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

    return fixture;
}

static void suite_before(void *f)
{
    struct codec_integration_fixture *fixture = (struct codec_integration_fixture *)f;
    zassume_not_null(fixture, NULL);

    fixture->val.vol = 255;
}
static void suite_teardown(void *f)
{
    free(f);
}

ZTEST_SUITE(codec_integration, NULL, suite_setup, suite_before, NULL, suite_teardown);

ZTEST_F(codec_integration, device_present)
{
    zassert_true(device_is_ready(fixture->codec_dev), "Device not ready");
}
