# Spectrum One PCB Assembly  
Hardware version v0.1.0

This document describes PCB level assembly requirements for the Spectrum One hardware.

## Scope
This document covers PCB level assembly.

Firmware, electrical behaviour, calibration, and end user operation are out of scope.

## Bill of Materials
All components, values, quantities, and reference designators are defined in the Bill of Materials.

**The BOM is the single source of truth for parts.**

See:
- `hardware/v0.1.0/BOM.md`

This assembly document does not repeat the component list in order to avoid duplication and inconsistency.

## Board overview
- 2 layer FR4 PCB
- ESP32 development module installed using soldered pin header sockets
- Power supplied via the ESP32 module USB C connector (5 V)
- 3.3 V logic throughout the board

## Assembly notes

### ESP32 module mounting
- Pin header sockets are soldered to the PCB
- The ESP32 module is inserted into the sockets after soldering
- The ESP32 module itself is not soldered directly to the PCB

### LED bar array orientation
- The LED bar is a single multi LED package with a fixed pinout
- The component must be installed in the correct orientation as a whole
- Orientation is defined by the PCB silkscreen and the package pin 1 marking
- Installing the LED bar in reverse orientation will prevent correct operation

### LCD backpack connection
- LCD1602 I2C backpacks may use different pinout arrangements depending on manufacturer
- Pin order must be verified before any direct electrical connection
- The recommended approach is to connect the LCD backpack to the PCB using four wires terminated with a JST connector
- This avoids pinout mismatch issues and allows easier replacement

### LCD mounting
- The LCD module is mounted with a 10 mm standoff distance at four mounting points
- Ensure the LCD is mechanically supported and not under strain after installation

### Orientation
- ESP32 module orientation follows the PCB silkscreen outline and arrow
- LCD related connectors must follow PCB silkscreen markings
- Pin 1 indicators must be respected where present

### Polarised components
- Polarised components must be oriented according to PCB markings

### Non polarised components
- Resistors and other non polarised passives may be installed in either orientation
- The capacitor located adjacent to the JST connector is non polarised and may be installed in either orientation

## Mechanical considerations
- Ensure pin header sockets are aligned and seated squarely before soldering
- Insert the ESP32 module only after all socket joints have cooled
- Avoid excessive heat on connectors, headers, and JST wiring
- Inspect all solder joints for wetting and continuity

## Reference files
- PCB layout: `pcb/`
- Schematic: `schematic/`
- Footprints: `footprints/`
- Symbols: `symbols/`
- Gerbers: `gerbers/`
- BOM: `BOM.md`

All reference files listed above correspond to hardware version v0.1.0.

## Assembly reference images

### Assembled PCB
![Spectrum One assembled PCB v0.1.0](../../media/spectrum_one_v0.1.0_assembled_pcb.jpg)

This image shows a fully assembled Spectrum One PCB for hardware version v0.1.0.

### Reference parts set
![Spectrum One reference parts set v0.1.0](../../media/spectrum_one_v0.1.0_kit.jpg)

This image shows the physical components used in the reference build.
Equivalent parts meeting the BOM specifications may be used.

