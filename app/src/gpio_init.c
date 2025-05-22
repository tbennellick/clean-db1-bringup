#include "gpio_init.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_init, CONFIG_LOG_DEFAULT_LEVEL);

void safe_init_gpio(const struct gpio_dt_spec *spec, int flags)
{
    if (!gpio_is_ready_dt(spec))
    {
        LOG_ERR("Gpio not ready %s %d",spec->port->name, spec->pin);
    }
    else
    {
        int res = gpio_pin_configure_dt(spec, flags);
        if (res !=0)
        {
            LOG_ERR("Cant init %s %d",spec->port->name, spec->pin);
        }
    }
}
