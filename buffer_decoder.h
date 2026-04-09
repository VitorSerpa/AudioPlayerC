#pragma once
#include "minimp3.h"

int buffer_decoder(uint8_t *bufferRaw, size_t *bytes_in_buffer, int16_t *bufferPCM, size_t *pcm_samples, mp3dec_frame_info_t *out_info);