# AIS RX — Dr. Heinz Doofenshmirtz (channel B, 162.025 MHz)

**Status: on-radio RF pulse diagnostic and optional bench probe only. NO
working AIS GMSK receiver yet. NO verified AIS messages or real AIS bits
claimed.** This branch is intentionally
independent of the production APRS image on `main`.

## Build and expected first result

Author byline for these **experimental AIS additions**: **Dr. Heinz
Doofenshmirtz** (`DR.H.DOOF` in the limited UV-K5 screen). All known prior
authors and the unchanged licensing are recorded in
[`AUTHORS.md`](../AUTHORS.md) and [`LICENSE`](../LICENSE).

This **AIS branch builds AIS RX by default** (`ENABLE_AIS_RX?=1`).
The explicit switch below documents the intended binary and prevents
accidentally building the legacy APRS edition:

```sh
git clone --branch ais-rx https://github.com/DE8MSH/uv-k5-firmware-ta1js.git
cd uv-k5-firmware-ta1js
make clean
make -j4 ENABLE_AIS_RX=1
arm-none-eabi-size f4hwn
wc -c f4hwn.bin
cc -std=c11 -Wall -Wextra -Werror -Iapp \
   app/ais_bits.c utils/ais_bits_test.c -o /tmp/ais_bits_test
/tmp/ais_bits_test
```

Use `arm-none-eabi-gcc` **10.3.1**, matching the separate
[experimental GitHub Actions workflow](../.github/workflows/ais-rx.yml).
The raw `f4hwn.bin` must fit within **61,440 bytes**. Alternatively, run
`./compile-with-docker.sh aisrx` to get
`compiled-firmware/ais-rx-162025-rx-only.bin`; the new `aisrx` case
avoids the legacy script's global Docker prune. The CI workflow publishes
short-lived `AIS-RX-AF9-AF1-EXPERIMENTAL` artifacts with TWO RX-only
binaries, not an APRS release.

**On the first boot:** the welcome display should show
`DR.H.DOOF v0.1` and `AIS-RX Edition`; the BK4819 should be configured
to receive at 162.025 MHz and present an *experimental* AF bypass at
the analogue audio output. The signal may be absent or unsuitable until
hardware measurements establish the actual bandwidth. **No AIS vessel,
MMSI, position, CRC pass or live GMSK symbols are displayed or received
by the embedded checker yet.** The only present AIS bit results are the
synthetic host test. PTT must be verified to remain disabled before use.

`ENABLE_AIS_RX=1` turns APRS and AM compensation off, enables all-band
**reception** and disables the sleep feature. PTT and entry into transmit mode
are blocked in `RADIO_PrepareTX`, `FUNCTION_Select` and `FUNCTION_Transmit`.
The normal `f4hwn.bin` is produced. Check that it remains under **61,440
bytes** before considering a flash. Back up the radio's EEPROM first.

## Stage 0: completely on-radio RSSI / RF pulse test — no audio cable

This is now the **primary field test** for anyone without an audio cable
or external sound interface. Flash the AIS-RX experimental image once
using the normal UV-K5 firmware flashing method; all subsequent
monitoring is displayed directly on the radio.

The AIS branch now overrides the normal VFO main screen with:

```text
AIS RX / RF TEST
       162.025
RX ONLY  |  NO TX
RSSI -110 dBm
RF PULSES: 0
NO AIS DECODE YET
```

These display fields represent a **coarse RSSI envelope test**, not
digital AIS reception. `app/ais_diag_core.[ch]` checks BK4819 RSSI
every ~10 ms, tracks the noise floor, and counts isolated RF peaks
roughly 20–120 ms wide and at least 12 dB above the floor.
It bridges one sample's brief signal dip, rejects one-tick spikes,
and rejects long continuous carriers. The screen refreshes about
five times per second. `utils/ais_diag_test.c` is a deterministic
host test of the pulse-counting algorithm.

* A positive RF pulse count **cannot** validate AIS, GMSK, NRZI,
  HDLC, an MMSI, or a particular ship. A nearby interferer could
  produce exactly the same counter increase.
* A zero count does **not** prove there are no AIS signals:
  weak, overlapping, adjacent-channel, or poorly timed bursts may
  be missed at this slow sampling rate.
* This diagnostic does not require or collect audio samples, use
  an ADC, or claim to have found a GMSK symbol stream.
* The BK4819's documented FSK RX modes remain limited to
  1.2/2.4-kbps formats. The DP32G030 SAR ADC channels used by this
  board (`BOARD_ADC_Init`) currently measure battery voltage and
  battery current, not discriminator audio. Thus, **a usable
  physical on-device 9.6-kbaud GMSK signal path has not been
  established**. Without one we cannot turn the existing
  `app/ais_bits.c` checker into live AIS decoding by software
  alone.

This is an explicitly limited but independently testable on-device
milestone. The earlier optional AF9/AF1 plus PC WAV experiment is
retained for developers; it is **not required** to use the new
on-radio RSSI display.

## Stage 1, already implemented

- On startup set active RX VFO to **162.025 MHz** (AIS channel B), FM, wide
  25 kHz filter and monitor mode (no squelch wait on the short AIS bursts).
- Every time `RADIO_SetupRegisters` reruns, force the BK4819 frequency back
  to 162.025 MHz and apply `AIS_RX_Configure()`. User channel changes cannot
  retune the hardware in this experimental build; the rest of the user
  interface is not yet locked to the probe channel.
- `BK4819_SetFilterBandwidth(WIDE, true)` prevents narrow-on-weak-signal
  filtering. `REG_2B[10:8]=111` bypasses the RX 300 Hz HPF, 3 kHz LPF and
  deemphasis. `REG_47[11:8]=9` selects Beken's documented *digital-radio RX
  bypass* AF route to the normal analogue audio output. Two compile-time
  routes are now available: AF9 digital bypass (default) and AF1 ordinary FM
  AF, each with all three software RX filters disabled. The actual analogue
  output bandwidth of both routes has NOT been measured.
- The chip's FSK modem is **disabled**. It cannot be treated as a 9600-bit/s
  GMSK demodulator based on the available documentation.

## Stage 1b, synthetic AIS bit checks

`app/ais_bits.[ch]` is a small, hardware-independent **post-demodulation**
checker. Its only input is a *hard-decision NRZI symbol* stream via
`AIS_BitsPushNRZI()`; it has no RF, FM-discriminator or symbol timing input
connected today. It recognizes HDLC flags, reverses NRZI, removes stuffed
zeroes, checks CRC-16/X-25 and extracts the six-bit message type and
30-bit MMSI. Buffers cover ordinary type-1 position reports and
type-5 static/voyage reports, not every possible AIS message length.

`utils/ais_bits_test.c` synthesizes a type-1 message (MMSI 123456789),
transmits the entire fake frame in NRZI with real HDLC bit stuffing and FCS,
then tests rejection after corrupting one bit, inverted NRZI level and the
published X-25 test vector (`123456789 -> 0x906E`). This is a **host unit
test**, *not* evidence that the BK4819 receives real AIS bits.

## Hardware facts from the supplied Beken PDFs

- *BK4819 Application Note v1.0*, **FSK**, printed pp. 12-14:
  `REG_58` documents 1200 and 2400 bps FSK/FFSK RX modes and their
  bandwidth settings; no 9600-bps GMSK RX setting is documented.
  `REG_72` is a **16-bit** tone/FSK clock word at about
  `frequency_hz * 10.32444` for the 26 MHz crystal. A naive 9600-bps
  setting would require ~99,115, larger than 65,535, so simply editing
  the existing APRS 1200-bps clock constant will not work.
- Same application note, **Tx/Rx Audio** printed pp. 4-5:
  `REG_2B[10:8]` can bypass all three RX audio filters.
  **Digital Walkie-Talkie**, printed p. 33: `REG_47[11:8]=9` is the
  documented RX audio bypass path (shown in the `RF_EnterBypass()` example).
- *BK4819(V3) Application Note 20210428*, FSK pp. 8-10 and Tx/Rx Audio
  pp. 3-5, describes the same core limits and also AF selector
  `REG_47[11:8]=8` as "FSK Out for Rx Test". That is the internal
  narrow-FSK modem's test output; it is **not** documented as raw GMSK.

## Stage 2: AF route comparison, register readback, and PC C decoder

Two firmware builds are now available for hardware comparisons. Both keep TX
blocked: AIS_AF_ROUTE=9 (default) selects Beken's digital-radio RX bypass,
REG_47[11:8]=9, with the Application Note's RX gain bit REG_47[1]=1;
AIS_AF_ROUTE=1 selects ordinary FM audio with software filters disabled.
We do not yet know whether either route preserves the necessary 9.6-kbaud
information through the physical headphone jack.

After startup the firmware emits one UART register readback line, e.g.:

    AISRX 162.025 AF9 38=.... 39=.... 43=.... 2B=.... 47=.... 58=....

38/39 are the tuning words; 43 is RX bandwidth; 2B is the audio filters;
47 is the AF path; 58 should show the old 1.2/2.4k hardware FSK modem
DISABLED. Register values check our programming, not real RF reception.

A new independent *host-side* C decoder now processes mono 16-bit PCM
FM-discriminator audio at either 48 or 96 ksample/s. It uses a slow
20-Hz DC tracker, a BT=0.4 Gaussian matched filter, 16 simultaneous
symbol phases, sign slicing and the existing AIS NRZI/HDLC/CRC checker.
It emits CRC-valid AIS message type and MMSI, NOT yet latitude/longitude.
This code does not accept complex I/Q, nor run on the handheld MCU.

Build the PC WAV decoder:

    cc -std=c11 -O2 -Wall -Wextra -Werror -Iapp \
      app/ais_bits.c utils/ais_af_decode.c utils/ais_af_wav.c \
      -lm -o /tmp/ais_af_wav
    /tmp/ais_af_wav captured-af9-mono-96k.wav

Run reproducible tests without RF hardware:

    cc -std=c11 -O2 -Wall -Wextra -Werror -Iapp \
      app/ais_bits.c utils/ais_af_decode.c utils/ais_af_test.c \
      -lm -o /tmp/ais_af_test
    /tmp/ais_af_test
    /tmp/ais_af_test --wav /tmp/ais-synthetic.wav
    /tmp/ais_af_wav /tmp/ais-synthetic.wav
    /tmp/ais_af_test --noise /tmp/ais-noise.wav
    /tmp/ais_af_wav /tmp/ais-noise.wav

Synthetic checks span 48k/96k, frequency-pulse shaping at BT=0.4,
NRZI polarity inversion, deliberately corrupted FCS, noise-only
input and the WAV command-line reader. They do **not** validate the
BK4819's AF path or establish real on-air decode.

For the hardware experiment, record 10 seconds of AF9 at 96 ksample/s
mono PCM16 via a safely attenuated AC-coupled audio-interface input.
Repeat with AF1, keeping antenna and RF conditions comparable. Collect
the startup register readbacks, audio RMS and peak, and PC CRC-valid
packet counts. If either route produces real CRC-verified packets,
only then investigate a practical input path into the DP32G030 MCU.

## Why the host decoder is not yet installed on the radio

The existing firmware configures the DP32G030 SAR ADC for **channel 4
(battery voltage)** and **channel 9 (battery current)** in
[board.c](../board.c), not for an AF signal from the BK4819. Merely
routing the BK4819 AF to the speaker does **not** establish an electrical
connection to an MCU ADC input or a 48/96-ksample/s DMA acquisition
path. We should not repurpose either battery-monitor channel, assert
that the BK4819's 1.2/2.4-kbps FSK FIFO contains AIS, or attempt UART
audio streaming without verifying the hardware and throughput.

A **verified physical audio sample path or digital 9.6-kbaud symbol
path** is the prerequisite for an on-device GMSK decoder. The real
AF9/AF1 WAV experiment isolates that question first; if neither
recording preserves 9.6-kbaud information, firmware-only message
decoding cannot fix the lost signal.

## Bench checklist / next decision

1. Flash **only an intentionally built `ENABLE_AIS_RX=1` image**, after an
   EEPROM backup; verify the radio never enters TX when PTT is pressed.
2. Use an SDR or RF signal generator to check actual tuning at 162.025 MHz;
   record the headphone/AF bypass path at at least 48 ksample/s while
   providing a controlled **received** AIS burst. Confirm the 9.6-kbaud
   frequency information is still present; measure, don't assume.
3. Decode the AF9 and AF1 WAVs with the PC program and compare their CRC
   results. Only if a real recording yields repeatable valid AIS should we
   investigate a path for binary symbols into the DP32G030 MCU.
4. Wire a verified NRZI-symbol source to `AIS_BitsPushNRZI()`; then use
   its CRC-valid frame callback before adding position parsing or display.

## Current C decoder candidate (not copied or vendored)

**[ibelinp/libaisdemod](https://github.com/ibelinp/libaisdemod)**:
2026 C99, MIT license, no library dependencies, synthetic tests. Its
input is **complex baseband I/Q** at high sample rates (96 kHz or more)
and its output is CRC-validated AIS/AIVDM including position and MMSI.
That makes it a practical **PC/SDR reference**, but **not a drop-in decoder**
for the BK4819's analogue AF route or the UV-K5 MCU. The useful downstream
parts to study are `src/frame.c` and `src/message.c`; do not integrate
its I/Q front end into this tiny firmware without a feasibility review.

As a GPL alternative, **[hessu/gnuais](https://github.com/hessu/gnuais)**
is C and was updated in February 2026, but introduces GPL-2.0 licensing
considerations and is targeted at host sound-card/SDR processing.
