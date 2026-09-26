/* Standalone host tests for on-radio RF pulse counter; NOT AIS decode.
 * Experimental AIS branch: Dr. Heinz Doofenshmirtz.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "../app/ais_diag_core.h"
#include <assert.h>
#include <stdio.h>

static void feed(AIS_Diag *d, uint16_t rssi, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        AIS_DiagFeedRSSI(d, rssi);
}

int main(void)
{
    AIS_Diag d;
    AIS_DiagReset(&d);
    feed(&d, 62u, 35u); /* measured noise floor */
    assert(d.initialized && d.rf_pulses == 0u);

    /* Two or three 10-ms samples are enough to detect an RF pulse
     * of roughly one AIS slot, but are NOT proof of AIS content. */
    feed(&d, 93u, 3u);
    feed(&d, 62u, 3u);
    assert(d.rf_pulses == 1u && d.last_pulse_ticks == 3u);

    feed(&d, 100u, 1u); /* reject one-tick glitch */
    feed(&d, 62u, 4u);
    assert(d.rf_pulses == 1u);

    feed(&d, 94u, 30u); /* reject a long continuous carrier */
    feed(&d, 62u, 4u);
    assert(d.rf_pulses == 1u);

    feed(&d, 94u, 2u);
    feed(&d, 62u, 1u); /* bridge one below-threshold tick */
    feed(&d, 96u, 2u);
    feed(&d, 62u, 3u);
    assert(d.rf_pulses == 2u && d.last_pulse_ticks == 4u);

    feed(&d, 0xFFFFu, 1u); /* reject invalid ADC/RSSI words */
    assert(d.last_rssi <= 0x01FFu);
    printf("PASS: standalone on-radio RF pulse counter and glitch rejection; "
           "NO AIS PACKET DECODING CLAIMED\n");
    return 0;
}
