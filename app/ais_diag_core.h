/* Standalone AIS-RX RF activity diagnostic, not an AIS decoder.
 * Experimental AIS branch: Dr. Heinz Doofenshmirtz.
 * Earlier contributors: AUTHORS.md. SPDX-License-Identifier: Apache-2.0
 */
#ifndef APP_AIS_DIAG_CORE_H
#define APP_AIS_DIAG_CORE_H
#include <stdint.h>
#include <stdbool.h>

/* BK4819 RSSI is 9-bit and quantized in ~0.5 dB steps.
 * Do NOT interpret activity counts as validated AIS packets.
 */
typedef struct {
    uint32_t baseline_q4;
    uint16_t last_rssi;
    uint16_t rf_pulses;
    uint8_t high_ticks;
    uint8_t low_ticks;
    uint8_t last_pulse_ticks;
    bool initialized;
} AIS_Diag;

void AIS_DiagReset(AIS_Diag *d);
void AIS_DiagFeedRSSI(AIS_Diag *d, uint16_t raw_rssi);

#endif
