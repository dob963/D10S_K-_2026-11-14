# D10S PY32F071 target image

This folder contains the first integrated target entry point for the receive-only Spectrum application.

## Target contract

- Core: ARM Cortex-M0+ / reported PY32F071xB.
- RF: BK4829, using the source-derived opt-in initialization profile.
- Display: ST7565, 128x64.
- Application origin: `0x08002800`.
- Reported memory: 128 KiB flash / 16 KiB SRAM; application flash region is configured as 118 KiB.
- Initial scan spans: 144–146 MHz and 430–440 MHz, 129 bins (128 exact intervals) per band. The grids are validated to divide evenly in Hz.
- Initial settle: 8 ms per bin. This is a provisional value, not a hardware-measured optimum.
- Initial mode: receive-only. Key 9 freezes the last complete sweep for inspection; the next press restarts the current band's scan from its first bin. No PTT, transmit, beacon or messenger path is implemented.

## Startup and linker

`startup.c` defines the 48-word PY32F071 vector table, leaves the reserved external IRQ30 slot empty, relocates VTOR to the application base, initializes .data/.bss and enters `main()`. The image validator checks both the core reserved vectors and IRQ30.

`py32f071xB.ld` defines the application flash/RAM regions, the vector table, loadable data and a 2 KiB minimum stack reservation. It is an independently authored linker script based on the memory geometry and application offset recorded in the evidence document.

## Build

The GitHub Actions workflow installs `gcc-arm-none-eabi`, compiles all target sources with warnings as errors, links `D10S_K1.elf`, emits an intermediate raw image and checks the vector table (including reserved IRQ30), stack pointer, reset vector, flash bounds and per-function stack-frame usage.

A local equivalent, with the GNU Arm Embedded toolchain installed:

```sh
mkdir -p build/target
# Compile the sources listed in .github/workflows/spectrum-core-tests.yml
# with -mcpu=cortex-m0plus -mthumb -ffreestanding.
# Link with:
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -nostdlib \
  -T target/py32f071xB.ld -Wl,--gc-sections \
  -Wl,-Map=build/target/D10S_K1.map \
  build/target/*.o -lgcc -o build/target/D10S_K1.elf
arm-none-eabi-size build/target/D10S_K1.elf
arm-none-eabi-objcopy -O binary build/target/D10S_K1.elf \
  build/target/D10S_K1.intermediate.bin
python3 target/validate_image.py build/target/D10S_K1.intermediate.bin
```

## Important release status

A successful link and vector-table check establish only that the executable is structurally linked for the configured memory layout. They do not verify:
- exact MCU marking or the user's PCB revision;
- candidate PF9/PB8/PB9 and PA5/PA7/PB2/PA6 continuity;
- external RF switch / GPIO0 enable, LNA route, AGC, PLL lock or measured RSSI;
- LCD behavior and keypad matrix behavior on the radio;
- actual bootloader acceptance or firmware update path;
- RAM/stack safety under hardware load. The current CI build reports 5,544 bytes of BSS, reserves 2 KiB for stack, and bounds individual function frames to 768 bytes (observed maximum 560 bytes); cumulative nested stack use is not yet measured.

The BK4829 profile asserts the source-documented GPIO0 RX_ENABLE output and the selected LNA GPIO; GPIO1 PA_ENABLE is never asserted. These settings are source-derived but their physical connectivity on the user's exact PCB is not verified. The target main is therefore a bring-up build and may still fail to open the actual RF receive path if the board differs from the reviewed source lineage. The intermediate `.bin` is **not cleared for flashing** and is not published as a test firmware. Do not send it to the radio until the outstanding hardware release gates are closed.
