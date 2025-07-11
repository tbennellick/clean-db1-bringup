#pragma once

#include "max9867.h"
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>

struct max9867_config {
  struct i2c_dt_spec i2c;
  int clock_source;
  int mclk_default;
  const struct device *mclk_dev;
  clock_control_subsys_t mclk_name;
};

struct max9867_data {
  uint32_t sample_rate;
  max9867_input_t input_source;
};

/* MAX9867 Register Addresses */
typedef enum {
  MAX9867_STATUS = 0x00,
  MAX9867_JACK_STATUS = 0x01,
  MAX9867_AUX_HIGH = 0x02,
  MAX9867_AUX_LOW = 0x03,
  MAX9867_INTERRUPT_ENABLE = 0x04,
  MAX9867_SYS_CLK = 0x05,
  MAX9867_SACLK_CTRL_HI = 0x06,
  MAX9867_SACLK_CTRL_LO = 0x07,
  MAX9867_DAI_IF_MODE1 = 0x08,
  MAX9867_DAI_IF_MODE2 = 0x09,
  MAX9867_CODEC_FILTER = 0x0A,
  MAX9867_SIDETONE = 0x0B,
  MAX9867_DAC_LEVEL = 0x0C,
  MAX9867_ADC_LEVEL = 0x0D,
  MAX9867_LINE_IN_LEV_L = 0x0E,
  MAX9867_LINE_IN_LEV_R = 0x0F,
  MAX9867_VOL_L = 0x10,
  MAX9867_VOL_R = 0x11,
  MAX9867_MIC_GAIN_L = 0x12,
  MAX9867_MIC_GAIN_R = 0x13,
  MAX9867_ADC_IN_CONF = 0x14,
  MAX9867_MIC_CONF = 0x15,
  MAX9867_MODE = 0x16,
  MAX9867_SYS_SHDN = 0x17,
  MAX9867_REVISION = 0xFF
} max9867_reg_t;

#define MAX9867_SYS_CLK_PSCLK_MASK (0x03 << 4)
#define MAX9867_SYS_CLK_PSCLK_OFF (0x00 << 4)
#define MAX9867_SYS_CLK_PSCLK_10_20MHZ (0x01 << 4)
#define MAX9867_SYS_CLK_FREQ_MASK (0x0F)
#define MAX9867_SYS_CLK_PLL_EN_BIT (1 << 7)
#define MAX9867_SACLK_CTRL_HI_NI_MASK (0x7F)
#define MAX9867_DAI_IF_MODE1_TDM_MODE_BIT (1 << 2)
#define MAX9867_DAI_IF_MODE1_MAS (1 << 7)
#define MAX9867_DAI_IF_MODE1_WCI (1 << 6)
#define MAX9867_DAI_IF_MODE1_BCI (1 << 5)
#define MAX9867_DAI_IF_MODE1_nSDODLY (1 << 4)
#define MAX9867_DAI_IF_MODE1_DLY (1 << 3)
#define MAX9867_DAI_IF_MODE1_HIZOFF (1 << 2)
#define MAX9867_DAI_IF_MODE2_LVOLFIX (1 << 4)
#define MAX9867_DAI_IF_MODE2_DMONO (1 << 3)
#define MAX9867_DAI_IF_MODE2_BSEL_MASK (0x07 << 0)
#define MAX9867_DAI_IF_MODE2_BSEL_NORM (0x02 << 0)
#define MAX9867_DAC_LEVEL_DACMUTE (1 << 7)
#define MAX9867_ADC_LEVEL_0DB (0x03 << 0)
#define MAX9867_LINE_IN_LEV_X_MUTE (1 << 6)
#define MAX9867_LINE_IN_LEV_X_GAIN_MASK 0x0F

#define MAX9867_VOL_X_MUTE (1 << 6)
#define MAX9867_MIC_GAIN_X_PRE_MASK (0x3 << 5)
#define MAX9867_MIC_GAIN_X_PRE_0DB (0x01 << 5)
#define MAX9867_MIC_GAIN_X_0DB (0x14)
#define MAX9867_ADC_IN_CONF_MIC_ONLY (1 << 6)
#define MAX9867_ADC_IN_CONF_LINE_ONLY (1 << 7)

#define MAX9867_SYS_SHDN_SHDN_nSHDN (1 << 7)
#define MAX9867_SYS_SHDN_SHDN_LNLEN (1 << 6)
#define MAX9867_SYS_SHDN_SHDN_LNREN (1 << 5)
#define MAX9867_SYS_SHDN_SHDN_DALEN (1 << 3)
#define MAX9867_SYS_SHDN_SHDN_DAREN (1 << 2)
#define MAX9867_SYS_SHDN_SHDN_ADLEN (1 << 1)
#define MAX9867_SYS_SHDN_SHDN_ADREN (1 << 0)
