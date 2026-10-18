# Powering the ESP32-8048S070C from the Arduino Uno over the UART cable

Research date: 2026-10-18. Question: can the 7" Sunton ESP32-8048S070C be powered from the Arduino Uno (GRBL 1.1) through the 4-pin cable that carries TX/RX, using the Uno's 5V and GND?

## Answer

**Physically yes, but not recommended. The Uno cannot supply the board's current reliably.** The board's 4-pin "Power + Serial" connector P1 really is a power input. Pin 1 (`VIN`) feeds the board's main 5V rail through a P-MOSFET, so a wire from the Uno's 5V pin would power the board. However, the 7" board's backlight alone is rated 9.6 V / 120 mA (~270 mA at 5 V). With WiFi on, the board's total estimated draw is ~350 mA typical and ~550-730 mA peak at 5 V. A USB-powered Uno is limited to the 500 mA that a USB 2.0 port supplies, and that budget also has to cover the Uno itself. A barrel-jack-powered Uno is worse: its NCP1117 regulator can only dissipate enough heat for roughly 100-450 mA total at 9-12 V input. **Recommended wiring:** power the ESP32-8048S070C from its own 5 V supply rated ≥1 A (its USB-C, or a 5 V buck from the CNC PSU into P1 pin 1). Connect only **GND, TX, RX** to the Uno and leave P1 pin 1 unconnected. Add a resistor divider on Uno TX → ESP RX, because the board passes the Uno's 5 V signal to a 3.3 V-only pin through only a 100 Ω resistor.

## Power budget (all figures at 5 V)

| Load / source | Typical | Peak | Basis |
|---|---|---|---|
| ESP32-S3-WROOM-1, WiFi RX/listening | ~75 mA | — | 95-97 mA @ 3.3 V ([WROOM-1 Table 6-4](#src-wroom)), converted at an **assumed** 85 % buck efficiency |
| ESP32-S3-WROOM-1, WiFi TX (802.11b, 20.5 dBm) | — | ~276 mA | 355 mA @ 3.3 V peak ([WROOM-1 Table 6-4](#src-wroom)), same 85 % assumption |
| Espressif's supply-sizing requirement for the module | — | ~388 mA | "Current delivered by external power supply ≥ 0.5 A" @ 3.3 V ([WROOM-1 Table 6-2](#src-wroom)) |
| 7" backlight at full brightness (panel-rated current) | ~271 mA | ~339 mA | 9.6 V × 120 mA typ ([panel spec §9](#src-panel)), 85 % boost efficiency **assumed**. Peak = 25 mA/LED absolute max × 6 strings (string count is **inferred**, see §2) |
| Panel logic + gate-voltage DC-DC, GT911, CH340C, SD | **unknown** | **unknown** | Not published (see §2) |
| **ESP32-8048S070C total** | **~350 mA + unknowns** | **~550 mA (TX) to ~730 mA (design bound) + unknowns** | Sum of the rows above |
| Uno 5V pin, Uno on USB | ≤ 500 mA **shared with Uno + shield logic** | polyfuse trips at 1.0 A | USB 2.0 high-power port; MF-MSMF050-2 hold 0.5 A / trip 1.0 A ([Bourns](#src-bourns)) |
| Uno 5V pin, barrel jack 9 V | 195 mA (min pad) - 446 mA (large copper) | same | NCP1117 SOT-223 thermal limit, T_A = 25 °C, T_J = 150 °C (§3b) |
| Uno 5V pin, barrel jack 12 V | 112 mA - 255 mA | same | same |

The barrel-jack figures are the **total** load on the Uno's 5 V rail, including the ATmega328P, ATmega16U2, LEDs and CNC-shield logic. Arduino does not publish the Uno's own consumption: the official datasheet's "Maximum Power Consumption" row literally reads "xx mA" ([Uno R3 datasheet §2.2](#src-uno-ds)).

---

## 1. The ESP32-8048S070C UART connector (P1)

Source: the vendor schematic "ESP32-8048S070", sheet *MCU*, block **POWER**, revisions V1.0 (2023-01-30) and V1.1. It is from 深圳市晶彩智能有限公司 (Shenzhen Jingcai Intelligent, i.e. Sunton). I could not reach it on the vendor's own site; it is mirrored in [rzeldent/platformio-espressif32-sunton](#src-sch-mcu10). That repo also lists the connector as "Power + Serial: JST1.25 4p".

P1 is an `HC-1.25-4PWT` (JST-1.25 style, 4 pins + 2 shell pins to GND):

| P1 pin | Net | Goes to |
|---|---|---|
| 1 | `VIN` | Drain (pin 3) of **Q1 AO3401** P-MOSFET → source (pin 2) = board **`5V`** rail |
| 2 | `TXD2` | **R6 100 Ω** → `U0TXD` (ESP32-S3 GPIO43, the ESP's **output**) |
| 3 | `RXD2` | **R7 100 Ω** → `U0RXD` (ESP32-S3 GPIO44, the ESP's **input**) |
| 4 | `GND` | GND (shell pins 5/6 also GND) |

So: Uno D0 (RX) → P1 pin 2, Uno D1 (TX) → P1 pin 3, GND → pin 4. The net names "TXD2/RXD2" are just labels; they are wired to UART0.

**Can the board be powered through it? Yes.** Q1's gate (pin 1) is tied straight to GND. The symbol's body diode points from D (`VIN`) to S (`5V`). Power entering on pin 1 first passes through the body diode. Once the rail rises past the gate threshold, V_GS ≈ −5 V and the channel switches fully on. This is the standard "P-FET reverse-polarity protection" arrangement. The board's `5V` rail is the same rail that USB-C feeds through D1 and that supplies the 3.3 V buck (U3, sheet MCU block *3.3V*) and the backlight boost driver (U5, block *LED Driver*). Consequence: **P1 pin 1 is not an isolated input.** A fully enhanced MOSFET conducts in both directions, so whenever the board is powered from USB-C, the rail voltage also appears on P1 pin 1 (see §6).

**Protection:** only reverse-polarity protection (Q1). No fuse, no TVS and no over-voltage protection is drawn on P1. **Never connect P1 pin 1 to the Uno's `VIN` pin** (7-12 V jack voltage). P1 pin 1 drives the 5 V rail directly, and the backlight driver and 3.3 V buck sit on it.

## 2. Current draw of the ESP32-8048S070C

**ESP32-S3 module.** ESP32-S3-WROOM-1 datasheet v1.8 ([source](#src-wroom)):
- Table 6-4 (p. 28), at 3.3 V and 100 % TX duty: TX 802.11b 1 Mbps @ 20.5 dBm **355 mA peak**; 802.11g 54 Mbps 297 mA; 802.11n HT20 286 mA. RX 95 mA (HT20) / 97 mA (HT40).
- Table 6-2 (p. 27): the external supply must deliver **≥ 0.5 A**.

The board makes 3.3 V with a buck converter (U3; pins BS/SW/GND/FB/IN/EN, schematic block *3.3V*). The schematic does **not** give U3's part number, so its efficiency is **unverified**. The 85 % used in the table is an assumption. The U3 part marking plus its datasheet would settle it.

**Backlight.** Panel spec "JC8048B070N" from the same vendor ([source](#src-panel)):
- p. 3 general spec: "Backlight Type: 18 white leds 9.6V/120mA".
- §9 Backlight Characteristic: V_LED 9.6 V typ, I_LED 120 mA typ.
- §5.1: LED forward current 25 mA max per LED.

That is 1.15 W of LED power at the rated current. The LEDs are driven by a boost converter U5 (VIN/SW/EN/OV/FB/GND) from the `5V` rail, enabled by GPIO2 `BL_CTR` through R13. The schematic does **not** give U5's part number or the values of the current-sense resistors R12/R30. The actual LED current the board is set to is therefore **unverified**: it may be below the panel's 120 mA. The upper bound comes from the panel ratings. 18 LEDs at 9.6 V implies 3 in series × 6 parallel strings (an **inference**, 3.2 V/LED). At 25 mA/string absolute max that gives 150 mA (1.44 W). The firmware PWM-dims via GPIO2 (`-DTFT_BACKLIGHT_PIN=2` in `platformio.ini`), so reducing brightness reduces this load roughly in proportion. That proportionality is also unverified without the U5 datasheet.

**Panel logic.** The panel's DVDD is 3.0-3.6 V (spec §5.2). AVDD (10.4 V), VGH (16 V), VGL (−7 V) and VCOM are generated on-board from 3.3 V (sheet *LCM*, block *DC-DC*). The panel spec publishes **no** supply-current figures, so this load is **unknown**. GT911, CH340C and the SD card currents were not researched.

**Realistic figure.** ~350 mA typical (WiFi associated, backlight 100 %) and ~550 mA during WiFi TX bursts, plus the unknown panel-logic load. Espressif's 0.5 A sizing rule plus the backlight's absolute max gives a design bound of ~730 mA. **The definitive answer is a measurement:** put a USB-C power meter in line with the board while it serves a web upload with the backlight at 100 %.

## 3. What the Uno's 5V pin can supply

Arduino UNO R3 official schematic, "UNO-TH_Rev3e" (2019-06-03) ([source](#src-uno-sch)).

### 3a. Uno powered by USB
USB-B VBUS → **F1 MF-MSMF050-2 "500mA"** polyfuse → `USBVCC` → **T1 FDN340P** P-MOSFET → `+5V`. Comparator U5A (LMV358) compares VIN/2 (RN1A/RN1B 10 k/10 k) with +3V3. It switches T1 **off** when VIN > ~6.6 V, i.e. when the barrel jack is used, and **on** otherwise.
- Bourns MF-MSMF050 ([source](#src-bourns), Electrical Characteristics table): I_hold 0.50 A and I_trip 1.00 A at 23 °C.
- The USB 2.0 spec allows a high-power port to supply 500 mA (5 unit loads, USB 2.0 §7.2.1, [usb.org](#src-usb)). Not re-read for this note.
- Usable for the display: 500 mA minus the Uno and CNC-shield logic (both unpublished). That covers the ESP board's **typical** load but **not its peak with margin**. The polyfuse will not trip below ~1 A, but the PC/hub port may brown out or shut down. A brownout resets GRBL mid-job, and GRBL then loses machine position.

### 3b. Uno powered by barrel jack / VIN
Jack → D1 (M7, 1N4007-class) → `VIN` → **U1 NCP1117ST50T3G** (SOT-223) → `+5V` (schematic).

Note: the Uno R3 *datasheet* BOM instead lists "U1 SPX1117M3-L-5 Regulator" ([Uno datasheet](#src-uno-ds), BOM table). Board revisions may use either part. Both are 1117-class SOT-223 LDOs. Only the NCP1117 numbers were checked.

NCP1117 datasheet Rev. 31 ([source](#src-ncp1117)):
- Maximum Ratings (p. 2): SOT-223 R_θJA = **160 °C/W** (minimum-size pad), R_θJC = 15 °C/W, T_J max 150 °C, V_in max 20 V. Note 2: thermal shutdown ≈ 175 °C. Note 3: I_out ≤ 1.0 A when V_in > 12 V.
- p. 3: output current limit 1000 mA min / 1500 mA typ at 25 °C.
- **Figure 21 (p. 8):** with 2 oz copper, R_θJA falls from 160 °C/W to ~70 °C/W at 30 mm × 30 mm copper. The Uno's actual copper area around U1 was not measured, so the real figure lies between these two (**unverified**).

Thermal budget, P = (V_in − 5 V) × I. The calculation ignores the D1 drop, which is conservative: D1 lowers V_in to the regulator by roughly 0.7-1 V and helps slightly (drop **unverified** for this exact diode).

| Jack voltage | T_A | P_max (160 °C/W … 70 °C/W) | I_max total on Uno 5 V |
|---|---|---|---|
| 9 V | 25 °C | 0.78 W … 1.79 W | **195 … 446 mA** |
| 9 V | 40 °C (enclosure) | 0.69 W … 1.57 W | 172 … 393 mA |
| 12 V | 25 °C | 0.78 W … 1.79 W | **112 … 255 mA** |
| 12 V | 40 °C | 0.69 W … 1.57 W | 98 … 224 mA |

When the regulator overheats, its thermal shutdown cycles the Uno's 5 V. The 800 mA / 1 A headline rating is not reachable at these input voltages. **A jack-powered Uno cannot power the 7" board.**

## 4. CNC Shield V3 considerations

The designer of the Protoneer Arduino CNC Shield (Bertus Kruger) states on the product page: "The shield has a single power input for the stepping driver side of it and the logic bit gets powered by the Arduino" ([Protoneer, comment 2014-06-04](#src-protoneer)). So the shield's driver logic (A4988/DRV8825 VDD, endstop pull-ups) is an extra load on the same Uno 5 V rail. Motor power is a separate input and does not feed the Uno's 5 V. The same page says V3.00 added a "Comms Header (RX+TX, I2C)" and V3.01 "Added a 5V Breakout". These may be a convenient tap point for GND/TX/RX, but the exact pinout of those headers was not checked against a schematic (**unverified**). The Protoneer site did not resolve at research time; the Web Archive snapshot of 2025-01-14 was used.

**Noise / ground loops:** I found no primary source covering ground-loop or noise behaviour when the display and the CNC electronics share GND, so this is **unverified**. A GND wire is needed for UART regardless.

## 5. Logic levels on the same cable (safety)

- **Uno TX → ESP RX: out of spec.** Uno D1 is the ATmega328P's 5 V push-pull TXD, which reaches P1 pin 3 → R7 **100 Ω** → `U0RXD` (GPIO44). The board has **no level shifter or divider**, only the 100 Ω series resistor (schematic, block POWER). ESP32-S3 datasheet v2.2 ([source](#src-s3)):
  - Table 5-4 (p. 65): V_IH max = **VDD + 0.3 V** (3.6 V).
  - Table 5-1 (p. 64): input power pins 3.6 V max.

  A 5 V input is therefore outside the rated range. The excess current flows into the pad's protection structure, limited only by 100 Ω. Espressif publishes no injection-current rating, so the long-term effect is **unverified**. Fix: a divider at the ESP end, e.g. Uno TX → 1 kΩ → P1 pin 3, with 2 kΩ from P1 pin 3 to GND (≈3.3 V), or a proper level shifter.
- **ESP TX → Uno RX: not guaranteed by the datasheets, works in practice.**
  - ESP32-S3 V_OH min = 0.8 × VDD = 2.64 V (Table 5-4, high-impedance load).
  - ATmega328P V_IH min = 0.6 × V_CC = **3.0 V** at V_CC = 2.4-5.5 V (Microchip DS40002061B, Table 30-1, p. 322, [source](#src-m328)).

  The README's "ESP32 3.3V output exceeds the ATmega328P's logic-high threshold" (README "Wire Connections" note) is true only for typical, not worst-case, values. Helping in practice: on the Uno, D0 is also pulled toward the ATmega16U2's idle-high TXD through **RN4A 1 kΩ** (Uno schematic, nets `M8TXD`/D0), so the ESP mainly has to pull it low.
- **Contention with the on-board USB-UART.** The CH340C's TXD is wired **directly** to `U0RXD`, on the same node that P1 pin 3 reaches through R7 (schematic block *USB to TTL*, U2 pin 2). Both the CH340C and the Uno can therefore drive GPIO44 at once. That is why the repo already says to disconnect the Arduino before flashing. How strongly the CH340C drives when idle is **unverified**; the WCH CH340 datasheet's output drive specs would settle it.
- **Current repo documentation:** `README.md` documents wiring only for the 2.8" CYD (GPIO27/22, 3 wires, no 5 V). The `esp32-8048S070C` env in `platformio.ini` uses GPIO43/44 and does **not** define `GRBL_SHARED_SERIAL`. Per its comment, that is intentional for debugging, but it means debug logging also goes out on the UART that GRBL listens to. No 7"-specific wiring is documented.

## 6. Flashing interaction: back-feed paths if P1 pin 1 is wired to Uno 5V

From the two schematics:

1. **ESP USB-C → Uno.** USB VBUS → **D1 1N5819W** Schottky → ESP `5V` rail → **Q1 (fully on, conducts both ways)** → P1 pin 1 → Uno `+5V`. With the Uno otherwise unpowered, the ESP's USB port would power the whole Uno plus the CNC-shield logic.
2. **Into the Uno's USB.** Uno `+5V` → **T1 FDN340P**, which is *on* whenever there is no jack power (its body diode points USBVCC → +5V, but the channel conducts both ways when on) → F1 → Uno USB-B VBUS. Back-feeding the Uno's USB connector is possible whenever the Uno has no jack power.
3. **Into the ESP's USB.** D1 (Schottky, cathode on the board rail) blocks current from the Uno's 5 V back into the ESP's USB-C VBUS. The ESP rail sits one Schottky drop (a few hundred mV, exact value **unverified**) below its VBUS. So when both are present, the Uno's 5.0 V side carries most of the ESP board's load.
4. **Both USB cables plugged into the same PC.** The two hosts' VBUS rails get tied together through T1 and Q1, separated only by D1. The ESP-side host cannot push current into the Uno-side host's VBUS by more than the Schottky allows. The Uno-side host's port, however, supplies the ESP board's load as well, which aggravates §3a.

Not wiring P1 pin 1 avoids all of these. The data-line contention on UART0 during flashing (§5) is separate and exists either way.

---

## Sources

- <a id="src-sch-mcu10"></a>Sunton/Jingcai "ESP32-8048S070" schematic, sheet MCU, V1.0 (2023-01-30): https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/assets/schematics/ESP32-8048S070-MCU-V1.0.jpg. V1.1: https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/assets/schematics/ESP32-8048S070-MCU-V1.1.png. Sheet LCM V1.1: https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/assets/schematics/ESP32-8048S070-LCM-V1.1.png. Connector list: https://github.com/rzeldent/platformio-espressif32-sunton (vendor files, mirrored by a third party)
- <a id="src-panel"></a>Panel spec JC8048B070N (Jingcai), pp. 3, 6 (§5), 10 (§9): https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/assets/lcd/JC8048B070N.pdf
- <a id="src-wroom"></a>Espressif ESP32-S3-WROOM-1/1U Datasheet v1.8, Tables 6-2, 6-3, 6-4 (pp. 27-28): https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf
- <a id="src-s3"></a>Espressif ESP32-S3 Series Datasheet v2.2, Tables 5-1, 5-4 (pp. 64-65): https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- <a id="src-uno-sch"></a>Arduino UNO R3 schematic (UNO-TH_Rev3e): https://docs.arduino.cc/resources/schematics/A000066-schematics.pdf
- <a id="src-uno-ds"></a>Arduino UNO R3 datasheet / user manual (A000066), §2.2 Power Consumption, BOM: https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf
- <a id="src-ncp1117"></a>onsemi NCP1117 datasheet Rev. 31 (Aug 2021), pp. 2, 3, 8 (Fig. 21): https://www.onsemi.com/pdf/datasheet/ncp1117-d.pdf
- <a id="src-bourns"></a>Bourns MF-MSMF series datasheet, Electrical Characteristics: https://www.bourns.com/docs/Product-Datasheets/mf-msmf.pdf
- <a id="src-m328"></a>Microchip ATmega48A/PA/88A/PA/168A/PA/328/P datasheet DS40002061B, Table 30-1 (p. 322): https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf
- <a id="src-protoneer"></a>Protoneer, "Arduino CNC Shield" (Web Archive 2025-01-14): https://web.archive.org/web/20250114193502/https://blog.protoneer.co.nz/arduino-cnc-shield/
- <a id="src-usb"></a>USB-IF, USB 2.0 Specification, §7.2.1: https://www.usb.org/document-library/usb-20-specification

### Open items that would firm this up
- Part numbers of U3 (3.3 V buck) and U5 (backlight boost), plus R12/R30 values: read the chip markings on the board.
- A measured current draw of the board at 5 V (USB-C power meter, backlight 100 %, during a WiFi upload).
- The Uno's own 5 V consumption with the CNC shield fitted (measure at the USB port).
