#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>
#include "power.h"


struct codec_integration_fixture{
    audio_property_value_t val;
    const struct device * codec_dev;
    struct audio_codec_cfg audio_cfg;
};

static void *suite_setup(void)
{
    struct codec_integration_fixture *fixture = malloc(sizeof(struct codec_integration_fixture));
    fixture->codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));

    TC_PRINT("\n\n\n\n\n\n\n\n######################################################\n");

    return fixture;
}

static void suite_before(void *f)
{
    struct codec_integration_fixture *fixture = (struct codec_integration_fixture *)f;
    zassume_not_null(fixture, NULL);
    fixture->audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
    fixture->audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
    fixture->audio_cfg.dai_cfg.i2s.word_size = 16;
    fixture->audio_cfg.dai_cfg.i2s.channels = 2;
    fixture->audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
    fixture->audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    fixture->audio_cfg.dai_cfg.i2s.frame_clk_freq = 8000;
    fixture->audio_cfg.dai_cfg.i2s.mem_slab = NULL;
    fixture->audio_cfg.dai_cfg.i2s.block_size = 0;


    power_all(false);
    k_sleep(K_MSEC(150)); /* 1.8v audio rail takes  ~100ms to discharge*/
    power_all(true);

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

ZTEST_F(codec_integration, configure_device)
{
    int ret = audio_codec_configure(fixture->codec_dev, &fixture->audio_cfg);
    zassert_equal(ret, 0, "Failed to configure audio codec: %d", ret);
}

ZTEST_F(codec_integration, configure_device_unimp_dai_type)
{
    fixture->audio_cfg.dai_type = AUDIO_DAI_TYPE_LEFT_JUSTIFIED; // Unsupported DAI type
    int ret = audio_codec_configure(fixture->codec_dev, &fixture->audio_cfg);
    zassert_equal(ret, -ENOSYS, "Configuration with unimplemented DAI type should fail");
}

ZTEST_F(codec_integration, configure_device_bad_dai_type)
{
    fixture->audio_cfg.dai_type = AUDIO_DAI_TYPE_PCMA;
    int ret = audio_codec_configure(fixture->codec_dev, &fixture->audio_cfg);
    zassert_equal(ret, -ENOTSUP, "Configuration with bad DAI type should fail");
}

ZTEST_F(codec_integration, configure_device_bad_word_size)
{
    fixture->audio_cfg.dai_cfg.i2s.word_size = 24;
    int ret = audio_codec_configure(fixture->codec_dev, &fixture->audio_cfg);
    zassert_equal(ret, -EPFNOSUPPORT, "Configuration with unsupported word size should fail ret = %d", ret);
}





ZTEST_F(codec_integration, set_property_input_volume_all_channels_should_fail)
{
    fixture->val.vol = 10;
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_ALL, fixture->val);
    zassert_not_equal(ret, 0, "Setting input volume for all channels should fail");
}

ZTEST_F(codec_integration, set_property_input_volume_front_left)
{
    fixture->val.vol = 10;
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_FRONT_LEFT, fixture->val);
    zassert_equal(ret, 0, "Failed to set input volume for front left channel: %d", ret);
}

ZTEST_F(codec_integration, set_property_input_volume_front_right)
{
    fixture->val.vol = 10;
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_FRONT_RIGHT, fixture->val);
    zassert_equal(ret, 0, "Failed to set input volume for front right channel: %d", ret);
}

ZTEST_F(codec_integration, set_property_input_volume_rear_left)
{
    fixture->val.vol = 10;
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_REAR_LEFT, fixture->val);
    zassert_equal(ret, 0, "Failed to set input volume for rear left channel: %d", ret);
}

ZTEST_F(codec_integration, set_property_input_volume_rear_right)
{
    fixture->val.vol = 10;
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_REAR_RIGHT, fixture->val);
    zassert_equal(ret, 0, "Failed to set input volume for rear right channel: %d", ret);
}

ZTEST_F(codec_integration, set_property_input_volume_front_too_high)
{
    fixture->val.vol = 16; /* Max gain is 15*/
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_FRONT_LEFT, fixture->val);
    zassert_equal(ret, -EDOM, "Failed to set input volume for front left channel: %d", ret);

    ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                   AUDIO_CHANNEL_FRONT_RIGHT, fixture->val);
    zassert_equal(ret, -EDOM, "Failed to set input volume for front right channel: %d", ret);
}


ZTEST_F(codec_integration, set_property_input_volume_rear_too_high)
{
    fixture->val.vol = 51; /* Max gain is 50*/
    int ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                       AUDIO_CHANNEL_REAR_RIGHT, fixture->val);
    zassert_equal(ret, -EDOM, "Failed to set input volume for rear right channel: %d", ret);
    ret = audio_codec_set_property(fixture->codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                   AUDIO_CHANNEL_REAR_LEFT, fixture->val);
    zassert_equal(ret, -EDOM, "Failed to set input volume for rear left channel: %d", ret);
}