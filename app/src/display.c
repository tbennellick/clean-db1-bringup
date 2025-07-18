#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#ifdef CONFIG_DISPLAY_LOGO
#include "test_logo.h"
#endif

#define RGB565(r,g,b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))


static void fill_buffer_rgb565(uint16_t color, uint8_t *buf,
			       size_t buf_size)
{
	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(buf + idx + 0) = (color >> 8) & 0xFFu;
		*(buf + idx + 1) = (color >> 0) & 0xFFu;
	}
}


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
    buf_desc.pitch = capabilities.x_resolution;

	display_blanking_off(display_dev);

    memset(buf, 0xff, buf_size);
//    fill_buffer_rgb565(RGB565(0xff, 0, 0 ),buf, buf_size);

    if (display_write(display_dev, 0, 0, &buf_desc, buf) < 0) {
        LOG_ERR("Display write failed");
        k_free(buf);
        return -1;
    }

    uint8_t * logo = buf;
#ifdef CONFIG_DISPLAY_LOGO
    buf_desc.buf_size = TEST_LOGO_WIDTH * TEST_LOGO_HEIGHT * 2;
    buf_desc.width = TEST_LOGO_WIDTH;
    buf_desc.height = TEST_LOGO_HEIGHT;
    buf_desc.pitch = TEST_LOGO_WIDTH;
    logo = test_logo;
#else
    buf_desc.width = 206;
    buf_desc.height = 100;
    buf_desc.pitch = 100;
    buf_desc.buf_size = buf_desc.width * buf_desc.height * 2;
    fill_buffer_rgb565(RGB565(0, 0, 0xff ),buf, buf_desc.buf_size);
    logo = buf;
#endif

    if (display_write(display_dev, 20 , 100, &buf_desc, logo) < 0) {
        LOG_ERR("Display write failed");
        k_free(buf);
        return -1;
    }

    return 0;
}
