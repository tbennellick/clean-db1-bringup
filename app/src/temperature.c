#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"

LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);

#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),


#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};
static const int adc_channels_count = ARRAY_SIZE(adc_channels);
#else
#error "Unsupported board."
#endif



typedef struct {
    const struct device *adc_dev;
    struct adc_sequence adc_seq;
} temp_timer_ctx_t;

static struct k_timer temp_timer;
static temp_timer_ctx_t timer_ctx;

/* Sample aggregation variables */
static temp_block_t current_block;
static uint16_t sample_count = 0;

static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

static void temp_timer_handler(struct k_timer *timer)
{
    temp_timer_ctx_t *ctx = k_timer_user_data_get(timer);
    int16_t sample_buffer;
    
    ctx->adc_seq.buffer = &sample_buffer;
    
    int ret = adc_read(ctx->adc_dev, &ctx->adc_seq);
    if (ret < 0) {
        LOG_ERR("ADC read failed: %d", ret);
        return;
    }
    
    current_block.samples[sample_count] = sample_buffer;
    sample_count++;
    
    /* If block is full, send it to message queue */
    if (sample_count >= TEMP_BLOCK_SIZE) {
        current_block.count = sample_count;
        current_block.timestamp_ms = k_uptime_get_32();
        
        ret = k_msgq_put(&temp_msgq, &current_block, K_NO_WAIT);
        if (ret != 0) {
            LOG_WRN("Temperature message queue full, dropping block");
        }
        
        sample_count = 0;
    }
}

int init_temperature(void)
{
    int i, ret;

    ret = adc_is_ready_dt(&adc_channels[0]);
    if (!ret) {
        LOG_ERR("ADC device is not ready");
        return -ENODEV;
    }

//
//    for (i = 0; i < adc_channels_count; i++) {
//        ret = adc_channel_setup_dt(&adc_channels[i]);
//        zassert_equal(ret, 0, "Setting up of channel %d failed with code %d", i, ret);
//    }
//
//    for (i = 0; i < BUFFER_SIZE; ++i) {
//        m_sample_buffer[i] = INVALID_ADC_VALUE;
//    }
//
//#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(test_counter)) && \
//	defined(CONFIG_COUNTER)
//    init_counter();
//#endif
}


//int init_temperature(void)
//{
//    int ret;
//    struct adc_channel_cfg adc_channel_cfg;
//    struct adc_sequence adc_seq;
//
//    sample_count = 0;
//    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);
//
//    timer_ctx.adc_dev = DEVICE_DT_GET(ADC_NODE);
//    if (!device_is_ready(timer_ctx.adc_dev)) {
//        LOG_ERR("ADC device not ready");
//        return -ENODEV;
//    }
//
//
//    /* Configure ADC channel */
//    adc_channel_cfg.gain = ADC_GAIN_1;
//    adc_channel_cfg.reference = ADC_REF_EXTERNAL0;
//    adc_channel_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
//    adc_channel_cfg.channel_id = ADC_CHANNEL;
//    adc_channel_cfg.differential = 0;
//
//    ret = adc_channel_setup(timer_ctx.adc_dev, &adc_channel_cfg);
//    if (ret < 0) {
//        LOG_ERR("ADC channel setup failed: %d", ret);
//        return ret;
//    }
//
//    /* Configure ADC sequence in timer context */
//    adc_seq.channels = BIT(ADC_CHANNEL);
//    adc_seq.buffer_size = sizeof(int16_t);
//    adc_seq.resolution = 16;
//    adc_seq.oversampling = 0;
//    adc_seq.calibrate = false;
//    timer_ctx.adc_seq = adc_seq;
//
//    k_timer_init(&temp_timer, temp_timer_handler, NULL);
//    k_timer_user_data_set(&temp_timer, &timer_ctx);
//    k_timer_start(&temp_timer, K_MSEC(TEMP_SAMPLE_INTERVAL_MS), K_MSEC(TEMP_SAMPLE_INTERVAL_MS));
//    return 0;
//}

int temperature_read_block(temp_block_t *block, k_timeout_t timeout)
{
    if (block == NULL) {
        return -EINVAL;
    }
    
    int ret = k_msgq_get(&temp_msgq, block, timeout);
    if (ret == 0) {
        LOG_DBG("Temperature block read: %d samples, timestamp: %u ms", 
                block->count, block->timestamp_ms);
    }
    
    return ret;
}

