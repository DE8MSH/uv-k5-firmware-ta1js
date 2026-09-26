/* Copyright 2026 AIS-RX experiment, Dr. Heinz Doofenshmirtz.
 * Prior UV-K5 authors: AUTHORS.md. SPDX-License-Identifier: Apache-2.0
 *
 * Usage: ./ais_af_wav recording_48k_mono_pcm16.wav
 * This is a PC-side experimental discriminator AUDIO decoder, not SDR I/Q.
 */
#include "ais_af_decode.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void show_message(const AIS_FrameInfo *frame, void *user)
{
    (void)user;
    printf("AIS CRC OK: type=%u MMSI=%09lu payload=%u bytes\n",
           (unsigned int)frame->type, (unsigned long)frame->mmsi,
           (unsigned int)frame->payload_bytes);
}

int main(int argc, char **argv)
{
    FILE *in;
    uint8_t wav_header[12], chunk[8], fmt[40];
    uint8_t *raw = NULL;
    int16_t *audio = NULL;
    uint32_t sample_rate = 0, data_size = 0;
    long data_offset = 0;
    unsigned int format = 0, channels = 0, bits = 0;
    AIS_AF_Result result;
    int rc = 1;
    if (argc != 2) {
        fprintf(stderr, "usage: %s discriminator_pcm16_mono_48k_or_96k.wav\n",
                argv[0]);
        return 2;
    }
    in = fopen(argv[1], "rb");
    if (!in) {
        perror(argv[1]);
        return 2;
    }
    if (fread(wav_header, 1, sizeof(wav_header), in) != sizeof(wav_header) ||
        memcmp(wav_header, "RIFF", 4) != 0 ||
        memcmp(wav_header + 8, "WAVE", 4) != 0) {
        fprintf(stderr, "not a RIFF/WAVE file\n");
        goto done;
    }
    while (fread(chunk, 1, sizeof(chunk), in) == sizeof(chunk)) {
        const uint32_t size = read32(chunk + 4);
        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (size < 16u || size > sizeof(fmt) ||
                fread(fmt, 1, size, in) != size) {
                fprintf(stderr, "unsupported WAV fmt chunk\n");
                goto done;
            }
            format = read16(fmt);
            channels = read16(fmt + 2);
            sample_rate = read32(fmt + 4);
            bits = read16(fmt + 14);
        } else if (memcmp(chunk, "data", 4) == 0) {
            data_offset = ftell(in);
            data_size = size;
            if (data_offset < 0)
                goto done;
            break;
        } else if (fseek(in, (long)size, SEEK_CUR) != 0)
            goto done;
        if ((size & 1u) && fseek(in, 1, SEEK_CUR) != 0)
            goto done;
    }
    if (!data_offset || format != 1u || channels != 1u ||
        bits != 16u || (sample_rate != 48000u && sample_rate != 96000u) ||
        (data_size & 1u) || data_size == 0u ||
        data_size > sample_rate * 2u * 120u) {
        fprintf(stderr, "requires mono PCM16 WAV, 48k/96k, at most 120 seconds\n");
        goto done;
    }
    raw = (uint8_t *)malloc(data_size);
    audio = (int16_t *)malloc(data_size);
    if (!raw || !audio || fseek(in, data_offset, SEEK_SET) != 0 ||
        fread(raw, 1, data_size, in) != data_size) {
        fprintf(stderr, "read/memory error\n");
        goto done;
    }
    for (size_t i = 0; i < data_size / 2u; ++i)
        audio[i] = (int16_t)read16(raw + 2u * i);
    if (AIS_AF_DecodeMonoPcm16(audio, data_size / 2u, sample_rate,
                              show_message, NULL, &result) != 0) {
        fprintf(stderr, "decoder rejected the WAV\n");
        goto done;
    }
    printf("Audio: rate=%lu Hz, duration=%.3f s, rms=%.4f FS, peak=%.4f FS\n",
           (unsigned long)sample_rate,
           (double)(data_size / 2u) / (double)sample_rate,
           result.rms, result.peak);
    printf("Frames: CRC valid=%u, flag candidates (all phases)=%u, "
           "CRC rejected=%u\n",
           result.unique_frames, result.candidate_flags, result.rejected_crc);
    if (!result.unique_frames)
        puts("NO VERIFIED AIS: either no burst, wrong AF route, filtering, "
             "low SNR, wrong sample format or timing. No on-air claim.");
    rc = 0;
done:
    free(raw);
    free(audio);
    fclose(in);
    return rc;
}
