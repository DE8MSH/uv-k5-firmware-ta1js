# UV-K5 AIS-RX — experimenteller AIS-Empfänger (162,025 MHz)

> **AIS-Laborversion, kein fertiger AIS-Decoder.** Dieser Branch `ais-rx`
> startet den Quansheng UV-K5/K6/5R als **reinen Empfänger auf AIS-Kanal B
> (162,025 MHz)**. Die GMSK-Demodulation ist in der **Funkgeräte-Firmware noch nicht
> implementiert**. Ein neuer C-Decoder für den **PC** kann bereits
> künstliche, gaußgefilterte AIS-Diskriminator-Aufnahmen einschließlich
> NRZI/HDLC/CRC auswerten; reale BK4819-Aufnahmen sind noch ungetestet.

## Ohne Audiokabel: direkt auf dem Funkgerät testen

Die Firmware hat jetzt einen **eigenständigen AIS-Kanal-B-RF-Test auf dem
UV-K5-Display**, ganz ohne Audiokabel, Audiointerface oder Computer während
des Empfangs. Nur zum anfänglichen Flashen wird wie bisher dein übliches
Programmierverfahren benötigt.

Nach dem Start wird auf dem LCD angezeigt:

```text
AIS RX / RF TEST
  162.025
RX ONLY  |  NO TX
RSSI -110 dBm
RF PULSES: 0
NO AIS DECODE YET
```

RSSI zeigt die momentane empfangene Signalstärke. `RF PULSES` zählt
kurze, deutlich stärkere Funksignale von etwa 20–120 ms Länge auf
162,025 MHz. Das ist **nur eine grobe RF-Aktivitätsprüfung**;
jede andere kurze Aussendung, Störung oder ein Signal aus einem
Nachbarkanal kann ebenfalls den Zähler erhöhen. **Ein Zählerstand
von 1 bedeutet keinesfalls, dass eine gültige AIS-Nachricht erkannt
wurde.** Die Abtastung läuft im normalen Firmware-Zeitfenster alle
ungefähr 10 ms, das Display wird etwa fünfmal pro Sekunde aktualisiert.
PTT ist in dieser Test-Firmware gesperrt.

**Unser Ziel bleibt, vollständige AIS-Nachrichten im Funkgerät zu
decodieren.** Dafür fehlt noch ein nachgewiesener Weg, auf dem
der DP32G030 Mikrocontroller die **echten 9.600-bit/s-GMSK-Symbole**
des BK4819 erhalten kann. Seine vorhandenen, in der Originalplatine
genutzten SAR-ADC-Kanäle dienen zur Batterie- und Strommessung; die
dokumentierte BK4819-FSK-Funktion kennt nur niedrigere Datenraten.
Deshalb ist es derzeit **nicht belegt**, dass eine reine
Firmware-Lösung für vollständigen AIS-Empfang ohne Hardwareänderung
überhaupt möglich ist. Der RF-Test liefert uns einen ersten
ehrlichen Hardware-Befund direkt auf dem Gerät.

**AIS-Zweig: Dr. Heinz Doofenshmirtz.** Auf dem kleinen Display erscheint
dafür die Kurzform `DR.H.DOOF`. Die vollständige Liste der früheren Autoren,
Entwickler und Quellen steht in [AUTHORS.md](AUTHORS.md). Der bisherige
APRS-Ausgangszweig bleibt unverändert auf [`main`](../../tree/main).

## Was diese erste Version bereits kann

- **Ohne Zusatzkabel:** auf dem UV-K5-Display RSSI und grob gezählte kurze
  HF-Impulse auf 162,025 MHz anzeigen (keine AIS-Paketbestätigung).
- BK4819 beim Start und beim erneuten Einrichten des Empfängers auf
  **162,025 MHz (AIS B)** stellen, FM-Empfang, 25-kHz-Kanalfilter.
- 300-Hz-Hochpass, 3-kHz-Tiefpass und Deemphasis des RX-Audiowegs umgehen;
  experimentellen Digital-RX-AF-Bypass an den analogen Audioausgang legen.
- Squelch für den anfänglichen Empfangsversuch umgehen und den
  Energiesparmodus abschalten.
- **Nur RX:** PTT und mehrere interne Wege in den TX-Modus sind gesperrt.
- Mit dem unabhängigen C-Modul künstliche 9.600-bit/s-NRZI/HDLC-Daten
  auf AIS-Flags, Bit-Stuffing, CRC-16/X-25, Nachrichtentyp und MMSI prüfen.
- Mit dem neuen **PC-Audiodecoder** künstlich erzeugte GMSK-
  Diskriminator-Aufnahmen bei 48/96 kHz bis zur gültigen AIS-CRC decodieren.
- Wahlweise **zwei Empfangs-Audiowege** kompilieren: AF9 (Beken-Digital-
  Bypass, Standard) und AF1 (ungefilterter FM-Audioweg, Kontrollversuch).
  Der UART gibt nach dem Start eine Zeile mit den gelesenen BK4819-
  Registern aus.

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
`AIS-RX-AF9-AF1-EXPERIMENTAL`-Archiv laden und entpacken.
Es enthält die beiden rohen Test-Firmwares `ais-rx-af9-digital-bypass.bin`
(Standard) und `ais-rx-af1-fm-audio.bin` (Kontrollversuch). Diese
Test-Artefakte werden nur **sieben Tage** aufbewahrt. Sie gehören nicht zu den regulären
APRS-Releases und sind ausdrücklich keine getesteten AIS-Empfangsgeräte.

### Optional: Docker

Die angepasste Version des vorhandenen Skripts unterstützt `aisrx`:

```bash
./compile-with-docker.sh aisrx
# Ergebnis: compiled-firmware/ais-rx-162025-rx-only.bin
```

**Hinweis:** Der neue `aisrx`-Zweig im Docker-Skript vermeidet das
frühere globale `docker system prune`. Die weiterhin vorhandenen alten
Docker-Buildvarianten können dagegen ungenutzte Docker-Ressourcen bereinigen.

## Zweiter Meilenstein: zwei Audio-Routen und PC-WAV-Decoder

Wir haben **AF9 und AF1 getrennt gebaut**, damit sich überprüfen lässt,
welcher Weg die schnellen 9.600-bit/s-GMSK-Signale tatsächlich bis zum
Audioausgang durchlässt. Dabei ist **nicht** vorausgesetzt, dass beide
Wege funktionieren. Der BK4819 gibt beim Start über die bestehende UART
eine Zeile dieser Form aus:

```text
AISRX 162.025 AF9 38=.... 39=.... 43=.... 2B=.... 47=.... 58=....
```

Diese Zeile bestätigt die Registerwerte, **nicht** den erfolgreichen
AIS-Empfang. Eine eigene `AF1`-Firmware entsteht lokal so:

```bash
make clean
make -j4 ENABLE_AIS_RX=1 AIS_AF_ROUTE=1
cp f4hwn.bin ais-rx-af1-fm-audio.bin
```

**PC-Decoder kompilieren** (Linux/macOS; ein gewöhnlicher C-Compiler reicht):

```bash
cc -std=c11 -O2 -Wall -Wextra -Iapp \
  app/ais_bits.c utils/ais_af_decode.c utils/ais_af_wav.c \
  -lm -o ais_af_wav
```

Am Funkgerät ein paar Sekunden von Kanal B aufnehmen: Kopfhörer/AF-Ausgang
mit geeignet gedämpftem Audioeingang verbinden und **mono PCM16 mit
96 kHz** aufnehmen (48 kHz wird ebenfalls unterstützt). Das
Testprogramm akzeptiert **nur WAV-Dateien mit Mono-PCM16, 48/96 kHz**;
keine Stereo-Dateien, MP3s oder komplexen SDR-I/Q-Samples. Bei Bedarf
eine bereits vorhandene Aufnahme mit FFmpeg konvertieren:

```bash
ffmpeg -i af9-recording.wav -ar 96000 -ac 1 -c:a pcm_s16le af9-mono96.wav
./ais_af_wav af9-mono96.wav
```

Die Ausgabe nennt Audiopegel, gefundene HDLC-Flags und CRC-geprüfte
AIS-Nachrichten (zunächst **Typ und MMSI**, noch keine Positionen).
**Keine gültige CRC** bedeutet nicht automatisch, dass kein Schiff
sendet: RX-Bypass, Audiohardware, Filter, Pegel, Signalqualität oder
Symboltiming können die Ursache sein.

Der PC-Algorithmus ist vorerst ein bewusst einfacher Laborversuch:
langsamer DC-Abgleich, gaußförmiger Matched Filter (BT=0,4), 16
Timing-Hypothesen, NRZI- und HDLC-Decodierung mit CRC-16/X-25.
Die GitHub-Actions-Tests bestätigen dies für **synthetische**
48-/96-kHz-Aufnahmen, für invertierte Polarität sowie für manipulierte
CRC und reines Rauschen. Es liegt **noch keine nachgewiesene
On-Air-Decodierung** mit dem UV-K5 vor.

## Das erste erwartete Ergebnis am UV-K5

Nach dem Flashen zeigt der Startbildschirm `DR.H.DOOF v0.1` und
`AIS-RX Edition`. Danach erscheint die **eigenständige RSSI-/RF-Impuls-
Anzeige** auf dem UV-K5. Du kannst also zunächst **ohne Audiokabel**
prüfen, ob auf 162,025 MHz zeitlich kurze RF-Aktivität ankommt.
**Schiffe, MMSI, Koordinaten oder gültige AIS-Bits zeigt diese Version
noch nicht an.**

Der erste Test ist jetzt die integrierte RF-Impuls-Anzeige des Geräts.
Die ältere optionale Audioaufnahme mit dem PC-WAV-Decoder bleibt nur ein
**zusätzlicher Laborweg**, falls später ein passendes Kabel verfügbar ist.
Für echten, vollständig integrierten AIS-Empfang muss zuerst geklärt
werden, ob der BK4819 GMSK-Symbole ohne eine interne Hardwareänderung
in den Mikrocontroller liefern kann.
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
