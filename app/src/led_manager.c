#include "led_manager.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
LOG_MODULE_REGISTER(led_manager,CONFIG_LOG_DEFAULT_LEVEL);

#define USER_R_NODE DT_ALIAS(user_red)
static const struct gpio_dt_spec user_r = GPIO_DT_SPEC_GET(USER_R_NODE, gpios);
#define USER_G_NODE DT_ALIAS(user_green)
static const struct gpio_dt_spec user_g = GPIO_DT_SPEC_GET(USER_G_NODE, gpios);
#define USER_B_NODE DT_ALIAS(user_blue)
static const struct gpio_dt_spec user_b = GPIO_DT_SPEC_GET(USER_B_NODE, gpios);

typedef struct {
	const struct device *red_dev;
	const struct device *green_dev;
	const struct device *blue_dev;
	led_manager_colour_t colour;
	k_timeout_t on_time;
	k_timeout_t off_time;
	bool state;
} ctx_t;

static ctx_t ctx = {
	.colour = LED_MANAGER_COLOUR_OFF
};

static void set_colour(led_manager_colour_t colour) {
    gpio_pin_set_dt(&user_r,colour & LED_MANAGER_COLOUR_RED);
    gpio_pin_set_dt(&user_g,colour & LED_MANAGER_COLOUR_GREEN);
    gpio_pin_set_dt(&user_b,colour & LED_MANAGER_COLOUR_BLUE);
}

static void blink_timer_handler(struct k_timer *timer) {
	if (ctx.state) {
		set_colour(LED_MANAGER_COLOUR_OFF);
	} else {
		set_colour(ctx.colour);
	}
	ctx.state = !ctx.state;

	k_timer_start(timer, ctx.state ? ctx.on_time : ctx.off_time, K_NO_WAIT);
}

K_TIMER_DEFINE(blink_timer, blink_timer_handler, NULL);

void led_manager_set(led_manager_colour_t colour, led_manager_mode_t mode) {
	k_timer_stop(&blink_timer);
	ctx.colour = colour;

	if (mode == LED_MANAGER_MODE_CONT) {
		set_colour(ctx.colour);
	} else {
		ctx.on_time = K_MSEC((mode >> 16U) & 0xFFFFU);
		ctx.off_time = K_MSEC(mode & 0xFFFFU);
		ctx.state = false;
		blink_timer_handler(&blink_timer);  // Call the handler, which will start the timer
	}
}

void led_manager_init(void) {

    if (!gpio_is_ready_dt(&user_r)) {k_fatal_halt(-100);}
    if (!gpio_is_ready_dt(&user_g)) {k_fatal_halt(-100);}
    if (!gpio_is_ready_dt(&user_b)) {k_fatal_halt(-100);}

    if ( 0 != gpio_pin_configure_dt(&user_r, GPIO_OUTPUT_ACTIVE)) {k_fatal_halt(-101);}
    if ( 0 != gpio_pin_configure_dt(&user_g, GPIO_OUTPUT_ACTIVE)) {k_fatal_halt(-101);}
    if ( 0 != gpio_pin_configure_dt(&user_b, GPIO_OUTPUT_ACTIVE)) {k_fatal_halt(-101);}

}

//static int auto_led_manager_init(const struct device *_dev) {
//    ARG_UNUSED(_dev);
//    led_manager_init();
//    return 0;
//}
//SYS_INIT(auto_led_manager_init, PRE_KERNEL_2, 50);
