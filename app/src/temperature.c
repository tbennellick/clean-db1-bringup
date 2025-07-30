#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <math.h>

#include <fsl_ctimer.h>
#include <fsl_lpadc.h>
#include <fsl_inputmux.h>
#include <fsl_clock.h>
#include <fsl_reset.h>
#include <zephyr/drivers/adc.h>

#include "temperature.h"
#include "debug_leds.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */




/* ADC configuration context */
typedef struct {
    temp_block_t current_block;
    uint16_t sample_count;
} temp_adc_ctx_t;

static temp_adc_ctx_t adc_ctx;
static struct k_msgq temp_msgq;
static char __aligned(4) temp_msgq_buffer[TEMP_MSGQ_SIZE * sizeof(temp_block_t)];

/* LPADC interrupt handler for hardware-triggered ADC conversions */
static void lpadc_hardware_trigger_isr(const void *arg)
{
    uint32_t status;
    lpadc_conv_result_t conv_result;
    int ret;
    
    /* Check for trigger completion flag */
    status = LPADC_GetStatusFlags(ADC0);
    if (status & (uint32_t)kLPADC_TriggerCompletionFlag) {
        /* Read conversion result */
        if (LPADC_GetConvResult(ADC0, &conv_result, 0U)) {

//            LOG_HEXDUMP_DBG(&conv_result.convValue, sizeof(conv_result.convValue), "ADC hw trigger result");
//
//            /* Store sample in current block */
//            adc_ctx.current_block.samples[adc_ctx.sample_count] = (int16_t)conv_result.convValue;
//            adc_ctx.sample_count++;
//
//            /* If block is full, send it to message queue */
//            if (adc_ctx.sample_count >= TEMP_BLOCK_SIZE) {
//                adc_ctx.current_block.count = adc_ctx.sample_count;
//                adc_ctx.current_block.timestamp_ms = k_uptime_get_32();
//
//                ret = k_msgq_put(&temp_msgq, &adc_ctx.current_block, K_NO_WAIT);
//                if (ret != 0) {
//                    LOG_WRN("Temperature message queue full, dropping block");
//                }
//
//                adc_ctx.sample_count = 0;
//            }
//

        }
        
        /* Clear the trigger completion flag */
        LPADC_ClearStatusFlags(ADC0, (uint32_t)kLPADC_TriggerCompletionFlag);
    }
}

static void attach_timer_adc_trigger(void)
{
    CLOCK_EnableClock(kCLOCK_InputMux);
    
    /* Route CTIMER0 Match 3 to ADC0 Trigger via INPUTMUX */
//    INPUTMUX_AttachSignal(INPUTMUX, kINPUTMUX_Ctimer0M3ToAdc0Trigger, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
}

static int16_t m_sample_buffer;
int zephyr_adc_setup(void)
{
    struct adc_dt_spec temp_adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
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

    struct adc_sequence sequence = {
            .buffer = &m_sample_buffer,
            .buffer_size = sizeof(m_sample_buffer),
    };

    memset(&m_sample_buffer, 0xaa, sizeof(m_sample_buffer));

    ret = adc_sequence_init_dt(&temp_adc_channel, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC sequence initialization failed: %d", ret);
        return ret;
    }
    return 0;
}

void setup_adc(void)
{
    lpadc_config_t lpadc_config;
    lpadc_conv_trigger_config_t trigger_config;
    lpadc_conv_command_config_t cmd_config;

    LPADC_GetDefaultConfig(&lpadc_config);
    lpadc_config.enableAnalogPreliminary = true;
    lpadc_config.referenceVoltageSource = kLPADC_ReferenceVoltageAlt2;
    lpadc_config.powerLevelMode = kLPADC_PowerLevelAlt1;
    lpadc_config.triggerPriorityPolicy = kLPADC_TriggerPriorityPreemptImmediately;
    
    LPADC_Init(ADC0, &lpadc_config);
    
//    /* Configure trigger for hardware triggering */
//    LPADC_GetDefaultConvTriggerConfig(&trigger_config);
//    trigger_config.enableHardwareTrigger = true;
//    trigger_config.targetCommandId = 1; /* Use command 1 */
//    trigger_config.priority = 0; /* High priority */
//
//    LPADC_SetConvTriggerConfig(ADC0, 0, &trigger_config);
//
//    /* Configure conversion command for our channel */
//    LPADC_GetDefaultConvCommandConfig(&cmd_config);
//    cmd_config.channelNumber = 0; /* Channel 0 from device tree */
//    cmd_config.sampleChannelMode = kLPADC_SampleChannelSingleEndSideA;
//    cmd_config.sampleTimeMode = kLPADC_SampleTimeADCK3;
//    cmd_config.hardwareAverageMode = kLPADC_HardwareAverageCount1;
//    cmd_config.conversionResolutionMode = kLPADC_ConversionResolutionStandard;
//
//    LPADC_SetConvCommandConfig(ADC0, 1, &cmd_config);
//
//    /* Enable trigger completion interrupt */
//    LPADC_EnableInterrupts(ADC0, (uint32_t)kLPADC_Trigger0CompletionInterruptEnable);
//
//    /* Note: IRQ_CONNECT removed to avoid conflict with Zephyr driver */
//    /* IRQ_CONNECT(DT_IRQN(DT_NODELABEL(lpadc0)), DT_IRQ(DT_NODELABEL(lpadc0), priority),
//                lpadc_hardware_trigger_isr, NULL, 0); */
//    /* irq_enable(DT_IRQN(DT_NODELABEL(lpadc0))); */
//
//    /* Perform auto-calibration */
    LPADC_DoAutoCalibration(ADC0);
}

int init_temperature(void)
{
    int ret;

    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

    adc_ctx.sample_count = 0;
    adc_ctx.current_block.count = 0;
    adc_ctx.current_block.timestamp_ms = 0;

    setup_timer_100hz();
    zephyr_adc_setup();
    setup_adc();
    attach_timer_adc_trigger();
//    CTIMER_StartTimer(CTIMER0);
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

