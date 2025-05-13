#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rip, CONFIG_APP_LOG_LEVEL);

#define RIPEN DT_ALIAS(rip_en)
const struct gpio_dt_spec rip_en = GPIO_DT_SPEC_GET(RIPEN, gpios);


void init_rip(void)
{
    if (!gpio_is_ready_dt(&rip_en)) {
        LOG_ERR("RIP EN GPIO not available");
	}

    if ( 0 != gpio_pin_configure_dt(&rip_en, GPIO_OUTPUT_ACTIVE)) {
        k_fatal_halt(-101);
    }

    gpio_pin_set_dt(&rip_en, 1);
}
