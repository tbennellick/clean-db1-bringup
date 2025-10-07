#include "zephyr/drivers/gpio.h"

#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/usb/class/usbd_msc.h>
LOG_MODULE_REGISTER(usb, LOG_LEVEL_DBG);

/* In future this needs replacing*/
#include <sample_usbd.h>

static struct usbd_context *usb_context;
USBD_DEFINE_MSC_LUN(mmc, "SD2", "Zephyr", "eMMC", "0.00");

#define HS_USB_SEL DT_ALIAS(hs_usb_sel)
static const struct gpio_dt_spec hs_usb_sel = GPIO_DT_SPEC_GET(HS_USB_SEL, gpios);

int init_usb(void) {

	int rc = disk_access_init("SD2");
	if (rc) {
		LOG_ERR("Failed to init disk: %d", rc);
	}

	/* Configure USB mux - active low, set to 0 to enable */
	gpio_pin_configure_dt(&hs_usb_sel, GPIO_OUTPUT);
	gpio_pin_set_dt(&hs_usb_sel, 0);

	usb_context = sample_usbd_init_device(NULL);
	if (usb_context == NULL) {
		LOG_ERR("Failed to initialize USB context");
		return -ENODEV;
	}

	int err = usbd_enable(usb_context);
	if (err) {
		LOG_ERR("Failed to enable device support");
		return err;
	}

	return 0;
}