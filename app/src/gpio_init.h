#pragma once
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

void safe_init_gpio(const struct gpio_dt_spec *spec, int flags);