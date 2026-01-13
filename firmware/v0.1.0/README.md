# Spectrum One

**ESP32 WiFi Activity Monitor**  
From Breadboard Prototype to Custom PCB

Spectrum One is a compact ESP32-based device that visualises WiFi activity using a 16×2 LCD and a 10-segment LED bar.

It makes otherwise invisible RF behaviour observable through repeated WiFi scans. 
Values shown are derived from WiFi RSSI and are intended for visualisation and comparison, not calibrated measurement.

---

## What this repository contains

This repository contains the **firmware source code** and related open hardware design files for Spectrum One.

Specifically:
- ESP32 firmware (MIT licensed)
- Hardware schematics and PCB files (CERN-OHL-S)
- Configuration headers and build files

It does not contain:
- The book text
- Manuscripts or instructional chapters

These are published separately and are not part of this repository.


---

## Book and documentation

The project is fully documented in the book:

**ESP32 WiFi Activity Monitor**  
**Spectrum One · From Breadboard Prototype to Custom PCB**

The book explains:
- Design decisions
- RF behaviour and limitations
- Firmware architecture
- Hardware revisions
- Measurement heuristics
- Practical experiments

The book is proprietary and funds ongoing development of the project.
It is available through major book platforms.

---

## Physical builds

Ready-to-use physical builds of Spectrum One are available directly from the author.

Buying a physical device:
- Supports further development
- Does not change the open source status of the firmware or hardware
- Does not grant rights to the book content

---

## Licensing overview

This project uses **separated licensing by material type**.

- **Source code and binaries**  
  MIT License

- **Hardware designs (schematics, PCB, Gerbers)**  
  CERN Open Hardware Licence v2 – Strongly Reciprocal

- **Books and written material**  
  Proprietary, all rights reserved

Full terms are defined in the `LICENSE` file.

---

## Build the firmware

ESP-IDF is required.

```bash
idf.py build
idf.py flash
idf.py monitor
