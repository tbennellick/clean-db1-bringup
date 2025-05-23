#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "gpio_init.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(exg, LOG_LEVEL_DBG);

#define EXG_CLKSEL DT_ALIAS(exg_clksel)
static const struct gpio_dt_spec exg_clksel = GPIO_DT_SPEC_GET(EXG_CLKSEL, gpios);
#define EXG_NRST DT_ALIAS(exg_nrst)
static const struct gpio_dt_spec exg_nrst = GPIO_DT_SPEC_GET(EXG_NRST, gpios);
#define EXG_NPWDN DT_ALIAS(exg_npwdn)
static const struct gpio_dt_spec exg_npwdn = GPIO_DT_SPEC_GET(EXG_NPWDN, gpios);

#define EXG_CS_TEMP DT_ALIAS(exg_cs_temp)
static const struct gpio_dt_spec exg_cs_temp = GPIO_DT_SPEC_GET(EXG_CS_TEMP, gpios);

void init_exg(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(ti_ads1298);

    safe_init_gpio(&exg_clksel, GPIO_OUTPUT_HIGH);
    safe_init_gpio(&exg_nrst, GPIO_OUTPUT_HIGH);
    safe_init_gpio(&exg_npwdn, GPIO_OUTPUT_HIGH);
    safe_init_gpio(&exg_cs_temp, GPIO_OUTPUT_HIGH);
	if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready\n", dev->name);
        k_sleep(K_MSEC(200));
    }
    LOG_WRN("Device ready: %s\n", dev->name);
    k_sleep(K_MSEC(200));

    struct sensor_value p;
    while (1)
    {

        gpio_pin_set_dt(&exg_cs_temp, 0);
        sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
        gpio_pin_set_dt(&exg_cs_temp, 1);

        sensor_channel_get(dev, SENSOR_CHAN_PRESS, &p);

        LOG_INF("pressure: %d.%06d\n", p.val1, p.val2);
        k_sleep(K_MSEC(500));
    }

}
