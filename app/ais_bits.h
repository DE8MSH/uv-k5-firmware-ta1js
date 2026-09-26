/* Experimental AIS-RX work attributed to Dr. Heinz Doofenshmirtz (2026).
 * Prior firmware authors and their copyright headers are retained; see
 * AUTHORS.md and LICENSE for the upstream lineage.
 * SPDX-License-Identifier: Apache-2.0
 */
/* Experimental AIS NRZI/HDLC frame checker; no RF demodulator included.
 * This interface accepts ONE hard-decision NRZI symbol at a time.
 */
#ifndef APP_AIS_BITS_H
#define APP_AIS_BITS_H

#include <stdint.h>

#define AIS_MAX_RAW_BITS 640u

typedef struct {
    uint8_t type;
    uint32_t mmsi;
    uint16_t payload_bytes;
} AIS_FrameInfo;

typedef void (*AIS_FrameCallback)(const AIS_FrameInfo *info, void *user);

typedef struct {
    uint8_t last_symbol;
    uint8_t have_symbol;
    uint8_t shift;
    uint8_t in_frame;
    uint16_t nbits;
    uint8_t raw[AIS_MAX_RAW_BITS];
    uint32_t flags;
    uint32_t crc_ok;
    uint32_t crc_bad;
    AIS_FrameCallback callback;
    void *user;
} AIS_BitChecker;

void AIS_BitsInit(AIS_BitChecker *s, AIS_FrameCallback cb, void *user);
void AIS_BitsPushNRZI(AIS_BitChecker *s, uint8_t symbol);

#endif
