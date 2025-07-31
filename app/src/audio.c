#include <zephyr/kernel.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include "max9867.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(audio, LOG_LEVEL_INF);

#define TIMEOUT          2000
#define SAMPLE_RATE   8000

#define I2S_DEV_NODE_RX DT_NODELABEL(sai1)


/* TODO: Think about this when considering system architecture  */
#define NUM_BLOCKS 20
#define SAMPLE_NO 64

#define BLOCK_SIZE (2 * SAMPLE_NO)

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif


char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

#define RX_THREAD_STACK_SIZE 1024
#define RX_THREAD_PRIORITY 4

static struct k_thread rx_thread_data;
K_THREAD_STACK_DEFINE(rx_thread_stack, RX_THREAD_STACK_SIZE);


void rx_thread_func(void *p1, void *p2, void *p3)
{
    const struct device *dev_i2s = (const struct device *)p1;
    void *rx_block;
    size_t rx_size;
    int ret;
    
    LOG_INF("SAI RX thread started");
    
    while(1)
    {
        ret = i2s_read(dev_i2s, &rx_block, &rx_size);
        if (ret < 0)
        {
            LOG_ERR("Failed to read I2S RX stream (%d)", ret);
            return;
        }

        k_mem_slab_free(&rx_0_mem_slab, rx_block);
    }
}

#ifdef CONFIG_BOARD_DB1_REV_62_2

#define TX_THREAD_STACK_SIZE 1024
#define TX_THREAD_PRIORITY 5

/* TX memory slab for outgoing audio data */
char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
        _k_mem_slab_buf_tx_0_mem_slab[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
        Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
                               WB_UP(BLOCK_SIZE), NUM_BLOCKS);


static struct k_thread tx_thread_data;
K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);

/* We are currently using the SAI TX clocks because that is how the hardware is wired.
 * Even when operating synchronously(slave to RX), It seems that the TX section will
 * only generate frame sync when the FIFO is non-empty hence we have this thread filling
 * The TX FIFO continuously.
 * See DS: 67.6.1.8 - TCR4:ONDEM description
 * This will be dropped with the next board rev. */
void tx_thread_func(void *p1, void *p2, void *p3)
{
    const struct device *dev_i2s = (const struct device *)p1;
    void *tx_block;
    int ret;
    
    LOG_INF("TX thread started");
    
    while(1)
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
}

static int configure_and_start_tx(const struct device *dev_i2s)
{
    struct i2s_config i2s_cfg_tx;
    void *tx_block;
    int ret;

    i2s_cfg_tx.word_size = 16U;
    i2s_cfg_tx.channels = 2U;
    i2s_cfg_tx.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_tx.frame_clk_freq = SAMPLE_RATE;
    i2s_cfg_tx.block_size = BLOCK_SIZE;
    i2s_cfg_tx.timeout = TIMEOUT;
    i2s_cfg_tx.mem_slab = &tx_0_mem_slab;
    i2s_cfg_tx.options = 0;

    ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg_tx);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S TX stream (%d)", ret);
        return ret;
    }

    ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_NO_WAIT);
    if (ret != 0) {
        LOG_ERR("Failed to allocate TX buffer (%d)", ret);
        return ret;
    }

    ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
    if (ret < 0) {
        LOG_ERR("Failed to write initial TX buffer (%d)", ret);
        k_mem_slab_free(&tx_0_mem_slab, tx_block);
        return ret;
    }

    ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S TX stream (%d)", ret);
        return ret;
    }

    LOG_INF("TX stream started");

    k_thread_create(&tx_thread_data, tx_thread_stack,
                    K_THREAD_STACK_SIZEOF(tx_thread_stack),
                    tx_thread_func,
                    (void *)dev_i2s, NULL, NULL,
                    TX_THREAD_PRIORITY, 0, K_NO_WAIT);

    return 0;
}
#endif

int init_i2s(void)
{
    static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
    struct i2s_config i2s_cfg_rx;
    int ret;
    
    if (!device_is_ready(dev_i2s)) {
        LOG_ERR("I2S device not ready");
        return -ENODEV;
    }

    i2s_cfg_rx.word_size = 16U;
    i2s_cfg_rx.channels = 2U;
    i2s_cfg_rx.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_rx.frame_clk_freq = SAMPLE_RATE;
    i2s_cfg_rx.block_size = BLOCK_SIZE;
    i2s_cfg_rx.timeout = TIMEOUT;
    i2s_cfg_rx.options = 0;
//    i2s_cfg_rx.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
//    i2s_cfg_rx.options |= I2S_OPT_LOOPBACK;

    i2s_cfg_rx.mem_slab = &rx_0_mem_slab;

    ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg_rx);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
        return ret;
    }

#ifdef CONFIG_BOARD_DB1_REV_62_2
    /* Datasheet */
//    If both the transmitter and receiver use the receiver bit clock and frame sync:
//    • Configure the receiver for asynchronous operation and the transmitter for synchronous operation.
//    • Enable the transmitter in Synchronous mode only after configuring both the receiver and transmitter.
//    • Enable the receiver last and disable the receiver first.

    ret = configure_and_start_tx(dev_i2s);
    if (ret < 0) {
        LOG_ERR("Failed to configure and start TX stream (%d)", ret);
        return ret;
    }
#endif


    ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S RX stream (%d)", ret);
        return ret;
    }

    /* Start RX thread to handle incoming audio data */
    k_thread_create(&rx_thread_data, rx_thread_stack,
                    K_THREAD_STACK_SIZEOF(rx_thread_stack),
                    rx_thread_func,
                    (void *)dev_i2s, NULL, NULL,
                    RX_THREAD_PRIORITY, 0, K_NO_WAIT);

    LOG_INF("Audio system initialized - RX and TX threads running");
    return 0;

}

int init_codec() {
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


int init_audio(void) {
    int ret = init_i2s();
    if (ret < 0) {
        LOG_ERR("Failed to initialize I2S: %d", ret);
        return ret;
    }

    ret = init_codec();
    if (ret < 0) {
        LOG_ERR("Failed to initialize audio codec: %d", ret);
        return ret;
    }
    return 0;
}

