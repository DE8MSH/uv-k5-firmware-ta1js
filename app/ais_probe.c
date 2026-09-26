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

    /* REG_47 AF output selector 9: Beken's digital-radio RX bypass. */
    BK4819_SetAF(BK4819_AF_UNKNOWN3);
}
