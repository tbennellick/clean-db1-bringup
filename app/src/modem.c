#include "debug_leds.h"
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem, CONFIG_LOG_DEFAULT_LEVEL);

/* NB, this module requires DTR to be strapped low (R56 pad on translater side) */
/* To be fixed in the next hardware rev.*/

#define MODEM_RESET DT_ALIAS(modem_reset)
static const struct gpio_dt_spec pin_lte_reset = GPIO_DT_SPEC_GET(MODEM_RESET, gpios);

#define MODEM_ON DT_ALIAS(modem_on)
static const struct gpio_dt_spec pin_lte_n_power_on = GPIO_DT_SPEC_GET(MODEM_ON, gpios);

#define PIN_HS_USB_SELECT DT_ALIAS(hs_usb_sel)
static const struct gpio_dt_spec pin_hs_usb_select = GPIO_DT_SPEC_GET(PIN_HS_USB_SELECT, gpios);

#define UART_DEVICE_NODE DT_ALIAS(modem_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define MSG_SIZE 32

static char rx_buf[MSG_SIZE];
static int rx_buf_pos = 0;

void frame_on_newline(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}
	/* From samples/drivers/uart/echo_bot/src/main.c buggy but good enough for PoC*/
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if (c == '\r') {
			break;
		}
		if (c == '\n') {
			if (rx_buf_pos == 0) {
				break;
			}
			rx_buf[rx_buf_pos] = '\0';
			LOG_INF("Modem receive: %s", rx_buf);
			rx_buf_pos = 0;
			rx_buf[0] = '\0';
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		} else {
			LOG_ERR("Modem receive buffer overflow");
			rx_buf_pos = 0;
			rx_buf[0] = '\0';
		}
	}
}

/* If you came here trying to debug non-deterministic start up problems with the
 * modem, perhaps it is a bench PSU current limit? The modem has high in-rush */
void modem_reset()
{
	gpio_pin_set_dt(&pin_lte_n_power_on, 0);
	k_sleep(K_MSEC(200)); /* Min 150ms DS 4.2.9*/
	gpio_pin_set_dt(&pin_lte_n_power_on, 1);

	k_sleep(K_MSEC(50)); /* No DS spec */

	gpio_pin_set_dt(&pin_lte_reset, 1);
	k_sleep(K_MSEC(60)); /* Min 0.05s DS 4.2.10 */
	gpio_pin_set_dt(&pin_lte_reset, 0);
	k_sleep(K_MSEC(50)); /* No DS spec */
}

void uart_send(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void modem_test(void)
{
	LOG_INF("Modem send AT+CGMM");
	uart_send("AT+CGMM\r\n");
	k_sleep(K_MSEC(100));
}

/* This is the useful bits pulled out of dev-modem branch, untested*/
int init_modem(void)
{
	if (!gpio_is_ready_dt(&pin_lte_reset)) {
		k_fatal_halt(-100);
	}
	if (!gpio_is_ready_dt(&pin_lte_n_power_on)) {
		k_fatal_halt(-100);
	}
	if (!gpio_is_ready_dt(&pin_hs_usb_select)) {
		k_fatal_halt(-100);
	}

	if (0 != gpio_pin_configure_dt(&pin_lte_reset, GPIO_OUTPUT_INACTIVE)) {
		k_fatal_halt(-101);
	}
	if (0 != gpio_pin_configure_dt(&pin_lte_n_power_on, GPIO_OUTPUT_ACTIVE)) {
		k_fatal_halt(-101);
	}
	if (0 != gpio_pin_configure_dt(&pin_hs_usb_select, GPIO_OUTPUT_INACTIVE)) {
		k_fatal_halt(-101);
	}

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return -ENODEV;
	}

	int ret = uart_irq_callback_user_data_set(uart_dev, frame_on_newline, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	uart_irq_rx_enable(uart_dev);

	modem_reset();

	modem_test();
	k_sleep(K_MSEC(1000));
	uart_irq_rx_disable(uart_dev);

	return 0;
}
