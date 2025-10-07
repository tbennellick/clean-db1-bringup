
#include "boot_id.h"
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

#define PERF_FILE_SIZE_MB 10
#define PERF_FILE_NAME    "perf_test"
#define WRITE_CHUNK_SIZE  4096
static uint8_t test_buf[WRITE_CHUNK_SIZE];

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

#define MAX_PATH 128
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

int measure_write_speed(const char *base_path) {
	char path[MAX_PATH];
	struct fs_file_t file;
	int64_t start_time, end_time;
	uint64_t total_bytes = PERF_FILE_SIZE_MB * 1024 * 1024;
	size_t bytes_written = 0;
	int ret;

	fs_file_t_init(&file);

	/* Fill write buffer with pattern */
	for (int i = 0; i < WRITE_CHUNK_SIZE; i++) {
		test_buf[i] = i & 0xFF;
	}

	snprintf(path, sizeof(path), "%s/%s", base_path, PERF_FILE_NAME);

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("Failed to open file for write test: %d", ret);
		return ret;
	}

	start_time = k_uptime_get();

	while (bytes_written < total_bytes) {
		size_t to_write = MIN(WRITE_CHUNK_SIZE, total_bytes - bytes_written);
		ssize_t written = fs_write(&file, test_buf, to_write);
		if (written < 0) {
			LOG_ERR("Write failed: %d", (int)written);
			fs_close(&file);
			return written;
		}
		bytes_written += written;
	}

	fs_close(&file);
	end_time = k_uptime_get();

	int64_t duration_ms = end_time - start_time;
	if (duration_ms > 0) {
		uint32_t speed_kbps = ((uint64_t)total_bytes * 1000) / (duration_ms * 1024);
		uint32_t speed_mbps = ((uint64_t)total_bytes * 8) / (duration_ms * 1000);
		LOG_INF("Write: %lld bytes in %lld ms = %u KB/s (%u Mb/s)", total_bytes, duration_ms, speed_kbps, speed_mbps);
	}

	return 0;
}

int measure_read_speed(const char *base_path) {
	char path[MAX_PATH];
	struct fs_file_t file;
	int64_t start_time, end_time;
	size_t bytes_read = 0;
	int ret;

	fs_file_t_init(&file);

	snprintf(path, sizeof(path), "%s/%s", base_path, PERF_FILE_NAME);

	ret = fs_open(&file, path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("Failed to open file for read test: %d", ret);
		return ret;
	}

	start_time = k_uptime_get();

	while (true) {
		ssize_t read = fs_read(&file, test_buf, WRITE_CHUNK_SIZE);
		if (read <= 0) {
			break;
		}
		bytes_read += read;
	}

	fs_close(&file);
	end_time = k_uptime_get();

	int64_t duration_ms = end_time - start_time;
	if (duration_ms > 0) {
		uint32_t speed_kbps = (bytes_read * 1000) / (duration_ms * 1024);
		uint32_t speed_mbps = (bytes_read * 8) / (duration_ms * 1000);
		LOG_INF("Read: %zu bytes in %lld ms = %u KB/s (%u Mb/s)", bytes_read, duration_ms, speed_kbps, speed_mbps);
	}

	return 0;
}

int setup_disk(void) {
	static const char *disk_pdrv = DISK_DRIVE_NAME;
	uint64_t memory_size_bytes;
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

	memory_size_bytes = (uint64_t)block_count * block_size;
	LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_bytes >> 20));

	err = disk_access_ioctl(disk_pdrv, DISK_IOCTL_CTRL_DEINIT, NULL);
	if (err) {
		LOG_ERR("Storage deinit ERROR!");
		return err;
	}
	return 0;
}

void make_session_dir(uint8_t *path) {
	if (fs_mkdir(path) != 0) {
		LOG_ERR("Failed to create dir %s", path);
		/* If code gets here, it has at least successes to create the
		 * file so allow function to return true.
		 */
	}
}

void format(void) {
	int rc = fs_mkfs(FS_FATFS, (uintptr_t)"SD2:", NULL, 0);
	if (rc < 0) {
		LOG_ERR("Format failed");
	}
}

int init_storage(void) {
	char boot_root_path[MAX_PATH];
	snprintf(boot_root_path, sizeof(boot_root_path), "%s/%s", disk_mount_pt, get_boot_id());

	// format();

	int ret = setup_disk();
	if (ret != 0) {
		LOG_ERR("Storage init failed");
		return ret;
	}

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);
	if (res != 0) {
		LOG_INF("Problem mounting disk %d", res);
		return res;
	}

	make_session_dir(boot_root_path);

	measure_write_speed(boot_root_path);
	measure_read_speed(boot_root_path);

	lsdir(disk_mount_pt);
	lsdir(boot_root_path);

	fs_unmount(&mp);
	return 0;
}
