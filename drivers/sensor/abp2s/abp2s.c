#define DT_DRV_COMPAT honeywell_abp2s

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "abp2s.h"
#include "check_status.h"

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(ABP2S, CONFIG_SENSOR_LOG_LEVEL);
LOG_MODULE_REGISTER(ABP2S, LOG_LEVEL_DBG);

#define ABP2S_CMD_NOP 0xF0
#define ABP2S_CMD_MEASURE 0xAA
#define ABP2S_MAX_TRX_LEN 7
#define ABP2S_STATUS_BIT_BUSY (1<<5)
#define ABP2S_TIMEOUT_MS 5

uint8_t rx_bytes[ABP2S_MAX_TRX_LEN] = {0};
struct spi_buf rx_buf = { .buf = rx_bytes, .len = sizeof(rx_bytes) };
const struct spi_buf_set pressure_rx_set = { .buffers = &rx_buf, .count = 1 };

uint8_t tx_bytes[ABP2S_MAX_TRX_LEN] = {0};
struct spi_buf tx_buf = { .buf = tx_bytes, .len = sizeof(tx_bytes) };
const struct spi_buf_set pressure_tx_set = { .buffers = &tx_buf, .count = 1 };


//
//static int abp2_read(const struct device *dev, uint8_t *val)
//{
//	const struct abp2_dev_config *cfg = dev->config;
//
//	uint8_t cmd_buf[7] = {0};
//
//	const struct spi_buf rx_buf = { .buf = cmd_buf, .len = sizeof(cmd_buf) };
//	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
//
//	int ret = spi_read_dt(&cfg->bus, &rx);
//
//	if (ret !=0) {
//        LOG_ERR("Failed to read from SPI device (%d)", ret);
//		return ret;
//	}
//
//    LOG_HEXDUMP_DBG(cmd_buf, sizeof(cmd_buf), "read");
//    memcpy(val, cmd_buf, sizeof(cmd_buf));
//
//	return 0;
//}


static int abp2_status(const struct device *dev, uint8_t *status)
{
    const struct abp2_dev_config *cfg = dev->config;

    tx_bytes[0] = ABP2S_CMD_NOP;
    tx_buf.len = 1;
    rx_bytes[0] = 0;
    rx_buf.len = 1;

    int ret = spi_transceive_dt(&cfg->bus, &pressure_tx_set, &pressure_rx_set);

    if (ret !=0) {
        LOG_ERR("Failed to read from SPI device (%d)", ret);
        return ret;
    }
    *status = rx_bytes[0];
    return 0;
}

static int abp2_measurement_start(const struct device *dev)
{
    const struct abp2_dev_config *cfg = dev->config;

    tx_bytes[0] = ABP2S_CMD_MEASURE;
    tx_buf.len = 1;
    rx_buf.len = 1;

    int ret = spi_transceive_dt(&cfg->bus, &pressure_tx_set, &pressure_rx_set);

    if (ret !=0) {
        LOG_ERR("Failed to read from SPI device (%d)", ret);
        return ret;
    }
    return 0;
}

static int abp2_mesurement_get(const struct device *dev, uint8_t *status)
{
    const struct abp2_dev_config *cfg = dev->config;

    tx_bytes[0] = ABP2S_CMD_NOP;
    tx_buf.len = 1;
    rx_bytes[0] = 0;
    rx_buf.len = 1;

    int ret = spi_transceive_dt(&cfg->bus, &pressure_tx_set, &pressure_rx_set);

    if (ret !=0) {
        LOG_ERR("Failed to read from SPI device (%d)", ret);
        return ret;
    }
    *status = rx_bytes[0];
    return 0;
}


static int abp2_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct abp2_data *drv_data = dev->data;
	int ret;
    uint8_t status = 0;


    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

    abp2_status(dev, &status);
    LOG_DBG("stat = %x", status);

    ret = abp2_measurement_start(dev);
    if (ret < 0) {
        LOG_ERR("Failed to start measurement on ABP2S device (%d)", ret);
        return ret;
    }

    int64_t start_time = k_uptime_get();
    uint32_t loop_count =0;
    while(1)
    {
        ret = abp2_status(dev, &status);
        if (ret < 0) {
            LOG_ERR("Failed to read status byte from ABP2S device (%d)", ret);
            return ret;
        }
        int64_t elapsed_time = k_uptime_get() - start_time;

        if ((status & ABP2S_STATUS_BIT_BUSY)==0)
        {
            LOG_DBG("Ready after %d loops and %lld ms", loop_count, elapsed_time);
            break;
        }
        loop_count++;

        if (elapsed_time > ABP2S_TIMEOUT_MS)
        {
            LOG_DBG("Timeout after %d loops and %lld ms", loop_count, elapsed_time);
            break;
        }
    }


//    drv_data->pressure = (int32_t) sys_get_be24(&buf[1]);
//    drv_data->temperature = (int32_t ) sys_get_be24(&buf[4]);

	return 0;
}

static int abp2_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct abp2_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

    /* TODO Convert to mBar or something  (Datasheet unclear on -ve values) */
	val->val1 = (drv_data->pressure&& 0xFF0000) >> 16;
	val->val2 = (drv_data->pressure&& 0x00FFFF);

	return 0;
}

static int abp2_probe(const struct device *dev)
{
    uint8_t status =0;
    int ret = abp2_status(dev, &status);
    if (ret < 0) {
        LOG_ERR("Failed to read status byte from ABP2S device (%d)", ret);
        return ret;
    }

    LOG_DBG("Device status byte 0x%02x", status);
    if(abp2s_check_status(status))
    {
        return 0;
    }
    return -ENODEV;
}


static int abp2_init(const struct device *dev)
{
	const struct abp2_dev_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return abp2_probe(dev);
}

static DEVICE_API(sensor, abp2_driver_api) = {
//	.attr_set = abp2_attr_set,
	.sample_fetch = abp2_sample_fetch, // Device to driver (private)
	.channel_get  = abp2_channel_get, // driver to thread
};

#define ABP2S_DEFINE(inst)                                                                       \
	static struct abp2_data abp2_data_##inst;                                            \
                                                                                                   \
	static const struct abp2_dev_config abp2_config_##inst = {                           \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst,                                                                      \
			(SPI_WORD_SET(8) | SPI_TRANSFER_MSB ), 0),  \
                                                                                                   \
		IF_ENABLED(CONFIG_ADT7310_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, abp2_init, NULL, &abp2_data_##inst,               \
				     &abp2_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &abp2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ABP2S_DEFINE)
