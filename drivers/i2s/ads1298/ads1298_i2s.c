#define DT_DRV_COMPAT ti_ads1298_i2s

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ads1298_i2s.h"
#include "ads1298_reg.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ADS1298_I2S, LOG_LEVEL_DBG);


static int ads1298_send_command(const struct device *dev, uint8_t cmd)
{
    return 0;
}

static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint8_t len, uint8_t *data)
{
    return 0;
}

static int ads1298_write_reg(const struct device *dev, uint8_t reg, uint8_t len, const uint8_t *data)
{
    return 0;
}



static int ads1298_base_setup_device(const struct device *dev)
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



//static void ads1298_data_work_handler(struct k_work *work)
//{
//    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
//    struct ads1298_i2s_data *data = CONTAINER_OF(dwork, struct ads1298_i2s_data, data_work);
//    const struct device *dev = data->dev;
//    const struct ads1298_i2s_config *cfg = dev->config;
//    void *mem_block;
//    int ret;
//
//    if (!data->running) {
//        return;
//    }

/* ON interrupt*/
__unused
static void ads1298_data_process(const struct device *dev)
{
    struct ads1298_i2s_data *data = dev->data;
    int ret;
    void *mem_block;


	/* TODO: Check DRDY signal ? */

    ret = k_mem_slab_alloc(data->mem_slab, &mem_block, K_NO_WAIT);
    if (ret == 0) {
        /* Read data packet */
        /* TODO: Actually get the data*/
//        ret = ads1298_spi_transceive(dev, NULL, (uint8_t *)mem_block, ADS1298_PACKET_SIZE);
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

/* From i2s_mcux_sai.c*/
//    struct stream *strm = &dev_data->rx;
//    void *buffer;
//    int status, ret = 0;
//
//    LOG_DBG("i2s_mcux_read");
//    if (strm->state == I2S_STATE_NOT_READY) {
//        LOG_ERR("invalid state %d", strm->state);
//        return -EIO;
//    }
//
//    status = k_msgq_get(&strm->out_queue, &buffer, SYS_TIMEOUT_MS(strm->cfg.timeout));
//    if (status != 0) {
//        if (strm->state == I2S_STATE_ERROR) {
//            ret = -EIO;
//        } else {
//            LOG_DBG("need retry");
//            ret = -EAGAIN;
//        }
//        return ret;
//    }
//
//    *mem_block = buffer;
//    *size = strm->cfg.block_size;



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

        /* TODO */

		LOG_INF("ADS1298 I2S stream started");
		break;
		
	case I2S_TRIGGER_STOP:
		if (!data->running) {
			return -EALREADY;
		}
		
		data->running = false;

        /* TODO */

		LOG_INF("ADS1298 I2S stream stopped");
		break;
		
	default:
		return -ENOTSUP;
	}
	
	return 0;
}

static inline void wait_tpor(void)
{
    /* tPOR =  2^18 tCLK periods = 0.131s @ 2MHz tCLK */
    k_sleep(K_MSEC(150)); /* Osc should start and DRDY toggle at 250Hz */
}

static inline void ads1298_reset_sequence(const struct ads1298_i2s_config *cfg)
{
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

static int ads1298_probe(const struct device *dev)
{
    uint8_t  id = 0;
    int ret = ads1298_read_reg(dev, ADS1298_REG_ID, 1, &id);
    LOG_DBG("read ID: 0x%02x", id);
    if( ret != 0 || id != 0x92) /* TODO, there are multiple valid IDs*/
    {
        LOG_ERR("Failed to probe ADS1298, ID: 0x%02x with %d", id, ret);
        return -ENODEV;
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
    if (ret) return ret;

    /* Following flow in fig 93 */

    /* The first step calls out 11.1 and fig 105*/
    ret = gpio_pin_configure_dt(&cfg->nrst_gpio, GPIO_OUTPUT_HIGH);
    if (ret) return ret;

    /* Todo, do the other gpios need to tristated before reset incase this is called more than once in a reboot*/
    ads1298_reset_sequence(cfg);

    ret = gpio_pin_configure_dt(&cfg->clksel_gpio, GPIO_OUTPUT_HIGH);
    if (ret) return ret;

    ret = gpio_pin_configure_dt(&cfg->start_conv_gpio, GPIO_OUTPUT_LOW);
    if (ret) return ret;

    ret = gpio_pin_configure_dt(&cfg->npwdn_gpio, GPIO_OUTPUT_HIGH);
    if (ret) return ret;


    /* Initialize device */
	data->dev = dev;
	data->running = false;
//	k_work_init_delayable(&data->data_work, ads1298_data_work_handler);
	
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