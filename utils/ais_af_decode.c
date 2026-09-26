/* Copyright 2026 AIS-RX experiment, Dr. Heinz Doofenshmirtz.
 * Prior UV-K5 authors: see AUTHORS.md. SPDX-License-Identifier: Apache-2.0
 *
 * OFF-RADIO proof of concept. The BK4819 digital-RX AF bypass is not known
 * to supply usable discriminator samples; no real-RF performance is claimed.
 * Baseband algorithm: 20-Hz DC tracker, Gaussian BT=0.4 matched smoothing,
 * 16 parallel timing hypotheses, sign slice, then AIS NRZI/HDLC/CRC checker.
 */
#include "ais_af_decode.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AIS_AUDIO_PHASES 16u
#define AIS_AUDIO_MAX_SPS 10u
#define AIS_AUDIO_MAX_TAPS (3u * AIS_AUDIO_MAX_SPS + 3u)
#define AIS_PI 3.14159265358979323846

typedef struct {
    AIS_AF_MessageCB callback;
    void *user;
    unsigned int unique;
    size_t current_symbol;
    size_t last_symbol;
    uint32_t last_mmsi;
    uint8_t last_type;
    uint8_t have_last;
} AIS_AF_Context;

static void received_frame(const AIS_FrameInfo *info, void *opaque)
{
    AIS_AF_Context *ctx = (AIS_AF_Context *)opaque;
    /* Neighboring timing phases will often decode the same burst. A real
     * repeat by the same vessel later in the WAV must NOT be suppressed. */
    if (ctx->have_last && ctx->last_type == info->type &&
        ctx->last_mmsi == info->mmsi &&
        ctx->current_symbol >= ctx->last_symbol &&
        ctx->current_symbol - ctx->last_symbol < 96u)
        return;
    ctx->have_last = 1u;
    ctx->last_symbol = ctx->current_symbol;
    ctx->last_mmsi = info->mmsi;
    ctx->last_type = info->type;
    ++ctx->unique;
    if (ctx->callback)
        ctx->callback(info, ctx->user);
}

int AIS_AF_DecodeMonoPcm16(const int16_t *pcm, size_t count,
                          unsigned int rate, AIS_AF_MessageCB cb, void *user,
                          AIS_AF_Result *result)
{
    float *audio;
    float taps[AIS_AUDIO_MAX_TAPS];
    AIS_BitChecker phases[AIS_AUDIO_PHASES];
    AIS_AF_Context ctx;
    unsigned int sps;
    unsigned int radius;
    double sigma, weight_total = 0.0;
    double dc = 0.0, energy = 0.0, peak = 0.0;
    double alpha;
    size_t symbol = 0;

    if (!pcm || !result || count == 0 ||
        (rate != 48000u && rate != 96000u))
        return -1;
    sps = rate / 9600u; /* exactly 5 or 10 samples/symbol */
    radius = (3u * sps) / 2u; /* 3-symbol Gaussian FIR */
    if (2u * radius + 1u > AIS_AUDIO_MAX_TAPS)
        return -1;

    audio = (float *)malloc(count * sizeof(*audio));
    if (!audio)
        return -2;

    memset(result, 0, sizeof(*result));
    memset(&ctx, 0, sizeof(ctx));
    ctx.callback = cb;
    ctx.user = user;

    /* The tracker is intentionally slow compared with 9600-baud data.
     * Running on quiet samples before the burst helps its initial estimate. */
    alpha = 2.0 * AIS_PI * 20.0 /
            ((double)rate + 2.0 * AIS_PI * 20.0);
    for (size_t i = 0; i < count; ++i) {
        double v = (double)pcm[i] / 32768.0;
        double mag = fabs(v);
        if (mag > peak) peak = mag;
        energy += v * v;
        dc += alpha * (v - dc);
        audio[i] = (float)(v - dc);
    }
    result->rms = sqrt(energy / (double)count);
    result->peak = peak;

    /* Gaussian of the expected AIS frequency pulse, BT=0.4.
     * sqrt(log 2)/(2*pi*BT) = ~0.3313 symbol times. */
    sigma = (double)sps * sqrt(log(2.0)) / (2.0 * AIS_PI * 0.4);
    for (unsigned int t = 0; t < 2u * radius + 1u; ++t) {
        double z = ((double)t - (double)radius) / sigma;
        double w = exp(-0.5 * z * z);
        taps[t] = (float)w;
        weight_total += w;
    }
    for (unsigned int t = 0; t < 2u * radius + 1u; ++t)
        taps[t] = (float)(taps[t] / weight_total);

    for (unsigned int p = 0; p < AIS_AUDIO_PHASES; ++p)
        AIS_BitsInit(&phases[p], received_frame, &ctx);

    /* Symbol-major order is essential for inter-phase deduplication. We
     * interpolate between ADC samples for 16 phases even at 48 ksample/s. */
    for (;;) {
        double base = ((double)symbol + 2.0) * (double)sps;
        if (base + (double)sps + (double)radius + 2.0 >= (double)count)
            break;
        ctx.current_symbol = symbol;
        for (unsigned int p = 0; p < AIS_AUDIO_PHASES; ++p) {
            double center = base +
                (double)p * (double)sps / (double)AIS_AUDIO_PHASES;
            size_t at = (size_t)center;
            float frac = (float)(center - (double)at);
            double sum = 0.0;
            for (unsigned int t = 0; t < 2u * radius + 1u; ++t) {
                size_t i = at + t - radius;
                float x = audio[i] + (audio[i + 1u] - audio[i]) * frac;
                sum += (double)x * (double)taps[t];
            }
            AIS_BitsPushNRZI(&phases[p], (uint8_t)(sum >= 0.0));
        }
        ++symbol;
    }

    for (unsigned int p = 0; p < AIS_AUDIO_PHASES; ++p) {
        result->candidate_flags += phases[p].flags;
        result->rejected_crc += phases[p].crc_bad;
    }
    result->unique_frames = ctx.unique;
    free(audio);
    return 0;
}
