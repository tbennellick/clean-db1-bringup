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

#define VEML6030_PSM_DISABLED 0x00

static const uint8_t veml6030_it_values[] = {
    VEML6030_IT_25,
    VEML6030_IT_50,
    VEML6030_IT_100,
    VEML6030_IT_200,
    VEML6030_IT_400,
    VEML6030_IT_800
};

//static const float veml6030_resolution[VEML6030_ALS_GAIN_ELEM_COUNT][VEML6030_ALS_IT_ELEM_COUNT] = {
//	/* 25ms    50ms    100ms   200ms   400ms   800ms  Integration Time */
//	{0.2304f, 0.1152f, 0.0576f, 0.0288f, 0.0144f, 0.0072f}, /* Gain 1 */
//	{0.1152f, 0.0576f, 0.0288f, 0.0144f, 0.0072f, 0.0036f}, /* Gain 2 */
//	{1.8432f, 0.9216f, 0.4608f, 0.2304f, 0.1152f, 0.0576f}, /* Gain 1/8 */
//	{0.9216f, 0.4608f, 0.2304f, 0.1152f, 0.0576f, 0.0288f}, /* Gain 1/4 */
//};

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

//static bool is_veml6030_gain_in_range(uint32_t gain_selection)
//{
//	return (gain_selection < VEML6030_ALS_GAIN_ELEM_COUNT);
//}
//
//static bool is_veml6030_it_in_range(uint32_t it_selection)
//{
//	return (it_selection < VEML6030_ALS_IT_ELEM_COUNT);
//}


/* This warning is in the veml7700 code, is it relevant?*/
/**
 * @brief Waits for a specific amount of time which depends
 * on the current integration time setting.
 *
 * According to datasheet for a measurement to complete one has
 * to wait for at least the integration time. But tests showed
 * that a much longer wait time is needed. Simply adding 50 or
 * 100ms to the integration time is not enough so we doubled
 * the integration time to get our wait time.
 *
 * This function is only called if the sensor is used in "single shot"
 * measuring mode. In this mode the sensor measures on demand an
 * measurements take time depending on the configures integration time.
 * In continuous mode, activated by one of the power saving modes,
 * you can always use the last sample value and no waiting is required.
 *
 * For more information see the "Designing the VEML6030 Into an Application"
 * application notes about the power saving modes.
 */
static void veml6030_sleep_by_integration_time(const struct veml6030_data *data)
{
	switch (data->it) {
	case VEML6030_IT_25:
		k_msleep(50);
		break;
	case VEML6030_IT_50:
		k_msleep(100);
		break;
	case VEML6030_IT_100:
		k_msleep(200);
		break;
	case VEML6030_IT_200:
		k_msleep(400);
		break;
	case VEML6030_IT_400:
		k_msleep(800);
		break;
	case VEML6030_IT_800:
		k_msleep(1600);
		break;
	}
}

//static int veml6030_counts_to_lux(const struct veml6030_data *data, uint16_t counts,
//				  uint32_t *lux)
//{
//	if (!is_veml6030_gain_in_range(data->gain) || !is_veml6030_it_in_range(data->it)) {
//		return -EINVAL;
//	}
//	*lux = counts * veml6030_resolution[data->gain][data->it];
//	return 0;
//}
//
//static int veml6030_lux_to_counts(const struct veml6030_data *data, uint32_t lux,
//				       uint16_t *counts)
//{
//	if (!is_veml6030_gain_in_range(data->gain) || !is_veml6030_it_in_range(data->it)) {
//		return -EINVAL;
//	}
//	*counts = lux / veml6030_resolution[data->gain][data->it];
//	return 0;
//}
//
//static int veml6030_check_gain(const struct sensor_value *val)
//{
//	return val->val1 >= VEML6030_ALS_GAIN_1 && val->val1 <= VEML6030_ALS_GAIN_1_4;
//}
//
//static int veml6030_check_it(const struct sensor_value *val)
//{
//	return val->val1 >= VEML6030_ALS_IT_25 && val->val1 <= VEML6030_ALS_IT_800;
//}
//
//static int veml6030_check_int_mode(const struct sensor_value *val)
//{
//	return (val->val1 >= VEML6030_ALS_PERS_1 && val->val1 <= VEML6030_ALS_PERS_8) ||
//	       val->val1 == VEML6030_INT_DISABLED;
//}

//static int veml6030_build_als_conf_param(const struct veml6030_data *data, uint16_t *return_value)
//{
//	if (!is_veml6030_gain_in_range(data->gain) || !is_veml6030_it_in_range(data->it)) {
//		return -EINVAL;
//	}
//	uint16_t param = 0;
//	/* Bits 15:13 -> reserved */
//	/* Bits 12:11 -> gain selection (ALS_GAIN) */
//	param |= data->gain << 11;
//	/* Bit 10 -> reserved */
//	/* Bits 9:6 -> integration time (ALS_IT) */
//	param |= veml6030_it_values[data->it] << 6;
//	/* Bits 5:4 -> interrupt persistent protection (ALS_PERS) */
//	if (data->int_mode != VEML6030_INT_DISABLED) {
//		param |= data->int_mode << 4;
//		/* Bit 1 -> interrupt enable (ALS_INT_EN) */
//		param |= BIT(1);
//	}
//	/* Bits 3:2 -> reserved */
//	/* Bit 0 -> shut down setting (ALS_SD) */
//	if (data->shut_down) {
//		param |= BIT(0);
//	}
//	*return_value = param;
//	return 0;
//}
//
//static uint16_t veml6030_build_psm_param(const struct veml6030_config *conf)
//{
//	/* We can directly use the devicetree configuration value. */
//	return conf->psm;
//}

static int veml6030_write(const struct device *dev, uint8_t cmd, uint16_t data)
{
	const struct veml6030_config *conf = dev->config;
	uint8_t send_buf[3];

	send_buf[0] = cmd;                /* byte 0: command code */
	sys_put_le16(data, &send_buf[1]); /* bytes 1,2: command arguments */

	return i2c_write_dt(&conf->bus, send_buf, ARRAY_SIZE(send_buf));
}

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

//static int veml6030_write_als_conf(const struct device *dev)
//{
//	const struct veml6030_data *data = dev->data;
//	uint16_t param;
//	int ret = 0;
//
//	ret = veml6030_build_als_conf_param(data, &param);
//	if (ret < 0) {
//		return ret;
//	}
//	LOG_DBG("Writing ALS configuration: 0x%04x", param);
//	return veml6030_write(dev, VEML6030_CMDCODE_ALS_CONF, param);
//}

static int veml6030_write_psm(const struct device *dev)
{
	const struct veml6030_config *conf = dev->config;
	uint16_t psm_param = conf->psm;
	return veml6030_write(dev, VEML6030_CMDCODE_PSM, psm_param);
}

static int veml6030_write_thresh_low(const struct device *dev)
{
	const struct veml6030_data *data = dev->data;
	return veml6030_write(dev, VEML6030_CMDCODE_ALS_WL, data->thresh_low);
}
static int veml6030_write_thresh_high(const struct device *dev)
{
	const struct veml6030_data *data = dev->data;

	LOG_DBG("Writing high threshold counts: %d", data->thresh_high);
	return veml6030_write(dev, VEML6030_CMDCODE_ALS_WH, data->thresh_high);
}
//
//static int veml6030_set_shutdown_flag(const struct device *dev, uint8_t new_val)
//{
//	struct veml6030_data *data = dev->data;
//	uint8_t prev_sd;
//	int ret;
//
//	prev_sd = data->shut_down;
//	data->shut_down = new_val;
//
//	ret = veml6030_write_als_conf(dev);
//	if (ret < 0) {
//		data->shut_down = prev_sd;
//	}
//	return ret;
//}
//
//static int veml6030_fetch_als(const struct device *dev)
//{
//	struct veml6030_data *data = dev->data;
//	uint16_t counts;
//	int ret;
//
//	ret = veml6030_read(dev, VEML6030_CMDCODE_ALS, &counts);
//	if (ret < 0) {
//		return ret;
//	}
//
//	data->als_counts = counts;
//	ret = veml6030_counts_to_lux(data, counts, &data->als_lux);
//	if (ret < 0) {
//		return ret;
//	}
//
//	LOG_DBG("Read ALS measurement: counts=%d, lux=%d", data->als_counts, data->als_lux);
//
//	return 0;
//}
//
//static int veml6030_fetch_white(const struct device *dev)
//{
//	struct veml6030_data *data = dev->data;
//	uint16_t counts;
//	int ret;
//
//	ret = veml6030_read(dev, VEML6030_CMDCODE_WHITE, &counts);
//	if (ret < 0) {
//		return ret;
//	}
//
//	data->white_counts = counts;
//	LOG_DBG("Read White Light measurement: counts=%d", data->white_counts);
//
//	return 0;
//}

//static int veml6030_fetch_int_flags(const struct device *dev)
//{
//	struct veml6030_data *data = dev->data;
//	uint16_t int_flags = 0;
//	int ret;
//
//	ret = veml6030_read(dev, VEML6030_CMDCODE_ALS_INT, &int_flags);
//	if (ret < 0) {
//		return ret;
//	}
//
//	data->int_flags = int_flags & VEML6030_ALS_INT_MASK;
//	LOG_DBG("Read int state: 0x%02x", data->int_flags);
//
//	return 0;
//}

//static int veml6030_attr_set(const struct device *dev, enum sensor_channel chan,
//			     enum sensor_attribute attr, const struct sensor_value *val)
//{
//	if (chan != SENSOR_CHAN_LIGHT) {
//		return -ENOTSUP;
//	}
//
//	struct veml6030_data *data = dev->data;
//	int ret = 0;
//
//	if (attr == SENSOR_ATTR_LOWER_THRESH) {
//		ret = veml6030_lux_to_counts(data, val->val1, &data->thresh_low);
//		if (ret < 0) {
//			return ret;
//		}
//		return veml6030_write_thresh_low(dev);
//	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
//		ret = veml6030_lux_to_counts(data, val->val1, &data->thresh_high);
//		if (ret < 0) {
//			return ret;
//		}
//		return veml6030_write_thresh_high(dev);
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_GAIN) {
//		if (veml6030_check_gain(val)) {
//			data->gain = (enum veml6030_als_gain)val->val1;
//			return veml6030_write_als_conf(dev);
//		} else {
//			return -EINVAL;
//		}
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_ITIME) {
//		if (veml6030_check_it(val)) {
//			data->it = (enum veml6030_als_it)val->val1;
//			return veml6030_write_als_conf(dev);
//		} else {
//			return -EINVAL;
//		}
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_INT_MODE) {
//		if (veml6030_check_int_mode(val)) {
//			data->int_mode = (enum veml6030_int_mode)val->val1;
//			return veml6030_write_als_conf(dev);
//		} else {
//			return -EINVAL;
//		}
//	} else {
//		return -ENOTSUP;
//	}
//}

//static int veml6030_attr_get(const struct device *dev, enum sensor_channel chan,
//			     enum sensor_attribute attr, struct sensor_value *val)
//{
//	if (chan != SENSOR_CHAN_LIGHT) {
//		return -ENOTSUP;
//	}
//
//	struct veml6030_data *data = dev->data;
//
//	if (attr == SENSOR_ATTR_LOWER_THRESH) {
//		val->val1 = data->thresh_low;
//	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
//		val->val1 = data->thresh_high;
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_GAIN) {
//		val->val1 = data->gain;
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_ITIME) {
//		val->val1 = data->it;
//	} else if ((enum sensor_attribute_veml6030)attr == SENSOR_ATTR_VEML6030_INT_MODE) {
//		val->val1 = data->int_mode;
//	} else {
//		return -ENOTSUP;
//	}
//
//	val->val2 = 0;
//
//	return 0;
//}

//static int veml6030_perform_single_measurement(const struct device *dev)
//{
//	struct veml6030_data *data = dev->data;
//	int ret;
//
//	/* Start sensor */
//	ret = veml6030_set_shutdown_flag(dev, 0);
//	if (ret < 0) {
//		return ret;
//	}
//
//	k_msleep(5);
//	/* Wait for measurement to complete */
//	veml6030_sleep_by_integration_time(data);
//
//	/* Shut down sensor */
//	return veml6030_set_shutdown_flag(dev, 1);
//}

static int veml6030_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct veml6030_config *conf = dev->config;
    struct veml6030_data *data;
    int ret;
//
//    /* Start sensor for new measurement if power saving mode is disabled */
//    if ((chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_ALL) &&
//        conf->psm == VEML6030_PSM_DISABLED)
//    {
//        ret = veml6030_perform_single_measurement(dev);
//        if (ret < 0)
//        {
//            return ret;
//        }
//    }
//
//    if (chan == SENSOR_CHAN_LIGHT)
//    {
//        return veml6030_fetch_als(dev);
//    }
//    else if ((enum sensor_channel_veml6030) chan == SENSOR_CHAN_VEML6030_INTERRUPT)
//    {
//        data = dev->data;
//        if (data->int_mode != VEML6030_INT_DISABLED)
//        {
//            return veml6030_fetch_int_flags(dev);
//        }
//        else
//        {
//            return -ENOTSUP;
//        }
//    }
//    else if ((enum sensor_channel_veml6030) chan == SENSOR_CHAN_VEML6030_WHITE_RAW_COUNTS)
//    {
//        return veml6030_fetch_white(dev);
//    }
//    else if (chan == SENSOR_CHAN_ALL)
//    {
//        data = dev->data;
//        if (data->int_mode != VEML6030_INT_DISABLED)
//        {
//            ret = veml6030_fetch_int_flags(dev);
//            if (ret < 0)
//            {
//                return ret;
//            }
//        }
//        ret = veml6030_fetch_white(dev);
//        if (ret < 0)
//        {
//            return ret;
//        }
//        return veml6030_fetch_als(dev);
//    }
//    else
//    {
//        return -ENOTSUP;
//    }
}

static int veml6030_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct veml6030_data *data = dev->data;

//	if (chan == SENSOR_CHAN_LIGHT) {
//		val->val1 = data->als_lux;
//	} else if ((enum sensor_channel_veml6030)chan == SENSOR_CHAN_VEML6030_RAW_COUNTS) {
//		val->val1 = data->als_counts;
//	} else if ((enum sensor_channel_veml6030)chan == SENSOR_CHAN_VEML6030_WHITE_RAW_COUNTS) {
//		val->val1 = data->white_counts;
//	} else if ((enum sensor_channel_veml6030)chan == SENSOR_CHAN_VEML6030_INTERRUPT) {
//		val->val1 = data->int_flags;
//	} else {
//		return -ENOTSUP;
//	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int veml6030_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct veml6030_config *conf = dev->config;

	if (conf->psm != VEML6030_PSM_DISABLED) {
		switch (action) {
		case PM_DEVICE_ACTION_SUSPEND:
			return veml6030_set_shutdown_flag(dev, 1);

		case PM_DEVICE_ACTION_RESUME:
			return veml6030_set_shutdown_flag(dev, 0);

		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

#endif /* CONFIG_PM_DEVICE */

static int veml6030_init(const struct device *dev)
{
	const struct veml6030_config *conf = dev->config;
	struct veml6030_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	ret = veml6030_write_psm(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set initial data values */
	data->thresh_low = 0;
	data->thresh_high = 0xFFFF;
	data->gain = VEML6030_GAIN_0_25;
	data->it = VEML6030_IT_100;
//	data->int_mode = VEML6030_INT_DISABLED;
	data->als_counts = 0;
	data->als_lux = 0;
	data->white_counts = 0;
	data->shut_down = (conf->psm != VEML6030_PSM_DISABLED) ? 0 : 1;

	/* Initialize sensor configuration */
	ret = veml6030_write_thresh_low(dev);
	if (ret < 0) {
		return ret;
	}

	ret = veml6030_write_thresh_high(dev);
	if (ret < 0) {
		return ret;
	}

//	ret = veml6030_write_als_conf(dev);
//	if (ret < 0) {
//		return ret;
//	}

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
