
#include "zephyr/drivers/regulator.h"
#include "zephyr/random/random.h"

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

#include <ff.h>

LOG_MODULE_REGISTER(storage, LOG_LEVEL_DBG);

#define DISK_DRIVE_NAME "SD2"
#define DISK_MOUNT_PT   "/" DISK_DRIVE_NAME ":"

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

#define FS_RET_OK FR_OK

#define MAX_PATH          128
#define SOME_FILE_NAME    "some.dat"
#define SOME_DIR_NAME     "some"
#define SOME_REQUIRED_LEN MAX(sizeof(SOME_FILE_NAME), sizeof(SOME_DIR_NAME))

static const char *disk_mount_pt = DISK_MOUNT_PT;

static int lsdir(const char *path) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	int count = 0;

	fs_dir_t_init(&dirp);
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	for (;;) {
		res = fs_readdir(&dirp, &entry);
		if (res || entry.name[0] == 0) { /* entry.name[0] == 0 means end-of-dir */
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s", entry.name);
		} else {
			LOG_INF("[FILE] %s (size = %zu)", entry.name, entry.size);
		}
		count++;
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);
	if (res == 0) {
		res = count;
	}

	return res;
}

static bool create_some_entries(const char *base_path) {
	char path[MAX_PATH];
	struct fs_file_t file;
	int base = strlen(base_path);

	fs_file_t_init(&file);

	if (base >= (sizeof(path) - SOME_REQUIRED_LEN)) {
		LOG_ERR("Not enough concatenation buffer to create file paths");
		return false;
	}

	LOG_INF("Creating some dir entries in %s", base_path);
	strncpy(path, base_path, sizeof(path));

	path[base++] = '/';
	path[base] = 0;
	// strcat(&path[base], SOME_FILE_NAME);
	uint16_t num = sys_rand16_get();

	snprintf(path, sizeof(path), "%s/%d", base_path, num);

	if (fs_open(&file, path, FS_O_CREATE) != 0) {
		LOG_ERR("Failed to create file %s", path);
		return false;
	}
	uint8_t d[] = "Hello World\n";
	fs_write(&file, d, sizeof(d));
	fs_close(&file);

	path[base] = 0;
	strcat(&path[base], SOME_DIR_NAME);

	if (fs_mkdir(path) != 0) {
		LOG_ERR("Failed to create dir %s", path);
		/* If code gets here, it has at least successes to create the
		 * file so allow function to return true.
		 */
	}
	return true;
}

int setup_disk(void) {
	static const char *disk_pdrv = DISK_DRIVE_NAME;
	uint64_t memory_size_mb;
	uint32_t block_count;
	uint32_t block_size;

	int err = disk_access_ioctl(disk_pdrv, DISK_IOCTL_CTRL_INIT, NULL);
	if (err != 0) {
		LOG_ERR("Storage init ERROR!");
		return err;
	}

	err = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
	if (err != 0) {
		LOG_ERR("Unable to get sector count");
		return err;
	}
	LOG_INF("Block count %u", block_count);

	err = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
	if (err) {
		LOG_ERR("Unable to get sector size");
		return err;
	}

	LOG_INF("Sector size %u", block_size);

	memory_size_mb = (uint64_t)block_count * block_size;
	LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));

	err = disk_access_ioctl(disk_pdrv, DISK_IOCTL_CTRL_DEINIT, NULL);
	if (err) {
		LOG_ERR("Storage deinit ERROR!");
		return err;
	}
	return 0;
}

int init_storage(void) {
	int ret = setup_disk();
	if (ret != 0) {
		LOG_ERR("Storage init failed");
		return ret;
	}

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);

	if (res != FS_RET_OK) {
		LOG_INF("Problem mounting disk %d", res);
		return res;
	}
	create_some_entries(disk_mount_pt);
	lsdir(disk_mount_pt);

	fs_unmount(&mp);
	return 0;
}
