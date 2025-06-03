#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#include "debug_leds.h"
#include "led_manager.h"
#include "power.h"
#include "imu.h"
#include "rip.h"
#include "pressure.h"

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

    //    init_power(); Called early in kernel startup

    init_imu();

//    init_rip();

    clock_ip_name_t clk;
    for(uint8_t i=0; i<10; i++)
    {
        LOG_WRN("FC %d, %x", i , SYSCON->FCCLKSEL[i]);
//        clk = CLOCK_EnableClock(s_lpflexcommClocks[i]);

    }
    LOG_WRN("");
    LOG_WRN("FRO DIV %d",SYSCON->FROHFDIV);
//    LOG_WRN("SCG SGS %d", );
/* Instance is just the nuber eg 1*/
//     RESET_ClearPeripheralReset(s_lpflexcommResets[instance]);
//    LP_FLEXCOMM_SetPeriph(instance, periph, 0);
    init_pressure();
    printk("Spam!....\n");

	while (1)
        {
            k_sleep(K_MSEC(2000));
            printk(".");
            debug_led_toggle();
        }
	return 0;
}

