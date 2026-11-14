# BK4829 hardware adapter contract

Status: portable bus/scanner adapter, source-derived RX-mode/LNA profile and a candidate PY32F071 GPIO/SPI callback backend are implemented. The pin map and RF behavior are not physically verified on the user's board. BK serial timing now follows the pinned source's 35-NOP edge delay.

## Boundary

The portable scanner uses d10s_spectrum_radio_ops_t. The hardware adapter maps these calls to the confirmed UV-K1 V3 PCB and BK4829. Register addresses and GPIO choices remain outside the scanner.

## Current components

- bk4829_bus.c: three-wire register read/write operations.
- bk4829.c: frequency-word conversion, REG_38/REG_39 writes, and 9-bit REG_67 RSSI extraction.
- bk4829_scanner_adapter.c: scanner bridge, including first RSSI conversion discard after each tune.
- bk4829_uvk1_v3_profile.c: opt-in baseline and RX-mode sequence plus VHF/UHF LNA selector derived from the pinned upstream UV-K1 V3 BK4829 driver. See docs/bk4829-uvk1-v3-profile.md.
- py32f071_uvk1_v3.c: candidate PF9/PB8/PB9 three-wire BK4829 GPIO callbacks and PA5/PA7/PB2/PA6 ST7565 SPI/GPIO callbacks. The target may supply a cycle-based delay callback or use the built-in SysTick delay backend after deriving HCLK from RCC state. The BK bus uses 35 HCLK ticks per serial edge/turnaround, matching the 35-NOP SHORT_DELAY in the pinned UV-K1 V3 driver.

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
- The source-derived UV-K1 V3 profile includes the pinned baseline register sequence and RX-mode setup. BK serial timing uses 35 HCLK cycles for each edge and the SDA turnaround, matching the pinned driver's 35-NOP delay.
- It asserts the source-named GPIO0 RX_ENABLE output and selects GPIO4 below 28 MHz or GPIO3 at/above 28 MHz, following the pinned UV-K1 V3 driver. GPIO1 PA_ENABLE is never asserted; it is reserved for the separate TX path, which D10S does not implement. The profile flag rx_mode_configured means the radio IC was programmed for RX; it does not prove RF reception or correct physical board routing.
- The profile is source-traced but **not hardware-validated on the user's exact radio**. The 28 MHz boundary, RF path, external switch, LNA and GPIO wiring must be confirmed for the user's PCB.
- ATT remains unimplemented as a board-level control. Scanner-side compensation is only a caller-supplied measured raw RSSI offset and is not a physical ATT driver.

## Bring-up gates

1. Confirm exact UV-K1 V3 PCB revision and MCU marking.
2. Check continuity and pin direction for PF9/PB8/PB9 and PA5/PA7/PB2/PA6; these are candidate pins from the pinned Armel target source and now have a source-level D10S GPIO/SPI backend.
3. Confirm whether the radio's board uses the same BK4829 GPIO-controlled RF paths and 28 MHz split.
4. Verify that the source-named BK4829 GPIO0 RX_ENABLE output reaches the correct external receive/RF-switch path on the exact PCB revision.
5. Verify BK4829 reset, clock, baseline init, RX mode, antenna switch and both LNA paths on a current-limited/debug setup.
6. Verify tuning with known RF signals; current adapter returns programmed frequency, not a measured PLL lock frequency.
7. Measure RSSI repeatability, first-sample discard, AGC settling and Delay/Turbo behavior by band.
8. Measure ATT steps and only then supply compensation offsets.
9. Verify HCLK and the 35-cycle BK delay on hardware; then validate ST7565 GPIO/SPI mapping, display orientation and contrast on the physical LCD.
10. Complete startup/linker/vector integration; inspect the ARM map and RAM/stack budget.
11. Test bootloader/DFU and normal startup on the exact radio before any BIN is called flashable.

A host test or Cortex-M0+ object compile cannot validate any of these RF, GPIO, panel or bootloader conditions.
