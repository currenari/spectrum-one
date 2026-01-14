# Spectrum One – Bill of Materials

This document lists the physical components used in the Spectrum One reference hardware.

The BOM reflects the exact parts and construction choices used during development and validation.
It is provided to support reproducibility of the hardware design. Equivalent parts with matching
specifications may be used unless otherwise noted.

---

## ESP32 Development Board (1×)

- Type: ESP32 Dev Kit V1
- Module: ESP32-WROOM-32
- Form factor: 30-pin
- USB interface: USB-C

---

## Pin Headers for ESP32 Development Board (2×)

- Type: pin header
- Pitch: 2.54 mm
- Positions: 15-pin
- Rows: single row
- Mounting: through-hole
- Use: PCB to ESP32 development board connection

---

## LCD 1602 Display with I2C Backpack (1×)

- Display type: LCD 1602
- Controller: HD44780 compatible
- Display format: 16 characters × 2 lines
- Native interface: parallel (HD44780)

### I2C backpack

- Controller: PCF8574
- Interface: I2C
- Pinout: GND, VCC, SDA, SCL
- Default I2C address: 0x27
- Address selection: configurable via A0, A1, A2 jumpers

---

## JST XH Connector and Cable Assembly

### PCB-mounted connector (1×)

- Type: JST XH series
- Pitch: 2.54 mm
- Positions: 4-pin
- Mounting: through-hole, soldered directly to the PCB
- Use: LCD power and I2C connection

### Cable assembly (1×)

- Type: JST XH compatible cable
- Configuration: 4-pin
- One end: JST XH housing (mates with PCB connector)
- Other end: individual wires soldered directly to the LCD I2C backpack
- Note: no connector fitted on the LCD side in the reference build

---

## LED Bar Graph Display (1×)

- Part type: LED bar graph
- Model: HSN-2510SR
- Colour: red
- Segment count: 10 independent LEDs
- Pin count: 20 pins (10 per side)
- Electrical configuration: no common anode, no common cathode

---

## Resistors (10×)

- Resistance: 220 Ω
- Tolerance: 5 percent
- Power rating: 0.25 W
- Type: through-hole axial

---

## Capacitor (1×)

- Capacitance: 100 nF
- Type: ceramic
- Package: through-hole
- Purpose: local supply decoupling

---

## Push Button (1×)

- Type: momentary tactile switch
- Configuration: SPST, normally open
- Package: through-hole, 6 × 6 mm

---

## Display Mounting Hardware

### Nylon standoffs (4×)

- Type: hexagonal PCB standoff
- Material: nylon
- Thread: M3, female–female
- Length: 10 mm
- Use: spacing and mounting of LCD module above PCB

### Nylon screws (8×)

- Type: socket cap head screw
- Material: nylon
- Thread: M3
- Length: 6 mm
- Drive: hex (Allen)
- Use: fixing LCD module to nylon standoffs

---

## LCD I2C Address Note

The reference build uses an LCD1602 with a PCF8574 I2C backpack configured at address `0x27`.

Some visually identical LCD backpacks use address `0x3F` instead.
If the display remains blank, change the firmware LCD address accordingly and rebuild.

This behaviour is covered in detail in the firmware documentation.

