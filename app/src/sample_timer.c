#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <math.h>

#include <fsl_ctimer.h>
#include <fsl_clock.h>
#include <fsl_reset.h>

#include "debug_leds.h"
#include "temperature.h"

//LOG_MODULE_REGISTER(sample_timer, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(sample_timer, LOG_LEVEL_DBG);

/* The Zephyr Driver is drivers/counter/counter_mcux_ctimer.c */


#define TIMER DT_NODELABEL(ctimer0)

#define SW_TRIGGER_ADC
#ifdef SW_TRIGGER_ADC
void timer_callback(const struct device *dev, void *user_data)
{
    debug_led_toggle(1);
//    LOG_DBG("Timer ISR");
//    trigger_temperature_sample();
}
#endif

#ifndef SW_TRIGGER_ADC
/* This is taken from mcux_lpc_ctimer_set_top_value() */
/* It must be called after counter_set_top_value()*/
void setup_hardware_output(struct counter_top_cfg *top_cfg) {
    ctimer_match_config_t match_config = { .matchValue = (*top_cfg).ticks,
            .enableCounterReset = true,
            .enableCounterStop = false,
            .outControl = kCTIMER_Output_NoAction,
            .outPinInitState = false,
            .enableInterrupt = true };

    /* This bit is added to enable hardware O/P*/
    match_config.outControl = kCTIMER_Output_Toggle;

    CTIMER_SetupMatch(CTIMER0, 3, &match_config);
}
#endif

int init_sample_clock(void)
{
    const struct device *const counter_dev = DEVICE_DT_GET(TIMER);

    if (!device_is_ready(counter_dev)) {
        LOG_ERR("timer device not ready.");
        return -ENODEV;
    }

    struct counter_top_cfg top_cfg;
    memset(&top_cfg, 0, sizeof(top_cfg));

    /* This timer toggles O/P, ADC triggers on rising edge so run double speed (half period)*/
    uint32_t timer_ticks_per_half = counter_us_to_ticks(counter_dev, CONFIG_NASAL_TEMP_SAMPLE_PERIOD)/2;
    top_cfg.ticks =  timer_ticks_per_half;
#ifdef SW_TRIGGER_ADC
    top_cfg.callback = timer_callback;
#else
    top_cfg.callback = NULL;
#endif
    int err = counter_set_top_value(counter_dev, &top_cfg);
    if (err != 0) {
        LOG_ERR("Failed to set timer top value: %d", err);
        return err;
    }
#ifndef SW_TRIGGER_ADC
    setup_hardware_output(&top_cfg);
#endif
    err = counter_start(counter_dev);
    if (err != 0) {
        LOG_ERR("Failed to start timer: %d", err);
        return err;
    }

    return 0;
}
