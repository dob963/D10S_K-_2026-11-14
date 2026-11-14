# Evidence and release gates

This document separates known project inputs from assumptions that must not be promoted to hardware facts.

## Current known inputs
- The GitHub project is public and named D10S_K-_2026-11-14.
- The target requested by the user is Quansheng UV-K1 / UV-K1(8) V3.
- The user reports bootloader 7.03.01.
- The intended RF chip is BK4829.
- The intended display is ST7565 128x64.
- The Spectrum UI is an original D10S design; published behavior notes were reviewed as requirements, not copied as application source.

## Source-derived research leads
- Armel's UV-K1/K5 V3 project is a useful source for PY32F071 platform/build conventions, but its Spectrum UI is not the design target.
- KA52M K1/V3 contains a substantial spectrum implementation and BK4829-named driver; its ancestry and individual register assumptions require review before reuse.
- Fagci's K1 scanner is a useful reference for measurement and scanning structure, but its fixed threshold/single-sample behavior is not sufficient for the D10S detector.
- Public Karina notes describe Delay/Turbo scan trade-offs and document range/cursor/memory-cell defects. They are behavior notes, not a verified complete source port.

## Hard safety rules
- Never treat an uncalibrated RSSI value as dBm.
- Do not transplant a BK4819-labelled register table to BK4829 without target-specific corroboration. The reviewed Armel UV-K1/K5 V3 source explicitly uses its legacy BK4819 API against the BK4829 platform; that is a specific source-proven compatibility path, not a universal guarantee for unrelated boards or chip variants.
- Never infer the exact RF-switch/LNA GPIO map from a different PCB revision.
- Never label a layout-test ELF or partial raw image as flashable firmware.
- Never flash the previously rejected 22 KB incomplete image.

## Required before production BIN
1. Pin and review the complete licensed platform source and toolchain.
2. Confirm the exact UV-K1 PCB revision and MCU marking.
3. Verify the BK4829 register profile and RF path selection for that board.
4. Verify LCD wiring, SPI mode, orientation and backlight.
5. Complete target build, startup and linker integration. **Implemented in CI:** Cortex-M0+ objects, full PY32F071 ELF link, vector-table validation (including reserved IRQ30), application memory bounds, raw image size/origin and stack-frame report.
6. Validate the 7.03.01 DFU transfer format, image page programming and recovery route on the exact radio.
7. Run hardware tests with known VHF/UHF signals and document RF-path, PLL/AGC, RSSI/noise/peak behavior, LCD and keypad behavior.
8. Measure the cumulative stack high-water mark and verify reset/recovery under power interruption.

Until those gates are met, any output is source/test material, not a firmware image approved for flashing.
