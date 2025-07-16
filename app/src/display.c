#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

int init_display(void)
{
	uint8_t *buf;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.",
			display_dev->name);
	}

	LOG_INF("Display sample for %s", display_dev->name);
	display_get_capabilities(display_dev, &capabilities);
    LOG_INF("Display capabilities: x_res=%d, y_res=%d, pixel_format=%d",
          capabilities.x_resolution, capabilities.y_resolution,
          capabilities.current_pixel_format);

	buf_size = capabilities.x_resolution * capabilities.y_resolution *2; // 2 bytes per pixel

	buf = k_malloc(buf_size);
	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting sample.");
		return 0;
	}

    buf_desc.frame_incomplete = false;
    buf_desc.buf_size = buf_size;
    buf_desc.width = capabilities.x_resolution;
    buf_desc.height = capabilities.y_resolution;
    buf_desc.pitch = capabilities.y_resolution;

	display_blanking_off(display_dev);
    memset(buf, 0x88, buf_size);
    if (display_write(display_dev, 0, 0, &buf_desc, buf) < 0) {
        LOG_ERR("Display write failed");
        k_free(buf);
        return -1;
    }

    return 0;
}
