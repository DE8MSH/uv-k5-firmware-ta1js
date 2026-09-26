# AIS-RX für UV-K5/K6 — GMSK-Machbarkeitszweig, **kein fertiger AIS-Decoder**

> **Wichtiger Status (26. September 2026):** Der BK4819 kann in diesem
> Branch auf **162,025 MHz** empfangen, und die alte 1.200-Baud-APRS-Firmware
> wurde vom experimentellen AIS-Build getrennt. Die bestehenden Tests
> validieren **synthetische** AIS-Symbole und aufgezeichnete synthetische
> Audiosignale. **Ein brauchbarer 9.600-bit/s-GMSK-Datenweg vom BK4819 in
> den DP32G030 auf der unveränderten Platine ist bisher nicht nachgewiesen.**
> Es werden **keine echten AIS-Nachrichten im Funkgerät** decodiert.

**AIS-Branch: Dr. Heinz Doofenshmirtz** (auf dem Display: `DR.H.DOOF`).
Die früheren Entwickler und die unveränderten Lizenzhinweise stehen
in [AUTHORS.md](AUTHORS.md) und [LICENSE](LICENSE). Die vollständige
technische Analyse ist in
[docs/ON_DEVICE_FEASIBILITY.md](docs/ON_DEVICE_FEASIBILITY.md).

## Weshalb wir den Impulszähler wieder entfernt haben

Der bisherige Bildschirm `RF PULSES` konnte **Rauschen, Störungen,
andere Funkübertragungen und echte AIS-Bursts nicht unterscheiden**.
Ein 20–120-ms-Impuls oder ein RSSI-Anstieg belegt **keine AIS-Bits**.
Ein solcher Zähler war deshalb kein brauchbarer AIS-Entwicklungsschritt.
Er wurde samt sämtlicher Implementierung und Zeitgeber-Aufrufe gelöscht.

Die experimentelle Firmware zeigt derzeit nur ihren **nachweisbaren**
Status an: eingestellter AIS-B-Kanal, RX-only, **GMSK-Datenweg
ungeprüft und AIS-Decoder aus**. Sie behauptet keinen Empfang von
Schiffen und zeigt keine erfundenen AIS-Zählerstände.

## Warum „alles im Funkgerät“ noch nicht funktioniert

Ein autonomer AIS-Empfänger braucht aus dem BK4819 echte
**9.600-Symbole/s-GMSK-Entscheidungen** oder ein ausreichend schnell
digitalisiertes FM-Diskriminatorsignal. Erst daraus können wir
NRZI, HDLC, Bit-Stuffing und CRC-16/X-25 tatsächlich prüfen.

- Die dokumentierten BK4819-FSK-/FFSK-Modi sind nicht als
  9.600-bit/s-GMSK-Empfänger beschrieben. Einfach die APRS-Baudrate
  im Register zu vervielfachen funktioniert nicht.
- Die AF-Ausgänge des BK4819 führen zur analogen Audiostufe.
  Die bisher für diese Platine konfigurierten MCU-ADC-Eingänge
  messen **Batteriespannung und Strom**, nicht das RX-Audiosignal.
- Rob Riggs (Mobilinkd) hat am UV-K6 [9.600-Baud-GMSK-Empfang
  über einen externen TNC nach **Hardwareänderungen**](https://github.com/mobilinkd/uv-k6-digital-mod/blob/master/UV-K6.ipynb)
  dokumentiert. Das demonstriert eine mögliche HF-/Audioqualität,
  aber **keinen integrierten GMSK-Eingang des Original-Mikrocontrollers**.

**Es gibt bislang keinen belegten Firmware-only-Weg für einen
vollständigen AIS-Empfänger auf diesem unveränderten UV-K5.**
Das ist keine Behauptung, dass ein bislang unbekannter BK4819-Modus
unmöglich wäre; wir haben einen solchen Modus aber nicht identifiziert.

Das einzig sinnvolle nächste On-Device-Ziel ist entweder der Nachweis
eines undokumentierten 9.600-Symbol/s-Digitalausgangs, der einen
bereits verdrahteten MCU-Eingang erreicht, oder eines bestehenden
ausreichend schnellen AF-zu-ADC-Pfads. Ohne einen dieser Wege
kann die Firmware keine echten AIS-Bits erhalten.
Ein valides **CRC-geprüftes AIS-Testpaket mit bekannter MMSI, komplett
im UV-K5 decodiert**, ist unsere nächste Akzeptanzbedingung.

## Was dieser Branch tatsächlich enthält

- Experimentelle **162,025-MHz-RX-only-Konfiguration**, Wide-FM,
  ohne Squelch/Powersave und mit mehrfacher Software-TX-Sperre.
- Zwei **optionale Labor-AF-Routen**: AF9 (digitaler RX-Bypass)
  und AF1 (gewöhnlicher FM-Audioausgang mit deaktivierten
  Software-Audiofiltern). Diese AF-Routen gehen **nicht**
  automatisch in einen ADC des Mikrocontrollers.
- `app/ais_bits.[ch]`: C-Prüfer für NRZI/HDLC/Bit-Stuffing,
  CRC-16/X-25 sowie AIS-Typ und MMSI **nach**
  einer noch fehlenden GMSK-Demodulation.
- `utils/ais_af_decode.[ch]` und `utils/ais_af_wav.c`:
  **PC-only** GMSK-Versuch für echte oder synthetische
  Mono-PCM16-WAVs bei 48/96 kHz.
- CI-Tests für künstliche AIS-Pakete, invertierte Polarität,
  korrupte CRCs und Rauschen. **Keiner dieser Tests simuliert
  die elektrisch fehlende BK4819-zu-MCU-Verbindung.**

Die alte APRS-Firmware läuft weiter unter
[`main`](https://github.com/DE8MSH/uv-k5-firmware-ta1js/tree/main).

## Kompilieren

Voraussetzung ist `arm-none-eabi-gcc` 10.3.1, Git und Make:

```sh
git clone --branch ais-rx https://github.com/DE8MSH/uv-k5-firmware-ta1js.git
cd uv-k5-firmware-ta1js
make clean
make -j4 ENABLE_AIS_RX=1 AIS_AF_ROUTE=9
arm-none-eabi-size f4hwn
```

Das Ergebnis `f4hwn.bin` muss unter 61.440 Bytes liegen.
`make ENABLE_AIS_RX=1 AIS_AF_ROUTE=1` erzeugt die alternative
AF1-Testversion. Alternativ kompiliert
`./compile-with-docker.sh aisrx` den AF9-Stand.

Die gesonderte [AIS-RX-GitHub-Actions-Pipeline](https://github.com/DE8MSH/uv-k5-firmware-ta1js/actions/workflows/ais-rx.yml)
baut und testet beide Varianten. Ihre .bin-Artefakte sind **nur
technische Labor-Builds, keine funktionsfähigen AIS-Empfänger**.
Ohne den nachgewiesenen GMSK-Datenweg lohnt ein Flashen für das
eigentliche AIS-Ziel derzeit **nicht**. Wer dennoch experimentiert:
vorher EEPROM sichern, nicht auf die Anzeige zur Navigation verlassen
und die TX-Sperre am Gerät prüfen.

## Projektstand und Quellen

Die eigentliche technische Entscheidung steht in
[docs/ON_DEVICE_FEASIBILITY.md](docs/ON_DEVICE_FEASIBILITY.md).
Weitere BK4819-Register und die optionalen AF9/AF1-Labortests stehen
in [docs/AIS_RX.md](docs/AIS_RX.md).
Die ursprüngliche APRS-Entwicklung ist weiterhin in
[docs/APRS.md](docs/APRS.md) dokumentiert.

Für 9.600-baud-Digitalbetrieb des UV-K6 unter Verwendung **eines
externen TNC und PCB-Hardwareänderungen** siehe
[Rob Riggs / Mobilinkd](https://github.com/mobilinkd/uv-k6-digital-mod/blob/master/UV-K6.ipynb).
Das Notebook ist eine **externe Quelle, kein Bestandteil** unseres
Codes und keine Bestätigung eines internen Decoder-Datenwegs.

## Urheberrecht

Experimenteller AIS-Zweig: **Dr. Heinz Doofenshmirtz**.
Die dokumentierten Vorgänger DE8MSH/TA1JS, F4HWN/Armel,
Egzumer, OneOfEleven, DualTachyon, fagci, F4JTV sowie die
weiteren im vorherigen README genannten Mitwirkenden sind
in [AUTHORS.md](AUTHORS.md) aufgeführt. Alle vorhandenen
Datei-Copyright-Vermerke und die [Apache-2.0-Lizenz](LICENSE)
bleiben unverändert.
