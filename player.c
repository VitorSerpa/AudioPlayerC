#include <stdlib.h>
#include <stdint.h>
#include "minimp3.h"
#include "file_reader.h"
#include "buffer_decoder.h"
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#define AUDIO_CHUNK_SIZE 4096  

const char *directory_path = "D:\\GitHub\\AudioPlayerC\\audios\\music_list.txt";
uint8_t bufferRaw[AUDIO_CHUNK_SIZE];
uint8_t bufferRaw1[AUDIO_CHUNK_SIZE];
int16_t bufferPCM[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];

uint8_t *pointer_to_buffer;

size_t bytes_in_buffer = 0;
size_t bytes_consumed = 0;
size_t pcm_samples = 0;
mp3dec_frame_info_t info;
int audio_initialized = 0;
int buffer_to_use = 0;

HWAVEOUT hWaveOut;

void init_audio(HWAVEOUT *hWaveOut, int sample_rate, int channels)
{
    WAVEFORMATEX wf = {0};

    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = channels;
    wf.nSamplesPerSec = sample_rate;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

    waveOutOpen(hWaveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);
}

void play_pcm(HWAVEOUT hWaveOut, int16_t *pcm_samples, size_t num_samples)
{
    WAVEHDR header = {0};

    header.lpData = (LPSTR)pcm_samples;
    header.dwBufferLength = (DWORD)(num_samples * sizeof(int16_t));

    waveOutPrepareHeader(hWaveOut, &header, sizeof(header));
    waveOutWrite(hWaveOut, &header, sizeof(header));

    while (!(header.dwFlags & WHDR_DONE))
        Sleep(1);

    waveOutUnprepareHeader(hWaveOut, &header, sizeof(header));
}

int async_read_buffer(FileReader *file, uint8_t *bufferRaw, size_t *bytes_in_buffer, size_t AudioChunkSize){
    int bytes_read = file_reader_read_chunk(file, bufferRaw + *bytes_in_buffer, AudioChunkSize);
    return bytes_read;
}

// int listMusics(const char *directorypath){
//     FILE* music_list_file;
//     music_list_file = fopen(directory_path, "r");
//     while(fgets(buffer, 100, music_list_file)) {
//         printf("WHILE");
//         printf("%s", buffer);
//     }
//     return 0;
// }

int main(int argc, char *argv[])
{
    FileReader *file = malloc(sizeof(FileReader));
    printf("AQUI");
    if (file_reader_open(file, "D:\\GitHub\\AudioPlayerC\\audios\\Radiohead - Jigsaw Falling Into Place.mp3") != 0)
        return -1;

    printf("Tamanho arquivo: %zu", file->file_size);
    while (1)
    {
        int bytes_read = 0;

        if(buffer_to_use == 0){
            bytes_read = async_read_buffer(file, bufferRaw, &bytes_in_buffer, AUDIO_CHUNK_SIZE);
            bytes_in_buffer += bytes_read;
            pointer_to_buffer = bufferRaw;
            buffer_to_use = 1;
        }else{
            bytes_read = async_read_buffer(file, bufferRaw1, &bytes_in_buffer, AUDIO_CHUNK_SIZE);
            bytes_in_buffer += bytes_read;
            pointer_to_buffer = bufferRaw1;
            buffer_to_use = 0;
        }

        while (bytes_in_buffer > 0)
        {
            if (buffer_decoder(pointer_to_buffer, &bytes_in_buffer, bufferPCM, &pcm_samples, &info))
            {
                if (pcm_samples > 0)
                {
                    if (!audio_initialized && info.hz > 0)
                    {
                        printf("Init audio: %d Hz | %d canais\n", info.hz, info.channels);
                        init_audio(&hWaveOut, info.hz, info.channels);
                        audio_initialized = 1;
                    }

                    if (audio_initialized)
                    {
                        play_pcm(hWaveOut, bufferPCM, pcm_samples);
                    }
                }
            }
        }

        if (bytes_read == 0)
            break;
    }

    return 0;
}
