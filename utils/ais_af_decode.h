/* Copyright 2026 AIS-RX experiment, Dr. Heinz Doofenshmirtz.
 * The UV-K5 firmware's earlier authors remain credited in AUTHORS.md.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-only, experimental discriminator-AF AIS receiver.
 * This accepts MONO 16-bit PCM containing analog FM-discriminator audio;
 * it does NOT accept complex SDR I/Q or the BK4819's hardware FSK FIFO.
 */
#ifndef AIS_AF_DECODE_H
#define AIS_AF_DECODE_H

#include <stddef.h>
#include <stdint.h>
#include "../app/ais_bits.h"

typedef void (*AIS_AF_MessageCB)(const AIS_FrameInfo *info, void *user);
typedef struct {
    unsigned int unique_frames; /* CRC-valid, duplicate symbol-phases suppressed */
    unsigned int candidate_flags;
    unsigned int rejected_crc;  /* all parallel symbol phases, not unique frames */
    double rms;                 /* PCM full scale: 0.0 .. 1.0 */
    double peak;
} AIS_AF_Result;

/* 48000 and 96000 Hz PCM are supported; up to 120 seconds in the WAV CLI.
 * Return 0 for a processed buffer, negative for bad parameters/memory.
 * No decodes is an ordinary outcome and DOES NOT validate the AF path. */
int AIS_AF_DecodeMonoPcm16(const int16_t *pcm, size_t count,
                          unsigned int rate, AIS_AF_MessageCB callback,
                          void *user, AIS_AF_Result *result);

#endif
