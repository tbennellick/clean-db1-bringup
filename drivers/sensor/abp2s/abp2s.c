#define DT_DRV_COMPAT honeywell_abp2s

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "abp2s.h"

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(ABP2S, CONFIG_SENSOR_LOG_LEVEL);
LOG_MODULE_REGISTER(ABP2S, LOG_LEVEL_DBG);

static int abp2_read(const struct device *dev, uint8_t *val)
{
	const struct abp2_dev_config *cfg = dev->config;

	uint8_t cmd_buf[7] = {0};

	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_read_dt(&cfg->bus, &rx);

	if (ret < 0) {
		return ret;
	}

    LOG_HEXDUMP_DBG(cmd_buf, sizeof(cmd_buf), "read");
    memcpy(val, cmd_buf, sizeof(cmd_buf));

	return 0;
}



static int abp2_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct abp2_data *drv_data = dev->data;
	int16_t value = 0;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}


    uint8_t buf[7] = {0};
    ret = abp2_read(dev, buf);

    //
//    if (ret) {
//        return ret;
//    }


	drv_data->sample = value;

	return 0;
}

static int abp2_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
//	int32_t value;
//	struct abp2_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

//	value = ADT7310_SAMPLE_TO_MICRO_DEG((int32_t)drv_data->sample);
//	val->val1 = value / 1000000;
//	val->val2 = value % 1000000;

	return 0;
}
//
//static int abp2_update_reg(const struct device *dev, uint8_t reg, uint8_t value, uint8_t mask)
//{
//	int ret;
//	uint8_t reg_value;
//
//	ret = abp2_reg_read(dev, reg, &reg_value);
//	if (ret < 0) {
//		return ret;
//	}
//
//	reg_value &= ~mask;
//	reg_value |= value;
//
//	return abp2_reg_write(dev, reg, reg_value);
//}

static int abp2_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{

	return 0;
}

//static int abp2_probe(const struct device *dev)
//{
////	uint8_t value;
////	int ret;
////
////
////	if (ret) {
////		return ret;
////	}
//
////	value &= 0xf8;
////	if (value != ADT7310_ID) {
////		LOG_ERR("Invalid device ID");
////		return -ENODEV;
////	}
//
//	return 1;
//}


static int abp2_init(const struct device *dev)
{
	const struct abp2_dev_config *cfg = dev->config;
	int ret;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

//	ret = abp2_probe(dev);
//	if (ret) {
//		return ret;
//	}
    ret = 0;

	return ret;
}

static DEVICE_API(sensor, abp2_driver_api) = {
	.attr_set = abp2_attr_set,
	.sample_fetch = abp2_sample_fetch,
	.channel_get  = abp2_channel_get,
};

#define ABP2S_DEFINE(inst)                                                                       \
	static struct abp2_data abp2_data_##inst;                                            \
                                                                                                   \
	static const struct abp2_dev_config abp2_config_##inst = {                           \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst,                                                                      \
			(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA), 0),  \
                                                                                                   \
		IF_ENABLED(CONFIG_ADT7310_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, abp2_init, NULL, &abp2_data_##inst,               \
				     &abp2_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &abp2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ABP2S_DEFINE)
