#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "debug_leds.h"
#include "fuel_gauge.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fg, LOG_LEVEL_INF);

struct sensor_value val;

typedef struct {
    enum sensor_channel chan;
    const char *name;
} sensor_channel_info_t;

static const sensor_channel_info_t gauge_channels[] = {
        { SENSOR_CHAN_GAUGE_VOLTAGE, "gauge_voltage" },
        { SENSOR_CHAN_GAUGE_AVG_CURRENT, "gauge_avg_current" },
        { SENSOR_CHAN_GAUGE_STDBY_CURRENT, "gauge_stdby_current" },
        { SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT, "gauge_max_load_current" },
        { SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, "gauge_state_of_charge" },
        { SENSOR_CHAN_GAUGE_STATE_OF_HEALTH, "gauge_state_of_health" },
        { SENSOR_CHAN_GAUGE_AVG_POWER, "gauge_avg_power" },
        { SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY, "gauge_full_charge_capacity" },
        { SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY, "gauge_remaining_charge_capacity" },
        { SENSOR_CHAN_GAUGE_TEMP, "gauge_temp" }
};

void init_fuel_gauge(void)
{
    const struct device *const dev = DEVICE_DT_GET_ONE(ti_bq274xx);
    if (!device_is_ready(dev)) {
        LOG_ERR("Fuel gauge is not ready");
        return;
    }

    int status = sensor_sample_fetch(dev);
    if (status < 0) {
        LOG_ERR("Failed to fetch sensor sample (ALL): %d", status);
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(gauge_channels); i++) {
        status = sensor_channel_get(dev, gauge_channels[i].chan, &val);
        if (status < 0) {
            LOG_ERR("Failed to get %s", gauge_channels[i].name);
            continue;
        }
        LOG_INF("%s: %g", gauge_channels[i].name, sensor_value_to_double(&val));
    }
}
