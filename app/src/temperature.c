#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"

LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);

#define ADC_NODE DT_NODELABEL(lpadc0)
#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define ADC_CHANNEL 3  /* ADC0_A3 channel for ANA_6 (mapped to PIO4_6) */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */

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
    int ret;
    struct adc_channel_cfg adc_channel_cfg;
    struct adc_sequence adc_seq;

    sample_count = 0;
    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

    timer_ctx.adc_dev = DEVICE_DT_GET(ADC_NODE);
    if (!device_is_ready(timer_ctx.adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }


    /* Configure ADC channel */
    adc_channel_cfg.gain = ADC_GAIN_1;
    adc_channel_cfg.reference = ADC_REF_EXTERNAL0;
    adc_channel_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
    adc_channel_cfg.channel_id = ADC_CHANNEL;
    adc_channel_cfg.differential = 0;

    ret = adc_channel_setup(timer_ctx.adc_dev, &adc_channel_cfg);
    if (ret < 0) {
        LOG_ERR("ADC channel setup failed: %d", ret);
        return ret;
    }
    
    /* Configure ADC sequence in timer context */
    adc_seq.channels = BIT(ADC_CHANNEL);
    adc_seq.buffer_size = sizeof(int16_t);
    adc_seq.resolution = 16;
    adc_seq.oversampling = 0;
    adc_seq.calibrate = false;
    timer_ctx.adc_seq = adc_seq;

    k_timer_init(&temp_timer, temp_timer_handler, NULL);
    k_timer_user_data_set(&temp_timer, &timer_ctx);
    k_timer_start(&temp_timer, K_MSEC(TEMP_SAMPLE_INTERVAL_MS), K_MSEC(TEMP_SAMPLE_INTERVAL_MS));
    return 0;
}

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

