#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power, CONFIG_LOG_DEFAULT_LEVEL);

#include <stdio.h>
#include <zephyr/devicetree.h>
#include "power.h"

#define POWER_VEN_SYS_BASE DT_ALIAS(power_ven_sys_base)
static const struct gpio_dt_spec ven_sys_base = GPIO_DT_SPEC_GET(POWER_VEN_SYS_BASE, gpios);
#define POWER_VEN_STORAGE DT_ALIAS(power_ven_storage)
static const struct gpio_dt_spec ven_storage = GPIO_DT_SPEC_GET(POWER_VEN_STORAGE, gpios);
#define POWER_VEN_BLE DT_ALIAS(power_ven_ble)
static const struct gpio_dt_spec ven_ble = GPIO_DT_SPEC_GET(POWER_VEN_BLE, gpios);
#define POWER_VEN_SYS DT_ALIAS(power_ven_sys)
static const struct gpio_dt_spec ven_sys = GPIO_DT_SPEC_GET(POWER_VEN_SYS, gpios);
#define POWER_VEN_BAT DT_ALIAS(power_ven_bat)
static const struct gpio_dt_spec ven_bat = GPIO_DT_SPEC_GET(POWER_VEN_BAT, gpios);

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

void init_power(void)
{

    safe_init_gpio(&ven_sys_base, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_storage, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_ble, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_sys, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_bat, GPIO_OUTPUT_LOW);

    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&ven_sys_base, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&ven_storage, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&ven_ble, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&ven_sys, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&ven_bat, 1);
}
