# AIS on the stock UV-K5: engineering feasibility audit

**Status 2026-09-26: no validated on-device 9600-bps AIS GMSK input path.**
This is a limitation of the currently documented BK4819 and board
connections, **not** proof that a hidden/undocumented solution is impossible.
A hardware-only RF pulse counter was removed because wideband noise, nearby
traffic and unrelated transmissions cannot be separated from AIS by RSSI.

## The actual requirement

For a valid AIS message, a receiver must recover the **9,600
symbols/second GMSK data** on 162.025 MHz (or 161.975 MHz), recover clock
and NRZI bits, find the HDLC flags, remove stuffed bits, validate the
CRC-16/X-25 FCS, and only then extract a message type or an MMSI.

The existing `app/ais_bits.c` can perform NRZI/HDLC/FCS on **hard
symbols supplied to it**. It does not have a live source of these
symbols. The on-host synthetic unit tests explicitly cannot establish
RF reception.

## Documented BK4819 options checked

1. The normal BK4819 FSK/FFSK receive FIFO in the included
   [Beken Application Note](bk4819/BK4819%20Application%20Note%20v1.0.pdf)
   documents the 1,200/2,400-bit/s modes used by APRS; **no
   documented 9,600-bit/s GMSK receive mode** is provided there.
   Increasing the Bell 202 bit-rate register is not a drop-in
   solution: the documented REG_72 ~10.32444 × frequency (at
   26 MHz) would require ~99,114 for 9,600, exceeding its
   16-bit range. Another undocumented clock or mode may exist,
   but it has not been identified or validated.
2. The BK4819 digital RX bypass AF output, configured at
   `REG_47[11:8]=9`, routes **analog baseband toward the
   audio amplifier and accessory speaker path**, not to a
   proven PCM sample port on the microcontroller.
   `AIS_AF_ROUTE=1` is a second normal-FM-AF comparison route,
   not an onboard ADC source.
3. In this firmware, `BOARD_ADC_Init()` connects the DP32G030
   MCU SAR ADC to `ADC_CH4` (battery voltage) and `ADC_CH9`
   (battery current). We found **no verified installed
   discriminator-AF-to-MCU-ADC wiring**. Do not repurpose
   these battery pins: that would sample power rails, not AIS.
4. `BK4819_GetRSSI()`, noise and squelch metrics measure
   received signal strength / envelope, **not phase, sign or
   timing of GMSK symbols**. Noise spikes and unrelated RF
   activity can produce indistinguishable short pulses; even
   a tuned, timed pulse is not an AIS packet.

## Corroborating practical hardware investigation

Rob Riggs (WX9O) at Mobilinkd documented
[UV-K6 digital-modulation modifications](https://github.com/mobilinkd/uv-k6-digital-mod/blob/master/UV-K6.ipynb),
including BK4819 RX filter bypass and changes to PCB audio-coupling
components, then successfully received 9,600-baud GMSK **through an
external Mobilinkd TNC4 modem**. This is credible evidence that the
RF receiver *can pass* 9.6-kbaud baseband after suitable modifications.
It does **not** show that stock UV-K5 firmware can digitize or decode
the same baseband internally. Their notebook also describes
component/PCB-revision differences and the risk of permanent damage
from hardware modifications. This external research is linked for
technical context; none of its hardware alterations or decoder is
included in this branch.

## What was changed after the false RF-pulse prototype

The former `RF PULSES` display, its 10-ms scheduler calls, and all
`ais_diag*` implementation files have been **deleted** from the
AIS branch. The screen now explicitly states:

```text
AIS RX - LAB ONLY
    162.025
RX ONLY / NO TX
GMSK: NO DATA PATH
AIS DECODE: OFF
NO PACKET CLAIMS
```

This is **not** a usable AIS receiver. RF tuning, TX lockout and the
optional AF9/AF1 bench builds remain available for a future *genuine*
signal-path experiment. Valid-looking RSSI spikes can no longer appear
as AIS progress.

## Real next milestone, without pretending noise is data

A *firmware-only* onboard AIS decoder is credible **only if** one of
these is found and demonstrated, not merely hypothesized:

- A BK4819 digital receive mode or exported logic interface that
  produces real **9,600-symbol/s GMSK decisions** into a GPIO/IRQ
  already routed to the DP32G030; or
- A documented **existing** BK4819 discriminator/baseband path to
  an MCU ADC at sufficient sample rate, with an actual GPIO/PCB
  route confirmed. A flat audio output at the speaker jack alone
  is insufficient because the MCU cannot read that jack.

Either path must be verified with a known AIS test frame received
over RF, using a CRC-valid decoded result from the **handheld itself**
as the acceptance test. On-device valid CRC + a known test MMSI is
the minimum result worth displaying. If neither path exists on the
stock PCB, **internal hardware rework** (an AF-to-ADC connection) or
a device with a native 9.6-kbaud modem is required; neither is
offered as a software-only firmware fix here.

Author of the AIS experiment: **Dr. Heinz Doofenshmirtz**.
Original authors and contributions are credited in
[../AUTHORS.md](../AUTHORS.md); earlier source-code copyright and
the existing Apache-2.0 license are retained.
