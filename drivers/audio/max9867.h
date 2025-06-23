#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>

/* MAX9867 Register Addresses */
#define MAX9867_STATUS           0x00
#define MAX9867_JACK_STATUS      0x01
#define MAX9867_AUX_HIGH         0x02
#define MAX9867_AUX_LOW          0x03
#define MAX9867_INTERRUPT_ENABLE 0x04
#define MAX9867_SYS_CLK          0x05
#define MAX9867_SACLK_CTRL_HI    0x06
#define MAX9867_SACLK_CTRL_LO    0x07
#define MAX9867_DAI_IF_MODE1     0x08
#define MAX9867_DAI_IF_MODE2     0x09
#define MAX9867_CODEC_FILTER     0x0A
#define MAX9867_SIDETONE         0x0B
#define MAX9867_DAC_LEVEL        0x0C
#define MAX9867_ADC_LEVEL        0x0D
#define MAX9867_LINE_IN_LEV_L    0x0E
#define MAX9867_LINE_IN_LEV_R    0x0F
#define MAX9867_VOL_L            0x10
#define MAX9867_VOL_R            0x11
#define MAX9867_MIC_GAIN_L       0x12
#define MAX9867_MIC_GAIN_R       0x13
#define MAX9867_ADC_IN_CONF      0x14
#define MAX9867_MIC_CONF         0x15
#define MAX9867_MODE             0x16
#define MAX9867_REVISION         0xFF

#define MAX9867_TDM_MODE_BIT (1<<2)


struct max9867_config {
	struct i2c_dt_spec i2c;
	const struct device *mclk_dev;
	uint32_t mclk_freq;
};

struct max9867_data {
	uint32_t sample_rate;
	uint8_t word_size;
	uint8_t channels;
};
