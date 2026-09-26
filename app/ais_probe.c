/* Experimental AIS-RX work attributed to Dr. Heinz Doofenshmirtz (2026).
 * Prior firmware authors and their copyright headers are retained; see
 * AUTHORS.md and LICENSE for the upstream lineage.
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Stage 1: force AIS channel B RX and route the least-filtered available
 * analogue AF to the normal audio output.  No GMSK bit clock exists yet.
 *
 * Reference: Beken Application Note v1.0, Tx/Rx Audio section (REG_2B),
 * Channel Spacing (REG_43), Digital Walkie-Talkie (REG_47[11:8] = 9).
 * The digital-bypass AF route is an EXPERIMENT, not a verified discriminator.
 */
#include "ais_probe.h"
#include "driver/bk4819.h"
#ifdef ENABLE_UART
#include "driver/uart.h"
#endif

#ifndef AIS_AF_ROUTE
#define AIS_AF_ROUTE 9
#endif
#if AIS_AF_ROUTE != 9 && AIS_AF_ROUTE != 1
#error AIS_AF_ROUTE must be 9 (digital RX bypass) or 1 (FM AF).
#endif

void AIS_RX_Configure(void)
{
    /* Hardware must remain in RX; never enable the BK4819's 1.2/2.4k FSK
     * modem and mistakenly claim its FIFO is a 9600-bps AIS bitstream. */
    BK4819_WriteRegister(BK4819_REG_59, 0x0000);
    BK4819_WriteRegister(BK4819_REG_58, 0x0000);

    BK4819_SetFrequency(AIS_RX_FREQUENCY_10HZ);
    BK4819_PickRXFilterPathBasedOnFrequency(AIS_RX_FREQUENCY_10HZ);
    BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE, true);

    BK4819_DisableScramble();
    BK4819_SetCompander(0);

    /* Disable 300 Hz HPF, 3 kHz LPF and deemphasis on the RX AF path. */
    BK4819_WriteRegister(BK4819_REG_2B,
        (uint16_t)(BK4819_ReadRegister(BK4819_REG_2B) | 0x0700u));

#if AIS_AF_ROUTE == 9
    /* Application Note: REG_47[11:8] = 9 is the digital-radio RX bypass.
     * REG_47[1] = 1 requests bypass RX gain in the Beken example; TX
     * selector REG_47[0] is intentionally left untouched in RX-only mode. */
    BK4819_SetAF(BK4819_AF_UNKNOWN3);
    BK4819_WriteRegister(BK4819_REG_47,
        (uint16_t)(BK4819_ReadRegister(BK4819_REG_47) | 0x0002u));
#else
    /* Control experiment: normal FM AF with external digital RX filters
     * disabled above. Analog headset/speaker filtering may still limit it. */
    BK4819_SetAF(BK4819_AF_FM);
#endif
}


/* UART lines deliberately carry register READBACK, not only the values
 * firmware tried to write. No assertion about real RF/AF reception follows. */
void AIS_RX_LogStatus(void)
{
#ifdef ENABLE_UART
    static const char names[6][3] = {"38", "39", "43", "2B", "47", "58"};
    static const BK4819_REGISTER_t regs[6] = {
        BK4819_REG_38, BK4819_REG_39, BK4819_REG_43,
        BK4819_REG_2B, BK4819_REG_47, BK4819_REG_58
    };
    char line[96];
    static const char hex[] = "0123456789ABCDEF";
    unsigned int pos = 0;
    const char prefix[] = "AISRX 162.025 AF";
    for (unsigned int i = 0; i < sizeof(prefix) - 1u; ++i)
        line[pos++] = prefix[i];
    line[pos++] = AIS_AF_ROUTE == 9 ? '9' : '1';
    for (unsigned int i = 0; i < 6u; ++i) {
        const uint16_t v = BK4819_ReadRegister(regs[i]);
        line[pos++] = ' ';
        line[pos++] = names[i][0];
        line[pos++] = names[i][1];
        line[pos++] = '=';
        line[pos++] = hex[(v >> 12) & 0x0Fu];
        line[pos++] = hex[(v >> 8) & 0x0Fu];
        line[pos++] = hex[(v >> 4) & 0x0Fu];
        line[pos++] = hex[v & 0x0Fu];
    }
    line[pos++] = '\r';
    line[pos++] = '\n';
    UART_Send(line, pos);
#endif
}
