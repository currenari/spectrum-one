# Spectrum One

ESP32 WiFi Activity Monitor  
From Breadboard Prototype to Custom PCB

Spectrum One is a compact ESP32 based device that visualises nearby WiFi activity using a 16x2 LCD and a 10 segment LED bar.

It makes RF behaviour more observable through repeated WiFi scans.
Values shown are derived from WiFi RSSI and are intended for visualisation and comparison, not calibrated measurement.

## What this repository contains

This repository contains the reference implementation of Spectrum One, organised by release version.

binaries/
Versioned reference firmware binaries.

firmware/
ESP IDF firmware source code, organised by release version.

hardware/
KiCad schematic, PCB, symbols, footprints, and fabrication outputs, organised by release version.

## What this repository does not contain

This repository excludes the book text, manuscripts, and instructional chapters.

The book is published separately.

## Book scope

This book provides a complete build reference for the Spectrum One device.

It includes:
- Breadboard wiring layouts and ESP32 pin usage
- A tested firmware build
- LED and LCD display behaviour
- Descriptions of how the device responds in use
- A bill of materials with part references
- The transition from a working breadboard build to a stable PCB version

The material is intended to support rebuilding, modification, and further development.

## Licensing overview

This project uses separated licensing by material type.

Source code and binaries  
MIT License

Hardware designs  
CERN Open Hardware Licence v2, Strongly Reciprocal

Book and written material  
Proprietary, all rights reserved

Full terms are defined in LICENSE.md.

## Start here

Firmware build and flash steps live in:
firmware/v0.1.0/README.md
