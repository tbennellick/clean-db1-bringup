#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

#include "debug_leds.h"
#include "led_manager.h"
#include "modem.h"
#include "power.h"
#include "imu.h"
#include "rip.h"
#include "pressure.h"
//#include "exg.h"
#include "fuel_gauge.h"
#include "als.h"
#include "audio.h"
#include "display.h"
#include "temperature.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void) {
    LOG_INF("BFP2 Main core %s\n", APP_VERSION_STRING);
    init_debug_leds();
    debug_led_on();
    led_manager_init();
    led_manager_set(LED_MANAGER_COLOUR_OFF, LED_MANAGER_MODE_CONT);
    //    init_power(); Called early in kernel startup

    init_imu();
    init_rip();
    init_pressure();
//    init_exg();
    init_fuel_gauge();
//    init_modem();
    init_als();
//    init_audio();
    init_display();
    init_temperature();

    LOG_INF("Init complete");
    
    temp_block_t temp_block;
    while (1)
    {
        k_sleep(K_SECONDS(1));
        printk(".");
        debug_led_toggle(0);
        int ret = temperature_read_block(&temp_block, K_NO_WAIT);
        if (ret == 0) {
            LOG_INF("Temp block # %d, at %u ms", temp_block.count, temp_block.timestamp_ms);
//            LOG_HEXDUMP_WRN(temp_block.samples, sizeof(temp_block.samples), "Temp samples");
            LOG_HEXDUMP_WRN(temp_block.samples, 20, "Temp samples");
            }
    }
    return 0;
}

