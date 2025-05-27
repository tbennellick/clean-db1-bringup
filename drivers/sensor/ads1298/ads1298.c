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


/* SCLK/tSCLK is the SPI clock, not the "main oscillator" */
/* CLK/tCLK is clock on CLK pin */
/*fDR/tDR is output data rate  250 SPS to 32 kSPS */
/* Requirements is  >200z so 250 SPS*/

/* CS must remain low for the entire duration of the serial communication.
 * After the serial communication is finished, always wait four or more tCLK periods before taking CS high.*/
/* There is a minimum SCK: tSCLK < (tDR – 4tCLK) / (NBITS × NCHANNELS + 24)*/
/* THis is 5~50kHz with boot settings */
/* SPI clock max = 20MHz (tsclk>50ns) */
/* For multibyte commands, a 4 tCLK period must separate the end of one byte (or opcode) and the next. */

/* Hack - TODO: move to device tree */
#define EXG_CS_TEMP DT_ALIAS(exg_cs_temp)
static const struct gpio_dt_spec exg_cs_temp = GPIO_DT_SPEC_GET(EXG_CS_TEMP, gpios);


#define SPI_BUF_SIZE 18
static __aligned(32) char spi_buffer_tx[SPI_BUF_SIZE] __used;
static __aligned(32) char spi_buffer_rx[SPI_BUF_SIZE] __used;

#define ADS1298_CMD_WAKEUP 0x02
#define ADS1298_CMD_STANDBY 0x04
#define ADS1298_CMD_RESET 0x06
#define ADS1298_CMD_START 0x08
#define ADS1298_CMD_STOP 0x0A
#define ADS1298_CMD_RDATAC 0x10
#define ADS1298_CMD_SDATAC 0x11
#define ADS1298_CMD_RDATA 0x12
#define ADS1298_CMD_RREG 0x20
#define ADS1298_CMD_WREG 0x40

#define EXG_MAX_SPI_LEN 8
uint8_t tx_buf[EXG_MAX_SPI_LEN];
uint8_t rx_buf[EXG_MAX_SPI_LEN];
struct spi_buf tx_bufs[] = {
        {
                .buf = tx_buf,
                .len = sizeof(tx_buf),
        },
};
struct spi_buf rx_bufs[] = {
        {
                .buf = rx_buf,
                .len = sizeof(rx_buf),
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



/* Bufs can be null */
static int ads1298_transact(const struct device *dev, const struct spi_buf_set *txbs, const struct spi_buf_set *rxbs)
{
    const struct ads1298_dev_config *cfg = dev->config;

    gpio_pin_set_dt(&exg_cs_temp, 0);

    int ret  = spi_transceive(cfg->bus.bus, &cfg->bus.config , txbs, rxbs);

    k_busy_wait(4);/* TODO make this dynamic */
    gpio_pin_set_dt(&exg_cs_temp, 1);

    if (ret !=0) {
        LOG_ERR("Failed to transact with SPI device (%d)", ret);
    }
    return ret;
}

__maybe_unused
static int ads1298_wakeup(const struct device *dev)
{
    tx_buf[0] = ADS1298_CMD_WAKEUP;
    tx_bufs[0].len =1;

    int ret = ads1298_transact(dev, &tx, NULL);

    k_busy_wait(1000);

    return ret;

}

__maybe_unused
static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint8_t len, uint8_t *val)
{

    if (reg > 0x1F) {
        LOG_ERR("Invalid register %d", reg);
        return -EINVAL;
    }
    if (len > 0x1f) {
        LOG_ERR("Invalid length %d", len);
        return -EINVAL;
    }

    tx_buf[0] = ADS1298_CMD_RREG | (reg & 0x1F); // Read command
    tx_bufs[0].len = len;
    rx_bufs[0].len = 2;

    int ret = ads1298_transact(dev, &tx, &rx);

    LOG_HEXDUMP_DBG(rx_bufs[0].buf, rx_bufs[0].len, "read");

	return ret;
}



static int ads1298_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
//	struct ads1298_data *drv_data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

    ret = ads1298_wakeup(dev);
    if (ret) {
        return ret;
    }

    uint8_t  buf[7] = {0};
    ret = ads1298_read_reg(dev, 0,1, buf);
    LOG_WRN("ret: %d", ret);
    if (ret) {
        return ret;
    }


//    drv_data->pressure = (int32_t) sys_get_be24(&buf[1]);
//    drv_data->temperature = (int32_t ) sys_get_be24(&buf[4]);

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
//	int ret;
//
//    uint8_t  buf[7] = {0};
//    ret = ads1298_read_reg(dev, 0,1, buf);
//
//    if (ret) {
//		return ret;
//	}
//    uint8_t status = buf[0];
//    /* Device valid if Status == 01X0000X */
//    if ((status & 0xDE) == 0x40)
//    {
//        LOG_ERR("Invalid status byte 0x%02x, want 01#0000#", status);
//        return -ENODEV;
//    }

	return 0;
}


static int ads1298_init(const struct device *dev)
{
	const struct ads1298_dev_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

    if (!gpio_is_ready_dt(&exg_cs_temp))
    {
        LOG_ERR("Gpio not ready %s %d",(char *)&exg_cs_temp.port->name, exg_cs_temp.pin);
    }
    else
    {
        int res = gpio_pin_configure_dt(&exg_cs_temp, GPIO_OUTPUT_HIGH);
        if (res !=0)
        {
            LOG_ERR("Cant init %s %d",(char *) &exg_cs_temp.port->name, exg_cs_temp.pin);
        }
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
			(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA), 0),  \
                                                                                                   \
		IF_ENABLED(CONFIG_ADT7310_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ads1298_init, NULL, &ads1298_data_##inst,               \
				     &ads1298_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ads1298_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADS1298_DEFINE)
