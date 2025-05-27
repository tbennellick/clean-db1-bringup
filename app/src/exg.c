#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "gpio_init.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(exg, LOG_LEVEL_DBG);



void init_exg(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(ti_ads1298);

	if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready\n", dev->name);
        k_sleep(K_MSEC(200));
    }
    LOG_WRN("Device ready: %s\n", dev->name);
    k_sleep(K_MSEC(200));

    struct sensor_value p;
    while (1)
    {

        sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);

        sensor_channel_get(dev, SENSOR_CHAN_PRESS, &p);

        LOG_INF("pressure: %d.%06d\n", p.val1, p.val2);
        k_sleep(K_MSEC(500));
    }

}
