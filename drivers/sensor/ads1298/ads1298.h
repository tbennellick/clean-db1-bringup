#ifndef ZEPHYR_DRIVERS_SENSOR_ADS1298_H_
#define ZEPHYR_DRIVERS_SENSOR_ADS1298_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

struct ads1298_data
{
    int32_t pressure;
    int32_t temperature;
};

struct ads1298_dev_config {
	struct spi_dt_spec bus;
};

#endif