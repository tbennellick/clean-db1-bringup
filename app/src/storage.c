
#include "boot_id.h"
#include "process_storage.h"
#include "uuid_lib.h"
#include "zephyr/drivers/regulator.h"
#include "zephyr/random/random.h"

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

// clang-format off
/* the order of these two lines matters (_T redefined) */
#include <ff.h>
#include <ctype.h>
// clang-format on

LOG_MODULE_REGISTER(storage, LOG_LEVEL_DBG);

#define DISK_DRIVE_NAME "SD2"
#define DISK_MOUNT_PT   "/" DISK_DRIVE_NAME ":"
static const char *disk_mount_pt = DISK_MOUNT_PT;

#define BFP_SESSION_DIR_DIGITS_COUNT 8
#define BFP_FILENAME_DIGITS_COUNT    8
#define BFP_EVENT_FILE_EXTENSION     ".binpb"
#define BFP_FILENAME_LEN             (BFP_FILENAME_DIGITS_COUNT + sizeof(BFP_EVENT_FILE_EXTENSION) - 1)

#define MAX_PATH 128

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

K_MSGQ_DEFINE(storage_msgq, sizeof(uint32_t), CONFIG_STORAGE_Q_LEN, 4);

__maybe_unused static int lsdir(const char *path) {
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

bool is_session_dir(struct fs_dirent *entry, int32_t *session_val) {
	/* Sanitise (This originates from storage and therefore potentially external influence)*/
	if ((*entry).type != FS_DIR_ENTRY_DIR) {
		return false;
	}

	if (strlen((*entry).name) != BFP_SESSION_DIR_DIGITS_COUNT) {
		return false;
	}

	for (int i = 0; i < BFP_SESSION_DIR_DIGITS_COUNT; i++) {
		if (0 == isdigit((*entry).name[i])) {
			return false;
		}
	}

	/* Convert */
	char *end;
	const long a = strtol((*entry).name, &end, 10);
	*session_val = a;

	return true;
}

int32_t get_next_session_count(void) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	int highest_previous_session_value = 0;

	fs_dir_t_init(&dirp);

	res = fs_opendir(&dirp, disk_mount_pt);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", DISK_MOUNT_PT, res);
	}

	for (;;) {
		res = fs_readdir(&dirp, &entry);
		if (res || entry.name[0] == 0) { /* entry.name[0] == 0 means end-of-dir */
			break;
		}

		int32_t this_session_val = 0;
		if (is_session_dir(&entry, &this_session_val)) {
			if (this_session_val > highest_previous_session_value) {
				highest_previous_session_value = this_session_val;
			}
		}
	}

	fs_closedir(&dirp);

	return highest_previous_session_value + 1;
}

void touch_file(char *path) {
	struct fs_file_t file;
	fs_file_t_init(&file);
	int res = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
	if (res) {
		LOG_ERR("File creation failed with %d %s", res, path);
	}
	res = fs_close(&file);
	if (res) {
		LOG_ERR("File close failed with %d %s", res, path);
	}
}

void make_session_dir() {
	char path[MAX_PATH + UUID_STRING_SIZE];

	uint32_t session_number = get_next_session_count();

	int ret = snprintf(path, sizeof(path), "%s/%08d", disk_mount_pt, session_number);
	if (ret < 0 || ret >= sizeof(path)) {
		LOG_ERR("Session dir path snprintf failed");
		return;
	}

	LOG_INF("Make session directory %s", path);
	ret = fs_mkdir(path);
	if (ret < 0) {
		LOG_ERR("Make session dir failed");
	}

	/* Write boot id (to be replaced with an event)*/
	ret = snprintf(path, sizeof(path), "%s/%08d/boot_%s", disk_mount_pt, session_number, (char *)get_boot_id());
	if (ret < 0 || ret >= sizeof(path)) {
		LOG_ERR("Session id path snprintf failed");
		return;
	}

	touch_file(path);

	/* Write device id (to be replaced with an event)*/
	ret = snprintf(path, sizeof(path), "%s/%08d/dev_%s", disk_mount_pt, session_number, (char *)get_device_id());
	if (ret < 0 || ret >= sizeof(path)) {
		LOG_ERR("Session id path snprintf failed");
		return;
	}
	touch_file(path);
}

void format(void) {
	int rc = fs_mkfs(FS_FATFS, (uintptr_t)"SD2:", NULL, 0);
	if (rc < 0) {
		LOG_ERR("Format failed");
	}
}

void append_file(const char *path, const void *data, size_t len) {
	struct fs_file_t file;
	int ret;

	fs_file_t_init(&file);

	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (ret != 0) {
		LOG_ERR("Failed to open file for append: %d", ret);
		return;
	}

	ssize_t written = fs_write(&file, data, len);
	if (written < 0) {
		LOG_ERR("Write failed: %d", (int)written);
	} else if ((size_t)written < len) {
		LOG_WRN("Short write: %d of %zu", (int)written, len);
	}

	fs_close(&file);
}

int storage_thread(void *arg1, void *arg2, void *arg3) {
	struct k_msgq *msgq = (struct k_msgq *)arg1;

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

	make_session_dir();

	process_storage_queue(msgq);

	fs_unmount(&mp);
	return 0;
}

struct k_msgq *init_storage(void) {
	static K_THREAD_STACK_DEFINE(storage_thread_stack, 4096);
	static struct k_thread storage_thread_data;

	k_thread_create(&storage_thread_data,
	                storage_thread_stack,
	                K_THREAD_STACK_SIZEOF(storage_thread_stack),
	                (k_thread_entry_t)storage_thread,
	                &storage_msgq,
	                NULL,
	                NULL,
	                K_PRIO_PREEMPT(7),
	                0,
	                K_NO_WAIT);
	return &storage_msgq;
}
