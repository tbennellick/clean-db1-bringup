#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <math.h>

/* NXP HAL includes for hardware trigger configuration */
#include <fsl_ctimer.h>
#include <fsl_lpadc.h>
#include <fsl_inputmux.h>
#include <fsl_clock.h>
#include <fsl_reset.h>

#include "temperature.h"

//LOG_MODULE_REGISTER(temperature, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(temperature, LOG_LEVEL_DBG);

#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
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

struct counter_alarm_cfg alarm_cfg;
#define DELAY 2000000
#define ALARM_CHANNEL_ID 0
#define TIMER DT_NODELABEL(ctimer0)

static void test_counter_interrupt_fn(const struct device *counter_dev,
                                      uint8_t chan_id, uint32_t ticks,
                                      void *user_data)
{
    struct counter_alarm_cfg *config = user_data;
    uint32_t now_ticks;
    uint64_t now_usec;
    int now_sec;
    int err;

    err = counter_get_value(counter_dev, &now_ticks);
    if (!counter_is_counting_up(counter_dev)) {
        now_ticks = counter_get_top_value(counter_dev) - now_ticks;
    }

    if (err) {
        printk("Failed to read counter value (err %d)", err);
        return;
    }

    now_usec = counter_ticks_to_us(counter_dev, now_ticks);
    now_sec = (int)(now_usec / USEC_PER_SEC);

    printk("!!! Alarm !!!\n");
    printk("Now: %u\n", now_sec);

    /* Set a new alarm with a double length duration */
    config->ticks = config->ticks * 2U;

    printk("Set alarm in %u sec (%u ticks)\n",
           (uint32_t)(counter_ticks_to_us(counter_dev,
                                          config->ticks) / USEC_PER_SEC),
           config->ticks);

    err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
                                    user_data);
    if (err != 0) {
        printk("Alarm could not be set\n");
    }
}


static void setup_timer_100hz(void)
{
    const struct device *const counter_dev = DEVICE_DT_GET(TIMER);

    if (!device_is_ready(counter_dev)) {
        printk("device not ready.\n");
        return;
    }

    counter_start(counter_dev);
    alarm_cfg.flags = 0;
    alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
    alarm_cfg.callback = test_counter_interrupt_fn;
    alarm_cfg.user_data = &alarm_cfg;

    int err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
                                    &alarm_cfg);
    printk("Set alarm in %u sec (%u ticks)\n",
           (uint32_t)(counter_ticks_to_us(counter_dev,
                                          alarm_cfg.ticks) / USEC_PER_SEC),
           alarm_cfg.ticks);

    if (-EINVAL == err) {
        printk("Alarm settings invalid\n");
    } else if (-ENOTSUP == err) {
        printk("Alarm setting request not supported\n");
    } else if (err != 0) {
        printk("Error\n");
    }

}
static void setup_timer_100hz_old(void)
{
    ctimer_config_t config;
    ctimer_match_config_t matchConfig;
    uint32_t srcClock_Hz;

    /*TODO: Get CTIMER clock frequency - assuming 150MHz from PLL0 */
    srcClock_Hz = 150000000U;
    
    CTIMER_GetDefaultConfig(&config);
    CTIMER_Init(CTIMER0, &config);
    
    /* Configure Match 3 for 100Hz trigger (10ms period) */
    matchConfig.enableCounterReset = true;
    matchConfig.enableCounterStop = false;
    matchConfig.matchValue = srcClock_Hz / 100; /* 100Hz = 1,500,000 cycles at 150MHz */
    matchConfig.outControl = kCTIMER_Output_Toggle;
    matchConfig.outPinInitState = kCTIMER_Output_Set;
    matchConfig.enableInterrupt = true;

    
    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_3, &matchConfig);
}

static void attach_timer_adc_trigger(void)
{
    CLOCK_EnableClock(kCLOCK_InputMux);
    
    /* Route CTIMER0 Match 3 to ADC0 Trigger via INPUTMUX */
//    INPUTMUX_AttachSignal(INPUTMUX, kINPUTMUX_Ctimer0M3ToAdc0Trigger, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
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

    setup_adc();
    setup_timer_100hz();
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

