#pragma once
#include "stdint.h"

void debug_led_set(uint8_t led, uint8_t state);
void debug_led_on(void);
void debug_led_off(void);
void debug_led_toggle(uint8_t led);
void debug_led_strobe(void);
void init_debug_leds(void);

