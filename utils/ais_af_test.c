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

int main(void)
{
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
