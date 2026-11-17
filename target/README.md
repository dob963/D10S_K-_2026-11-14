# D10S PY32F071 target image

This folder contains the integrated target entry point for the receive-only Spectrum application.

## Target contract

- Radio model: **standard Quansheng UV-K1**, as reported by the user; reported bootloader: **7.03.01**. UV-K1(8) V3 Mini Kong and UV-K5 V3 are explicitly outside this target.
- Core: ARM Cortex-M0+ / PY32F071 family expected for standard UV-K1; exact MCU package marking and PCB revision remain to be physically inspected.
- RF: BK4829, using the source-derived initialization profile.
- Display: ST7565, 128x64.
- Application origin: `0x08002800`.
- Reported memory: 128 KiB flash / 16 KiB SRAM; application flash region configured as 118 KiB.
- Initial scan spans: 144–146 MHz and 430–440 MHz, 129 bins per band.
- Initial dwell: 8 ms per bin; provisional and not hardware-calibrated.
- Initial mode: receive-only. No PTT, transmit, beacon or messenger path is implemented.

## Bootloader / DFU format review

The user reports standard UV-K1 with bootloader 7.03.01. Reviewed public tooling lists this combination for standard UV-K1; 7.00.07 is separately associated with UV-K5 V3 / UV-K1(8) V3 Mini Kong.

The D10S workflow emits a raw Cortex-M image. Reviewed DFU tooling sends raw images unchanged in 256-byte pages; vendor-packed stock update files use a separate decoding step. D10S must not apply vendor packing to its raw build output.

This is source-level protocol evidence only. The exact bootloader readback, application origin and recovery path have not been tested on the physical radio. **The current BIN is a release candidate, not yet approved for flashing.** See `docs/dfu-compatibility-evidence.md`.

## Key map

- `1`: select VHF 144–146 MHz.
- `2`: select UHF 430–440 MHz.
- `3`: toggle adaptive Turbo polling.
- `4`: cycle Delay through 8 / 12 / 16 / 24 ms.
- `5`: cycle six BK4829 REG_13 receive-gain presets. Labels are neutral stages `AT0`–`AT5`; attenuation in dB has not been calibrated.
- `6` / `7`: lower / raise the opening threshold margin by 2 raw RSSI units.
- `8`: restore the default opening margin of 12 raw RSSI units.
- `9`: freeze the last complete sweep; next press resumes from bin zero.
- `MENU/0`: toggle/place cursor; `UP/DOWN`: move one bin; `SIDE1/SIDE2`: move eight bins.
- `F`: toggle normal/moderately bold labels; `STAR`: toggle peak-marker visibility.

Delay and gain changes are deferred to safe sweep boundaries. A gain change invalidates stale detector/snapshot data and applies a 40 ms software settling interval. This interval and gain response are not yet validated on the physical radio. REG_13 stages are source-derived presets, not measured dB values.

## Startup and linker

`startup.c` defines the 48-word PY32F071 vector table, leaves reserved external IRQ30 empty, relocates VTOR to the application base, initializes .data/.bss and enters `main()`. The image validator checks the vector table, stack pointer, reset vector and flash bounds.

`py32f071xB.ld` defines the application flash/RAM regions, vector table, loadable data and a 2 KiB minimum stack reservation.

## Build locale

Con toolchain GNU Arm Embedded installata nel PATH:

```sh
make firmware
```

Il target usa `arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft`, esegue i controlli stack, genera `build/target/D10S_K1.elf` e `build/target/D10S_K1.bin`, quindi valida il layout vettori/flash. Per i test host: `make test`.

## Build and CI artifact

The GitHub Actions workflow compiles target sources with warnings as errors, links `D10S_K1.elf`, emits `D10S_K1.bin`, validates the image structure, performs stack diagnostics and runs host, Cortex-M0+ and Makefile tests.

**Latest release-candidate build:** [run 44037422516](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/44037422516), source commit `017ace764dbafd5d0fd20419db70129061f880b5`.
- Workflow conclusion: success; all 13 build, test, cross-compile, link, stack and packaging steps completed.
- Artifact: `D10S-K1-firmware-release-candidate`, ID `14307632270`.
- Artifact ZIP size: 39,042 bytes; SHA-256: `204b1dc67a054c5b9ed22288c7a9bb3352079302e70d4000c3712121cec9ef97`.
- Extracted `D10S_K1.bin`: 15,924 bytes; SHA-256: `b62c2a83839650d41d8509fe6bb9cc9bb9a036c819892c1d3fd1e1be9b87240d`.
- Vector values: initial SP `0x20004000`; Reset vector `0x08002905` (Thumb); app base `0x08002800`.
- Size report: text 15,924 bytes; data 0; BSS 6,096 bytes.
- Stack checks: maximum individual frame 312 bytes; worst analyzed call path 912/1,536 bytes (624 bytes margin).

### Stock firmware image reference

The upstream archive also contains `archive/quansheng.k1.stock.firmware.v7.03.01.bin` (71,268 bytes; Git blob SHA `7beb185168f0e04989320ff4589b9b8399b7bbdf`). Its vector words are initial SP `0x200031A0` and Reset vector `0x080028D5` (Thumb), consistent with the `0x08002800` application vector base. This is a binary image-layout reference, not a live BK4829 register dump. The stock image is not copied into D10S.
- Metadata status: `RELEASE_CANDIDATE_HARDWARE_VALIDATION_REQUIRED`.

This artifact includes the REG_19 source annotation, AFC/AFCR documentation, ST7565 frame renderer, 16-row 4x4-dither waterfall, center/span/RBW footer and 15 fps display limit.

CI success proves the checked source compiles and passes automated software and image-structure checks. It does not validate the physical UV-K1 PCB, RF path, LCD/keypad behavior, bootloader hand-off or recovery procedure.

## Hardware release gates

Before approving a flashable release, verify:
- exact MCU package marking, PCB revision and bootloader readback;
- application address and recovery path on the actual unit; preserve a stock firmware backup;
- candidate PF9/PB8/PB9 and PA5/PA7/PB2/PA6 continuity;
- RF switch / GPIO0 enable, LNA route, AGC settling, PLL lock and measured RSSI;
- LCD behavior, keypad matrix and physical key mapping;
- REG_13 gain response and 40 ms settling interval;
- cumulative nested stack use and RAM high-water mark under hardware load.

The BK4829 profile is source-derived; physical connectivity on the exact board is not verified. The scanner adapter reports the programmed frequency and does not independently measure PLL lock.

**Do not treat CI success as proof of hardware compatibility.** The release candidate must pass the hardware and DFU gates before installation.
