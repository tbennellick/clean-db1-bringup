/*
 * USB Mass Storage Device with RAM Disk for Speed Testing
 * Based on Zephyr USB MSD sample
 * Returns empty blocks on read to test pure USB transfer performance
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/logging/log.h>

#include "zephyr/drivers/gpio.h"
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define RAMDISK_VOLUME_NAME "RAM"
#define RAMDISK_SECTOR_SIZE 512
#define RAMDISK_SECTOR_COUNT (2097152 / 2)
#define RAMDISK_TOTAL_SIZE (RAMDISK_SECTOR_SIZE * RAMDISK_SECTOR_COUNT)

/* Track performance metrics */
static uint32_t write_count = 0;
static uint64_t total_write_bytes = 0;

/* RAM disk interface functions */
static int ramdisk_access_status(struct disk_info *disk) {
  return DISK_STATUS_OK;
}

static int ramdisk_access_init(struct disk_info *disk) {
  LOG_DBG("RAM disk initialized: %u sectors, %u bytes per sector",
          RAMDISK_SECTOR_COUNT, RAMDISK_SECTOR_SIZE);
  return 0;
}

static int ramdisk_access_read(struct disk_info *disk, uint8_t *data_buf,
                               uint32_t start_sector, uint32_t num_sectors) {
  /* Performance optimization: return zeros without accessing storage */
  memset(data_buf, 0, num_sectors * RAMDISK_SECTOR_SIZE);

  return 0;
}

static int ramdisk_access_write(struct disk_info *disk, const uint8_t *data_buf,
                                uint32_t start_sector, uint32_t num_sectors) {
  uint32_t offset = start_sector * RAMDISK_SECTOR_SIZE;
  uint32_t size = num_sectors * RAMDISK_SECTOR_SIZE;

  /* Bounds checking */
  if (offset + size > RAMDISK_TOTAL_SIZE) {
    LOG_ERR("Write beyond disk bounds");
    return -EINVAL;
  }

  /* Update performance counters */
  write_count++;
  total_write_bytes += size;

  return 0;
}

static int ramdisk_access_ioctl(struct disk_info *disk, uint8_t cmd,
                                void *buff) {
  switch (cmd) {
  case DISK_IOCTL_CTRL_INIT:
    return 0;
  case DISK_IOCTL_CTRL_DEINIT:
    return 0;
  case DISK_IOCTL_CTRL_SYNC:
    return 0;
  case DISK_IOCTL_GET_SECTOR_COUNT:
    *(uint32_t *)buff = RAMDISK_SECTOR_COUNT;
    return 0;
  case DISK_IOCTL_GET_SECTOR_SIZE:
    *(uint32_t *)buff = RAMDISK_SECTOR_SIZE;
    return 0;
  case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
    *(uint32_t *)buff = 1;
    return 0;
  default:
    return -EINVAL;
  }
}

static const struct disk_operations ramdisk_ops = {
    .init = ramdisk_access_init,
    .status = ramdisk_access_status,
    .read = ramdisk_access_read,
    .write = ramdisk_access_write,
    .ioctl = ramdisk_access_ioctl,
};

static struct disk_info ramdisk_info = {
    .name = RAMDISK_VOLUME_NAME,
    .ops = &ramdisk_ops,
};

/* Initialize RAM disk */
static int ramdisk_init(void) {
  /* Register the disk */
  return disk_access_register(&ramdisk_info);
}

USBD_DEFINE_MSC_LUN(ram, "RAM", "Zephyr", "RAMDisk", "0.00");

int main(void) {
  int ret;

  LOG_INF("USB Mass Storage Device Speed Test");
  LOG_INF("RAM Disk Size: %u KB (%u sectors)", RAMDISK_TOTAL_SIZE / 1024,
          RAMDISK_SECTOR_COUNT);

  /* Enable USB device */
  ret = usb_enable(NULL);
  if (ret != 0) {
    LOG_ERR("Failed to enable USB: %d", ret);
    return ret;
  }

  LOG_INF("USB MSD enabled. Connect to host and run speed tests.");
  LOG_INF("Example: dd if=/dev/sdX of=/dev/null bs=64k count=16");

  return 0;
}

#ifdef CONFIG_BOARD_DB1
#define POWER_VEN_SYS_BASE DT_ALIAS(power_ven_sys_base)
#define HS_USB_SEL DT_ALIAS(hs_usb_sel)

static const struct gpio_dt_spec ven_sys_base = GPIO_DT_SPEC_GET(POWER_VEN_SYS_BASE, gpios);
static const struct gpio_dt_spec hs_usb_sel = GPIO_DT_SPEC_GET(HS_USB_SEL, gpios);

static int auto_early_power_up(void) {
  gpio_pin_configure_dt(&ven_sys_base, GPIO_OUTPUT_LOW);
  gpio_pin_set_dt(&ven_sys_base, 1);

  /* Configure USB mux - active low, set to 0 to enable */
  gpio_pin_configure_dt(&hs_usb_sel, GPIO_OUTPUT);
  gpio_pin_set_dt(&hs_usb_sel, 1);

  return 0;
}

SYS_INIT(auto_early_power_up, POST_KERNEL, 50);
#endif


SYS_INIT(ramdisk_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
