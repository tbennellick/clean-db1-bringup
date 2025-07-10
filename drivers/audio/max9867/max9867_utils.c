#include "max9867_utils.h"

/* The mic signal chain has a preamp and gain stage.
 * This converts a single gain value into the corresponding
 * values for each block in the chain. */
void split_mic_gain(audio_property_value_t val, uint8_t *preamp_gain, uint8_t *mic_gain) {
    uint8_t vol = val.vol;

    if (vol <= 20)
    {
        *preamp_gain = 0;
        *mic_gain = 20 - vol;
    }
    else if (vol <= 40)
    {
        *preamp_gain = 3;
        *mic_gain = 40 - vol;
    }
    else if (vol <= 50)
    {
        *preamp_gain = 4;
        *mic_gain = 50 - vol;
    }
    else
    {
        *preamp_gain = 4;
        *mic_gain = 0;
    }
}

/* Calculate the appropriate clock divider (NI) to scale the
 * Frame clock(fs) from the Mclk(pmclk) See Table 4 of datasheet */
uint32_t calculate_ni(uint32_t pmclk, uint32_t fs) {
    uint64_t top = ((uint64_t) 65536 * 96 * fs);
    return (uint32_t) (top / pmclk);
}

