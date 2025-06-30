#include "als.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(als, LOG_LEVEL_INF);

int init_als(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(vishay_veml6030);
    struct sensor_value val;

    if (!device_is_ready(dev)) {
        printk("sensor: ALS not ready.\n");
        return 0;
    }

    if (sensor_sample_fetch(dev) < 0) {
        LOG_ERR("Sensor sample update error");
        return -EIO;
    }

    if (sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &val) < 0) {
        printf("Cannot read VEML6030 value\n");
        return 0;
    }

    LOG_INF("ALS reading: %g", sensor_value_to_double(&val));

    return 0;
}
