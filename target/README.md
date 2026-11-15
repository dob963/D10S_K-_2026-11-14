# D10S PY32F071 target image

This folder contains the integrated target entry point for the receive-only Spectrum application.

## Target contract

- Core: ARM Cortex-M0+ / reported PY32F071xB. Exact MCU marking and PCB revision remain unconfirmed.
- RF: BK4829, using the source-derived opt-in initialization profile.
- Display: ST7565, 128x64.
- Application origin: `0x08002800`.
- Reported memory: 128 KiB flash / 16 KiB SRAM; application flash region is configured as 118 KiB.
- Initial scan spans: 144–146 MHz and 430–440 MHz, 129 bins (128 exact intervals) per band. No other target bands are enabled yet.
- Initial dwell: 8 ms per bin. This is a provisional software setting, not a hardware-measured optimum.
- Initial mode: receive-only. No PTT, transmit, beacon or messenger path is implemented.

## Bootloader / DFU format review

The reported bootloader version 7.03.01 is listed by reviewed public tooling as a standard UV-K1 bootloader. The same reference maps 7.00.07 to UV-K5 V3 / UV-K1(8) v3 Mini Kong; these model variants must not be treated as interchangeable without confirming the physical unit.

The D10S target workflow emits a raw Cortex-M image. Reviewed DFU tooling sends raw images unchanged in 256-byte pages; vendor-packed stock update files use a separate XOR/version-block decoding step. D10S must not apply that vendor packing to its raw build output.

This is source-level protocol evidence only. The bootloader version, model, application origin and recovery path have not been read/tested on the actual radio. **The intermediate BIN remains NOT FOR FLASHING.** See `docs/dfu-compatibility-evidence.md`.

## Key map

- `1`: select VHF 144–146 MHz.
- `2`: select UHF 430–440 MHz.
- `3`: toggle adaptive Turbo polling.
- `4`: cycle Delay through 8 / 12 / 16 / 24 ms.
- `5`: cycle six BK4829 REG_13 receive-gain presets. Labels are neutral stages `AT0`–`AT5`; attenuation in dB has not been calibrated.
- `6`: lower the opening threshold margin by 2 raw RSSI units (minimum 2).
- `7`: raise the opening threshold margin by 2 raw RSSI units (maximum 64).
- `8`: restore the default opening margin of 12 raw RSSI units. The display shows the active margin as `Nxx`; this is a relative raw-RSSI margin, not dB.
- `9`: freeze the last complete sweep; next press resumes the current band from bin zero.
- `MENU/0`: toggle/place cursor; `UP/DOWN`: move one bin; `SIDE1/SIDE2`: move eight bins.
- `F`: toggle normal/moderately bold labels; `STAR`: toggle peak-marker visibility.

Delay and gain changes are deferred to safe sweep boundaries. A gain change invalidates stale detector/snapshot data and applies a 40 ms software settling interval before the next RSSI sample. This interval and the gain response have not yet been validated on the physical radio. REG_13 stages are source-derived presets, not measured attenuation values in dB.

## Startup and linker

`startup.c` defines the 48-word PY32F071 vector table, leaves the reserved external IRQ30 slot empty, relocates VTOR to the application base, initializes .data/.bss and enters `main()`. The image validator checks the core reserved vectors and IRQ30.

`py32f071xB.ld` defines the application flash/RAM regions, vector table, loadable data and a 2 KiB minimum stack reservation. It is an independently authored linker script based on the memory geometry and application offset recorded in the evidence document.

## Build and CI artifact

The GitHub Actions workflow installs the GNU Arm Embedded toolchain, compiles target sources with warnings as errors, links `D10S_K1.elf`, emits an intermediate raw image and checks the vector table, stack pointer, reset vector, flash bounds and per-function stack-frame usage. It also runs host tests, portable Cortex-M0+ compilation and Makefile tests.

**Latest verified CI:** [run 43704790473](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43704790473), commit `08ab113dca6247ba0af1917e4a5715710cde2150`.
- Job `host-tests`: success; all 10 CMake tests passed, including the new RSSI percentile bucket-boundary test.
- Warning-as-error compilation, Cortex-M0+ cross-compilation and Makefile build/tests: passed.
- Full PY32F071 application link and structural image-layout validation: passed.
- The noise-floor percentile calculation now uses a bounded two-level RSSI histogram (16 coarse × 32 fine buckets), avoiding the 256-element stack sorting array and adding no static/global scratch memory.
- Per-function stack check: 118 frames; cumulative static C call-chain estimate: 864 bytes; plus 40-byte exception reserve = 904/1,536 bytes, leaving 632 bytes. This includes a manually assigned 256-byte reserve for `__aeabi_idiv`; it is a static estimate, not a measured hardware high-water mark.
- Diagnostic artifact: `D10S-K1-diagnostic-build-not-for-flashing`, ID `14157807805`, ZIP 37,566 bytes.
- Artifact ZIP SHA-256: `4147416ee90699b86a009a9e081c43e12df3c2cfd0baf44c88f5c7b352056b05`.
- Extracted intermediate BIN: 15,388 bytes; SHA-256 `ed4f2632612a8df0023a17fea943b00e9d1f9d3d25fde2e8030f197aafeb004d`.
- ELF: 45,472 bytes; BSS: 5,824 bytes; raw BIN vector inspection reports initial SP `0x20004000` and reset vector `0x08002904` (Thumb). Hardware bootloader compatibility remains unverified.

The artifact contains engineering outputs only. The raw BIN was extracted and independently checksummed, and its vector table was inspected; it remains explicitly **NOT FOR FLASHING** and is not a firmware release. Hardware, bootloader-origin and recovery checks remain open.

## Hardware release gates

A successful link and structural image check do not verify:
- exact MCU marking, PCB revision, model variant or installed bootloader;
- candidate PF9/PB8/PB9 and PA5/PA7/PB2/PA6 continuity;
- external RF switch / GPIO0 enable, LNA route, AGC settling, PLL lock or measured RSSI;
- LCD behavior, keypad matrix and physical key mapping;
- REG_13 gain response and sufficiency of the 40 ms settling interval;
- actual bootloader acceptance, application address on the exact unit, or recovery procedure;
- cumulative nested stack use and RAM high-water mark under hardware load.

The BK4829 profile asserts the source-documented GPIO0 RX_ENABLE output and the selected LNA GPIO; GPIO1 PA_ENABLE is never asserted. These settings are source-derived, but their physical connectivity on the exact board is not verified. The scanner adapter returns the programmed frequency and does not independently measure PLL lock.

Before any controlled radio test, identify the exact board/MCU/bootloader, verify the pin map electrically, preserve a stock firmware backup and prove a recovery path. Use a conducted RF source and suitable attenuation/coupling for receive-path measurements. Complete every item in `docs/hardware-validation-checklist.md` before considering a flashable release.

**Do not flash the intermediate BIN.** CI success is software/build evidence, not proof of hardware compatibility or bootloader acceptance.
