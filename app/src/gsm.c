#include "gsm.h"
#include "debug_leds.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(gsm,CONFIG_LOG_DEFAULT_LEVEL);

#define PIN_LTE_RESET DT_ALIAS(gsm_reset)
static const struct gpio_dt_spec pin_lte_reset = GPIO_DT_SPEC_GET(PIN_LTE_RESET, gpios);

#define PIN_LTE_N_POWER_ENABLE DT_ALIAS(gsm_n_power_enable)
static const struct gpio_dt_spec pin_lte_n_power_on = GPIO_DT_SPEC_GET(PIN_LTE_N_POWER_ENABLE, gpios);

void gsm_init(void) 
{
    if (!gpio_is_ready_dt(&pin_lte_reset)) {k_fatal_halt(-100);}
    if (!gpio_is_ready_dt(&pin_lte_n_power_on)) {k_fatal_halt(-100);}

    if ( 0 != gpio_pin_configure_dt(&pin_lte_reset, GPIO_OUTPUT_ACTIVE)) {k_fatal_halt(-101);}
    if ( 0 != gpio_pin_configure_dt(&pin_lte_n_power_on, GPIO_OUTPUT_ACTIVE)) {k_fatal_halt(-101);}
}

void gsm_resetModule() 
{
	// Put module into RESET
    gpio_pin_set_dt(&pin_lte_reset, 0);
    gpio_pin_set_dt(&pin_lte_n_power_on, 0);
	debug_led_set(1, 1);
	k_sleep(K_MSEC(100));
	
	// Release module from RESET
	gpio_pin_set_dt(&pin_lte_reset, 1);
    gpio_pin_set_dt(&pin_lte_n_power_on, 1);
	debug_led_set(1, 0);
}

void gsm_enablePower(bool enable_power)
{
	gpio_pin_set_dt(&pin_lte_n_power_on, (int)enable_power);
}
