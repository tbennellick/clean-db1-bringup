#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "temperature.h"
#include "debug_leds.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(nt, LOG_LEVEL_DBG);

#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */
#define TEMP_THREAD_STACK_SIZE 1024
#define TEMP_THREAD_PRIORITY 5

static const struct adc_dt_spec temp_adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

K_MSGQ_DEFINE(temp_msgq, sizeof(temp_block_t), TEMP_MSGQ_SIZE, 4);
temp_block_t block;


static K_THREAD_STACK_DEFINE(temp_thread_stack, TEMP_THREAD_STACK_SIZE);
static struct k_thread temp_thread_data;

static void nt_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_DBG("Temperature sampling thread started");

    int ret;
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
        return;
    }

    uint8_t sample_count = 0;
    block.count = 0;

    while(1) {
        sequence.buffer = &block.samples[sample_count];
        sequence.buffer_size = sizeof(int16_t);
        ret = adc_read(temp_adc_channel.dev, &sequence);
        if (ret < 0) {
            LOG_ERR("ADC read failed: %d", ret);
            continue;
        }
        
        sample_count++;
        if(sample_count >= CONFIG_NASAL_TEMP_BLOCK_SIZE) {
            sample_count = 0;
            block.timestamp_ms = k_uptime_get_32();
            block.count++;
            ret = k_msgq_put(&temp_msgq, &block, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("Failed to put temperature block in message queue: %d", ret);
                continue;
            }
        }
//        LOG_DBG("Temperature sample %d: %d", block.count, block.samples[sample_count*2]);
        k_sleep(K_MSEC(CONFIG_NASAL_TEMP_SAMPLE_PERIOD_MS)); /* Small delay between samples */
    }
}

int init_temperature(void)
{
    int ret;

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

    /* Create and start the temperature sampling thread */
    k_thread_create(&temp_thread_data, temp_thread_stack,
                    K_THREAD_STACK_SIZEOF(temp_thread_stack),
                    nt_thread, NULL, NULL, NULL,
                    TEMP_THREAD_PRIORITY, 0, K_NO_WAIT);
    
    k_thread_name_set(&temp_thread_data, "nt_thr");
    
    return 0;
}

int temperature_read_block(temp_block_t *block, k_timeout_t timeout)
{
    if (block == NULL) {
        return -EINVAL;
    }
    return k_msgq_get(&temp_msgq, block, timeout);
 }

