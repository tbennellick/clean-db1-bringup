#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>

#include "zephyr/drivers/gpio.h"
#include <zephyr/storage/disk_access.h>

// #include <ff.h>

LOG_MODULE_REGISTER(main);

// #define STORAGE_PARTITION			fatfs_storage
// #define STORAGE_PARTITION_ID		FIXED_PARTITION_ID(STORAGE_PARTITION)


static struct usbd_context *usb_context;
USBD_DEFINE_MSC_LUN(mmc, "SD2", "Zephyr", "eMMC", "0.00");

int main(void)
{
	int err;

	// setup_disk();

	int rc = disk_access_init("SD2");
	if (rc) {
		LOG_ERR("Failed to init disk: %d", rc);
	}


	usb_context = sample_usbd_init_device(NULL);
	if (usb_context == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	err = usbd_enable(usb_context);
	if (err) {
		LOG_ERR("Failed to enable device support");
		return err;
	}
	return 0;
}


#ifdef CONFIG_BOARD_DB1
#define POWER_VEN_SYS_BASE DT_ALIAS(power_ven_sys_base)
static const struct gpio_dt_spec ven_sys_base = GPIO_DT_SPEC_GET(POWER_VEN_SYS_BASE, gpios);
#define HS_USB_SEL DT_ALIAS(hs_usb_sel)
static const struct gpio_dt_spec hs_usb_sel = GPIO_DT_SPEC_GET(HS_USB_SEL, gpios);
#define POWER_VEN_STORAGE DT_ALIAS(power_ven_storage)
static const struct gpio_dt_spec ven_storage = GPIO_DT_SPEC_GET(POWER_VEN_STORAGE, gpios);


static int auto_early_power_up(void) {
	gpio_pin_configure_dt(&ven_sys_base, GPIO_OUTPUT_LOW);
	gpio_pin_set_dt(&ven_sys_base, 1);

	/* Configure USB mux - active low, set to 0 to enable */
	gpio_pin_configure_dt(&hs_usb_sel, GPIO_OUTPUT);
	gpio_pin_set_dt(&hs_usb_sel, 0);

	gpio_pin_configure_dt(&ven_storage, GPIO_OUTPUT_LOW);
	gpio_pin_set_dt(&ven_storage, 1);

	return 0;
}

SYS_INIT(auto_early_power_up, POST_KERNEL, 50);
#endif
