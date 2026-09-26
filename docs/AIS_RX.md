# AIS RX experimental branch — technical status

**Maintainer credit for experimental AIS work: Dr. Heinz Doofenshmirtz.**
See [AUTHORS.md](../AUTHORS.md) for *all previously named upstream
contributors* and [LICENSE](../LICENSE) for the unchanged legal notices.

**26 September 2026: This is not an on-device AIS receiver.** The
experimental firmware can set the BK4819 to AIS channel B
(162.025 MHz) and prevent intentional transmission, but **does not
receive or validate live AIS GMSK symbols**. The old RSSI/pulse
counter was deleted; random noise spikes and unrelated VHF signals
could masquerade as a supposed AIS detection. No on-air performance,
MMSI or position on the radio has been demonstrated.

For the investigation of the **required internal connection**,
read [ON_DEVICE_FEASIBILITY.md](ON_DEVICE_FEASIBILITY.md). It
distinguishes the already documented BK4819 audio output, the
documented narrow-FSK modem and the actual wired MCU ADC inputs.

## Software components that have independent tests

- `app/ais_probe.[ch]` fixes the radio to **162.025 MHz**, FM, wide
  filter, disables the RX HPF/LPF/deemphasis and attempts one of
  two experimental AF output routes. The chip's narrow FSK
  modem is left disabled.
- `app/ais_bits.[ch]` is a standalone **post-GMSK symbol**
  processor. It accepts one already-demodulated NRZI **symbol**
  at a time via `AIS_BitsPushNRZI()`. It checks HDLC
  flags, bit-stuffing and CRC-16/X-25 before returning
  AIS message type, length and MMSI. It currently has
  **no live symbol input** on the handheld.
- `utils/ais_bits_test.c` independently synthesizes HDLC
  frames including stuffing/FCS to test good and deliberately
  corrupted type-1 AIS frames, MMSI and NRZI inversion.
- `utils/ais_af_decode.[ch]` and `utils/ais_af_wav.c`
  are **PC-only** 48-/96-ksample/s, mono PCM16
  instantaneous-FM/audio decoders, not code currently
  executable with the actual radio's AF: a 20-Hz
  DC tracker, Gaussian BT=0.4 smoothing, 16 symbol
  phase hypotheses and the same frame checker.
- `utils/ais_af_test.c` independently synthesizes
  Gaussian-shaped NRZI AIS audio, tests both sample rates,
  sign reversal, FCS corruption, noise-only input and
  end-to-end WAV round trips. These are **off-air,
  synthetic tests**, not BK4819 hardware validations.

## Rejected RSSI pulse approach

Previous experimental builds contained `app/ais_diag*`
and an on-radio `RF PULSES` display. It sampled BK4819
RSSI every approximately 10 ms, declared 20–120-ms RSSI
peaks as possible AIS bursts and displayed a cumulative
counter. This **could not separate noise transients,
short co-channel/adjacent-channel traffic or actual AIS**.
Even a pulse of exactly one AIS-slot duration is not
evidence of GMSK data or a correct HDLC CRC. Both
the embedded diagnostic and its host test were removed
from the build and source tree.

The current UV-K5 lab LCD is deliberately explicit:

```text
AIS RX - LAB ONLY
     162.025
RX ONLY / NO TX
GMSK: UNVERIFIED
AIS DECODE: OFF
NO PACKET CLAIMS
```

No fake RF-to-AIS counter is present. The old
APRS main branch remains independent.

## RF / BK4819 bench configuration

The chip is tuned using its 10-Hz frequency units:

```c
#define AIS_RX_FREQUENCY_10HZ 16202500u
```

The settings in `AIS_RX_Configure()` are **experiments,
not proof of an on-device 9.6-kbaud data path**.

| Branch build flag | BK4819 AF route | What it tries |
|---|---|---|
| `AIS_AF_ROUTE=9` | `REG_47[11:8]=9`, RX gain bypass | Digital-radio RX audio bypass |
| `AIS_AF_ROUTE=1` | `REG_47[11:8]=1` | FM audio with software RX audio filters disabled |

Both use the wide (nominal 25-kHz) channel filter and
`REG_2B[10:8]=111` to bypass the documented audio HPF,
LPF and deemphasis. The receiver bypass is routed toward
**analogue AF audio** and does not by itself constitute
an ADC or digital bit source in the DP32G030.

The BK4819 hardware FSK receiver in the bundled
[Application Note v1.0](bk4819/BK4819%20Application%20Note%20v1.0.pdf)
describes lower-rate FSK/FFSK RX settings, **not a
documented 9,600-symbol/s AIS GMSK RX mode**.
Its 16-bit `REG_72` tone/clock formula with the stated
26-MHz reference would require ~99,115 for a naive
9,600-bit/s configuration (outside 0–65,535).
That calculation argues against simply multiplying
an APRS constant; it does **not** disprove every
possible undocumented alternative clock/mode.

When UART support is compiled, `AIS_RX_LogStatus()`
reads BK4819 registers after RX setup; that is a
configuration audit, **not reception evidence**:

```text
AISRX 162.025 AF9 38=.... 39=.... 43=.... 2B=.... 47=.... 58=....
```

## Known UV-K5 board limitation

The current `BOARD_ADC_Init()` configuration in
[`board.c`](../board.c) uses SAR ADC **CH4 for
battery voltage** and **CH9 for battery current**.
No documented installed wiring is shown here from the
BK4819's analogue FM-discriminator output into
a microcontroller ADC at 48/96 ksample/s; similarly,
the existing 1.2/2.4k FSK FIFO is not a proven source
of 9.6-kbaud GMSK bits.

Rob Riggs / Mobilinkd demonstrated reception of
**9.600-baud GMSK on a modified UV-K6 using an
external TNC4 modem**, with PCB audio-coupling
modifications and appropriate firmware filtering:
[UV-K6 Digital Modulation Modification](https://github.com/mobilinkd/uv-k6-digital-mod/blob/master/UV-K6.ipynb).
This is a helpful practical external demonstration of
the RF/audio path, **not** proof that an unmodified
UV-K5 CPU can decode the same samples autonomously.
No Mobilinkd code or hardware instructions are
copied into this firmware.

## Build and CI

```sh
git clone --branch ais-rx https://github.com/DE8MSH/uv-k5-firmware-ta1js.git
cd uv-k5-firmware-ta1js
make clean
make -j4 ENABLE_AIS_RX=1 AIS_AF_ROUTE=9
arm-none-eabi-size f4hwn
# Optional AF-route comparison
make clean
make -j4 ENABLE_AIS_RX=1 AIS_AF_ROUTE=1
```

Use `arm-none-eabi-gcc` 10.3.1. Both images must fit
in 61,440 bytes. The dedicated
[experimental GitHub Actions workflow](https://github.com/DE8MSH/uv-k5-firmware-ta1js/actions/workflows/ais-rx.yml)
runs synthetic post-demodulator and off-air
audio tests, then builds both variants as artifacts.
The output is an **engineering lab firmware**, not
a validated AIS receiver. Flashing the lab-only
firmware is **not needed** to establish this limitation.
Keep an EEPROM backup and independently verify
the PTT TX block if you do flash it.

## Acceptance test for the NEXT genuine milestone

The next firmware release should not display an AIS
message, AIS packet count, MMSI or ship position
until **all** these requirements are actually met:

1. Demonstrate a physical **installed** receive-data
   route from the BK4819 into the DP32G030, or
   a documented / independently proven alternative
   integrated GMSK source. A speaker-jack waveform
   outside the radio alone does not meet this condition.
2. Receive a controlled, known, RF-modulated
   **9,600-symbol/s GMSK AIS burst** with the
   handheld receiving on 162.025 MHz. Clock
   recovery and a hard-decision symbol stream
   must run on the radio.
3. Feed those symbols into the already tested
   `AIS_BitsPushNRZI()` engine. A real on-device
   **valid HDLC frame with matching CRC and
   the known test MMSI** is the first success.
4. Repeatedly reject corrupt payloads and
   interference without generating fake
   vessel identifiers. Only then implement
   position decoding and a ship display.

If no verified stock-board internal path exists,
the options become *internal hardware modification*
or a radio with a native digital receive-data interface.
An unsupported RSSI-pulse heuristic must not
be substituted for the missing GMSK data path.

---
Earlier APRS details: [APRS.md](APRS.md).
Author attribution: [../AUTHORS.md](../AUTHORS.md).
