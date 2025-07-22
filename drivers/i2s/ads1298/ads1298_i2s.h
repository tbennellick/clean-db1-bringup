#pragma once

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

struct ads1298_i2s_config {
	struct spi_dt_spec spi_bus;
	struct gpio_dt_spec clksel_gpio;
	struct gpio_dt_spec nrst_gpio;
	struct gpio_dt_spec npwdn_gpio;
	struct gpio_dt_spec start_conv_gpio;
	struct gpio_dt_spec drdy_gpio;
	uint32_t sample_rate;
	uint8_t channels;
};

struct ads1298_i2s_data {
	struct k_mem_slab *mem_slab;
	struct k_msgq *data_queue;
    void * rx_in_msgs[CONFIG_EXG_RX_SAMPLE_COUNT];
    uint32_t timeout;
	bool running;
	bool read_busy;
	uint8_t current_config1;
	struct gpio_callback drdy_cb;
	struct k_work_delayable get_samples_work;
	const struct device *dev;
};

