#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */




/* ADC configuration context */
typedef struct {
    struct adc_sequence sequence;
    temp_block_t current_block;
    uint16_t sample_count;
    int16_t m_sample_buffer;
} temp_adc_ctx_t;

/* Static ADC device tree spec initialization */
static const struct adc_dt_spec temp_adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static temp_adc_ctx_t adc_ctx;
static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

/* Hardware timer device */
static const struct device *counter_dev;

/* Counter alarm callback for ADC sampling */
static void timer_alarm_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
    struct counter_alarm_cfg alarm_cfg;
    int ret;
    
    /* Perform ADC read using Zephyr ADC API */
    ret = adc_read_dt(&temp_adc_channel, &adc_ctx.sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed: %d", ret);
        return;
    }
    
    LOG_HEXDUMP_DBG(&adc_ctx.m_sample_buffer, sizeof(adc_ctx.m_sample_buffer), "ADC timer result");
    
    /* Store sample in current block */
    adc_ctx.current_block.samples[adc_ctx.sample_count] = adc_ctx.m_sample_buffer;
    adc_ctx.sample_count++;
    
    /* If block is full, send it to message queue */
    if (adc_ctx.sample_count >= TEMP_BLOCK_SIZE) {
        adc_ctx.current_block.count = adc_ctx.sample_count;
        adc_ctx.current_block.timestamp_ms = k_uptime_get_32();
        
        ret = k_msgq_put(&temp_msgq, &adc_ctx.current_block, K_NO_WAIT);
        if (ret != 0) {
            LOG_WRN("Temperature message queue full, dropping block");
        }
        
        adc_ctx.sample_count = 0;
    }
    
    /* Reschedule alarm for next period (relative to current time) */
    alarm_cfg.flags = 0; /* Relative alarm */
    alarm_cfg.ticks = counter_us_to_ticks(dev, 10000); /* 10ms = 10000us */
    alarm_cfg.callback = timer_alarm_callback;
    alarm_cfg.user_data = NULL;
    
    ret = counter_set_channel_alarm(dev, chan_id, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to reschedule counter alarm: %d", ret);
    }
}

/* Configure hardware timer using Zephyr Counter API for 100Hz ADC triggering */
static int configure_counter_for_adc_trigger(void)
{
    struct counter_alarm_cfg alarm_cfg;
    uint32_t freq;
    int ret;
    
    LOG_DBG("Configuring Counter (CTIMER0) for 100Hz ADC trigger");
    
    /* Get counter device */
    counter_dev = DEVICE_DT_GET(DT_NODELABEL(ctimer0));
    if (!device_is_ready(counter_dev)) {
        LOG_ERR("Counter device not ready");
        return -ENODEV;
    }
    
    /* Get counter frequency */
    freq = counter_get_frequency(counter_dev);
    LOG_DBG("Counter frequency: %u Hz", freq);
    
    /* Start counter */
    ret = counter_start(counter_dev);
    if (ret < 0) {
        LOG_ERR("Counter start failed: %d", ret);
        return ret;
    }
    
    /* Configure alarm for 100Hz (10ms period) */
    alarm_cfg.flags = 0; /* Relative alarm */
    alarm_cfg.ticks = counter_us_to_ticks(counter_dev, 10000); /* 10ms = 10000us */
    alarm_cfg.callback = timer_alarm_callback;
    alarm_cfg.user_data = NULL;
    
    ret = counter_set_channel_alarm(counter_dev, 0, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Counter alarm setup failed: %d", ret);
        return ret;
    }
    
    LOG_DBG("Counter alarm configured for %u ticks (%u us)", alarm_cfg.ticks, 10000);
    return 0;
}

int init_temperature(void)
{
    int ret;

    LOG_DBG("Initializing temperature ADC with hardware timer trigger");
    
    /* Initialize ADC using the static device tree spec */
    ret = adc_is_ready_dt(&temp_adc_channel);
    if (!ret) {
        LOG_ERR("ADC device is not ready");
        return -ENODEV;
    }

    ret = adc_channel_setup_dt(&temp_adc_channel);
    if (ret < 0) {
        LOG_ERR("ADC channel setup failed: %d", ret);
        return ret;
    }

    adc_ctx.sequence.buffer = &adc_ctx.m_sample_buffer;
    adc_ctx.sequence.buffer_size = sizeof(adc_ctx.m_sample_buffer);

    ret = adc_sequence_init_dt(&temp_adc_channel, &adc_ctx.sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }

    /* Initialize message queue */
    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

    adc_ctx.sample_count = 0;
    adc_ctx.current_block.count = 0;
    adc_ctx.current_block.timestamp_ms = 0;

    /* Configure hardware timer trigger using Zephyr Counter API */
    ret = configure_counter_for_adc_trigger();
    if (ret < 0) {
        LOG_ERR("Counter configuration failed: %d", ret);
        return ret;
    }
    
    LOG_INF("Temperature ADC initialized with hardware timer trigger at 100Hz using Zephyr Counter API");
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

