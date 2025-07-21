#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "exg_conf.h"
#include "gpio_init.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(exg, LOG_LEVEL_DBG);



void init_exg(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(ti_ads1298);
    struct exg_config exg_cfg;

	if (!device_is_ready(dev)) {
        LOG_ERR("Device %s is not ready\n", dev->name);
        k_sleep(K_MSEC(200));
    }
    LOG_DBG("Device ready: %s\n", dev->name);
}
