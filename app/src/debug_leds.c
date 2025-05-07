#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
const struct gpio_dt_spec debug_led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
const struct gpio_dt_spec debug_led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define LED2_NODE DT_ALIAS(led2)
const struct gpio_dt_spec debug_led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LED3_NODE DT_ALIAS(led3)
const struct gpio_dt_spec debug_led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

//No logging in this module so that it cannot block an led action

void debug_led_set(uint8_t led, uint8_t state)
{
    switch (led)
    {
        case 0:
            gpio_pin_set_dt(&debug_led0, state);
            break;
        case 1:
            gpio_pin_set_dt(&debug_led1, state);
            break;
        case 2:
            gpio_pin_set_dt(&debug_led2, state);
            break;
        case 3:
            gpio_pin_set_dt(&debug_led3, state);
            break;
        default:
            break;
    }
}

void debug_led_on(void)
{
    gpio_pin_set_dt(&debug_led0, 1);
}

void debug_led_off(void)
{
    gpio_pin_set_dt(&debug_led0, 0);
}

void debug_led_toggle(void)
{
    gpio_pin_toggle_dt(&debug_led0);
}

void debug_led_strobe(void)
{
    debug_led_on();
    k_msleep(1);
    debug_led_off();

}


void init_debug_leds(void)
{
    if (!gpio_is_ready_dt(&debug_led0)) {
        k_fatal_halt(-100);
	}
    if (!gpio_is_ready_dt(&debug_led1)) {
        k_fatal_halt(-100);
    }
    if (!gpio_is_ready_dt(&debug_led2)) {
        k_fatal_halt(-100);
    }
    if (!gpio_is_ready_dt(&debug_led3)) {
        k_fatal_halt(-100);
    }

    if ( 0 != gpio_pin_configure_dt(&debug_led0, GPIO_OUTPUT_ACTIVE)) {
        k_fatal_halt(-101);
    }
    if ( 0 != gpio_pin_configure_dt(&debug_led1, GPIO_OUTPUT_ACTIVE)) {
        k_fatal_halt(-102);
    }
    if ( 0 != gpio_pin_configure_dt(&debug_led2, GPIO_OUTPUT_ACTIVE)) {
        k_fatal_halt(-101);
    }
    if ( 0 != gpio_pin_configure_dt(&debug_led3, GPIO_OUTPUT_ACTIVE)) {
        k_fatal_halt(-101);
    }

    debug_led_set(0, 0);
    debug_led_set(1, 0);
    debug_led_set(2, 0);
    debug_led_set(3, 0);
}
