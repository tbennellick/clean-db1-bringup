#include "max9867_utils.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>


static void split_mic_gain(audio_property_value_t val, uint8_t * preamp_gain, uint8_t * mic_gain)
{
    *preamp_gain = 0;
    *mic_gain = 0;
}
