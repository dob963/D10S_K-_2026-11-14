# BK4829 hardware adapter contract

Status: portable bus and scanner adapter implemented; an opt-in source-derived UV-K1 V3 RX profile is present, but the physical board port is not verified.

## Boundary

The portable scanner uses d10s_spectrum_radio_ops_t. The hardware adapter maps these calls to the confirmed UV-K1 V3 PCB and BK4829. Register addresses and GPIO choices remain outside the scanner.

## Current components

- bk4829_bus.c: three-wire register read/write operations.
- bk4829.c: frequency-word conversion, REG_38/REG_39 writes, and 9-bit REG_67 RSSI extraction.
- bk4829_scanner_adapter.c: scanner bridge, including first RSSI conversion discard after each tune.
- bk4829_uvk1_v3_profile.c: opt-in RX baseline and VHF/UHF LNA/RX GPIO sequence derived from the pinned upstream UV-K1 V3 BK4829 driver. See docs/bk4829-uvk1-v3-profile.md.

## Required callback behavior

### Tune
- Reject frequencies outside the confirmed BK4829 and board-supported range.
- Program REG_38/REG_39 through the bus helper.
- Return the programmed frequency value only after all required writes succeed. This is not a PLL-lock measurement.
- Reset the first-RSSI-discard state.
- Return false on partial writes or bus failure.

### RSSI
- Discard the first conversion after each successful tune and return the following raw RSSI sample.
- Return only 0..511; never convert to uncalibrated dBm.
- Return false on bus error; never fabricate or reuse a cached reading.

### Receiver / LNA
- The source-derived UV-K1 V3 profile includes the pinned baseline register sequence and RX enable writes. It selects GPIO4 below 28 MHz or GPIO3 at/above 28 MHz, then enables RX using GPIO0 according to the reviewed source.
- It is source-traced but **not hardware-validated on the user's exact radio**. The 28 MHz boundary, RF path, external switch, LNA and GPIO wiring must be confirmed for the user's PCB before use.
- ATT remains unimplemented as a board-level control. Scanner-side compensation is only a caller-supplied measured raw RSSI offset and is not a physical ATT driver.

## Bring-up gates

1. Confirm exact UV-K1 V3 PCB revision and MCU marking.
2. Check continuity and pin direction for PF9/PB8/PB9; these are only candidate pins from the pinned Armel target source.
3. Confirm whether the radio's board uses the same BK4829 GPIO-controlled RF paths and 28 MHz split.
4. Verify BK4829 reset, clock, baseline init, RX enable, antenna switch and both LNA paths on a current-limited/debug setup.
5. Verify tuning with known RF signals; current adapter returns programmed frequency, not a measured PLL lock frequency.
6. Measure RSSI repeatability, first-sample discard, AGC settling and Delay/Turbo behavior by band.
7. Measure ATT steps and only then supply compensation offsets.
8. Port the ST7565 GPIO/SPI callbacks and validate display orientation/contrast.
9. Complete startup/linker/vector integration; inspect the ARM map and RAM/stack budget.
10. Test bootloader/DFU and normal startup on the exact radio before any BIN is called flashable.

A host test or Cortex-M0+ object compile cannot validate any of these RF, GPIO, panel or bootloader conditions.
