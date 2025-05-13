#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pressure, LOG_LEVEL_DBG);


//static inline float out_ev(struct sensor_value *val)
//{
//	return (val->val1 + (float)val->val2 / 1000000);
//}
//
//static int set_sampling_freq(const struct device *dev)
//{
//	int ret = 0;
//	struct sensor_value odr_attr;
//
//	odr_attr.val1 = 12.5; 	/* set accel/gyro sampling frequency to 12.5 Hz */
//	odr_attr.val2 = 0;
//
//	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
//	if (ret != 0) {
//		printf("Cannot set sampling frequency for accelerometer.\n");
//		return ret;
//	}
//
//	ret = sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
//	if (ret != 0) {
//		printf("Cannot set sampling frequency for gyro.\n");
//		return ret;
//	}
//
//	return 0;
//}



void init_pressure(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(honeywell_abp2s);

	if (!device_is_ready(dev)) {
		printk("%s: device not ready.\n", dev->name);
	}

//    if (set_sampling_freq(dev) != 0) {
//        return;
//    }

    struct sensor_value p;

        sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
        sensor_channel_get(dev, SENSOR_CHAN_PRESS, &p);

        LOG_INF("pressure: %d.%06d\n", p.val1, p.val2);


}
