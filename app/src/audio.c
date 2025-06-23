#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio, LOG_LEVEL_DBG);


int init_audio(void)
{
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct audio_codec_cfg audio_cfg;

	if (!device_is_ready(codec_dev)) {
		LOG_ERR("%s is not ready", codec_dev->name);
		return -1;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = 16;
	audio_cfg.dai_cfg.i2s.channels = 2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = 48000;
	audio_cfg.dai_cfg.i2s.mem_slab = NULL;
	audio_cfg.dai_cfg.i2s.block_size = 0;

	int ret = audio_codec_configure(codec_dev, &audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure audio codec: %d", ret);
		return ret;
	}

	LOG_DBG("Audio codec configured successfully");
	return 0;
}