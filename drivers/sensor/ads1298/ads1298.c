#define DT_DRV_COMPAT ti_ads1298

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ads1298.h"

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(ADS1298, CONFIG_SENSOR_LOG_LEVEL);
LOG_MODULE_REGISTER(ADS1298, LOG_LEVEL_DBG);



/* CS must remain low for the entire duration of the serial communication.
 * After the serial communication is finished, always wait four or more tCLK periods before taking CS high.*/
/* There is a minimum SCK
 * tSCLK < (tDR – 4tCLK) / (NBITS × NCHANNELS + 24)*/


#define SPI_BUF_SIZE 18
static __aligned(32) char spi_buffer_tx[SPI_BUF_SIZE] __used;
static __aligned(32) char spi_buffer_rx[SPI_BUF_SIZE] __used;


static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint8_t len, uint8_t *val)
{
	const struct ads1298_dev_config *cfg = dev->config;

    const struct spi_buf tx_bufs[] = {
            {
                    .buf = spi_buffer_tx,
                    .len = 4,
            },
    };
    const struct spi_buf rx_bufs[] = {
            {
                    .buf = spi_buffer_rx,
                    .len = 4,
            },
    };
    const struct spi_buf_set tx = {
            .buffers = tx_bufs,
            .count = ARRAY_SIZE(tx_bufs)
    };
    const struct spi_buf_set rx = {
            .buffers = rx_bufs,
            .count = ARRAY_SIZE(rx_bufs)
    };

    if (reg > 0x1F) {
        LOG_ERR("Invalid register %d", reg);
        return -EINVAL;
    }
    if (len > 0x1f) {
        LOG_ERR("Invalid length %d", len);
        return -EINVAL;
    }

    spi_buffer_tx[0] = 0x20 | (reg & 0x1F); // Read command
    spi_buffer_tx[1] = len;


    int ret;

    //    spi_cfg_cmd->operation |= SPI_HOLD_ON_CS;


    ret = spi_transceive_dt(&cfg->bus, &tx, &rx);
    if (ret) {
        printk("SPI transceive failed: %d\n", ret);
        return ret;
    }

	if (ret !=0) {
        LOG_ERR("Failed to read from SPI device (%d)", ret);
		return ret;
	}

    LOG_HEXDUMP_DBG(spi_buffer_tx, sizeof(spi_buffer_tx), "read");
    memcpy(val, spi_buffer_tx, sizeof(spi_buffer_tx));

	return 0;
}



static int ads1298_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ads1298_data *drv_data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

    uint8_t buf[7] = {0};
    ret = ads1298_read_reg(dev, 0,1, buf);
    if (ret) {
        return ret;
    }


    drv_data->pressure = (int32_t) sys_get_be24(&buf[1]);
    drv_data->temperature = (int32_t ) sys_get_be24(&buf[4]);

	return 0;
}

static int ads1298_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ads1298_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

    /* TODO Convert to mBar or something  (Datasheet unclear on -ve values) */
	val->val1 = (drv_data->pressure&& 0xFF0000) >> 16;
	val->val2 = (drv_data->pressure&& 0x00FFFF);

	return 0;
}


static int ads1298_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{

	return 0;
}

static int ads1298_probe(const struct device *dev)
{
	int ret;

    uint8_t  buf[7] = {0};
    ret = ads1298_read_reg(dev, 0,1, buf);

    if (ret) {
		return ret;
	}
    uint8_t status = buf[0];
    /* Device valid if Status == 01X0000X */
    if ((status & 0xDE) == 0x40)
    {
        LOG_ERR("Invalid status byte 0x%02x, want 01#0000#", status);
        return -ENODEV;
    }

	return 0;
}


static int ads1298_init(const struct device *dev)
{
	const struct ads1298_dev_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return ads1298_probe(dev);
//    return 0;
}

static DEVICE_API(sensor, ads1298_driver_api) = {
	.attr_set = ads1298_attr_set,
	.sample_fetch = ads1298_sample_fetch, // Device to driver (private)
	.channel_get  = ads1298_channel_get, // driver to thread
};

#define ADS1298_DEFINE(inst)                                                                       \
	static struct ads1298_data ads1298_data_##inst;                                            \
                                                                                                   \
	static const struct ads1298_dev_config ads1298_config_##inst = {                           \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst,                                                                      \
			(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA), 0),  \
                                                                                                   \
		IF_ENABLED(CONFIG_ADT7310_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ads1298_init, NULL, &ads1298_data_##inst,               \
				     &ads1298_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ads1298_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADS1298_DEFINE)
