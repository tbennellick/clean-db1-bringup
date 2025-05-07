#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#include "debug_leds.h"
#include "led_manager.h"
#include "power.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);
    init_debug_leds();
    debug_led_on();
    led_manager_init();
    led_manager_set(LED_MANAGER_COLOUR_RED, LED_MANAGER_MODE_CONT);
    k_sleep(K_MSEC(100));
    led_manager_set(LED_MANAGER_COLOUR_GREEN, LED_MANAGER_MODE_CONT);
    k_sleep(K_MSEC(100));
    led_manager_set(LED_MANAGER_COLOUR_BLUE, LED_MANAGER_MODE_CONT);
    k_sleep(K_MSEC(100));
    led_manager_set(LED_MANAGER_COLOUR_WHITE, LED_MANAGER_MODE_CONT);
    k_sleep(K_MSEC(100));
    led_manager_set(LED_MANAGER_COLOUR_OFF, LED_MANAGER_MODE_CONT);

    init_power();

	while (1)
        {
            k_sleep(K_MSEC(2000));
            printk("Spam!....\n");
            debug_led_toggle();
        }
	return 0;
}

