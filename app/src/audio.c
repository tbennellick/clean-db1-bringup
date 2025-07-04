#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include "max9867.h"
#include <math.h>
//#include <zephyr/sys/iterable_sections.h>

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

/* TX memory slab for outgoing audio data */
char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

#define TIMEOUT          2000
#define FRAME_CLK_FREQ   8000


void init_i2s(void)
{
    void *rx_block, *tx_block;
    size_t rx_size;
    static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
    struct i2s_config i2s_cfg_tx, i2s_cfg_rx;
    int ret;
    
    if (!device_is_ready(dev_i2s)) {
        LOG_ERR("I2S device not ready");
        return;
    }

//#ifdef SAI_TX
    /* Configure TX stream (master - generates clocks) */
    i2s_cfg_tx.word_size = 16U;
    i2s_cfg_tx.channels = 2U;
    i2s_cfg_tx.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_tx.frame_clk_freq = FRAME_CLK_FREQ;
    i2s_cfg_tx.block_size = BLOCK_SIZE;
    i2s_cfg_tx.timeout = TIMEOUT;
    i2s_cfg_tx.options = 0; /* TX will be master (generates bit clock and word clock) */
    i2s_cfg_tx.mem_slab = &tx_0_mem_slab;

    ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg_tx);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S TX stream (%d)", ret);
        return;
    }
//#endif

    /* Configure RX stream  */
    i2s_cfg_rx.word_size = 16U;
    i2s_cfg_rx.channels = 2U;
    i2s_cfg_rx.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_rx.frame_clk_freq = FRAME_CLK_FREQ;
    i2s_cfg_rx.block_size = BLOCK_SIZE;
    i2s_cfg_rx.timeout = TIMEOUT;
    i2s_cfg_rx.options = 0;
//    /* Enable loopback for testing - connects TX output to RX input internally */
//    i2s_cfg_rx.options |= I2S_OPT_LOOPBACK;
    i2s_cfg_rx.mem_slab = &rx_0_mem_slab;

    ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg_rx);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
        return;
    }


#ifdef SAI_TX

    //    /* Pre-fill TX buffers with sine wave data to maintain clock generation */
//    for (int i = 0; i < 3; i++) {
        ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_NO_WAIT);
        if (ret != 0) {
            LOG_ERR("Failed to allocate TX buffer (%d)", ret);
            return;
        }
//
////        /* Generate 1kHz sine wave for testing */
////        generate_sine_wave((int16_t*)tx_block, BLOCK_SIZE/2, 1000, FRAME_CLK_FREQ);
//
        ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
        if (ret < 0) {
            LOG_ERR("Failed to write initial TX buffer (%d)", ret);
            k_mem_slab_free(&tx_0_mem_slab, tx_block);
            return;
        }
//    }

    /* Start TX first (this will generate the bit clock and word clock) */
    ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S TX stream (%d)", ret);
        return;
    }
    
    LOG_INF("TX stream started - generating bit clock and word clock");


    for(uint32_t i = 0; i < 1000; i++)
    {
        ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_MSEC(100));
        if (ret == 0)
        {
            ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
            if (ret < 0)
            {
                LOG_ERR("Failed to write TX buffer (%d)", ret);
                k_mem_slab_free(&tx_0_mem_slab, tx_block);
            }
        }
        else
        {
            LOG_ERR("Failed to allocate TX buffer (%d)", ret);
            k_sleep(K_MSEC(100));
        }
    }
    LOG_INF("TX stuffing ended");

#endif

    ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S TX stream (%d)", ret);
        return;
    }


    while(1)
    {
        ret = i2s_read(dev_i2s, &rx_block, &rx_size);
        if (ret < 0)
        {
            LOG_ERR("Failed to read I2S RX stream (%d)", ret);
            return;
        }
        LOG_INF("Received %d bytes from I2S RX stream", rx_size);

        k_mem_slab_free(&rx_0_mem_slab, rx_block);
    }

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
