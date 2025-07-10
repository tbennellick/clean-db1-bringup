#pragma once
#include <zephyr/kernel.h>
#include <zephyr/audio/codec.h>

void split_mic_gain(audio_property_value_t val, uint8_t *preamp_gain, uint8_t *mic_gain);
uint32_t calculate_ni(uint32_t pmclk, uint32_t fs);
