#define DT_DRV_COMPAT vishay_veml6030

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include "veml6030.h"

LOG_MODULE_REGISTER(VEML6030, CONFIG_SENSOR_LOG_LEVEL);

#define VEML6030_CMDCODE_ALS_CONF 0x00
#define VEML6030_CMDCODE_ALS_WH   0x01
#define VEML6030_CMDCODE_ALS_WL   0x02
#define VEML6030_CMDCODE_PSM      0x03
#define VEML6030_CMDCODE_ALS      0x04
#define VEML6030_CMDCODE_WHITE    0x05
#define VEML6030_CMDCODE_ALS_INT  0x06

#define VEML6030_IT_25  0x0c
#define VEML6030_IT_50  0x08
#define VEML6030_IT_100 0x00
#define VEML6030_IT_200 0x01
#define VEML6030_IT_400 0x02
#define VEML6030_IT_800 0x03

#define VEML6030_GAIN_1 0x00
#define VEML6030_GAIN_2 0x01
#define VEML6030_GAIN_0_125 0x02
#define VEML6030_GAIN_0_25 0x03



struct veml6030_config {
	struct i2c_dt_spec bus;
	uint8_t psm;
};

struct veml6030_data {
	uint8_t shut_down;
	uint8_t gain;
	uint8_t it;
	uint8_t int_mode;
	uint16_t thresh_high;
	uint16_t thresh_low;
	uint16_t als_counts;
	uint32_t als_lux;
	uint16_t white_counts;
	uint32_t int_flags;
};


/* This fn in veml7700 drvier has a big comment about needing longer times. */
__unused static void veml6030_sleep_by_integration_time(const struct veml6030_data *data)
{
}

__maybe_unused
static int veml6030_write(const struct device *dev, uint8_t cmd, uint16_t data)
{
	const struct veml6030_config *conf = dev->config;
	uint8_t send_buf[3];

	send_buf[0] = cmd;                /* byte 0: command code */
	sys_put_le16(data, &send_buf[1]); /* bytes 1,2: command arguments */

	return i2c_write_dt(&conf->bus, send_buf, ARRAY_SIZE(send_buf));
}

__maybe_unused
static int veml6030_read(const struct device *dev, uint8_t cmd, uint16_t *data)
{
	const struct veml6030_config *conf = dev->config;

	uint8_t recv_buf[2];
	int ret = i2c_write_read_dt(&conf->bus, &cmd, sizeof(cmd), &recv_buf, ARRAY_SIZE(recv_buf));
	if (ret < 0) {
		return ret;
	}

	*data = sys_get_le16(recv_buf);

	return 0;
}


static int veml6030_sample_fetch(const struct device *dev, enum sensor_channel chan) {
//    const struct veml6030_config *conf = dev->config;
//	struct veml6030_data *data = dev->data;

        return 0;
}

static int veml6030_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
//	struct veml6030_data *data = dev->data;

	val->val1 = 0;
	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int veml6030_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct veml6030_config *conf = dev->config;
//
//	if (conf->psm != VEML6030_PSM_DISABLED) {
//		switch (action) {
//		case PM_DEVICE_ACTION_SUSPEND:
//			return veml6030_set_shutdown_flag(dev, 1);
//
//		case PM_DEVICE_ACTION_RESUME:
//			return veml6030_set_shutdown_flag(dev, 0);
//
//		default:
//			return -ENOTSUP;
//		}
//	}
//
	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static int veml6030_init(const struct device *dev)
{
	const struct veml6030_config *conf = dev->config;
//	struct veml6030_data *data = dev->data;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, veml6030_api) = {
	.sample_fetch = veml6030_sample_fetch,
	.channel_get = veml6030_channel_get,
//	.attr_set = veml6030_attr_set,
//	.attr_get = veml6030_attr_get,
};

#define VEML6030_INIT(n)                                                                           \
	static struct veml6030_data veml6030_data_##n;                                             \
                                                                                                   \
	static const struct veml6030_config veml6030_config_##n = {                                \
		.bus = I2C_DT_SPEC_INST_GET(n), .psm = DT_INST_PROP(n, psm_mode)};                 \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, veml6030_pm_action);                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, veml6030_init, PM_DEVICE_DT_INST_GET(n),                   \
				     &veml6030_data_##n, &veml6030_config_##n, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &veml6030_api);

DT_INST_FOREACH_STATUS_OKAY(VEML6030_INIT)
