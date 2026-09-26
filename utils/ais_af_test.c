/* Copyright 2026 AIS-RX experiment, Dr. Heinz Doofenshmirtz.
 * Prior UV-K5 authors: AUTHORS.md. SPDX-License-Identifier: Apache-2.0
 *
 * Independent HOST-ONLY synthetic discriminator-audio tests.
 * THIS DOES NOT TEST THE BK4819, THE SPEAKER JACK, OR REAL AIS RECEPTION.
 */
#include "ais_af_decode.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PI 3.14159265358979323846
#define MAX_SYMBOLS 512u
#define TEST_MS 200u

static AIS_FrameInfo delivered;
static unsigned int messages;
static uint32_t rng = 0x1271f00du;

static uint32_t random_word(void)
{
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

static void got_ais(const AIS_FrameInfo *f, void *user)
{
    (void)user;
    delivered = *f;
    ++messages;
}

static uint16_t fcs_x25(const uint8_t *data, unsigned int n)
{
    uint16_t c = 0xffffu;
    for (unsigned int i = 0; i < n; ++i) {
        c ^= data[i];
        for (unsigned int b = 0; b < 8; ++b)
            c = (c & 1u) ? (uint16_t)((c >> 1) ^ 0x8408u)
                         : (uint16_t)(c >> 1);
    }
    return (uint16_t)~c;
}

static void add_nrzi(int8_t *symbols, unsigned int *count, int *level,
                     unsigned int bit)
{
    assert(*count < MAX_SYMBOLS);
    if (bit == 0u)
        *level = -*level;
    symbols[(*count)++] = (int8_t)*level;
}

static void add_flag(int8_t *symbols, unsigned int *count, int *level)
{
    for (unsigned int i = 0; i < 8u; ++i)
        add_nrzi(symbols, count, level, (0x7eu >> i) & 1u);
}

static unsigned int make_symbols(int8_t symbols[MAX_SYMBOLS], int bad_fcs)
{
    uint8_t frame[23] = {0};
    unsigned int count = 0, ones = 0;
    int level = 1;
    const uint32_t mmsi = 123456789u;
    frame[0] = 0x04u; /* AIS type 1, repeat 0: 000001 00 */
    for (unsigned int i = 0; i < 30u; ++i) {
        const unsigned int pos = i + 8u;
        if ((mmsi >> (29u - i)) & 1u)
            frame[pos / 8u] |= (uint8_t)(1u << (7u - pos % 8u));
    }
    uint16_t crc = fcs_x25(frame, 21u);
    frame[21] = (uint8_t)crc;
    frame[22] = (uint8_t)(crc >> 8u);
    if (bad_fcs)
        frame[10] ^= 1u; /* deliberately DON'T regenerate FCS */

    for (unsigned int i = 0; i < 24u; ++i)
        add_nrzi(symbols, &count, &level, i & 1u);
    add_flag(symbols, &count, &level);
    for (unsigned int i = 0; i < sizeof(frame); ++i) {
        for (unsigned int b = 0; b < 8u; ++b) {
            unsigned int bit = (frame[i] >> b) & 1u;
            add_nrzi(symbols, &count, &level, bit);
            ones = bit ? ones + 1u : 0u;
            if (ones == 5u) {
                add_nrzi(symbols, &count, &level, 0u);
                ones = 0u;
            }
        }
    }
    add_flag(symbols, &count, &level);
    return count;
}

/* Gaussian-filtered NRZI symbol train: synthetic instantaneous FM frequency,
 * not complex IQ and not a physical RF transmission. BT=0.4, h=0.5. */
static void synth_pcm(int16_t *samples, size_t count, unsigned int rate,
                      int bad_fcs, int invert, unsigned int start_shift,
                      int quiet)
{
    int8_t symbols[MAX_SYMBOLS];
    unsigned int n = make_symbols(symbols, bad_fcs);
    double sigma = sqrt(log(2.0)) / (2.0 * TEST_PI * 0.4);
    double sps = (double)rate / 9600.0;
    size_t start = (size_t)rate / 20u + start_shift; /* 50 ms quiet prefix */
    for (size_t i = 0; i < count; ++i) {
        double freq = 0.0;
        double t = ((double)i - (double)start) / sps;
        int k0 = (int)floor(t);
        if (!quiet && t >= -2.0 && t < (double)n + 2.0) {
            for (int k = k0 - 3; k <= k0 + 3; ++k) {
                if (k >= 0 && (unsigned int)k < n) {
                    double z = (t - (double)k - 0.5) / sigma;
                    freq += (double)symbols[k] * exp(-0.5 * z * z);
                }
            }
        }
        /* Add realistic-ish DC offset and deterministic amplitude noise.
         * Polarity reversal tests NRZI's natural symbol inversion immunity. */
        double noise = ((double)((random_word() >> 16) & 0xffffu) /
                        32768.0 - 1.0) * 0.025;
        double x = 0.04 + (invert ? -0.68 : 0.68) * freq + noise;
        if (x > 0.999) x = 0.999;
        if (x < -0.999) x = -0.999;
        samples[i] = (int16_t)(x * 32767.0);
    }
}

static void run_one(unsigned int rate, int bad_fcs, int invert, int quiet)
{
    size_t len = (size_t)rate * TEST_MS / 1000u;
    int16_t *pcm = (int16_t *)malloc(len * sizeof(*pcm));
    AIS_AF_Result r;
    assert(pcm);
    messages = 0;
    synth_pcm(pcm, len, rate, bad_fcs, invert,
              (unsigned int)(rate / 9600u / 2u), quiet);
    assert(AIS_AF_DecodeMonoPcm16(pcm, len, rate, got_ais, NULL, &r) == 0);
    if (!bad_fcs && !quiet) {
        assert(r.unique_frames >= 1u);
        assert(messages >= 1u);
        assert(delivered.type == 1u);
        assert(delivered.mmsi == 123456789u);
        assert(delivered.payload_bytes == 21u);
    } else {
        assert(r.unique_frames == 0u);
        assert(messages == 0u);
    }
    printf("PASS: synthetic discriminator AF, %u Hz, bad_FCS=%d, "
           "inverted=%d, quiet=%d; verified=%u\n",
           rate, bad_fcs, invert, quiet, r.unique_frames);
    free(pcm);
}

/* Generate a real PCM16 WAVE fixture for the command-line reader. The
 * contents are independently synthesized Gaussian frequency pulses, not an
 * RF recording or samples taken from a BK4819. */
static void put_le16(FILE *f, unsigned int v)
{
    fputc((int)(v & 255u), f);
    fputc((int)((v >> 8u) & 255u), f);
}

static void put_le32(FILE *f, uint32_t v)
{
    put_le16(f, v & 0xffffu);
    put_le16(f, v >> 16u);
}

static void generate_wav(const char *name, int quiet)
{
    const unsigned int rate = 96000u;
    const size_t nsamples = (size_t)rate * TEST_MS / 1000u;
    const uint32_t size = (uint32_t)(nsamples * 2u);
    int16_t *pcm = (int16_t *)malloc(nsamples * sizeof(*pcm));
    assert(pcm);
    synth_pcm(pcm, nsamples, rate, 0, 0, 3u, quiet);
    FILE *f = fopen(name, "wb");
    assert(f);
    assert(fwrite("RIFF", 1, 4, f) == 4u);
    put_le32(f, 36u + size);
    assert(fwrite("WAVEfmt ", 1, 8, f) == 8u);
    put_le32(f, 16u);
    put_le16(f, 1u);  /* PCM */
    put_le16(f, 1u);  /* mono */
    put_le32(f, rate);
    put_le32(f, rate * 2u);
    put_le16(f, 2u);
    put_le16(f, 16u);
    assert(fwrite("data", 1, 4, f) == 4u);
    put_le32(f, size);
    for (size_t i = 0; i < nsamples; ++i)
        put_le16(f, (uint16_t)pcm[i]);
    assert(fclose(f) == 0);
    free(pcm);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--wav") == 0) {
        generate_wav(argv[2], 0);
        printf("Synthetic AIS-AF fixture written: %s\n", argv[2]);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--noise") == 0) {
        generate_wav(argv[2], 1);
        printf("Synthetic noise-only fixture written: %s\n", argv[2]);
        return 0;
    }
    if (argc != 1) {
        fprintf(stderr, "usage: %s [--wav|--noise FILE.wav]\n", argv[0]);
        return 2;
    }
    run_one(48000u, 0, 0, 0);
    run_one(96000u, 0, 0, 0);
    run_one(48000u, 0, 1, 0);
    run_one(96000u, 1, 0, 0);
    run_one(48000u, 0, 0, 1);
    assert(AIS_AF_DecodeMonoPcm16(NULL, 100u, 48000u,
                                   got_ais, NULL, NULL) < 0);
    puts("ALL HOST AF TESTS PASS (NO BK4819 RF HARDWARE TESTED)");
    return 0;
}
