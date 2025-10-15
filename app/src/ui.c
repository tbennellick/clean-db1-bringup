#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ui, LOG_LEVEL_DBG);

#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);

bool left_button(void) {
	return gpio_pin_get_dt(&button0);
}

bool right_button(void) {
	return gpio_pin_get_dt(&button1);
}

int init_ui(void) {
	int ret;

	if (!gpio_is_ready_dt(&button0)) {
		LOG_ERR("Button 0 device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&button1)) {
		LOG_ERR("Button 1 device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure button 0: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure button 1: %d", ret);
		return ret;
	}

	return 0;
}