# Evidence and release gates

This document separates known project inputs from assumptions that must not be promoted to hardware facts.

## Current known inputs
- The GitHub project is public and named D10S_K-_2026-11-14.
- The target requested by the user is Quansheng UV-K1 / UV-K1(8) V3.
- The user reports bootloader 7.03.01.
- The intended RF chip is BK4829.
- The intended display is ST7565 128x64.
- Karina is the preferred Spectrum behavior/UI reference; F4HWN Spectrum is explicitly not the desired interface.

## Source-derived research leads
- Armel's UV-K1/K5 V3 project is a useful source for PY32F071 platform/build conventions, but its Spectrum UI is not the design target.
- KA52M K1/V3 contains a substantial spectrum implementation and BK4829-named driver; its ancestry and individual register assumptions require review before reuse.
- Fagci's K1 scanner is a useful reference for measurement and scanning structure, but its fixed threshold/single-sample behavior is not sufficient for the D10S detector.
- Public Karina notes describe Delay/Turbo scan trade-offs and document range/cursor/memory-cell defects. They are behavior notes, not a verified complete source port.

## Hard safety rules
- Never treat an uncalibrated RSSI value as dBm.
- Never assume BK4819-labelled register tables are valid for BK4829 without corroboration.
- Never infer the exact RF-switch/LNA GPIO map from a different PCB revision.
- Never label a layout-test ELF or partial raw image as flashable firmware.
- Never flash the previously rejected 22 KB incomplete image.

## Required before production BIN
1. Pin and review the complete licensed platform source and toolchain.
2. Confirm the exact UV-K1 PCB revision and MCU marking.
3. Verify the BK4829 register profile and RF path selection for that board.
4. Verify LCD wiring, SPI mode, orientation and backlight.
5. Build the complete application with startup, linker script and all runtime modules.
6. Validate ELF vector table, memory bounds, raw image origin and page padding.
7. Validate the 7.03.01 DFU transfer format and recovery route.
8. Run hardware tests with known VHF/UHF signals and document RSSI/noise/peak behavior.

Until those gates are met, any output is source/test material, not a firmware image approved for flashing.
