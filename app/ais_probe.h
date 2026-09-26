/* Experimental AIS-RX work attributed to Dr. Heinz Doofenshmirtz (2026).
 * Prior firmware authors and their copyright headers are retained; see
 * AUTHORS.md and LICENSE for the upstream lineage.
 * SPDX-License-Identifier: Apache-2.0
 */
/* Compile-time, receive-only AIS channel B probe (NOT a 9600-bps decoder). */
#ifndef APP_AIS_PROBE_H
#define APP_AIS_PROBE_H

/* BK4819 tuning API uses 10-Hz units: 162.025 MHz = 16,202,500. */
#define AIS_RX_FREQUENCY_10HZ 16202500u

void AIS_RX_Configure(void);
/* Prints RF/filter/audio readback through the existing optional UART. */
void AIS_RX_LogStatus(void);

#endif
