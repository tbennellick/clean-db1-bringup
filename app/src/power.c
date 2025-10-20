#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power, CONFIG_LOG_DEFAULT_LEVEL);

#include <stdio.h>
#include <zephyr/devicetree.h>
#include "power.h"
#include "gpio_init.h"

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
#define DISP_BL DT_ALIAS(backlight)
static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(DISP_BL, gpios);


/* TODO */
/* Power up should be handled by the power susbsytem, that way it is automatic with sleep. */
void early_power_up_bodge(void)
{
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&ven_sys_base, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&ven_storage, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&ven_ble, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&ven_sys, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&ven_bat, 1);
}

void power_all(bool enable)
{
    gpio_pin_set_dt(&ven_sys_base, enable);
    gpio_pin_set_dt(&ven_storage, enable);
    gpio_pin_set_dt(&ven_ble, enable);
    gpio_pin_set_dt(&ven_sys, enable);
    gpio_pin_set_dt(&ven_bat, enable);
    if (enable) {
        LOG_INF("Powering up all rails");
    } else {
        LOG_INF("Powering down all rails");
    }
}

void init_power(void)
{

    safe_init_gpio(&ven_sys_base, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_storage, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_ble, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_sys, GPIO_OUTPUT_LOW);
    safe_init_gpio(&ven_bat, GPIO_OUTPUT_LOW);

}

void backlight_on(void)
{
    safe_init_gpio(&backlight, GPIO_OUTPUT_HIGH);
}


static int auto_early_power_up(void) {
    init_power();
    early_power_up_bodge();

    // power_all(true);
    return 0;
}

SYS_INIT(auto_early_power_up, POST_KERNEL, 50);
