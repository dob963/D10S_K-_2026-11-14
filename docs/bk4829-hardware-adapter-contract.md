# BK4829 hardware adapter contract

Status: interface contract only; no board-specific driver is declared complete.

## Boundary

The portable scanner talks only to `d10s_spectrum_radio_ops_t`. The target adapter is responsible for translating those calls to the verified UV-K1 V3 board and BK4829. It must not leak register addresses or GPIO assumptions into the scanner.

## Required callback behavior

### `tune_hz(context, requested_hz, actual_hz)`

- Reject frequencies outside the confirmed BK4829 and board-supported range.
- Program the synthesizer using the complete, verified BK4829 tuning procedure.
- Return the actual programmed frequency, not a rounded request copied blindly.
- Mark the RSSI acquisition state as `first_sample_pending`.
- Return false on bus, lock, range, or configuration failure. Do not report a successful tune after a partial write.

### `read_rssi(context, rssi)`

- On the first call after every successful tune, perform and discard one RSSI conversion, then perform a second conversion and return that second raw value. This matches the behavior observed in the reviewed upstream Spectrum/scanner paths.
- On subsequent calls at the same tuned frequency, return one fresh raw sample per call. Do not repeatedly discard samples.
- Return only the raw 9-bit value (0..511); do not convert it to dBm.
- Return false on bus/read failure. Never substitute a cached or fabricated RSSI value.
- Keep the discard state local to the adapter and reset it on every tune and receiver reinitialization.

The scanner's fixed settling delay or Turbo stability polling happens before its first `read_rssi` call. The adapter's two conversions are not a substitute for RF/AGC settling.

### `set_attenuation(context, level)`

- Change ATT/LNA/RF-path state only at a scanner sweep boundary.
- Reject unsupported levels and report hardware write failures.
- Do not claim that an ATT step corresponds to a calibrated dB value.
- Any compensation passed to the scanner must come from measurements for the same board, band, and ATT state, expressed in raw RSSI units.

## Bring-up sequence and release blockers

1. Confirm exact MCU marking, PCB revision, and UV-K1 V3 variant.
2. Verify CSN/SCL/SDA continuity and GPIO alternate-function/electrical configuration. PF9/PB8/PB9 are only candidate pins from the reviewed upstream target; they are not accepted as this radio's mapping until verified.
3. Implement and test the BK4829 three-wire bus with read-back and timeout/error handling.
4. Port the complete BK4829 initialization and RX setup sequence for the confirmed board. Do not transplant isolated register writes from a BK4819-labelled compatibility header.
5. Verify RF path, antenna switch, LNA and ATT control independently at each relevant band.
6. Verify tune/read behavior with a signal generator: requested versus actual frequency, first-sample discard, RSSI repeatability, settling time and Turbo exit.
7. Integrate the ST7565 driver behind the `d10s_spectrum_canvas_t` callbacks; verify orientation, contrast, text metrics and refresh timing on the physical LCD.
8. Integrate key input and safe startup/exit behavior. Spectrum-only firmware must remain receive-only.
9. Link the complete target image with the correct bootloader offset and vector relocation; inspect map, stack, RAM and image size.
10. Validate DFU programming and normal boot on the exact radio before publishing any BIN as flashable.

## Evidence rule

A host test can validate the portable scanner and a fake adapter. It cannot prove the BK4829 bus, RF initialization, LCD, keypad, or bootloader integration. A target compile proves syntax/ABI compatibility only. A flashable release requires a complete linked image plus hardware validation records.
