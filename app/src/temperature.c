#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */




typedef struct {
    const struct adc_dt_spec *adc_dev_dt;
    struct adc_sequence sequence;
    temp_block_t current_block;
    uint16_t sample_count;
    int16_t m_sample_buffer;
} temp_timer_ctx_t;

static struct k_timer temp_timer;
static temp_timer_ctx_t timer_ctx;

static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

static void temp_timer_handler(struct k_timer *timer)
{
    temp_timer_ctx_t *ctx = k_timer_user_data_get(timer);
    int16_t sample_buffer;
    int ret;

    ret = adc_read_dt(ctx->adc_dev_dt, &ctx->sequence);
    if (ret < 0)
    {
        LOG_ERR("ADC read failed: %d", ret);
        return;
    }

    LOG_HEXDUMP_DBG(&m_sample_buffer, sizeof(m_sample_buffer), "ADC read buffer");

    /* This can be optimised with pointer shuffling but it's going in the bin hopefully*/
    ctx->current_block.samples[ctx->sample_count] = ctx->m_sample_buffer;
    ctx->sample_count++;

    /* If block is full, send it to message queue */
    if (ctx->sample_count >= TEMP_BLOCK_SIZE) {
        ctx->current_block.count = ctx->sample_count;
        ctx->current_block.timestamp_ms = k_uptime_get_32();

        ret = k_msgq_put(&temp_msgq, &ctx->current_block, K_NO_WAIT);
        if (ret != 0) {
            LOG_WRN("Temperature message queue full, dropping block");
        }
        
        ctx->sample_count = 0;
    }
}

int init_temperature(void)
{
    int ret;

    LOG_DBG("Initializing temperature ADC");
    timer_ctx.adc_dev_dt = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

    ret = adc_is_ready_dt(timer_ctx.adc_dev_dt);
    if (!ret) {
        LOG_ERR("ADC device is not ready");
        return -ENODEV;
    }

    ret = adc_channel_setup_dt(timer_ctx.adc_dev_dt);
    if (ret < 0) {
        LOG_ERR("ADC channel setup failed: %d", ret);
        return ret;
    }

    timer_ctx.sequence.buffer = &timer_ctx.m_sample_buffer;
    timer_ctx.sequence.buffer_size = sizeof(timer_ctx.m_sample_buffer);

    ret = adc_sequence_init_dt(timer_ctx.adc_dev_dt, &timer_ctx.sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }


    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

    timer_ctx.sample_count = 0;
    timer_ctx.current_block.count = 0;
    timer_ctx.current_block.timestamp_ms = 0;

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

