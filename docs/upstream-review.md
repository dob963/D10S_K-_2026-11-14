# Upstream source review

## Selected platform reference

Repository: [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom)

Reviewed branch commit: `3261ca7883d5b0b60c0905d8766612dee7b4191d` (main, 2026-10-30). GitHub identifies the project license as Apache-2.0. Any later reuse of its code must retain applicable copyright notices, license terms, and attribution.

This is the current strongest public reference for the PY32F071 UV-K1 / UV-K5 V3 platform layer. It provides the target startup, HAL, linker script, display and keyboard drivers, BK4829-named RF driver, and a reproducible build environment.

## Useful findings from the reviewed source

- `App/CMakeLists.txt` compiles `App/driver/bk4829.c`; `ENABLE_SPECTRUM` adds `App/app/spectrum.c`.
- The RF driver uses the historical `BK4819_*` API and register names. This is not merely inferred from filenames: the driver comments explicitly mention stock BK4829 behavior, and `App/radio.c` contains BK4829-specific modulation notes.
- The driver reads relative/raw RSSI as `BK4819_ReadRegister(BK4819_REG_67) & 0x01FF`, i.e. a 9-bit value. The D10S core therefore preserves raw relative units and does not claim dBm.
- Frequency tuning is represented through registers 0x38 and 0x39. The driver also includes an explicit VHF/UHF RF-path selector using BK4819 GPIO outputs for the VHF and UHF LNA paths.
- The build preset is CMake/Ninja with an ARM GNU toolchain file; the upstream firmware script builds in Docker. This is a realistic route to a complete target image after the D10S application and feature set are integrated.

## What D10S reuses and what it does not

Use this project as a reviewed reference for the target platform, MCU support, register transport, LCD/keypad integration, and build layout. Keep the Spectrum algorithm and user interface independent.

The F4HWN Spectrum source is not adopted as the D10S interface. Its scan/render implementation is useful for understanding the platform, but D10S uses its own detector and will implement the requested Karina-inspired interaction and typography separately.

Do not copy the upstream RF startup or LNA path as a blind register table. Before integration, trace the full init and receive path in the pinned source, verify the exact UV-K1 board revision and PCB routing, then perform hardware tests. The existence of a working upstream firmware for the same model family is strong evidence for a starting point, not proof that every RF register, GPIO or initialization step is correct for every hardware revision.

## Current D10S implementation

The first independent processing module is in `src/spectrum/spectrum_detect.c`. It receives one completed sweep from a future radio-acquisition layer. It is host-tested and cross-compiles to a freestanding Cortex-M0+ object; it is not yet linked with the upstream platform or a complete application image.
