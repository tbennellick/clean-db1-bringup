#include <zephyr/kernel.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#include "max9867.h"
#include "max9867_private.h"
#include "max9867_utils.h"


#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(max9867, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT maxim_max9867

static int max9867_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
    const struct max9867_config *config = dev->config;
    return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

__maybe_unused static int max9867_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
    const struct max9867_config *config = dev->config;
    return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

static int set_line_input_gain(const struct device *dev, audio_channel_t channel, uint8_t vol)
{
    const struct max9867_config *config = dev->config;
    int ret;
    uint8_t level[2];
    if (vol < 0 || vol > 15) {
        LOG_ERR("Volume out of range: %d", vol);
        return -EDOM;
    }

    switch (channel) {
    case AUDIO_CHANNEL_FRONT_LEFT:
        ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_LINE_IN_LEV_L,
                                     MAX9867_LINE_IN_LEV_X_GAIN_MASK, 0x0f - vol);
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for Left Line in: %d", ret);
            return ret;
        }
        break;
    case AUDIO_CHANNEL_FRONT_RIGHT:
        ret = i2c_reg_update_byte_dt(&config->i2c, MAX9867_LINE_IN_LEV_R,
                                     MAX9867_LINE_IN_LEV_X_GAIN_MASK, 0x0f - vol);
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for Right Line in: %d", ret);
            return ret;
        }
        break;
    case AUDIO_CHANNEL_ALL:
        level[0] = 0x0f - vol;
        level[1] = 0x0f - vol;
        ret = i2c_burst_write_dt(&config->i2c, MAX9867_LINE_IN_LEV_L, level, sizeof(level));
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for both Line ins: %d", ret);
            return ret;
        }

        break;
    default:
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    return 0;
}

static int set_mic_input_gain(const struct device *dev, audio_channel_t channel, uint8_t vol)
{
    const struct max9867_config *config = dev->config;
    int ret;
    uint8_t level[2];

    if (vol < 0 || vol > 50) {
        LOG_ERR("Volume out of range: %d", vol);
        return -EDOM;
    }
    uint8_t preamp_gain, mic_gain;
    split_mic_gain((audio_property_value_t){.vol = vol}, &preamp_gain, &mic_gain);
    level[0] = level[1] = ((preamp_gain & 0x3) << 5) | (mic_gain & 0x1f);

    switch (channel) {
    case AUDIO_CHANNEL_FRONT_LEFT:
        ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MIC_GAIN_L, level[0]);
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for Left Mic in : %d", ret);
            return ret;
        }
        break;
    case AUDIO_CHANNEL_FRONT_RIGHT:
        ret = i2c_reg_write_byte_dt(&config->i2c, MAX9867_MIC_GAIN_R, level[0]);
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for Right Mic in : %d", ret);
            return ret;
        }
        break;
    case AUDIO_CHANNEL_ALL:
        ret = i2c_burst_write_dt(&config->i2c, MAX9867_MIC_GAIN_L, level, sizeof(level));
        if (ret < 0) {
            LOG_ERR("Failed to set input gain for both Mic inputs: %d", ret);
            return ret;
        }
        return 0;
    default:
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }
    return 0;
}

int set_registers_default(const struct device *dev)
{
    const struct max9867_config *config = dev->config;

    uint8_t blank[MAX9867_SYS_SHDN - MAX9867_AUX_LOW];
    memset(blank, 0, sizeof(blank));
    int ret = i2c_burst_write_dt(&config->i2c, MAX9867_INTERRUPT_ENABLE, blank, sizeof(blank));
    if (ret < 0) {
        LOG_ERR("Failed to reset registers to default: %d", ret);
        return ret;
    }
    return 0;
}

int select_source(const struct device *dev, max9867_input_t input)
{
    const struct max9867_config *dev_cfg = dev->config;
    struct max9867_data *data = dev->data;

    int ret;

    if (input > MAX9867_INPUT_MIC) {
        LOG_ERR("Invalid input terminal: %d", input);
        return -EINVAL;
    }

    if (data->input_source == input) {
        LOG_DBG("Input source already set to %d", input);
        return 0;
    }
    data->input_source = input;

    switch (input) {
    case MAX9867_INPUT_LINE_IN:
        LOG_DBG("Selecting line in");
        ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_SYS_SHDN,
                                    MAX9867_SYS_SHDN_SHDN_nSHDN | MAX9867_SYS_SHDN_SHDN_LNLEN |
                                        MAX9867_SYS_SHDN_SHDN_LNREN); /* Todo: maybe this needs
                                                                         adlen/adren also*/
        if (ret < 0) {
            LOG_ERR("Failed to enable Line in: %d", ret);
            return ret;
        }
        ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_ADC_IN_CONF,
                                    MAX9867_ADC_IN_CONF_LINE_ONLY);
        if (ret < 0) {
            LOG_ERR("Failed to set Mixer to Line only: %d", ret);
            return ret;
        }
        break;
    case MAX9867_INPUT_MIC:
        LOG_DBG("Selecting mic in");
        ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_SYS_SHDN,
                                    MAX9867_SYS_SHDN_SHDN_nSHDN | MAX9867_SYS_SHDN_SHDN_ADLEN |
                                        MAX9867_SYS_SHDN_SHDN_ADREN);
        if (ret < 0) {
            LOG_ERR("Failed to enable Mic in: %d", ret);
            return ret;
        }
        ret =
            i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_ADC_IN_CONF, MAX9867_ADC_IN_CONF_MIC_ONLY);
        if (ret < 0) {
            LOG_ERR("Failed to set Mixer to Mic only: %d", ret);
            return ret;
        }
        break;
    default:
        LOG_ERR("Unsupported input: %d", input);
        return -ENOTSUP;
    }

    return 0;
}

static int get_mclk(const struct max9867_config *dev_cfg, uint32_t *mclk_rate) {
    int err;
//    err = clock_control_on(dev_cfg->mclk_dev, dev_cfg->mclk_name);
//    if (err < 0)
//    {
//        LOG_INF("Can't enable mclk control %d", err);
//        return err;
//    }

    uint32_t canary = UINT32_MAX;

    err = clock_control_get_rate(dev_cfg->mclk_dev, dev_cfg->mclk_name, &canary);
    if (err < 0)
    {
        LOG_INF("Cant get mclk: %d", err);
        return err;
    }
    /* clock_control_get_rate returns 0 even when it cant get the rate. Not as docstring */
    if(canary == UINT32_MAX)
    {
        LOG_INF("MCLK rate is not available dynamically");
        return -ENODATA;
    }
    *mclk_rate = canary;
    return 0;
}

static void uint8_to_binary(uint8_t val, char *buf)
{
    for (int i = 7; i >= 0; i--) {
        buf[7 - i] = (val & (1 << i)) ? '1' : '0';
    }
    buf[8] = '\0';
}

__maybe_unused
void dump_registers(const struct device *dev)
{
    const struct max9867_config *config = dev->config;
    uint8_t reg[MAX9867_SYS_SHDN+1];
    int ret = i2c_burst_read_dt(&config->i2c, MAX9867_STATUS, reg, sizeof(reg));
    if (ret < 0) {
        LOG_ERR("Failed to read registers: %d", ret);
        return;
    }
    LOG_INF("MAX9867 Registers:");
    for (int i = 0; i < sizeof(reg); i += 4) {
        if (i + 3 < sizeof(reg)) {
            LOG_INF("\033[36m0x%02x\033[0m: \033[33m0x%02x\033[0m  \033[36m0x%02x\033[0m: \033[33m0x%02x\033[0m  \033[36m0x%02x\033[0m: \033[33m0x%02x\033[0m  \033[36m0x%02x\033[0m: \033[33m0x%02x\033[0m",
                    MAX9867_STATUS + i, reg[i],
                    MAX9867_STATUS + i + 1, reg[i + 1],
                    MAX9867_STATUS + i + 2, reg[i + 2],
                    MAX9867_STATUS + i + 3, reg[i + 3]);
            char bin0[9], bin1[9], bin2[9], bin3[9];
            uint8_to_binary(reg[i], bin0);
            uint8_to_binary(reg[i + 1], bin1);
            uint8_to_binary(reg[i + 2], bin2);
            uint8_to_binary(reg[i + 3], bin3);
            LOG_INF("  %s  | %s  | %s  | %s", bin0, bin1, bin2, bin3);
        } else {
            // Handle remaining registers if not divisible by 4
            for (int j = i; j < sizeof(reg); j++) {
                LOG_INF("\033[36m0x%02x\033[0m: \033[33m0x%02x\033[0m", MAX9867_STATUS + j, reg[j]);
                char bin[9];
                uint8_to_binary(reg[j], bin);
                LOG_INF("      %s", bin);
            }
        }
    }

}

static int max9867_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
    const struct max9867_config *dev_cfg = dev->config;
    struct max9867_data *data = dev->data;
    int ret;

    switch (cfg->dai_type) {
    case AUDIO_DAI_TYPE_I2S:
        LOG_DBG("Configuring I2S DAI");
        break;
        /* Codec also supports TDM, but it's not in audio_dai_type_t currently*/
    case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
        LOG_ERR("Left justified DAI type supported in MAX9867, not yet implemented "
                "in this driver");
        return -ENOSYS;
    default:
        LOG_ERR("DAI type unsupported");
        return -ENOTSUP;
    }

    if (cfg->dai_cfg.i2s.word_size != 16) {
        LOG_ERR("Only 16-bit word size is supported by MAX9867");
        return -EPFNOSUPPORT;
    }

    if (cfg->dai_cfg.i2s.frame_clk_freq < 8000 || cfg->dai_cfg.i2s.frame_clk_freq > 48000) {
        LOG_ERR("Frame clock frequency (%u) is out of range (8000-48000Hz)",
                cfg->dai_cfg.i2s.frame_clk_freq);
        return -EINVAL;
    }

    if(get_mclk(dev_cfg, &cfg->mclk_freq) != 0)
    {
        cfg->mclk_freq = dev_cfg->mclk_default;
        LOG_INF("MCLK frequency unavailable dynamically, using default: %u", dev_cfg->mclk_default);
    }

    if (cfg->mclk_freq % cfg->dai_cfg.i2s.frame_clk_freq != 0) {
        LOG_ERR("MCLK rate (%u) is not a multiple of frame clock frequency (%u)", cfg->mclk_freq,
                cfg->dai_cfg.i2s.frame_clk_freq);
        LOG_ERR("Driver does not currently support PLL");
        return -EPROTO;
    }

    data->sample_rate = cfg->dai_cfg.i2s.frame_clk_freq;

    ret = set_registers_default(dev);
    if (ret < 0) {
        LOG_ERR("Failed to reset registers: %d", ret);
        return ret;
    }

    /* Setup clocks in "Normal" mode */
    uint8_t pre_scaler = 0;
    if (cfg->mclk_freq < 10e6 || cfg->mclk_freq > 20e6) {
        LOG_ERR("Driver currently only supports MCLK rate (%u) between 10 and 20MHz",
                cfg->mclk_freq);
        return -ENOSR;
    } else {
        pre_scaler = MAX9867_SYS_CLK_PSCLK_10_20MHZ;
    }

    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_SYS_CLK, pre_scaler); /* FREQ unused */
    if (ret < 0) {
        LOG_ERR("Failed to set system clock prescaler: %d", ret);
        return ret;
    }

    uint32_t ni = calculate_ni(cfg->mclk_freq, cfg->dai_cfg.i2s.frame_clk_freq);

    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_SACLK_CTRL_HI,
                                (ni >> 8) & MAX9867_SACLK_CTRL_HI_NI_MASK);
    if (ret < 0) {
        LOG_ERR("Failed to set SA Clock high: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_SACLK_CTRL_LO, ni & 0xff);
    if (ret < 0) {
        LOG_ERR("Failed to set SA Clock high: %d", ret);
        return ret;
    }

    /* Default for MAX9867_DAI_IF_MODE1*/
    /* Default for MAX9867_DAI_IF_MODE2*/
    /* Default for MAX9867_DAI_IF_MODE1*/
    /* Default for MAX9867_CODEC_FILTER*/

    /* Mute the DACs*/
    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_DAC_LEVEL, MAX9867_DAC_LEVEL_DACMUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute DAC: %d", ret);
        return ret;
    }

    set_mic_input_gain(dev, AUDIO_CHANNEL_ALL, 50);
    set_line_input_gain(dev, AUDIO_CHANNEL_ALL, 15);

    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_VOL_L, MAX9867_VOL_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Playback volume L: %d", ret);
        return ret;
    }

    ret = i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_VOL_R, MAX9867_VOL_X_MUTE);
    if (ret < 0) {
        LOG_ERR("Failed to Mute Playback volume R: %d", ret);
        return ret;
    }

    select_source(dev, MAX9867_INPUT_MIC); /* Default to Mic input */

    i2c_reg_write_byte_dt(&dev_cfg->i2c, MAX9867_DAI_IF_MODE1, 0x08);

//    dump_registers(dev);

    return 0;
}

static int max9867_set_property(const struct device *dev, audio_property_t property,
                                audio_channel_t channel, audio_property_value_t val)
{

    struct max9867_data *data = dev->data;

    switch (property) {
    case AUDIO_PROPERTY_INPUT_VOLUME:
        break;
    case AUDIO_PROPERTY_INPUT_MUTE:
        LOG_ERR("Property %d not implemented yet", property);
        return -ENOSYS;
    default:
        LOG_ERR("Property %d not supported", property);
        return -ENOTSUP;
    }

    switch (data->input_source) {
    case MAX9867_INPUT_LINE_IN:
        return set_line_input_gain(dev, channel, val.vol);
    case MAX9867_INPUT_MIC:
        return set_mic_input_gain(dev, channel, val.vol);
    case MAX9867_INPUT_NONE:
    default:
        LOG_ERR("Input source not selected or unsupported: %d", data->input_source);
        return -ENOTSUP;
    }
}

static int max9867_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
    int ret;

    if (channel != AUDIO_CHANNEL_FRONT_LEFT && channel != AUDIO_CHANNEL_FRONT_RIGHT &&
        channel != AUDIO_CHANNEL_ALL) {
        LOG_ERR("Invalid channel: %d", channel);
        return -EINVAL;
    }

    ret = select_source(dev, input);
    if (ret < 0) {
        LOG_ERR("Failed to select source for channel %d: %d", channel, ret);
        return ret;
    }

    return 0;
}

static const struct audio_codec_api max9867_driver_api = {
    .configure = max9867_configure,
    //	.start_output = max9867_start_output, /* Maybe these are required?*/
    //	.stop_output = max9867_stop_output,
    .set_property = max9867_set_property,
    .route_input = max9867_route_input,
};

static int max9867_init(const struct device *dev)
{
    const struct max9867_config *dev_cfg = dev->config;
    struct max9867_data *data = dev->data;
    uint8_t revision;
    int ret;

    if (!device_is_ready(dev_cfg->i2c.bus)) {
        LOG_ERR("I2C bus %s not ready", dev_cfg->i2c.bus->name);
        return -ENODEV;
    }

    data->input_source = MAX9867_INPUT_NONE;

    ret = max9867_reg_read(dev, MAX9867_REVISION, &revision);
    if (ret < 0) {
        LOG_ERR("Failed to read revision register: %d", ret);
        return ret;
    }
    LOG_DBG("Chip revision: 0x%02x", revision);

    return set_registers_default(dev);
}

#define MAX9867_DEFINE(inst)                                                                       \
    static struct max9867_data max9867_data_##inst;                                                \
                                                                                                   \
    static const struct max9867_config max9867_config_##inst = {                                   \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                         \
        .clock_source = DT_INST_PROP_OR(inst, clk_source, 0),                                      \
        .mclk_default = DT_INST_PROP(inst, mclk_default), \
        .mclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, mclk)),                        \
        .mclk_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(inst, mclk, name)};       \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, max9867_init, NULL, &max9867_data_##inst, &max9867_config_##inst,  \
                          POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &max9867_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX9867_DEFINE)
