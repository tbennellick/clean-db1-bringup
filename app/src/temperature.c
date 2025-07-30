#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <math.h>

/* NXP HAL includes for hardware trigger configuration */
#include <fsl_ctimer.h>
#include <fsl_lpadc.h>
#include <fsl_inputmux.h>
#include <fsl_clock.h>

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
            LOG_HEXDUMP_DBG(&conv_result.convValue, sizeof(conv_result.convValue), "ADC hw trigger result");
            
            /* Store sample in current block */
            adc_ctx.current_block.samples[adc_ctx.sample_count] = (int16_t)conv_result.convValue;
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
        }
        
        /* Clear the trigger completion flag */
        LPADC_ClearStatusFlags(ADC0, (uint32_t)kLPADC_TriggerCompletionFlag);
    }
}

/* Configure CTIMER0 for 100Hz output using HAL directly */
static int configure_ctimer_hardware_trigger(void)
{
    ctimer_config_t config;
    ctimer_match_config_t matchConfig;
    uint32_t srcClock_Hz;
    
    LOG_DBG("Configuring CTIMER0 for 100Hz hardware trigger");
    
    /* Get CTIMER clock frequency - assuming 150MHz from PLL0 */
    srcClock_Hz = 150000000U;
    
    /* Initialize CTIMER0 */
    CTIMER_GetDefaultConfig(&config);
    CTIMER_Init(CTIMER0, &config);
    
    /* Configure Match 3 for 100Hz trigger (10ms period) */
    matchConfig.enableCounterReset = true;
    matchConfig.enableCounterStop = false;
    matchConfig.matchValue = srcClock_Hz / 100; /* 100Hz = 1,500,000 cycles at 150MHz */
    matchConfig.outControl = kCTIMER_Output_Toggle;
    matchConfig.outPinInitState = kCTIMER_Output_Set;
    matchConfig.enableInterrupt = false;
    
    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_3, &matchConfig);
    
    return 0;
}

/* Configure INPUTMUX to route CTIMER0 output to LPADC0 trigger */
static int configure_inputmux_routing(void)
{
    LOG_DBG("Configuring INPUTMUX: CTIMER0 Match 3 -> LPADC0 Trigger");
    
    /* Enable INPUTMUX peripheral clock before accessing registers */
    CLOCK_EnableClock(kCLOCK_InputMux);
    
    /* Route CTIMER0 Match 3 to ADC0 Trigger via INPUTMUX */
    INPUTMUX_AttachSignal(INPUTMUX, kINPUTMUX_Ctimer0M3ToAdc0Trigger, kINPUTMUX_Ctimer0M3ToAdc0Trigger);
    
    return 0;
}

/* Initialize and configure LPADC for hardware trigger directly using HAL */
static int configure_lpadc_hardware_trigger(void)
{
    lpadc_config_t lpadc_config;
    lpadc_conv_trigger_config_t trigger_config;
    lpadc_conv_command_config_t cmd_config;
    
    LOG_DBG("Configuring LPADC for true hardware trigger");
    
    /* Initialize LPADC */
    LPADC_GetDefaultConfig(&lpadc_config);
    lpadc_config.enableAnalogPreliminary = true;
    lpadc_config.referenceVoltageSource = kLPADC_ReferenceVoltageAlt2;
    lpadc_config.powerLevelMode = kLPADC_PowerLevelAlt1;
    lpadc_config.triggerPriorityPolicy = kLPADC_TriggerPriorityPreemptImmediately;
    
    LPADC_Init(ADC0, &lpadc_config);
    
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
    
    /* Enable trigger completion interrupt */
    LPADC_EnableInterrupts(ADC0, (uint32_t)kLPADC_Trigger0CompletionInterruptEnable);
    
    /* Note: IRQ_CONNECT removed to avoid conflict with Zephyr driver */
    /* IRQ_CONNECT(DT_IRQN(DT_NODELABEL(lpadc0)), DT_IRQ(DT_NODELABEL(lpadc0), priority),
                lpadc_hardware_trigger_isr, NULL, 0); */
    /* irq_enable(DT_IRQN(DT_NODELABEL(lpadc0))); */
    
    /* Perform auto-calibration */
    LPADC_DoAutoCalibration(ADC0);
    
    return 0;
}

int init_temperature(void)
{
    int ret;

    LOG_DBG("Initializing temperature ADC with hardware timer trigger");
    
    /* Initialize message queue */
    k_msgq_init(&temp_msgq, temp_msgq_buffer, sizeof(temp_block_t), TEMP_MSGQ_SIZE);

    adc_ctx.sample_count = 0;
    adc_ctx.current_block.count = 0;
    adc_ctx.current_block.timestamp_ms = 0;

    /* Configure LPADC for hardware triggering */
    ret = configure_lpadc_hardware_trigger();
    if (ret < 0) {
        LOG_ERR("LPADC hardware trigger configuration failed: %d", ret);
        return ret;
    }
    
    /* Configure hardware timer for ADC triggering */
    ret = configure_ctimer_hardware_trigger();
    if (ret < 0) {
        LOG_ERR("CTIMER configuration failed: %d", ret);
        return ret;
    }
    
    ret = configure_inputmux_routing();
    if (ret < 0) {
        LOG_ERR("INPUTMUX configuration failed: %d", ret);
        return ret;
    }
    
    /* Start CTIMER0 to begin hardware triggering */
    CTIMER_StartTimer(CTIMER0);
    
    LOG_INF("Temperature ADC initialized with true hardware timer trigger at 100Hz");
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

