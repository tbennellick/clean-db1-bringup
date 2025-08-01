#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <fsl_lpadc.h>
#include <fsl_inputmux.h>

#include "temperature.h"
#include "debug_leds.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */

/* Direct reference to the single ADC channel */
static const struct adc_dt_spec temp_adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

/* Todo: Unglobal?*/
static int16_t m_sample_buffer;
struct adc_sequence_options options;
struct adc_sequence sequence;


//typedef struct {
//    const struct device *adc_dev;
//    struct adc_sequence adc_seq;
//} temp_timer_ctx_t;
//
//static struct k_timer temp_timer;
//static temp_timer_ctx_t timer_ctx;
//
///* Sample aggregation variables */
//static temp_block_t current_block;
//static uint16_t sample_count = 0;

static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

enum adc_action adc_callback(const struct device *dev,
                                                 const struct adc_sequence *sequence,
                                                 uint16_t sampling_index)
{
    static int count = 0;
    static int64_t avg=0;
    printk("ADC ISR");
    if(count > 100)
    {
        debug_led_toggle(2);
        LOG_INF("ADC Callback: Sample %d, Avg: %lld", count, avg);
        count = 0;
        avg = 0;
    }else
    {
        count++;
    }
    return ADC_ACTION_REPEAT;
//    return ADC_ACTION_FINISH; //ADC_ACTION_CONTINUE;
//    return ADC_ACTION_REPEAT;
}

__maybe_unused
void external_trigger_test(void)
{    while (1)
    {
        LPADC_DoSoftwareTrigger(ADC0, 1);
        k_sleep(K_MSEC(10));
    }
}

__maybe_unused
void test__INPUTMUX_AttachSignal(INPUTMUX_Type *base, uint32_t index, inputmux_connection_t connection) {
    uint32_t pmux_id;
    uint32_t output_id;

    /* extract pmux to be used */
    pmux_id = ((uint32_t) (connection)) >> PMUX_SHIFT;
    /*  extract function number */
    output_id = ((uint32_t) (connection)) & ((1UL << PMUX_SHIFT) - 1U);
    /* programm signal */

    //*(volatile uint32_t *)(((uint32_t)base) + pmux_id + (index * 4U)) = output_id;
    LOG_DBG(" pointer %x", (((uint32_t) base) + pmux_id + (index * 4U)));
    LOG_DBG(" pointer no base %x", pmux_id + (index * 4U));
    LOG_DBG(" INdex  %d  PMUX %x, output_id %x", index, pmux_id, output_id);
}

//void trigger_temperature_sample(void)
//{
//
//    int ret = adc_read_async(temp_adc_channel.dev, &sequence, NULL);
//    if (ret < 0)
//    {
//        LOG_ERR("ADC async read failed: %d", ret);
//        return ret;
//    }
//
//
//}


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

    options.extra_samplings = 0;
    options.interval_us = 800000;
    options.callback = adc_callback;
    
    sequence.buffer = &m_sample_buffer;
    sequence.buffer_size = sizeof(m_sample_buffer);
    sequence.options = &options;



    ret = adc_sequence_init_dt(&temp_adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }

//    LOG_WRN("TEst");
//    k_sleep(K_MSEC(10));
//    debug_led_toggle(3);
//
//    trigger_temperature_sample();

    ret = adc_read_async(temp_adc_channel.dev, &sequence, NULL);
    if (ret < 0)
    {
        LOG_ERR("ADC async read failed: %d", ret);
        return ret;
    }


//    /* The code below is based on mcux_lpadc_start_channel() in adc_mcux_lpadc.c */
//    uint8_t first_channel = 3; /* Found by inspection with debugger.*/
//
//    lpadc_conv_trigger_config_t trigger_config;
//    LOG_DBG("Starting channel %d", first_channel);
//    LPADC_GetDefaultConvTriggerConfig(&trigger_config);
//    trigger_config.targetCommandId = first_channel + 1;
//
//    /* This bit is extra to the normal driver code */
//    trigger_config.enableHardwareTrigger = true;
//
//    LPADC_SetConvTriggerConfig(ADC0, 0, &trigger_config);
//
//    INPUTMUX_Init(INPUTMUX);
//    INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
//
//    /* DS26.1: Once the input multiplexer is configured, disable the clock to
//     * the INPUTMUX module in the AHBCLKCTRL register.*/
//    INPUTMUX_Deinit(INPUTMUX);
//
//    test__INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
//
////    external_trigger_test();
//


    return 0;

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

