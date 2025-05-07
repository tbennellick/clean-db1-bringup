#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#include "debug_leds.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);
    init_debug_leds();
    debug_led_on();

	while (1)
        {
            k_sleep(K_MSEC(2000));
            printk("Spam!....\n");
            debug_led_toggle();
        }
	return 0;
}

