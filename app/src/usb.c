#include "zephyr/drivers/gpio.h"

#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/usb/usbd.h>
LOG_MODULE_REGISTER(usb, LOG_LEVEL_DBG);

/* In future this needs replacing*/
#include <sample_usbd.h>

static struct usbd_context *usb_context;
USBD_DEFINE_MSC_LUN(mmc, "SD2", "Zephyr", "eMMC", "0.00");

#define HS_USB_SEL DT_ALIAS(hs_usb_sel)
static const struct gpio_dt_spec hs_usb_sel = GPIO_DT_SPEC_GET(HS_USB_SEL, gpios);

static void usb_msg_callback(struct usbd_context *const ctx, const struct usbd_msg *const msg) {
	switch (msg->type) {
	case USBD_MSG_VBUS_READY:
		LOG_INF("********** VBUS READY - USB cable connected **********");
		break;

	case USBD_MSG_VBUS_REMOVED:
		LOG_INF("********** VBUS REMOVED - USB cable disconnected **********");
		break;

	default:
		//		LOG_ERR("USB message: %s", usbd_msg_type_string(msg->type));
		break;
	}
}

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

	/* Register callback to receive USB events */
	int err = usbd_msg_register_cb(usb_context, usb_msg_callback);
	if (err) {
		LOG_ERR("Failed to register USB callback: %d", err);
		return err;
	}

	err = usbd_enable(usb_context);
	if (err) {
		LOG_ERR("Failed to enable device support");
		return err;
	}

	LOG_INF("USB initialized - watching for VBUS events");
	return 0;
}