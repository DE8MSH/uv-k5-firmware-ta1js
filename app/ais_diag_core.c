/* Standalone AIS-RX RF activity diagnostic, not an AIS decoder.
 * Experimental AIS branch: Dr. Heinz Doofenshmirtz.
 * Earlier contributors: AUTHORS.md. SPDX-License-Identifier: Apache-2.0
 */
#include "ais_diag_core.h"
#include <string.h>

void AIS_DiagReset(AIS_Diag *d)
{
    memset(d, 0, sizeof(*d));
}

void AIS_DiagFeedRSSI(AIS_Diag *d, uint16_t raw_rssi)
{
    if (raw_rssi > 0x01FFu)
        return;
    d->last_rssi = raw_rssi;
    if (!d->initialized) {
        d->baseline_q4 = (uint32_t)raw_rssi << 4;
        d->initialized = true;
        return;
    }

    /* Only a rough RF-activity test. 24 RSSI counts ~= 12 dB over the
     * moving baseline. The 10-ms scheduler can catch 2-3 samples of a
     * one-slot AIS burst, but other VHF transmissions can also match. */
    const uint16_t baseline = (uint16_t)(d->baseline_q4 >> 4);
    const bool above = raw_rssi > (uint16_t)(baseline + 24u);

    if (above) {
        if (d->high_ticks != 0xFFu)
            ++d->high_ticks;
        d->low_ticks = 0;
        return;
    }

    /* Require two low ticks to close a burst, bridging brief RSSI dips.
     * Exclude long transmissions (>120 ms) and sub-20-ms glitches. */
    if (d->high_ticks != 0) {
        if (++d->low_ticks >= 2u) {
            if (d->high_ticks >= 2u && d->high_ticks <= 12u) {
                if (d->rf_pulses != UINT16_MAX)
                    ++d->rf_pulses;
                d->last_pulse_ticks = d->high_ticks;
            }
            d->high_ticks = 0;
            d->low_ticks = 0;
        }
        return;
    }

    /* The slow upward / faster downward floor tracker does not follow a
     * brief VHF burst. Arithmetic uses integers to keep flash use low. */
    const int32_t sample_q4 = (int32_t)raw_rssi << 4;
    const int32_t delta = sample_q4 - (int32_t)d->baseline_q4;
    d->baseline_q4 = (uint32_t)((int32_t)d->baseline_q4 +
        (delta < 0 ? delta / 8 : delta / 64));
}
