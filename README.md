# Spectrum One

Spectrum One is a compact ESP32 based WiFi activity monitor that visualises
nearby WiFi activity using a 16x2 LCD and a 10 segment LED bar.

It makes RF behaviour more observable through repeated WiFi scans. Values shown
are derived from WiFi RSSI and are intended for visualisation and comparison,
not calibrated RF measurement.

This repository contains the reference implementation of the device.

---

## Repository contents

The repository is organised by release version and contains:

binaries/  
Versioned reference firmware binaries for flashing Spectrum One hardware.

firmware/  
ESP-IDF firmware source code.

hardware/  
KiCad schematics, PCB layouts, symbols, footprints, and fabrication outputs.

docs/  
Repository documentation and technical notes.

media/  
Images and diagrams used by the repository documentation.

---

## What this repository does not contain

This repository intentionally does not include:

- Book text or manuscripts
- Long-form instructional chapters
- Commercial publication layouts
- Book-specific photography or artwork

These materials are distributed separately.

---

## Hardware Assembly

This repository provides minimal assembly documentation to support reproduction of the open hardware design.

General assembly information is provided in `ASSEMBLY.md` at the repository root.

Version-specific assembly notes are located under:
- `hardware/v0.1.0/ASSEMBLY.md`

Full system design context and development documentation are provided separately in the published book.

---

## About the book

A separate book documents the development of Spectrum One from early breadboard
prototypes through to a finished PCB reference build.

The book covers hardware design decisions, firmware behaviour, display logic,
and the transition from experimental prototyping to a reproducible open
hardware design.

The book is published separately under the title:

ESP32 WiFi Activity Monitor: Spectrum One – From Breadboard Prototype to Custom PCB  
by Jay J. Reszka

The book is a commercial publication and is not included in this repository.

---

## Licensing

Licensing is separated by material type:

- Firmware source code and binaries  
  MIT License

- Hardware design files  
  CERN Open Hardware Licence v2.0 – Strongly Reciprocal (CERN-OHL-S-2.0)

- Repository documentation and media  
  Creative Commons Attribution 4.0 International (CC BY 4.0)

- Books and commercial written works  
  Proprietary. All rights reserved.

Full licensing terms are defined in `LICENSE.md`.



