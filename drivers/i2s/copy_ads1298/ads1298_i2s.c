#define DT_DRV_COMPAT ti_ads1298

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ads1298_i2s.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ADS1298_I2S, LOG_LEVEL_DBG);

/* ADS1298 Commands */
#define ADS1298_CMD_WAKEUP      0x02
#define ADS1298_CMD_STANDBY     0x04
#define ADS1298_CMD_RESET       0x06
#define ADS1298_CMD_START       0x08
#define ADS1298_CMD_STOP        0x0A
#define ADS1298_CMD_RDATAC      0x10
#define ADS1298_CMD_SDATAC      0x11
#define ADS1298_CMD_RDATA       0x12
#define ADS1298_CMD_RREG        0x20
#define ADS1298_CMD_WREG        0x40

/* ADS1298 Registers */
#define ADS1298_REG_ID          0x00
#define ADS1298_REG_CONFIG1     0x01
#define ADS1298_REG_CONFIG2     0x02
#define ADS1298_REG_CONFIG3     0x03
#define ADS1298_REG_LOFF        0x04
#define ADS1298_REG_CH1SET      0x05
#define ADS1298_REG_CH2SET      0x06
#define ADS1298_REG_CH3SET      0x07
#define ADS1298_REG_CH4SET      0x08
#define ADS1298_REG_CH5SET      0x09
#define ADS1298_REG_CH6SET      0x0A
#define ADS1298_REG_CH7SET      0x0B
#define ADS1298_REG_CH8SET      0x0C
#define ADS1298_REG_RLD_SENSP   0x0D
#define ADS1298_REG_RLD_SENSN   0x0E
#define ADS1298_REG_LOFF_SENSP  0x0F
#define ADS1298_REG_LOFF_SENSN  0x10
#define ADS1298_REG_LOFF_FLIP   0x11
#define ADS1298_REG_LOFF_STATP  0x12
#define ADS1298_REG_LOFF_STATN  0x13
#define ADS1298_REG_GPIO        0x14
#define ADS1298_REG_PACE        0x15
#define ADS1298_REG_RESP        0x16
#define ADS1298_REG_CONFIG4     0x17
#define ADS1298_REG_WCT1        0x18
#define ADS1298_REG_WCT2        0x19

/* Data packet size: 3 bytes status + 8 channels * 3 bytes each = 27 bytes */
#define ADS1298_PACKET_SIZE     27

static uint8_t spi_tx_buf[256];
static uint8_t spi_rx_buf[256];

static int ads1298_spi_transceive(const struct device *dev, const uint8_t *tx_data, 
                                 uint8_t *rx_data, size_t len)
{
	const struct ads1298_i2s_config *cfg = dev->config;
	
	const struct spi_buf tx_buf = {
		.buf = (void *)tx_data,
		.len = len
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1
	};
	
	if (rx_data) {
		const struct spi_buf rx_buf = {
			.buf = rx_data,
			.len = len
		};
		const struct spi_buf_set rx_bufs = {
			.buffers = &rx_buf,
			.count = 1
		};
		return spi_transceive_dt(&cfg->spi_bus, &tx_bufs, &rx_bufs);
	} else {
		return spi_write_dt(&cfg->spi_bus, &tx_bufs);
	}
}

static int ads1298_send_command(const struct device *dev, uint8_t cmd)
{
	spi_tx_buf[0] = cmd;
	return ads1298_spi_transceive(dev, spi_tx_buf, NULL, 1);
}

static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint8_t len, uint8_t *data)
{
	if (reg > 0x1F || len == 0 || len > 32) {
		return -EINVAL;
	}
	
	spi_tx_buf[0] = ADS1298_CMD_RREG | (reg & 0x1F);
	spi_tx_buf[1] = len - 1;
	
	int ret = ads1298_spi_transceive(dev, spi_tx_buf, spi_rx_buf, len + 2);
	if (ret == 0) {
		memcpy(data, &spi_rx_buf[2], len);
	}
	return ret;
}

static int ads1298_write_reg(const struct device *dev, uint8_t reg, uint8_t len, const uint8_t *data)
{
	if (reg > 0x1F || len == 0 || len > 32) {
		return -EINVAL;
	}
	
	spi_tx_buf[0] = ADS1298_CMD_WREG | (reg & 0x1F);
	spi_tx_buf[1] = len - 1;
	memcpy(&spi_tx_buf[2], data, len);
	
	return ads1298_spi_transceive(dev, spi_tx_buf, NULL, len + 2);
}

static void ads1298_reset_sequence(const struct device *dev)
{
	const struct ads1298_i2s_config *cfg = dev->config;
	
	/* Power-on reset sequence */
	gpio_pin_set_dt(&cfg->npwdn_gpio, 1);
	k_sleep(K_MSEC(150)); /* tPOR = 131ms at 2MHz */
	
	/* Hardware reset */
	gpio_pin_set_dt(&cfg->nrst_gpio, 0);
	k_usleep(1);
	gpio_pin_set_dt(&cfg->nrst_gpio, 1);
	k_usleep(10);
}

static int ads1298_configure_device(const struct device *dev)
{
	uint8_t data;
	int ret;
	
	/* Stop continuous data mode */
	ret = ads1298_send_command(dev, ADS1298_CMD_SDATAC);
	if (ret) return ret;
	
	/* Configure CONFIG3: enable internal reference */
	data = 0xc0;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG3, 1, &data);
	if (ret) return ret;
	
	/* Configure CONFIG1: 250 SPS, normal mode */
	data = 0x86;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG1, 1, &data);
	if (ret) return ret;
	
	/* Configure CONFIG2: test signals off */
	data = 0x00;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG2, 1, &data);
	if (ret) return ret;
	
	/* Configure all channels: normal electrode input, gain = 6 */
	data = 0x01;
	for (int ch = ADS1298_REG_CH1SET; ch <= ADS1298_REG_CH8SET; ch++) {
		ret = ads1298_write_reg(dev, ch, 1, &data);
		if (ret) return ret;
	}
	
	return 0;
}

static void ads1298_data_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ads1298_i2s_data *data = CONTAINER_OF(dwork, struct ads1298_i2s_data, data_work);
	const struct device *dev = data->dev;
	const struct ads1298_i2s_config *cfg = dev->config;
	void *mem_block;
	int ret;
	
	if (!data->running) {
		return;
	}
	
	/* Check DRDY signal */
	if (gpio_pin_get_dt(&cfg->drdy_gpio) == 0) {
		/* Data ready, allocate memory block */
		ret = k_mem_slab_alloc(data->mem_slab, &mem_block, K_NO_WAIT);
		if (ret == 0) {
			/* Read data packet */
			ret = ads1298_spi_transceive(dev, NULL, (uint8_t *)mem_block, ADS1298_PACKET_SIZE);
			if (ret == 0) {
				/* Queue the data block for processing */
				if (data->data_queue) {
					k_msgq_put(data->data_queue, &mem_block, K_NO_WAIT);
				}
			} else {
				k_mem_slab_free(data->mem_slab, mem_block);
			}
		}
	}
	
	/* Schedule next data read based on sample rate */
	uint32_t period_ms = 1000 / cfg->sample_rate;
	k_work_schedule(&data->data_work, K_MSEC(period_ms));
}

/* I2S Driver API Implementation */

static int ads1298_i2s_configure(const struct device *dev, enum i2s_dir dir, 
                                 const struct i2s_config *i2s_cfg)
{
	struct ads1298_i2s_data *data = dev->data;
	
	if (dir != I2S_DIR_RX) {
		return -ENOTSUP;
	}
	
	data->mem_slab = i2s_cfg->mem_slab;
	data->timeout = i2s_cfg->timeout;
	
	return 0;
}

static int ads1298_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	struct ads1298_i2s_data *data = dev->data;
	int ret;
	
	if (!data->mem_slab) {
		return -ENODEV;
	}
	
	ret = k_mem_slab_alloc(data->mem_slab, mem_block, K_MSEC(data->timeout));
	if (ret) {
		return ret;
	}
	
	/* For ADS1298, we simulate I2S read by providing EXG data */
	*size = ADS1298_PACKET_SIZE;
	
	return 0;
}

static int ads1298_i2s_trigger(const struct device *dev, enum i2s_dir dir, 
                               enum i2s_trigger_cmd cmd)
{
	struct ads1298_i2s_data *data = dev->data;
	int ret;
	
	if (dir != I2S_DIR_RX) {
		return -ENOTSUP;
	}
	
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (data->running) {
			return -EALREADY;
		}
		
		/* Start continuous data mode */
		ret = ads1298_send_command(dev, ADS1298_CMD_RDATAC);
		if (ret) {
			return ret;
		}
		
		/* Start data conversion */
		ret = ads1298_send_command(dev, ADS1298_CMD_START);
		if (ret) {
			return ret;
		}
		
		data->running = true;
		k_work_schedule(&data->data_work, K_MSEC(10));
		LOG_INF("ADS1298 I2S stream started");
		break;
		
	case I2S_TRIGGER_STOP:
		if (!data->running) {
			return -EALREADY;
		}
		
		data->running = false;
		k_work_cancel_delayable(&data->data_work);
		
		/* Stop data conversion */
		ret = ads1298_send_command(dev, ADS1298_CMD_STOP);
		if (ret) {
			return ret;
		}
		
		/* Stop continuous data mode */
		ret = ads1298_send_command(dev, ADS1298_CMD_SDATAC);
		if (ret) {
			return ret;
		}
		
		LOG_INF("ADS1298 I2S stream stopped");
		break;
		
	default:
		return -ENOTSUP;
	}
	
	return 0;
}

static int ads1298_i2s_init(const struct device *dev)
{
	const struct ads1298_i2s_config *cfg = dev->config;
	struct ads1298_i2s_data *data = dev->data;
	uint8_t device_id;
	int ret;
	
	/* Check SPI bus */
	if (!spi_is_ready_dt(&cfg->spi_bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}
	
	/* Initialize GPIOs */
	if (!gpio_is_ready_dt(&cfg->clksel_gpio)) {
		LOG_ERR("CLKSEL GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->clksel_gpio, GPIO_OUTPUT_HIGH);
	if (ret) return ret;
	
	if (!gpio_is_ready_dt(&cfg->nrst_gpio)) {
		LOG_ERR("NRST GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->nrst_gpio, GPIO_OUTPUT_HIGH);
	if (ret) return ret;
	
	if (!gpio_is_ready_dt(&cfg->npwdn_gpio)) {
		LOG_ERR("NPWDN GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->npwdn_gpio, GPIO_OUTPUT_HIGH);
	if (ret) return ret;
	
	if (!gpio_is_ready_dt(&cfg->start_conv_gpio)) {
		LOG_ERR("START_CONV GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->start_conv_gpio, GPIO_OUTPUT_LOW);
	if (ret) return ret;
	
	if (!gpio_is_ready_dt(&cfg->drdy_gpio)) {
		LOG_ERR("DRDY GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->drdy_gpio, GPIO_INPUT);
	if (ret) return ret;
	
	/* Initialize device */
	data->dev = dev;
	data->running = false;
	k_work_init_delayable(&data->data_work, ads1298_data_work_handler);
	
	/* Reset and configure ADS1298 */
	ads1298_reset_sequence(dev);
	
	ret = ads1298_configure_device(dev);
	if (ret) {
		LOG_ERR("Device configuration failed: %d", ret);
		return ret;
	}
	
	/* Verify device ID */
	ret = ads1298_read_reg(dev, ADS1298_REG_ID, 1, &device_id);
	if (ret) {
		LOG_ERR("Failed to read device ID: %d", ret);
		return ret;
	}
	
	if (device_id != 0x92) {
		LOG_ERR("Invalid device ID: 0x%02x (expected 0x92)", device_id);
		return -ENODEV;
	}
	
	/* Set START pin high to enable conversions */
	gpio_pin_set_dt(&cfg->start_conv_gpio, 1);
	
	LOG_INF("ADS1298 I2S driver initialized, device ID: 0x%02x", device_id);
	return 0;
}

static DEVICE_API(i2s, ads1298_i2s_api) = {
        .configure = ads1298_i2s_configure,
        .read = ads1298_i2s_read,
        .trigger = ads1298_i2s_trigger,
};

#define ADS1298_I2S_DEFINE(inst)                                                    \
	static struct ads1298_i2s_data ads1298_i2s_data_##inst;                    \
	                                                                            \
	static const struct ads1298_i2s_config ads1298_i2s_config_##inst = {       \
		.spi_bus = SPI_DT_SPEC_INST_GET(                                    \
			inst,                                                       \
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, 0),    \
		.clksel_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst),          \
						       clksel_gpios, 0),            \
		.nrst_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst),            \
						     nrst_gpios, 0),                \
		.npwdn_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst),           \
						      npwdn_gpios, 0),              \
		.start_conv_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst),      \
							   start_conv_gpios, 0),    \
		.drdy_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst),            \
						     drdy_gpios, 0),                \
		.sample_rate = DT_INST_PROP_OR(inst, sample_rate, 250),            \
		.channels = DT_INST_PROP_OR(inst, channels, 8),                    \
	};                                                                          \
	                                                                            \
	DEVICE_DT_INST_DEFINE(inst, ads1298_i2s_init, NULL,                        \
			      &ads1298_i2s_data_##inst,                             \
			      &ads1298_i2s_config_##inst,                           \
			      POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,                \
			      &ads1298_i2s_api);

DT_INST_FOREACH_STATUS_OKAY(ADS1298_I2S_DEFINE)