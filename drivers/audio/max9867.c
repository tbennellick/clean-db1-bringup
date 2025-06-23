#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max9867, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#include "max9867.h"

#define DT_DRV_COMPAT maxim_max9867


static int max9867_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct max9867_config *config = dev->config;
	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

__maybe_unused
static int max9867_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct max9867_config *config = dev->config;
	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

static int max9867_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
    const struct max9867_config *config = dev->config;
    struct max9867_data *data = dev->data;
	int ret;

    switch (cfg->dai_type) {
        case AUDIO_DAI_TYPE_I2S:
            LOG_DBG("Configuring I2S DAI");
            break;
        /* Codec also supports TDM, but it's not in audio_dai_type_t currently*/
        case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
            LOG_ERR("Left justified DAI type supported in MAX9867, not yet implemented in this driver");
            return -ENOSYS;
        default:
            LOG_ERR("DAI type unsupported");
            return -ENOTSUP;
    }

    if(data->word_size != 16) {
        LOG_ERR("Only 16-bit word size is supported by MAX9867");
        return -EPFNOSUPPORT;
    }


    data->sample_rate = cfg->dai_cfg.i2s.frame_clk_freq;
	data->word_size = cfg->dai_cfg.i2s.word_size;
	data->channels = cfg->dai_cfg.i2s.channels;

	LOG_DBG("Sample rate: %u Hz", data->sample_rate);
	LOG_DBG("Word size: %u bits", data->word_size);
	LOG_DBG("Channels: %u", data->channels);

    /* TODO Verify this works*/
    /* Set I2S mode */
    ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_DAI_IF_MODE1, MAX9867_TDM_MODE_BIT, 0);

    return 0;
}

__maybe_unused
static int max9867_start_output(const struct device *dev)
{
	LOG_DBG("Starting MAX9867 output");
	/* Implementation for starting output */
	return 0;
}

__maybe_unused
static int max9867_stop_output(const struct device *dev)
{
	LOG_DBG("Stopping MAX9867 output");
	/* Implementation for stopping output */
	return 0;
}

__maybe_unused
static int max9867_set_property(const struct device *dev,
				 audio_property_t property, audio_channel_t channel,
				 audio_property_value_t val)
{
	/* Property setting implementation */
	return -ENOTSUP;
}

__maybe_unused
static int max9867_apply_properties(const struct device *dev)
{
	/* Apply queued properties */
	return 0;
}

static const struct audio_codec_api max9867_driver_api = {
	.configure = max9867_configure,
//	.start_output = max9867_start_output,
//	.stop_output = max9867_stop_output,
//	.set_property = max9867_set_property,
//	.apply_properties = max9867_apply_properties,
};

static int max9867_init(const struct device *dev)
{
	const struct max9867_config *config = dev->config;
	uint8_t revision;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}
	
	ret = max9867_reg_read(dev, MAX9867_REVISION, &revision);
	if (ret < 0) {
		LOG_ERR("Failed to read revision register: %d", ret);
		return ret;
	}
	LOG_DBG("Chip revision: 0x%02x", revision);
	return 0;
}

#define MAX9867_DEFINE(inst)								\
	static struct max9867_data max9867_data_##inst;				\
											\
	static const struct max9867_config max9867_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, max9867_init, NULL,				\
			      &max9867_data_##inst, &max9867_config_##inst,		\
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,		\
			      &max9867_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX9867_DEFINE)