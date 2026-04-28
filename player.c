#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "minimp3.h"
#include "file_reader.h"
#include "buffer_decoder.h"
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

/* Frame MP3 máximo: MPEG1 Layer3 320kbps = 1441 bytes.
   Nunca teremos mais do que isso como sobra entre chunks.          */
#define MP3_MAX_FRAME_BYTES  1441
#define AUDIO_CHUNK_SIZE     65536
#define N_WAVE_BUFS          4

/* ── Ring buffer waveOut ─────────────────────────────────────── */

typedef struct {
    WAVEHDR hdr;
    int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
    int     prepared;
} WaveBuf;

static WaveBuf g_wbufs[N_WAVE_BUFS];
static int     g_wbuf_idx = 0;

void enqueue_pcm(HWAVEOUT hwo, const int16_t *src, size_t n_samples)
{
    WaveBuf *wb = &g_wbufs[g_wbuf_idx % N_WAVE_BUFS];
    if (wb->prepared) {
        while (!(wb->hdr.dwFlags & WHDR_DONE)) Sleep(1);
        waveOutUnprepareHeader(hwo, &wb->hdr, sizeof(wb->hdr));
        wb->prepared = 0;
    }
    memcpy(wb->pcm, src, n_samples * sizeof(int16_t));
    memset(&wb->hdr, 0, sizeof(wb->hdr));
    wb->hdr.lpData         = (LPSTR)wb->pcm;
    wb->hdr.dwBufferLength = (DWORD)(n_samples * sizeof(int16_t));
    waveOutPrepareHeader(hwo, &wb->hdr, sizeof(wb->hdr));
    waveOutWrite(hwo, &wb->hdr, sizeof(wb->hdr));
    wb->prepared = 1;
    g_wbuf_idx++;
}

void drain_wave_bufs(HWAVEOUT hwo)
{
    for (int i = 0; i < N_WAVE_BUFS; i++) {
        WaveBuf *wb = &g_wbufs[i];
        if (wb->prepared) {
            while (!(wb->hdr.dwFlags & WHDR_DONE)) Sleep(1);
            waveOutUnprepareHeader(hwo, &wb->hdr, sizeof(wb->hdr));
            wb->prepared = 0;
        }
    }
}

/* ── Double buffer ───────────────────────────────────────────── */

typedef struct {
    /* +MP3_MAX_FRAME_BYTES: espaço para a sobra do chunk anterior   */
    uint8_t data[2][AUDIO_CHUNK_SIZE + MP3_MAX_FRAME_BYTES];
    size_t  bytes[2];   /* bytes válidos (sobra + lido do arquivo)   */
    int     back;
    HANDLE  ev_filled;
    HANDLE  ev_consumed;
} DoubleBuffer;

typedef struct { FileReader *file; DoubleBuffer *db; } ReaderArgs;

/* ── Thread leitora ──────────────────────────────────────────── */

DWORD WINAPI reader_thread(LPVOID arg)
{
    ReaderArgs   *ctx = (ReaderArgs *)arg;
    DoubleBuffer *db  = ctx->db;

    while (1) {
        int    back   = db->back;
        size_t offset = db->bytes[back];   /* bytes já presentes (sobra) */

        /* Lê a partir do offset, preservando a sobra no início        */
        int n = file_reader_read_chunk(ctx->file,
                                       db->data[back] + offset,
                                       AUDIO_CHUNK_SIZE);
        db->bytes[back] = offset + (size_t)n;

        SetEvent(db->ev_filled);
        if (n == 0) break;

        WaitForSingleObject(db->ev_consumed, INFINITE);
        ResetEvent(db->ev_consumed);
    }
    return 0;
}

/* ── Áudio ───────────────────────────────────────────────────── */

void init_audio(HWAVEOUT *hwo, int hz, int channels)
{
    WAVEFORMATEX wf    = {0};
    wf.wFormatTag      = WAVE_FORMAT_PCM;
    wf.nChannels       = (WORD)channels;
    wf.nSamplesPerSec  = (DWORD)hz;
    wf.wBitsPerSample  = 16;
    wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    waveOutOpen(hwo, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);
}

/* ── main ────────────────────────────────────────────────────── */

int main(void)
{
    timeBeginPeriod(1);

    FileReader *file = malloc(sizeof(FileReader));
    if (file_reader_open(file,
            "D:\\GitHub\\AudioPlayerC\\audios\\"
            "Radiohead - Jigsaw Falling Into Place.mp3") != 0)
        return -1;

    printf("Tamanho arquivo: %zu\n", file->file_size);

    DoubleBuffer db  = {0};
    db.back          = 0;
    db.ev_filled     = CreateEvent(NULL, TRUE, FALSE, NULL);
    db.ev_consumed   = CreateEvent(NULL, TRUE, FALSE, NULL);

    ReaderArgs rargs = { .file = file, .db = &db };
    HANDLE hThread   = CreateThread(NULL, 0, reader_thread, &rargs, 0, NULL);

    int16_t             framePCM[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
    mp3dec_frame_info_t info;
    size_t              pcm_samples;
    HWAVEOUT            hWaveOut        = NULL;
    int                 audio_initialized = 0;

    while (1) {
        WaitForSingleObject(db.ev_filled, INFINITE);
        ResetEvent(db.ev_filled);

        int front = db.back;
        int back  = 1 - front;
        db.back   = back;

        /* ── Decodifica o front buffer ───────────────────────── */
        uint8_t *start_ptr = db.data[front];
        size_t   remaining = db.bytes[front];

        while (remaining > 0) {
            size_t before = remaining;

            if (buffer_decoder(start_ptr, &remaining, framePCM, &pcm_samples, &info)) {
                if (pcm_samples > 0) {
                    if (!audio_initialized && info.hz > 0) {
                        printf("Init audio: %d Hz | %d canais\n",
                               info.hz, info.channels);
                        init_audio(&hWaveOut, info.hz, info.channels);
                        audio_initialized = 1;
                    }
                    if (audio_initialized)
                        enqueue_pcm(hWaveOut, framePCM,
                                    pcm_samples * info.channels);
                }
            }

            /* Se não houve progresso, o que sobrou é um frame incompleto.
               Interrompe para não ficar em loop infinito.               */
            if (remaining == before) break;
        }

        /* ── Carrega a sobra para o início do novo back ──────── */
        /*  remaining > 0: bytes do último frame partido pelo limite
            do chunk. Copiamos para o início do novo back buffer;
            o leitor vai completar o frame na próxima leitura.          */
        if (remaining > 0) {
            uint8_t *leftover = start_ptr + (db.bytes[front] - remaining);
            memcpy(db.data[back], leftover, remaining);
        }
        db.bytes[back] = remaining;   /* leitor começa a escrever aqui */

        /* Libera o leitor agora que o novo back está preparado         */
        SetEvent(db.ev_consumed);

        if (db.bytes[front] == 0) break;
    }

    if (hWaveOut) {
        drain_wave_bufs(hWaveOut);
        waveOutClose(hWaveOut);
    }

    timeEndPeriod(1);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(db.ev_filled);
    CloseHandle(db.ev_consumed);
    free(file);
    return 0;
}
