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

    /* TODO: Write all registers to default values as there is no reset procedure.*/

    /* TODO Verify this works*/
    /* Set I2S mode */
    ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_DAI_IF_MODE1, MAX9867_DAI_IF_MODE1_TDM_MODE_BIT, 0);
    if (ret < 0) {
        LOG_ERR("Failed to set I2S mode: %d", ret);
        return ret;
    }

    /* TODO: Break these out to functions */
    /* Assuming 12MHz Mclk for now */
    /* Set prescaler to 1 */
    ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_SYS_CLK, MAX9867_SYS_CLK_PSCLK_MASK, MAX9867_SYS_CLK_PSCLK_10_20MHZ);
    if (ret < 0) {
        LOG_ERR("Failed to set system clock prescaler: %d", ret);
        return ret;
    }

    ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_SYS_CLK, MAX9867_SYS_CLK_FREQ_MASK, 0x08 ); /* From DS Tab4*/
    if( ret < 0) {
        LOG_ERR("Failed to set fixed divider: %d", ret);
        return ret;
    }

    ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_SYS_CLK, MAX9867_SYS_CLK_PLL_EN_BIT, 0);
    if (ret < 0) {
        LOG_ERR("Failed to disable PLL: %d", ret);
        return ret;
    }
    /* We don't set NI as we are using exact integer mode.*/

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_DAI_IF_MODE1, MAX9867_DAI_IF_MODE1_MAS |
                                                                    MAX9867_DAI_IF_MODE1_HIZOFF);
    if (ret < 0) {
        LOG_ERR("Failed to set DAI mode1: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_DAI_IF_MODE2, MAX9867_DAI_IF_MODE2_LVOLFIX |
                                                                    MAX9867_DAI_IF_MODE2_BSEL_NORM);
    if (ret < 0) {
        LOG_ERR("Failed to set DAI mode2: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_CODEC_FILTER, 0); /* Disable Filters*/
    if (ret < 0) {
        LOG_ERR("Failed to disable filters: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_SIDETONE, 0); /* Disable O/P mixer */
    if (ret < 0) {
        LOG_ERR("Failed to disable O/P mixer: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_DAC_LEVEL, MAX9867_DAC_LEVEL_DACMUTE); /* Mute DAC */
    if (ret < 0) {
        LOG_ERR("Failed to Mute DAC: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_ADC_LEVEL, MAX9867_ADC_LEVEL_0DB & (MAX9867_ADC_LEVEL_0DB<<4)); /* Set ADC to 0dB */
    if (ret < 0) {
        LOG_ERR("Failed to Set ADC gain: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_LINE_IN_LEV_L, MAX9867_LINE_IN_LEV_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Line in L: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_LINE_IN_LEV_R, MAX9867_LINE_IN_LEV_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Line in R: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_VOL_L, MAX9867_VOL_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Playback volume L: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_VOL_R, MAX9867_VOL_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Playback volume R: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MIC_GAIN_L, MAX9867_MIC_GAIN_X_PRE_0DB | MAX9867_MIC_GAIN_X_0DB);
    if (ret < 0) {
        LOG_ERR("Failed to Set MIC signal chain L: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MIC_GAIN_R, MAX9867_MIC_GAIN_X_PRE_0DB | MAX9867_MIC_GAIN_X_0DB);
    if (ret < 0) {
        LOG_ERR("Failed to Set MIC signal chain R: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_ADC_IN_CONF, 0); /* Disable ADC Aux input and input mixer */
    if (ret < 0) {
        LOG_ERR("Failed to disable ADC input: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MIC_CONF, 0); /* Disable DMIC input */
    if (ret < 0) {
        LOG_ERR("Failed to disable DMIC input: %d", ret);
        return ret;
    }

    /* TODO: Jack Det IRQ */

    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MODE, 0); /* Disable jack features */
    if (ret < 0) {
        LOG_ERR("Failed to disable jack features: %d", ret);
        return ret;
    }

    /* TODO: Enable Line in here with switching */
    ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_SYS_SHDN,  MAX9867_SYS_SHDN_SHDN_ADLEN | MAX9867_SYS_SHDN_SHDN_ADREN);
    if (ret < 0) {
        LOG_ERR("Failed to enable ADCs: %d", ret);
        return ret;
    }

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