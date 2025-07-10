#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pressure, LOG_LEVEL_DBG);

void test_pressure(const struct device *dev) {
    struct sensor_value p;

    sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);

    sensor_channel_get(dev, SENSOR_CHAN_PRESS, &p);
    LOG_INF("pressure: %fmbar", (double)sensor_value_to_float(&p));

    sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &p);
    LOG_INF("temperature: %f°c", (double)sensor_value_to_float(&p));
}

void init_pressure(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(honeywell_abp2s);

	if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready\n", dev->name);
        k_sleep(K_MSEC(200));
        return;
    }

    test_pressure(dev);

}
