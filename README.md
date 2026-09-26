# UV-K5 AIS-RX — experimenteller AIS-Empfänger (162,025 MHz)

> **AIS-Laborversion, kein fertiger AIS-Decoder.** Dieser Branch `ais-rx`
> startet den Quansheng UV-K5/K6/5R als **reinen Empfänger auf AIS-Kanal B
> (162,025 MHz)**. Die GMSK-Demodulation eines echten Funksignals ist
> **noch nicht implementiert**. Die eigenständige Bit-/HDLC-/CRC-Prüfung
> funktioniert bisher nur mit künstlich erzeugten Testdaten.

**AIS-Zweig: Dr. Heinz Doofenshmirtz.** Auf dem kleinen Display erscheint
dafür die Kurzform `DR.H.DOOF`. Die vollständige Liste der früheren Autoren,
Entwickler und Quellen steht in [AUTHORS.md](AUTHORS.md). Der bisherige
APRS-Ausgangszweig bleibt unverändert auf [`main`](../../tree/main).

## Was diese erste Version bereits kann

- BK4819 beim Start und beim erneuten Einrichten des Empfängers auf
  **162,025 MHz (AIS B)** stellen, FM-Empfang, 25-kHz-Kanalfilter.
- 300-Hz-Hochpass, 3-kHz-Tiefpass und Deemphasis des RX-Audiowegs umgehen;
  experimentellen Digital-RX-AF-Bypass an den analogen Audioausgang legen.
- Squelch für den anfänglichen Empfangsversuch umgehen und den
  Energiesparmodus abschalten.
- **Nur RX:** PTT und mehrere interne Wege in den TX-Modus sind gesperrt.
- Mit dem unabhängigen, bereits programmierten C-Testmodul künstliche
  9.600-bit/s-NRZI/HDLC-Daten auf AIS-Flags, Bit-Stuffing, CRC-16/X-25,
  AIS-Nachrichtentyp und MMSI prüfen.

**Was ausdrücklich noch nicht geht:** echte GMSK-Symbole aus dem BK4819
gewinnen, reale AIS-Pakete empfangen/decodieren oder MMSI, Schiffsnamen,
Koordinaten und Kurs auf dem Funkgerät anzeigen. Die 9.600-bit/s-Bitprüfung
ist noch **nicht** an die Funkhardware angeschlossen. Ob der gewählte
AF-Bypass genügend Signalbandbreite liefert, muss zuerst am realen Gerät
gemessen werden. **Nicht als Navigations- oder Sicherheitsgerät verwenden.**

## Firmware selbst bauen

Voraussetzungen: Git, Make, die ARM-Embedded-Toolchain
(`arm-none-eabi-gcc`, **10.3.1 empfohlen**) und für den optionalen
Flash-Vorgang Python 3 mit `pyserial`. Linux, macOS oder Windows mit WSL
sind geeignete Entwicklungsumgebungen.

```bash
git clone --branch ais-rx https://github.com/DE8MSH/uv-k5-firmware-ta1js.git
cd uv-k5-firmware-ta1js
make clean
make -j4 ENABLE_AIS_RX=1
arm-none-eabi-size f4hwn
wc -c f4hwn.bin
```

**Ergebnis:** `f4hwn.bin` ist die **rohe** experimentelle AIS-RX-Firmware.
Sie darf **61.440 Byte** nicht überschreiten. Auf diesem Branch baut auch
`make` standardmäßig die AIS-Version; `ENABLE_AIS_RX=1` steht oben
absichtlich nochmals ausdrücklich im Befehl.

### Ganz ohne lokale ARM-Toolchain

Jeder Push auf `ais-rx` startet den separaten
[GitHub-Actions-Build „AIS RX experimental build“](../../actions/workflows/ais-rx.yml).
Dort den letzten **erfolgreichen Lauf** öffnen, unter **Artifacts** das
`AIS-RX-EXPERIMENTAL-NOT-DECODED`-Archiv laden und entpacken.
Es enthält ebenfalls die rohe `f4hwn.bin`; diese Test-Artefakte werden
nur **sieben Tage** aufbewahrt. Sie gehören nicht zu den regulären
APRS-Releases und sind ausdrücklich keine getesteten AIS-Empfangsgeräte.

### Optional: Docker

Die angepasste Version des vorhandenen Skripts unterstützt `aisrx`:

```bash
./compile-with-docker.sh aisrx
# Ergebnis: compiled-firmware/ais-rx-162025-rx-only.bin
```

**Hinweis:** Das ältere Docker-Helferskript bereinigt Docker-Images und
ungenutzte Docker-Ressourcen. Wer dies nicht möchte, benutzt den Make-Build
oben oder den GitHub-Actions-Download.

## Das erste erwartete Ergebnis am UV-K5

Nach dem Flashen sollte der Startbildschirm `DR.H.DOOF v0.1` und
`AIS-RX Edition` anzeigen. Der BK4819 wird auf **162,025 MHz** gestellt;
die Empfangsdaten laufen versuchsweise über den Filter-Bypass zum
Audioausgang. Mit passender Antenne in einem Gebiet mit AIS-Funkverkehr
könnten dort kurze AIS-Signalbursts messbar sein. **Ob überhaupt ein
brauchbares Audiosignal herauskommt, ist noch ungetestet.** Auf dem
Funkgerät selbst werden in dieser ersten Version noch **keine Schiffe**
und **keine AIS-validierten Bits** angezeigt.

Der erste sinnvolle Hardwaretest ist daher eine Aufnahme des Audioausgangs
mit mindestens **48 ksample/s**, während ein bekannter AIS-Testburst
**nur empfangen** wird. Danach prüfen wir Spektrum, Pegel und
9.600-bit/s-Symbolinformation; erst im nächsten Schritt wird ein
GMSK-Demodulator mit `AIS_BitsPushNRZI()` verbunden.
Die Prüfschritte und BK4819-Register sind in
[docs/AIS_RX.md](docs/AIS_RX.md) dokumentiert.

## Sicher flashen

**Vorher EEPROM sichern** (einschließlich Kalibrierung), z. B. mit
`utils/eeprom_tool.py` oder
[`k5prog`](https://github.com/sq5bpf/k5prog). Fremde Firmware wird
auf eigenes Risiko geflasht.

1. Funkgerät ausschalten; **PTT gedrückt halten** und einschalten,
   um den Bootloader zu starten.
2. Bei Bedarf `python3 -m pip install pyserial` ausführen.
3. Das **rohe** `f4hwn.bin` auf die zutreffende serielle Schnittstelle
   flashen (Portnamen unten entsprechend ersetzen).

```bash
python3 utils/k5flash.py /dev/ttyUSB0 f4hwn.bin '*AIS RX v0.1'
```

Unter macOS kann der Port `/dev/cu.wchusbserial*` heißen, unter Windows
beispielsweise `COM3`. **Nicht** das `.packed.bin` verwenden.
Nach dem Neustart als erstes testen, dass PTT **nicht sendet**.

## AIS-Bits ohne Funkhardware testen

```bash
cc -std=c11 -Wall -Wextra -Werror -Iapp \
   app/ais_bits.c utils/ais_bits_test.c -o /tmp/ais_bits_test
/tmp/ais_bits_test
```

Dieser Host-Test prüft CRC-16/X-25, NRZI, AIS-Typ 1, MMSI und die Ablehnung
einer korrupten Nachricht. Er ist **kein** Empfangstest des BK4819.

## Decoder-Referenz und ursprüngliche Firmware

Die aktuelle, interessante C99-Referenz ist
[Pieter Ibelings’ `libaisdemod`](https://github.com/ibelinp/libaisdemod)
(MIT, 2026). Sie demoduliert **komplexe SDR-I/Q-Samples**, nicht unmittelbar
den experimentellen BK4819-Audioausgang, und ist deshalb **noch nicht in die
Firmware integriert**. Wir wollen zunächst die Übertragbarkeit des
`frame.c`-/`message.c`-Teils und die Signalqualität am echten Gerät
prüfen. Eine zweite C-Referenz ist
[hessu/gnuais](https://github.com/hessu/gnuais) (GPL-2.0).

Der ursprüngliche [APRS-Code](docs/APRS.md) und die bestehenden
APRS-Werkzeuge verbleiben im Repository, sind bei `ENABLE_AIS_RX=1`
jedoch abgeschaltet. Für einen bisherigen APRS-Versuchsbuild explizit
`make clean && make ENABLE_AIS_RX=0 ENABLE_APRS=1` verwenden;
die produktiven APRS-Releases stammen weiterhin aus `main`.

## Autoren und Lizenz

**AIS-Branch: Dr. Heinz Doofenshmirtz.** Alle im bisherigen README
genannten Vorgänger und Mitwirkenden — darunter DE8MSH/TA1JS, F4HWN,
Egzumer, OneOfEleven, DualTachyon, fagci und die weiteren Beteiligten —
sind einzeln in [AUTHORS.md](AUTHORS.md) aufgeführt.
Die vorhandenen Copyright-Köpfe und die
[Apache-2.0-Lizenz](LICENSE) bleiben erhalten. Fremde Decoder-Codes
werden hier nicht unbemerkt übernommen.
