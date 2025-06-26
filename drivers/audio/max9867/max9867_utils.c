#include "max9867_utils.h"


void split_mic_gain(audio_property_value_t val, uint8_t * preamp_gain, uint8_t * mic_gain)
{
    uint8_t vol = val.vol;
    
    if (vol <= 20) {
        *preamp_gain = 0;
        *mic_gain = 20 - vol;
    } else if (vol <= 40) {
        *preamp_gain = 3;
        *mic_gain = 40 - vol;
    } else if (vol <=50) {
        *preamp_gain = 4;
        *mic_gain = 50-vol;
    } else {
        *preamp_gain = 4;
        *mic_gain = 0;
    }
}
