#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include "max9867.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(audio, LOG_LEVEL_INF);

#define I2S_DEV_NODE_RX DT_ALIAS(i2s_node0)

#define NUM_BLOCKS 20
#define SAMPLE_NO 64


#define BLOCK_SIZE (2 * SAMPLE_NO)

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif


/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * e.g. i2s_mcux_flexcomm, permanently keep ownership of a few RX buffers. Add a few more
 * RX blocks to satisfy this requirement
 */
/* There is info on this chaos in i2s.h:285*/
char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

#define TIMEOUT          2000
#define FRAME_CLK_FREQ   44000

static int configure_stream(const struct device *dev_i2s)
{
	int ret;
	struct i2s_config i2s_cfg;

    /* COnsumed by drivers/i2s/i2s_mcux_sai.c */
    i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;
    i2s_cfg.options = 0; //I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;


    /* Useful for testing?*/
    //	i2s_cfg.options |= I2S_OPT_LOOPBACK;


    i2s_cfg.mem_slab = &rx_0_mem_slab;
    ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
        return -EIO;
    }
	return 0;
}

void init_i2s(void)
{
    void *rx_block;
    size_t rx_size;
    static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
    int ret = configure_stream(dev_i2s);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
        return;
    }

//    ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
//    if (ret < 0) {
//        LOG_ERR("Failed to start I2S RX stream (%d)", ret);
//        return;
//    }
//
//    for(uint8_t i =0; i<10; i++) {
//        ret = i2s_read(dev_i2s, &rx_block, &rx_size);
//        if (ret < 0) {
//            LOG_ERR("Failed to read I2S RX stream (%d)", ret);
//            return;
//        }
//        LOG_INF("Received %d bytes from I2S RX stream", rx_size);
//    }

}


int init_audio(void) {

    const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
    struct audio_codec_cfg audio_cfg;

    if (!device_is_ready(codec_dev))
    {
        LOG_ERR("%s is not ready", codec_dev->name);
        return -ENODEV;
    }

    audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
    audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
    audio_cfg.dai_cfg.i2s.word_size = 16;
    audio_cfg.dai_cfg.i2s.channels = 2;
    audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
    audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    audio_cfg.dai_cfg.i2s.frame_clk_freq = 8000;
    audio_cfg.dai_cfg.i2s.mem_slab = NULL;
    audio_cfg.dai_cfg.i2s.block_size = 0;

    int ret = audio_codec_configure(codec_dev, &audio_cfg);
    if (ret < 0)
    {
        LOG_ERR("Failed to configure audio codec: %d", ret);
        return ret;
    }

    ret = audio_codec_route_input(codec_dev, AUDIO_CHANNEL_ALL, MAX9867_INPUT_MIC);
    if (ret < 0)
    {
        LOG_ERR("Failed to route input: %d", ret);
        return ret;
    }
    ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_INPUT_VOLUME,
                                   AUDIO_CHANNEL_ALL, (audio_property_value_t) {.vol = 10});
    if (ret < 0)
    {
        LOG_ERR("Failed to set input volume: %d", ret);
        return ret;
    }
    LOG_DBG("Audio codec configured successfully");
    return 0;
}
