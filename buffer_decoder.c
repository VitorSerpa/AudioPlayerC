#include "buffer_decoder.h"
#include "minimp3.h"

int buffer_decoder(uint8_t *bufferRaw, size_t *bytes_in_buffer, int16_t *bufferPCM, size_t *pcm_samples, mp3dec_frame_info_t *out_info) {
    static mp3dec_t mp3d;
    static int initialized = 0;

    if (!initialized) {
        mp3dec_init(&mp3d);
        initialized = 1;
    }

    int samples = mp3dec_decode_frame(&mp3d, bufferRaw, *bytes_in_buffer, bufferPCM, out_info);

    if (out_info->frame_bytes == 0) {
        *pcm_samples = 0;
        return 0;
    }

    *bytes_in_buffer -= out_info->frame_bytes;

    if (*bytes_in_buffer > 0) {
        memmove(bufferRaw, bufferRaw + out_info->frame_bytes, *bytes_in_buffer);
    }

    *pcm_samples = samples;

    return samples > 0; 
}


