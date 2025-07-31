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

//LOG_MODULE_REGISTER(sample_timer, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(sample_timer, LOG_LEVEL_DBG);

/* The Zephyr Driver is drivers/counter/counter_mcux_ctimer.c */
/* However, we are not really using it here as it does not do what we need */
/* There is maybe an avenue with: */
/* 	.set_top_value = mcux_lpc_ctimer_set_top_value, */


#define TEMP_SAMPLE_INTERVAL_MS 10  /* 100Hz = 10ms interval */
#define TEMP_MSGQ_SIZE 32  /* Number of blocks that can be queued */

struct counter_alarm_cfg alarm_cfg;
#define TIMER_PERIOD_US 800000 //10000
#define ALARM_CHANNEL_ID 3
#define TIMER DT_NODELABEL(ctimer0)


void test_counter_interrupt_fn(const struct device *counter_dev,
                                      uint8_t chan_id, uint32_t ticks,
                                      void *user_data)
{
    struct counter_alarm_cfg *config = user_data;
    uint32_t now_ticks;
    int err;

    err = counter_get_value(counter_dev, &now_ticks);
    if (err) {
        printk("Failed to read counter value (err %d)", err);
        return;
    }

    if (!counter_is_counting_up(counter_dev)) {
        now_ticks = counter_get_top_value(counter_dev) - now_ticks;
    }


//    static uint64_t count=0;
//    count++;
//    if((count % 100) == 0) {
//        printk("Counter ticks: %u, count: %llu\n", now_ticks, count);
//    }

    //    uint64_t now_usec;
//    int now_sec;
//    now_usec = counter_ticks_to_us(counter_dev, now_ticks);
//    now_sec = (int)(now_usec / USEC_PER_SEC);

    /* Ideally this would be achieved by setting matchConfig.enableCounterReset = true; */
    /* However the Zephyr driver does not support this yet.*/
    err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
                                    user_data);
    if (err != 0) {
        printk("Alarm could not be set\n");
    }
}

void timer_callback(const struct device *dev, void *user_data)
{
    debug_led_toggle(1);
    LOG_DBG("Timer ISR");
}


//__unused
//static void setup_timer_100hz_hal_direct(void)
//{
//    ctimer_config_t config;
//    ctimer_match_config_t matchConfig;
//    uint32_t srcClock_Hz;
//
//    /*TODO: Get CTIMER clock frequency - assuming 150MHz from PLL0 */
//    srcClock_Hz = 150000000U;
//
//    CTIMER_GetDefaultConfig(&config);
//    CTIMER_Init(CTIMER0, &config);
//
//    /* Configure Match 3 for 100Hz trigger (10ms period) */
//    /* Match 3 corresponds with an option on INPUTMUX device */
//    matchConfig.enableCounterReset = true;
//    matchConfig.enableCounterStop = false;
//    matchConfig.matchValue = srcClock_Hz / 100; /* 100Hz = 1,500,000 cycles at 150MHz */
//    matchConfig.outControl = kCTIMER_Output_Toggle;
//    matchConfig.outPinInitState = kCTIMER_Output_Set;
//    matchConfig.enableInterrupt = true;
//
//
//    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_3, &matchConfig);
//}

int init_sample_clock(void)
{
    const struct device *const counter_dev = DEVICE_DT_GET(TIMER);

    if (!device_is_ready(counter_dev)) {
        LOG_ERR("timer device not ready.");
        return -ENODEV;
    }

    struct counter_top_cfg top_cfg;
    top_cfg.ticks = counter_us_to_ticks(counter_dev, TIMER_PERIOD_US);
    top_cfg.callback = timer_callback;
    top_cfg.user_data = NULL;
//    top_cfg.flags = 0;
    int err = counter_set_top_value(counter_dev, &top_cfg);
    if (err != 0) {
        LOG_ERR("Failed to set timer top value: %d", err);
        return err;
    }

    /* THis is taken from mcux_lpc_ctimer_set_top_value() */
    ctimer_match_config_t match_config = { .matchValue = top_cfg.ticks,
            .enableCounterReset = true,
            .enableCounterStop = false,
            .outControl = kCTIMER_Output_NoAction,
            .outPinInitState = false,
            .enableInterrupt = true };

    /* This bit is added to enable hardware O/P*/
    match_config.outControl = kCTIMER_Output_Toggle;

    CTIMER_SetupMatch(CTIMER0, 3, &match_config);


    counter_start(counter_dev);
//    alarm_cfg.flags = 0;
//    alarm_cfg.ticks = counter_us_to_ticks(counter_dev, TIMER_PERIOD_US);
//    alarm_cfg.callback = test_counter_interrupt_fn;
//    alarm_cfg.user_data = &alarm_cfg;
//
//    int err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
//                                        &alarm_cfg);
//    if (err != 0) {
//        LOG_ERR("Failed to set timer alarm: %d", err);
//        return err;
//    }

/* 	.set_top_value = mcux_lpc_ctimer_set_top_value, */

    return 0;
}
