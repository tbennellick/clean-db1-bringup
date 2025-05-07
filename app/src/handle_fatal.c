#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

//#include "led_manager.h"


LOG_MODULE_DECLARE(fatal, CONFIG_KERNEL_LOG_LEVEL);


/* From kernel/fatal.c */
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
    ARG_UNUSED(esf);

    LOG_PANIC();

    printk("Fatal error with reason %d\n", reason);
    printk("Rebooting in 10 seconds\n");

    // Flash the LED in a distinctive pattern by cycling through all colours at a high rate
//    for (led_manager_colour_t i = 0; i < 100; i++) {
//        led_manager_set(i, LED_MANAGER_MODE_CONT);
//        k_busy_wait(100000);
//    }

    for (uint32_t i = 0; i < 100; i++) {
        k_busy_wait(100000);
    }

    sys_reboot(SYS_REBOOT_COLD);


    CODE_UNREACHABLE;
}

//void fataL_test(void)
//{
//    k_sleep(K_SECONDS(2));
//    k_msgq_put(0, &msgq,K_NO_WAIT);
//}