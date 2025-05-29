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
/* That is 4*514e-9 = 2.056us*/
/* There is a minimum SCK: tSCLK < (tDR – 4tCLK) / (NBITS × NCHANNELS + 24)*/
/* THis is ~50kHz with boot settings */
/* SPI clock max = 20MHz (tsclk>50ns) */
/* For multibyte commands, a 4 tCLK period must separate the end of one byte (or opcode) and the next. */

#define EXG_CLKSEL DT_ALIAS(exg_clksel)
static const struct gpio_dt_spec exg_clksel = GPIO_DT_SPEC_GET(EXG_CLKSEL, gpios);
#define EXG_NRST DT_ALIAS(exg_nrst)
static const struct gpio_dt_spec exg_nrst = GPIO_DT_SPEC_GET(EXG_NRST, gpios);
#define EXG_NPWDN DT_ALIAS(exg_npwdn)
static const struct gpio_dt_spec exg_npwdn = GPIO_DT_SPEC_GET(EXG_NPWDN, gpios);
#define EXG_START_CONV DT_ALIAS(exg_start_conv)
static const struct gpio_dt_spec exg_start_conv = GPIO_DT_SPEC_GET(EXG_START_CONV, gpios);


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

#define ADS1298_REG_ID 0x00
#define ADS1298_REG_CONFIG1 0x01
#define ADS1298_REG_CONFIG2 0x02
#define ADS1298_REG_CONFIG3 0x03
#define ADS1298_REG_LOFF 0x04
#define ADS1298_REG_CH1SET 0x05
#define ADS1298_REG_CH2SET 0x06
#define ADS1298_REG_CH3SET 0x07
#define ADS1298_REG_CH4SET 0x08
#define ADS1298_REG_CH5SET 0x09
#define ADS1298_REG_CH6SET 0x0A
#define ADS1298_REG_CH7SET 0x0B
#define ADS1298_REG_CH8SET 0x0C
#define ADS1298_REG_RLD_SENSP 0x0D
#define ADS1298_REG_RLD_SENSN 0x0E
#define ADS1298_REG_LOFF_SENSP 0x0F
#define ADS1298_REG_LOFF_SENSN 0x10
#define ADS1298_REG_LOFF_FLIP 0x11
#define ADS1298_REG_LOFF_STATP 0x12
#define ADS1298_REG_LOFF_STATN 0x13
#define ADS1298_REG_GPIO 0x14
#define ADS1298_REG_PACE 0x15
#define ADS1298_REG_RESP 0x16
#define ADS1298_REG_CONFIG4 0x17
#define ADS1298_REG_WCT1 0x18
#define ADS1298_REG_WCT2 0x19



#define MAX_REG_RW_BYTES 256
#define REG_WRITE_OPCODE_LEN 2 /* 1 byte for command and reg  + 1 byte for length */
#define EXG_MAX_SPI_LEN (MAX_REG_RW_BYTES+REG_WRITE_OPCODE_LEN)
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

    int ret  = spi_transceive(cfg->bus.bus, &cfg->bus.config , txbs, rxbs);

    if (ret !=0) {
        LOG_ERR("Failed to transact with SPI device (%d)", ret);
    }
    return ret;
}

__maybe_unused
static int ads1298_wakeup(const struct device *dev)
{
    actual_tx_buffer[0] = ADS1298_CMD_WAKEUP;
    tx_bufs[0].len =1;

    int ret = ads1298_transact(dev, &tx, NULL);

    k_busy_wait(1000);

    return ret;

}

__maybe_unused
static int ads1298_read_reg(const struct device *dev, uint8_t reg, uint16_t len, uint8_t *val)
{

    if (reg > 0x1F) {
        LOG_ERR("Invalid register %d", reg);
        return -EINVAL;
    }
    if (len > MAX_REG_RW_BYTES || len < 1) {
        LOG_ERR("Invalid length %d", len);
        return -EINVAL;
    }

    actual_tx_buffer[0] = ADS1298_CMD_RREG | (reg & 0x1F); // Read command
    actual_tx_buffer[1] = len - 1; // Length of registers to read (0-31)
    tx_bufs[0].len = len + REG_WRITE_OPCODE_LEN; // 1 byte for command + len bytes for data
    rx_bufs[0].len = tx_bufs[0].len;

    int ret = ads1298_transact(dev, &tx, &rx);

    LOG_HEXDUMP_DBG(rx_bufs[0].buf, rx_bufs[0].len, "read");
    memcpy(val, &rx_bufs[0].buf[2], len);
	return ret;
}

__maybe_unused
static int ads1298_write_reg(const struct device *dev, uint8_t reg, uint16_t len, uint8_t *val)
{

    if (reg > 0x1F) {
        LOG_ERR("Invalid register %d", reg);
        return -EINVAL;
    }
    if (len > MAX_REG_RW_BYTES || len < 1) {
        LOG_ERR("Invalid length %d", len);
        return -EINVAL;
    }

    actual_tx_buffer[0] = ADS1298_CMD_WREG | (reg & 0x1F);
    actual_tx_buffer[1] = len - 1; /* -1 from datasheet*/
    memcpy(&actual_tx_buffer[2], val, len);
    tx_bufs[0].len = len + REG_WRITE_OPCODE_LEN;

    return ads1298_transact(dev, &tx, NULL);
}


__maybe_unused
static int ads1298_set_sdatac_mode(const struct device *dev)
{

    actual_tx_buffer[0] = ADS1298_CMD_SDATAC;
    tx_bufs[0].len = 1;

    int ret = ads1298_transact(dev, &tx, NULL);
    LOG_DBG("Set SDATAC mode ret: %d", ret);

    return ret;
}



static int ads1298_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
//	struct ads1298_data *drv_data = dev->data;
//	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

//    ret = ads1298_wakeup(dev);
//    if (ret) {
//        return ret;
//    }



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


void hack_safe_init_gpio(const struct gpio_dt_spec *spec, int flags)
{
    if (!gpio_is_ready_dt(spec))
    {
        LOG_ERR("Gpio not ready %s %d",spec->port->name, spec->pin);
    }
    else
    {
        int res = gpio_pin_configure_dt(spec, flags);
        if (res !=0)
        {
            LOG_ERR("Cant init %s %d",spec->port->name, spec->pin);
        }
    }
}

static inline void wait_tpor(void)
{
    /* tPOR =  2^18 tCLK periods = 0.131s @ 2MHz tCLK */
    k_sleep(K_MSEC(150)); /* Osc should start and DRDY toggle at 250Hz */
}

static inline void exg_reset(void)
{
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

static int ads1298_init(const struct device *dev)
{
	const struct ads1298_dev_config *cfg = dev->config;
    uint8_t  buf[7] = {0};
    int ret;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

    /* Following flow in fig 93 */

    /* First step calls out 11.1 and fig 105*/
    hack_safe_init_gpio(&exg_nrst, GPIO_OUTPUT_HIGH);
    exg_reset();

    hack_safe_init_gpio(&exg_clksel, GPIO_OUTPUT_HIGH); /* High = internal clock*/
//    hack_safe_init_gpio(&exg_start_conv, GPIO_OUTPUT_HIGH); /* High to observe nDRDY as sign of life*/
    hack_safe_init_gpio(&exg_start_conv, GPIO_OUTPUT_LOW);

    hack_safe_init_gpio(&exg_npwdn, GPIO_OUTPUT_HIGH);
    exg_reset();

    ret = ads1298_set_sdatac_mode(dev);
    buf[0] = 0xc0;
    ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG3, 1, buf);
    buf[0] = 0x86;
    ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG1, 1, buf);
    buf[0] = 0x00;
    ret = ads1298_write_reg(dev, ADS1298_REG_CONFIG2, 1, buf);

    buf[0] = 0x01;
    ret = ads1298_write_reg(dev, ADS1298_REG_CH1SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH2SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH3SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH4SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH5SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH6SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH7SET, 1, buf);
    ret = ads1298_write_reg(dev, ADS1298_REG_CH8SET, 1, buf);

#define ID_CHECK
#ifdef ID_CHECK
    buf[0] = 0x00;
    ret = ads1298_read_reg(dev, ADS1298_REG_ID, 1, buf);
    LOG_WRN("read ID: 0x%02x", buf[0]);
#endif

    gpio_pin_set_dt(&exg_start_conv, 1);

    k_sleep(K_MSEC(500));
//    if (ret) {
//        return ret;
//    }
//

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
