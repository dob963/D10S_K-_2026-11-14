# Evidence and release gates

This document separates known project inputs from assumptions that must not be promoted to hardware facts.

## Current known inputs
- The GitHub project is public and named D10S_K-_2026-11-14.
- The requested target is Quansheng UV-K1; UV-K1(8) V3 Mini Kong compatibility is not assumed from the same family name.
- The user reports bootloader 7.03.01. Published tooling maps 7.03.01 to standard UV-K1; 7.00.07 is mapped to UV-K5 V3 / UV-K1(8) v3 Mini Kong. Confirm the exact model and MCU marking on the physical radio.
- The intended RF chip is BK4829.
- The intended display is ST7565 128x64.
- The Spectrum UI is an original D10S design; published behavior notes were reviewed as requirements, not copied as application source.

## Source-derived research leads
- Armel's UV-K1/K5 V3 project is a useful source for PY32F071 platform/build conventions, but its Spectrum UI is not the design target.
- KA52M K1/V3 contains a substantial spectrum implementation and BK4829-named driver; its ancestry and individual register assumptions require review before reuse.
- Fagci's K1 scanner is a useful reference for measurement and scanning structure, but its fixed threshold/single-sample behavior is not sufficient for the D10S detector.
- Public Karina notes describe Delay/Turbo scan trade-offs and document range/cursor/memory-cell defects. They are behavior notes, not a verified complete source port.
- DFU research now records raw-image handling, 256-byte page transfer and the 7.03.01 K1 allowlist in `docs/dfu-compatibility-evidence.md`. This is protocol-source review, not a radio flash test.

## Hard safety rules
- Never treat an uncalibrated RSSI value as dBm.
- Do not transplant a BK4819-labelled register table to BK4829 without target-specific corroboration. The reviewed Armel UV-K1/K5 V3 source explicitly uses its legacy BK4819 API against the BK4829 platform; that is a specific source-proven compatibility path, not a universal guarantee for unrelated boards or chip variants.
- Never infer the exact RF-switch/LNA GPIO map from a different PCB revision.
- Never label a layout-test ELF or partial raw image as flashable firmware.
- Never flash the previously rejected 22 KB incomplete image.

## Required before production BIN
1. Pin and review the complete licensed platform source and toolchain.
2. Confirm the exact UV-K1 model, PCB revision and MCU marking; distinguish standard UV-K1 from UV-K1(8) V3 Mini Kong.
3. Verify the BK4829 register profile and RF path selection for that board.
4. Verify LCD wiring, SPI mode, orientation and backlight.
5. Complete target build, startup and linker integration. **Implemented in CI:** Cortex-M0+ objects, full PY32F071 ELF link, vector-table validation (including reserved IRQ30), application memory bounds, raw image size/origin and stack-frame report.
6. **Partially completed by source review:** raw BIN handling, 256-byte DFU pages and the 7.03.01 standard-UV-K1 allowlist are documented. Still required: read the bootloader version from the actual radio, verify exact model and application address, and test the flash/recovery route on that unit.
7. Run hardware tests with known VHF/UHF signals and document RF-path, PLL/AGC, RSSI/noise/peak behavior, LCD and keypad behavior.
8. Measure the cumulative stack high-water mark and verify reset/recovery under power interruption.

Until those gates are met, any output is source/test material, not a firmware image approved for flashing.
