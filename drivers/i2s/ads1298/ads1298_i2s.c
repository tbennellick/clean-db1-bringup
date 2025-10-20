#define DT_DRV_COMPAT ti_ads1298_i2s

#include "ads1298_i2s.h"
#include "ads1298_public.h"
#include "ads1298_reg.h"
#include "ads1298_utils.h"

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(ADS1298_I2S, LOG_LEVEL_DBG);

#define MAX_REG_RW_BYTES     256
#define REG_WRITE_OPCODE_LEN 2 /* 1 byte for command and reg  + 1 byte for length */
#define EXG_MAX_SPI_LEN      (MAX_REG_RW_BYTES + REG_WRITE_OPCODE_LEN)
uint8_t actual_tx_buffer[EXG_MAX_SPI_LEN];
uint8_t actual_rx_buffer[EXG_MAX_SPI_LEN];
struct spi_buf tx_bufs[] = {
	{
		.buf = actual_tx_buffer,
		.len = sizeof(actual_tx_buffer),
	},
};
struct spi_buf rx_bufs[] = {
	{
		.buf = actual_rx_buffer,
		.len = sizeof(actual_rx_buffer),
	},
};
const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};
const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

/* Buffers can be null */
static int ads1298_transact(const struct device *dev, const struct spi_buf_set *txbs, const struct spi_buf_set *rxbs) {
	const struct ads1298_i2s_config *cfg = dev->config;

	int ret = spi_transceive(cfg->spi_bus.bus, &cfg->spi_bus.config, txbs, rxbs);

	if (ret != 0) {
		LOG_ERR("Failed to transact with SPI device (%d)", ret);
	}
	return ret;
}

static int ads1298_send_command(const struct device *dev, ads1298_cmd_t cmd) {
	actual_tx_buffer[0] = cmd;
	tx_bufs[0].len = 1;

	int ret = ads1298_transact(dev, &tx, NULL);
	LOG_DBG("Set SDATAC mode ret: %d", ret);

	return ret;
}

static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint8_t *val) {
	struct ads1298_i2s_data *data = dev->data;

	if (reg > 0x1F) {
		LOG_ERR("Invalid register %d", reg);
		return -EINVAL;
	}

	actual_tx_buffer[0] = ADS1298_CMD_RREG | (reg & 0x1F); /* Read command */
	actual_tx_buffer[1] = 0;                               /* Number of reg to read -1 */
	actual_tx_buffer[2] = 0;                               /* Dummy byte for read command */
	tx_bufs[0].len = 3;
	rx_bufs[0].len = 3;

	int ret = ads1298_transact(dev, &tx, &rx);
	if (ret) {
		LOG_ERR("Failed to read register %d: %d", reg, ret);
		return ret;
	}
	*val = ((uint8_t *)rx_bufs[0].buf)[2];

	if (reg == ADS1298_REG_CONFIG1) {
		data->current_config1 = *val;
	}

	return ret;
}

static int ads1298_write_reg(const struct device *dev, uint8_t reg, uint8_t val) {
	struct ads1298_i2s_data *data = dev->data;
	if (reg == ADS1298_REG_CONFIG1) {
		data->current_config1 = val;
	}

	if (reg > 0x1F) {
		LOG_ERR("Invalid register %d", reg);
		return -EINVAL;
	}

	actual_tx_buffer[0] = ADS1298_CMD_WREG | (reg & 0x1F);
	actual_tx_buffer[1] = 0;
	actual_tx_buffer[2] = val;
	tx_bufs[0].len = 3;
	int ret = ads1298_transact(dev, &tx, NULL);
	if (ret) {
		LOG_ERR("Failed to write register %d: %d", reg, ret);
		return ret;
	}
	return ret;
}

static int ads1298_base_setup_device(const struct device *dev) {
	uint8_t value;
	int ret;

	/* Stop continuous data mode */
	ret = ads1298_send_command(dev, ADS1298_CMD_SDATAC);
	if (ret) {
		return ret;
	}

	/* Configure CONFIG3: enable internal reference */
	value = 0xc0;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG3, value);
	if (ret) {
		return ret;
	}

	/* Configure CONFIG1: 250 SPS, normal mode */
	value = 0x86;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG1, value);
	if (ret) {
		return ret;
	}

	/* Configure CONFIG2: test signals off */
	value = 0x00;
	ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG2, value);
	if (ret) {
		return ret;
	}

	/* Configure all channels: normal electrode input, gain = 6 */
	value = 0x01;
	for (int ch = ADS1298_REG_CH1SET; ch <= ADS1298_REG_CH8SET; ch++) {
		ret = ads1298_write_reg(dev, ch, value);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static void ads1298_get_samples_work_handler(struct k_work *work) {
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ads1298_i2s_data *data = CONTAINER_OF(dwork, struct ads1298_i2s_data, get_samples_work);
	const struct device *dev = data->dev;
	int ret;
	void *mem_block;

	if (!data->running) {
		return;
	}

	if (data->read_busy) {
		LOG_WRN("DRDY interrupt triggered before previous read completed");
		return;
	}

	data->read_busy = true;

	ret = k_mem_slab_alloc(data->mem_slab, &mem_block, K_NO_WAIT);
	if (ret == 0) {

		rx_bufs[0].len = ADS1298_SAMPLE_LEN;
		ret = ads1298_transact(dev, NULL, &rx);
		if (ret == 0) {
			k_msgq_put(&data->data_queue, &mem_block, K_NO_WAIT);
		} else {
			k_mem_slab_free(data->mem_slab, mem_block);
		}
	}
	data->read_busy = false;
}

static void ads1298_drdy_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	struct ads1298_i2s_data *data = CONTAINER_OF(cb, struct ads1298_i2s_data, drdy_cb);
	k_work_reschedule(&data->get_samples_work, K_NO_WAIT);
}

static int ads1298_i2s_configure(const struct device *dev, enum i2s_dir dir, const struct i2s_config *i2s_cfg) {
	struct ads1298_i2s_data *data = dev->data;

	if (dir != I2S_DIR_RX) {
		return -ENOTSUP;
	}

	data->mem_slab = i2s_cfg->mem_slab;
	data->timeout = i2s_cfg->timeout;

	return 0;
}

static int ads1298_i2s_read(const struct device *dev, void **mem_block, size_t *size) {
	struct ads1298_i2s_data *data = dev->data;
	int ret;
	void *buffer;

	if (data->running == false) {
		LOG_ERR("ADS1298 not running");
		return -EIO;
	}

	ret = k_msgq_get(&data->data_queue, &buffer, SYS_TIMEOUT_MS(data->timeout));
	if (ret != 0) {
		return ret;
	}

	*mem_block = buffer;
	return 0;
}

static int ads1298_i2s_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd) {
	struct ads1298_i2s_data *data = dev->data;
	const struct ads1298_i2s_config *cfg = dev->config;
	int ret;

	if (dir != I2S_DIR_RX) {
		return -ENOTSUP;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (data->running) {
			return -EALREADY;
		}

		data->running = true;
		data->read_busy = false;

		// Put the Device Back in RDATAC Mode
		ret = ads1298_send_command(dev, ADS1298_CMD_RDATAC);
		if (ret) {
			return ret;
		}

		/* Enable DRDY interrupt after Tsettle delay based on data rate */
		uint32_t tsettle_us = ads1298_get_tsettle_us(data->current_config1);
		k_busy_wait(tsettle_us);
		gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		/* Start Conversion */
		gpio_pin_set_dt(&cfg->start_conv_gpio, 1);

		LOG_INF("ADS1298 I2S stream started");
		break;

	case I2S_TRIGGER_STOP:
		if (!data->running) {
			return -EALREADY;
		}

		data->running = false;

		/* Disable DRDY interrupt */
		const struct ads1298_i2s_config *cfg = dev->config;
		gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_DISABLE);

		/* Stop conversion */
		gpio_pin_set_dt(&cfg->start_conv_gpio, 0);

		/* Go Back in to SDATAC mode so registers can be read. (DS unclear) */
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

static inline void wait_tpor(void) {
	/* tPOR =  2^18 tCLK periods = 0.131s @ 2MHz tCLK */
	k_sleep(K_MSEC(150)); /* Osc should start and DRDY toggle at 250Hz */
}

static inline void ads1298_reset_sequence(const struct ads1298_i2s_config *cfg) {
	const struct gpio_dt_spec exg_nrst = cfg->nrst_gpio;
	/* Fig 105 Wait for power rails to settle before setting 'signals' */
	wait_tpor();
	/* Fig 105 Issue reset pulse */
	gpio_pin_set_dt(&exg_nrst, 0);
	k_busy_wait(1); /* tRST = 2 * tCLKmax = 36n */
	gpio_pin_set_dt(&exg_nrst, 1);
	/* Fig105 wait 18x tCKmax => 9.25us */
	k_busy_wait(10);
	/* end of 11.1/fig105*/
}

static int ads1298_probe(const struct device *dev) {
	uint8_t id = 0;
	int ret = ads1298_read_reg(dev, ADS1298_REG_ID, &id);
	LOG_DBG("read ID: 0x%02x", id);
	if (ret != 0 || id != 0x92) /* TODO, there are multiple valid IDs*/
	{
		LOG_ERR("Failed to probe ADS1298, ID: 0x%02x with %d", id, ret);
		return -ENODEV;
	}
	return 0;
}

static int ads1298_i2s_init(const struct device *dev) {
	const struct ads1298_i2s_config *cfg = dev->config;
	struct ads1298_i2s_data *data = dev->data;
	int ret;

	/* Check SPI bus */
	if (!spi_is_ready_dt(&cfg->spi_bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	/* Check GPIOs */
	if (!gpio_is_ready_dt(&cfg->clksel_gpio)) {
		LOG_ERR("CLKSEL GPIO not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->nrst_gpio)) {
		LOG_ERR("NRST GPIO not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->npwdn_gpio)) {
		LOG_ERR("NPWDN GPIO not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->start_conv_gpio)) {
		LOG_ERR("START_CONV GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->drdy_gpio)) {
		LOG_ERR("DRDY GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->drdy_gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	/* Following flow in fig 93 */

	/* The first step calls out 11.1 and fig 105*/
	ret = gpio_pin_configure_dt(&cfg->nrst_gpio, GPIO_OUTPUT_HIGH);
	if (ret) {
		return ret;
	}

	/* Todo, do the other gpios need to tristated before reset incase this is called more than once in a reboot*/
	ads1298_reset_sequence(cfg);

	ret = gpio_pin_configure_dt(&cfg->clksel_gpio, GPIO_OUTPUT_HIGH);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->start_conv_gpio, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->npwdn_gpio, GPIO_OUTPUT_HIGH);
	if (ret) {
		return ret;
	}

	/* Initialize device */
	data->dev = dev;
	data->running = false;
	data->read_busy = false;
	data->current_config1 = 0x06; /* To match value on HW reset */
	k_msgq_init(&data->data_queue, (char *)data->rx_in_msgs, sizeof(ads1298_sample_t *), CONFIG_EXG_RX_SAMPLE_COUNT);

	/* Initialize work queue for data processing */
	k_work_init_delayable(&data->get_samples_work, ads1298_get_samples_work_handler);

	/* Configure DRDY interrupt */
	gpio_init_callback(&data->drdy_cb, ads1298_drdy_callback, BIT(cfg->drdy_gpio.pin));
	ret = gpio_add_callback(cfg->drdy_gpio.port, &data->drdy_cb);
	if (ret) {
		LOG_ERR("Failed to add DRDY callback: %d", ret);
		return ret;
	}

	ret = ads1298_base_setup_device(dev);
	if (ret) {
		LOG_ERR("Device configuration failed: %d", ret);
		return ret;
	}

	ret = ads1298_probe(dev);
	if (ret) {
		LOG_ERR("Device probe failed: %d", ret);
		return ret;
	}

	LOG_INF("ADS1298 I2S driver initialized");
	return 0;
}

static DEVICE_API(i2s, ads1298_i2s_api) = {
	.configure = ads1298_i2s_configure,
	.read = ads1298_i2s_read,
	.trigger = ads1298_i2s_trigger,
};

#define ADS1298_I2S_DEFINE(inst)                                                                                       \
	static struct ads1298_i2s_data ads1298_i2s_data_##inst;                                                            \
                                                                                                                       \
	static const struct ads1298_i2s_config ads1298_i2s_config_##inst = {                                               \
		.spi_bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, 0),                  \
		.clksel_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), clksel_gpios, 0),                                    \
		.nrst_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), nrst_gpios, 0),                                        \
		.npwdn_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), npwdn_gpios, 0),                                      \
		.start_conv_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), start_conv_gpios, 0),                            \
		.drdy_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), drdy_gpios, 0),                                        \
		.sample_rate = DT_INST_PROP_OR(inst, sample_rate, 250),                                                        \
		.channels = DT_INST_PROP_OR(inst, channels, 8),                                                                \
	};                                                                                                                 \
                                                                                                                       \
	DEVICE_DT_INST_DEFINE(inst,                                                                                        \
	                      ads1298_i2s_init,                                                                            \
	                      NULL,                                                                                        \
	                      &ads1298_i2s_data_##inst,                                                                    \
	                      &ads1298_i2s_config_##inst,                                                                  \
	                      POST_KERNEL,                                                                                 \
	                      CONFIG_I2S_INIT_PRIORITY,                                                                    \
	                      &ads1298_i2s_api);

DT_INST_FOREACH_STATUS_OKAY(ADS1298_I2S_DEFINE)
