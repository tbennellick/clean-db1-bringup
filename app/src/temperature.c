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


static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];



int init_temperature(void)
{
    int ret;

    LOG_DBG("Initializing temperature ADC");

    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

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

    static int16_t m_sample_buffer;
    struct adc_sequence_options options;
    struct adc_sequence sequence;


    memset(&sequence, 0, sizeof(sequence));
    memset(&options, 0, sizeof(options));
    memset(&m_sample_buffer, 0xaa, sizeof(m_sample_buffer));

    /* Set .channels, .resolution and .oversampling from dt */
    ret = adc_sequence_init_dt(&temp_adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }

    uint8_t sample_count =0;
    temp_block_t block;
    block.count = 0;
    while(1)
    {
        sequence.buffer = &block.samples[sample_count*2];
        sequence.buffer_size = sizeof(int16_t);
        ret = adc_read(temp_adc_channel.dev, &sequence);
        if (ret < 0) {
            LOG_ERR("ADC read failed: %d", ret);
            return ret;
        }
        if(sample_count>= TEMP_BLOCK_SIZE)
        {
            sample_count = 0;
            block.timestamp_ms =  k_uptime_get_32();
            block.count++;
            k_msgq_put(&temp_msgq, &block, K_NO_WAIT);
        }

    }

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

