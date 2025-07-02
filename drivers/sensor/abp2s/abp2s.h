#ifndef ZEPHYR_DRIVERS_SENSOR_ABP2S_H_
#define ZEPHYR_DRIVERS_SENSOR_ABP2S_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

struct abp2_data
{
    uint32_t pressure_counts;
    uint32_t temperature_counts;
};

struct abp2_dev_config {
	struct spi_dt_spec bus;
};

#endif