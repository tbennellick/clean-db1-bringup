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
/* Driver is drivers/adc/adc_mcux_lpadc.c */

#define AVERAGE_WINDOW 100

/* ADC configuration context */
typedef struct {
    int64_t avg;
    uint16_t sample_count;
} temp_adc_ctx_t;

static temp_adc_ctx_t adc_ctx;

static void lpadc_hardware_trigger_isr(const void *arg)
{
    uint32_t status;
    lpadc_conv_result_t conv_result;
    int ret;
    
    status = LPADC_GetStatusFlags(ADC0);
    if (status & (uint32_t)kLPADC_TriggerCompletionFlag) {
        if (LPADC_GetConvResult(ADC0, &conv_result, 0U))
        {
            adc_ctx.avg  += conv_result.convValue;
            adc_ctx.sample_count++;
            if (adc_ctx.sample_count >= AVERAGE_WINDOW) {
                adc_ctx.avg /= AVERAGE_WINDOW;
                adc_ctx.sample_count = 0;
                LOG_INF("Temperature ADC average: %d", adc_ctx.avg);
            }
        }else {
            LOG_ERR("Failed to get ADC conversion result");
        }
        debug_led_toggle(1);
        LPADC_ClearStatusFlags(ADC0, (uint32_t)kLPADC_TriggerCompletionFlag);
    }
}

//static void attach_timer_adc_trigger(void)
//{
//    CLOCK_EnableClock(kCLOCK_InputMux);
//
//    /* Route CTIMER0 Match 3 to ADC0 Trigger via INPUTMUX */
////    INPUTMUX_AttachSignal(INPUTMUX, kINPUTMUX_Ctimer0M3ToAdc0Trigger, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
//}

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

    /* Perform auto-calibration */
    LPADC_DoAutoCalibration(ADC0);


    /* Configure trigger for hardware triggering */
    LPADC_GetDefaultConvTriggerConfig(&trigger_config);
    trigger_config.enableHardwareTrigger = true;
    trigger_config.targetCommandId = 1; /* Use command 1 */
    trigger_config.priority = 0; /* High priority */

    LPADC_SetConvTriggerConfig(ADC0, 0, &trigger_config);

    /* Configure conversion command for our channel */
    LPADC_GetDefaultConvCommandConfig(&cmd_config);
    cmd_config.channelNumber = 0; /* Channel 0 from device tree */
    cmd_config.sampleChannelMode = kLPADC_SampleChannelSingleEndSideA;
    cmd_config.sampleTimeMode = kLPADC_SampleTimeADCK3;
    cmd_config.hardwareAverageMode = kLPADC_HardwareAverageCount1;
    cmd_config.conversionResolutionMode = kLPADC_ConversionResolutionStandard;

    LPADC_SetConvCommandConfig(ADC0, 1, &cmd_config);
    LPADC_EnableInterrupts(ADC0, (uint32_t)kLPADC_Trigger0CompletionInterruptEnable);

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(lpadc0)), DT_IRQ(DT_NODELABEL(lpadc0), priority), lpadc_hardware_trigger_isr, NULL, 0);
    irq_enable(DT_IRQN(DT_NODELABEL(lpadc0)));
}

int init_temperature(void)
{
    int ret;


    adc_ctx.sample_count = 0;
    adc_ctx.avg = 0;

    setup_adc();
//    attach_timer_adc_trigger();
    return 0;
}

