# AIS RX experiment (channel B, 162.025 MHz)

**Status: bench probe only. NO working AIS GMSK receiver yet. NO on-air
decodes and NO real AIS bitstream claimed.** This branch is intentionally
independent of the production APRS image on `main`.

## Build

```sh
make clean
make ENABLE_AIS_RX=1
cc -std=c11 -Wall -Wextra -Werror -Iapp \
   app/ais_bits.c utils/ais_bits_test.c -o /tmp/ais_bits_test
/tmp/ais_bits_test
```

`ENABLE_AIS_RX=1` turns APRS and AM compensation off, enables all-band
**reception** and disables the sleep feature. PTT and entry into transmit mode
are blocked in `RADIO_PrepareTX`, `FUNCTION_Select` and `FUNCTION_Transmit`.
The normal `f4hwn.bin` is produced. Check that it remains under **61,440
bytes** before considering a flash. Back up the radio's EEPROM first.

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
  bypass* AF route to the normal analogue audio output. The quality and
  electrical accessibility of that signal have **not** been measured.
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

## Bench checklist / next decision

1. Flash **only an intentionally built `ENABLE_AIS_RX=1` image**, after an
   EEPROM backup; verify the radio never enters TX when PTT is pressed.
2. Use an SDR or RF signal generator to check actual tuning at 162.025 MHz;
   record the headphone/AF bypass path at at least 48 ksample/s while
   providing a controlled **received** AIS burst. Confirm the 9.6-kbaud
   frequency information is still present; measure, don't assume.
3. If the AF signal is usable, first demodulate and validate it on a PC. Only
   then look for a way to bring the needed samples or binary symbols to the
   DP32G030 MCU without losing bits on a ~26 ms burst.
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
