# D10S PY32F071 target image

This folder contains the first integrated target entry point for the receive-only Spectrum application.

## Target contract

- Core: ARM Cortex-M0+ / reported PY32F071xB. Exact MCU marking and PCB revision remain unconfirmed.
- RF: BK4829, using the source-derived opt-in initialization profile.
- Display: ST7565, 128x64.
- Application origin: `0x08002800`.
- Reported memory: 128 KiB flash / 16 KiB SRAM; application flash region is configured as 118 KiB.
- Initial scan spans: 144–146 MHz and 430–440 MHz, 129 bins (128 exact intervals) per band. No other target bands are enabled yet.
- Initial dwell: 8 ms per bin. This is a provisional software setting, not a hardware-measured optimum.
- Initial mode: receive-only. No PTT, transmit, beacon or messenger path is implemented.

### Key map

- `1`: select VHF 144–146 MHz.
- `2`: select UHF 430–440 MHz.
- `3`: toggle adaptive Turbo polling.
- `4`: cycle Delay through 8 / 12 / 16 / 24 ms.
- `5`: cycle six BK4829 REG_13 receive-gain presets: ATT 0, ATT -6, ATT -15, ATT -27, BYP and BYP+.
- `6`–`8`: reserved.
- `9`: freeze the last complete sweep; next press resumes the current band from bin zero.
- `MENU/0`: toggle/place cursor; `UP/DOWN`: move one bin; `SIDE1/SIDE2`: move eight bins.
- `F`: toggle normal/moderately bold labels; `STAR`: toggle peak-marker visibility.

Delay and gain changes are deferred to safe sweep boundaries. A gain change invalidates stale detector/snapshot data and applies a 40 ms software settling interval before the next RSSI sample. This interval and the gain response have not yet been validated on the physical radio. REG_13 stages are source-derived presets, not measured attenuation values in dB.

## Startup and linker

`startup.c` defines the 48-word PY32F071 vector table, leaves the reserved external IRQ30 slot empty, relocates VTOR to the application base, initializes .data/.bss and enters `main()`. The image validator checks the core reserved vectors and IRQ30.

`py32f071xB.ld` defines the application flash/RAM regions, vector table, loadable data and a 2 KiB minimum stack reservation. It is an independently authored linker script based on the memory geometry and application offset recorded in the evidence document.

## Build and CI artifact

The GitHub Actions workflow installs the GNU Arm Embedded toolchain, compiles target sources with warnings as errors, links `D10S_K1.elf`, emits an intermediate raw image and checks the vector table, stack pointer, reset vector, flash bounds and per-function stack-frame usage. It also runs host tests, portable Cortex-M0+ compilation and Makefile tests.

A successful CI run uploads the ELF, map, intermediate raw image and build metadata as a 14-day diagnostic artifact explicitly marked **NOT FOR FLASHING**. This artifact is for engineering inspection, not a firmware release.

The successful run for commit `b49e1b11b9bb2411212952c0a6347106f1410aaa` produced:
- `D10S_K1.intermediate.bin`: 14,452 bytes; SHA-256 `09eca9f08b85dbda140f13ee7141fef84ed0e6469fc9b8354c59f32f846d99ba`.
- ELF size report: 14,452 bytes text, 0 bytes data, 5,568 bytes BSS.
- Artifact name: `D10S-K1-diagnostic-build-not-for-flashing`.

The intermediate BIN is not a released or approved flash image.

## Hardware release gates

A successful link and structural image check do not verify:
- exact MCU marking, PCB revision or installed bootloader;
- candidate PF9/PB8/PB9 and PA5/PA7/PB2/PA6 continuity;
- external RF switch / GPIO0 enable, LNA route, AGC settling, PLL lock or measured RSSI;
- LCD behavior, keypad matrix and physical key mapping;
- REG_13 gain response and sufficiency of the 40 ms settling interval;
- actual bootloader acceptance, update format or recovery procedure;
- cumulative nested stack use and RAM high-water mark under hardware load.

The BK4829 profile asserts the source-documented GPIO0 RX_ENABLE output and the selected LNA GPIO; GPIO1 PA_ENABLE is never asserted. These settings are source-derived, but their physical connectivity on the exact board is not verified. The scanner adapter returns the programmed frequency and does not independently measure PLL lock.

Before any controlled radio test, identify the exact board/MCU/bootloader, verify the pin map electrically, preserve a stock firmware backup and prove a recovery path. Use a conducted RF source and suitable attenuation/coupling for receive-path measurements. Complete every item in `docs/hardware-validation-checklist.md` before considering a flashable release.

**Do not flash the intermediate BIN.** CI success is software/build evidence, not proof of hardware compatibility or bootloader acceptance.
