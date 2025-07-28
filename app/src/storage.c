#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <string.h>

#include "storage.h"

//LOG_MODULE_REGISTER(storage, CONFIG_APP_LOG_LEVEL);
LOG_MODULE_REGISTER(storage, LOG_LEVEL_DBG);

#define STORAGE_PARTITION_LABEL "EMMC"
#define STORAGE_MOUNT_POINT "/lfs"

/* LittleFS configuration */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage_cfg);
static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage_cfg,
    .storage_dev = (void *)STORAGE_PARTITION_LABEL,
    .mnt_point = STORAGE_MOUNT_POINT,
};

static bool storage_initialized = false;

static const struct gpio_dt_spec emmc_reset = GPIO_DT_SPEC_GET(DT_ALIAS(emmc_reset), gpios);


int init_storage(void)
{
    int rc;
    
    if (storage_initialized) {
        LOG_WRN("Storage already initialized");
        return 0;
    }

    if (!gpio_is_ready_dt(&emmc_reset)) {
        LOG_ERR("eMMC reset GPIO not ready");
        return -ENODEV;
    }

    rc = gpio_pin_configure_dt(&emmc_reset, GPIO_OUTPUT_ACTIVE);
    if (rc != 0) {
        LOG_ERR("Failed to configure eMMC reset pin: %d", rc);
        return rc;
    }
    LOG_INF("eMMC reset pin asserted");

    /* Hold reset for minimum 1ms as per eMMC spec */
    k_sleep(K_MSEC(50));

    gpio_pin_set_dt(&emmc_reset, 0);
    LOG_INF("eMMC reset pin de-asserted");

    /* Wait for eMMC internal initialization - spec requires 74+ clock cycles */
    k_sleep(K_MSEC(200));

    /* Initialize disk access */
    rc = disk_access_init(STORAGE_PARTITION_LABEL);
    if (rc != 0) {
        LOG_ERR("Storage init failed: %d", rc);
        return rc;
    }

    /* Mount the filesystem */
    rc = fs_mount(&lfs_storage_mnt);
    if (rc != 0) {
        LOG_ERR("LittleFS mount failed: %d", rc);

        /* TODO: Remove this when there is a better system around it*/
        /* Try to format the filesystem if mount fails */
        LOG_INF("Attempting to format eMMC with LittleFS...");
        rc = fs_mkfs(FS_LITTLEFS, (uintptr_t)STORAGE_PARTITION_LABEL, NULL, 0);
        if (rc != 0) {
            LOG_ERR("LittleFS format failed: %d", rc);
            return rc;
        }
        
        /* Try mounting again after format */
        rc = fs_mount(&lfs_storage_mnt);
        if (rc != 0) {
            LOG_ERR("LittleFS mount failed after format: %d", rc);
            return rc;
        }
    }

    storage_initialized = true;
    LOG_INF("eMMC storage with LittleFS mounted at %s", STORAGE_MOUNT_POINT);
    
    storage_test();
    
    return 0;
}

int storage_test(void)
{
    struct fs_file_t file;
    int rc;
    const char *test_file = STORAGE_MOUNT_POINT "/test.txt";
    const char *test_data = "Hello from eMMC LittleFS!\n";
    char read_buffer[64];

    if (!storage_initialized) {
        LOG_ERR("Storage not initialized");
        return -EINVAL;
    }

    /* Initialize file object */
    fs_file_t_init(&file);

    /* Write test */
    rc = fs_open(&file, test_file, FS_O_CREATE | FS_O_WRITE);
    if (rc < 0) {
        LOG_ERR("Failed to create test file: %d", rc);
        return rc;
    }

    rc = fs_write(&file, test_data, strlen(test_data));
    if (rc < 0) {
        LOG_ERR("Failed to write test file: %d", rc);
        fs_close(&file);
        return rc;
    }

    fs_close(&file);
    LOG_INF("Test file written successfully");

    /* Read test */
    rc = fs_open(&file, test_file, FS_O_READ);
    if (rc < 0) {
        LOG_ERR("Failed to open test file for reading: %d", rc);
        return rc;
    }

    rc = fs_read(&file, read_buffer, sizeof(read_buffer) - 1);
    if (rc < 0) {
        LOG_ERR("Failed to read test file: %d", rc);
        fs_close(&file);
        return rc;
    }

    read_buffer[rc] = '\0';
    fs_close(&file);

    LOG_INF("Test file content: %s", read_buffer);

    /* Delete test file */
    rc = fs_unlink(test_file);
    if (rc != 0) {
        LOG_WRN("Failed to delete test file: %d", rc);
    }

    return 0;
}

bool is_storage_ready(void)
{
    return storage_initialized;
}

const char* get_storage_mount_point(void)
{
    return STORAGE_MOUNT_POINT;
}