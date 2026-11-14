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
- Initial mode: receive-only. No PTT, transmit, beacon or messenger path is implemented.

## Startup and linker

`startup.c` defines the 48-word PY32F071 vector table, relocates VTOR to the application base, initializes .data/.bss and enters `main()`.

`py32f071xB.ld` defines the application flash/RAM regions, the vector table, loadable data and a 2 KiB minimum stack reservation. It is an independently authored linker script based on the memory geometry and application offset recorded in the evidence document.

## Build

The GitHub Actions workflow installs `gcc-arm-none-eabi`, compiles all target sources with warnings as errors, links `D10S_K1.elf`, emits an intermediate raw image and checks the vector table, stack pointer, reset vector and flash bounds.

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
- RAM/stack safety under hardware load.

The BK4829 profile intentionally does not assert the unverified GPIO0 external RX-enable output. Therefore the target main is a bring-up build and may not open the physical RF receive path on the user's PCB. The intermediate `.bin` is **not cleared for flashing** and is not published as a test firmware. Do not send it to the radio until the outstanding hardware release gates are closed.
