#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"
#include "debug_leds.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */

static const struct adc_dt_spec temp_adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

/* Todo: Unglobal?*/
static int16_t m_sample_buffer[100];
struct adc_sequence_options options;
struct adc_sequence sequence;

static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

enum adc_action adc_callback(const struct device *dev,
                                                 const struct adc_sequence *sequence,
                                                 uint16_t sampling_index)
{
    debug_led_toggle(2);
    LOG_DBG("ADC callback: sampling_index %d", sampling_index);
    LOG_HEXDUMP_DBG(sequence->buffer, sequence->buffer_size, "ADC buffer");
//    return ADC_ACTION_REPEAT;
    if(sampling_index > 20) {
        memset(m_sample_buffer, 0xaa, sizeof(m_sample_buffer));
        return ADC_ACTION_REPEAT;
    }
    else
        return ADC_ACTION_CONTINUE;
}


int init_temperature(void)
{
    int ret;

    LOG_DBG("Initializing temperature ADC");

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

    memset(&sequence, 0, sizeof(sequence));
    memset(&options, 0, sizeof(options));
    memset(&m_sample_buffer, 0xaa, sizeof(m_sample_buffer));

    /* Set .channels, .resolution and .oversampling from dt */
    ret = adc_sequence_init_dt(&temp_adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }

    sequence.buffer = &m_sample_buffer;
    sequence.buffer_size = sizeof(m_sample_buffer);
    sequence.options = &options;
    options.extra_samplings = 27;
    options.interval_us = 800000;//CONFIG_NASAL_TEMP_SAMPLE_PERIOD;
    options.callback = adc_callback;

    /* Trigger the first read, continues based on return value in callback*/
    ret = adc_read_async(temp_adc_channel.dev, &sequence, NULL);
    if (ret < 0)
    {
        LOG_ERR("ADC async read failed: %d", ret);
        return ret;
    }

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

