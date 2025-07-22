#pragma once

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

/* CONFIG1 register bits */
#define ADS1298_CONFIG1_DR_MASK       0x07
#define ADS1298_CONFIG1_HR            BIT(7)



