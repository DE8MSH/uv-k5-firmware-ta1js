/* Standalone on-device AIS RF activity diagnostic; NO AIS decode.
 * Experimental AIS branch: Dr. Heinz Doofenshmirtz.
 * Earlier firmware contributors: AUTHORS.md.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ais_diag.h"
#include "driver/bk4819.h"
#include "misc.h"

AIS_Diag gAisDiagnostic;

void AIS_DiagPoll10ms(void)
{
    if (gRxIdleMode)
        return;

    const uint16_t before = gAisDiagnostic.rf_pulses;
    AIS_DiagFeedRSSI(&gAisDiagnostic, BK4819_GetRSSI());

    /* Refresh the on-radio LCD only 5 times a second, avoiding excessive
     * SPI traffic. The normal UI will render ONLY when its main screen
     * is active; menu and battery warnings still take precedence. */
    static uint8_t ticks;
    if (++ticks >= 20u || before != gAisDiagnostic.rf_pulses) {
        ticks = 0;
        gUpdateDisplay = true;
    }
}
