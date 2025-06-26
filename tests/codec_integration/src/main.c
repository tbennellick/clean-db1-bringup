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

    fixture->audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
    fixture->audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
    fixture->audio_cfg.dai_cfg.i2s.word_size = 16;
    fixture->audio_cfg.dai_cfg.i2s.channels = 2;
    fixture->audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
    fixture->audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    fixture->audio_cfg.dai_cfg.i2s.frame_clk_freq = 8000;
    fixture->audio_cfg.dai_cfg.i2s.mem_slab = NULL;
    fixture->audio_cfg.dai_cfg.i2s.block_size = 0;


    TC_PRINT("\n\n\n\n\n\n\n\n######################################################\n");

    return fixture;
}

static void suite_before(void *f)
{
    struct codec_integration_fixture *fixture = (struct codec_integration_fixture *)f;
    zassume_not_null(fixture, NULL);

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
