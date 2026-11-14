# Spectrum detector: first implementation

## Input contract

The detector consumes one completed sweep: an ordered array of frequency/RSSI pairs. Frequencies must be strictly increasing with a constant bin spacing, and the array must contain 3 to 256 bins. Uniform spacing is required for valid three-point frequency interpolation. RSSI is the raw unsigned 9-bit relative measurement (0..511) from the BK4829 receiver path; out-of-range input is rejected and it is not converted to dBm.

The caller must reset tracking when it changes band or frequency grid. The receiver driver remains responsible for tuning, waiting for settling, acquiring RSSI, and recording the actual tuned frequency in every bin.

## Multi-band plan

The portable scan-plan module accepts up to eight caller-defined bands. The caller-owned band array must remain valid for the lifetime of the plan. Each band defines start/stop frequency, bin count and dwell sweeps. The plan validates a positive integer, uniform bin spacing, exposes exact per-bin frequencies, rotates bands after the configured dwell, and resets its dwell counter on manual band selection. The application must call `d10s_spectrum_reset_tracking()` whenever the active band changes so old noise history and held peaks are not carried into a different RF range.

No band limits are hard-coded: final ranges must come from the confirmed hardware variant and the selected scan plan.

## Non-blocking scanner

The scanner state machine calls a hardware adapter in three phases: tune, wait for a caller-supplied settling interval, then read raw RSSI. It stores the adapter-reported actual tuned frequency for each bin, rejects RSSI outside 0..511, and emits one completed sweep at a time. Automatic band rotation resets noise history, candidate confirmation and peak hold. Manual band selection also invalidates the old published graph. The timer comparison is wrap-safe for a 32-bit millisecond counter. Adapter failures stop the scanner in an error state; a caller must reinitialize it to resume.

The scanner keeps a work buffer and a separate published buffer. The display accessor returns only the last complete sweep, together with its band index, ATT level and generation counter. A display must use the snapshot's band index and stored frequency values for its cursor; it must not calculate an EXP cursor from the scanner's currently active band. During automatic rotation the last snapshot remains explicitly tagged with the band it belongs to until the next band completes a sweep. Manual band changes and ATT changes invalidate it. The snapshot pointer is owned by the scanner and remains valid only until a later completed sweep or explicit invalidation.

The Delay setter accepts a new nonzero settle interval only at the beginning of a sweep. This prevents changing timing while a frequency is already waiting for settling or while a sweep contains mixed acquisition conditions.

The radio adapter supplies only `tune_hz(requested, &actual)` and `read_rssi(&raw_value)`. This keeps all BK4829 register access and the board's LNA/RF path out of the portable scanner until those details are verified for the exact PCB.

## ATT handling

The scanner exposes a manual ATT callback. ATT changes are allowed only at a sweep boundary; a mid-sweep change is rejected so one sweep cannot contain mixed gain settings. The application supplies a measured RSSI correction in raw units for that hardware ATT level. The scanner adds this correction to each raw sample with saturation at 511 and resets detector history when ATT changes. No ATT steps, dB values or compensation constants are hard-coded because they must be measured on the actual board and receiver path. The last published snapshot is invalidated whenever ATT changes so the display cannot combine levels acquired with different gain settings.

## Processing path

1. Copy and sort the sweep RSSI values using bounded insertion sort.
2. Select the configured percentile (default 20th) as the current sweep's noise observation.
3. Keep the last five sweep observations, select their configured rolling percentile (default median/50th), then smooth the floor with a 3:1 previous/current IIR update.
4. Set the opening threshold to noise floor plus a configurable margin.
5. Search interior bins for the strongest local maximum meeting the threshold and prominence requirement.
6. Require the candidate to remain within the configured bin tolerance for consecutive sweeps (default 3).
7. Refine the peak frequency with a three-point parabolic interpolation.
8. Update peak hold only after confirmation; it persists until tracking is reset.

## Defaults and limits

- Per-sweep percentile: 20
- Rolling history: 5 sweeps, 50th percentile
- Noise margin: 12 raw RSSI units
- Minimum local prominence: 5 raw RSSI units
- Confirmations: 3 consecutive sweeps
- Frequency movement tolerance: 1 bin
- Maximum bins: 256
- Maximum bands: 8

These values are initial algorithm defaults, not hardware-calibrated constants. They must be tuned using measured sweeps from the exact UV-K1 board and receiver configuration.

The percentile estimator assumes that most bins in a sweep are not occupied by strong signals. A very wideband signal or a sweep dominated by interference can bias the estimate upward. A later hardware integration stage should log representative quiet, noisy, and occupied sweeps before changing this estimator.

## Current validation

The portable C modules are covered by host tests for detector confirmation, single-sweep spike rejection, rolling noise-floor robustness, 9-bit RSSI validation, sub-bin interpolation, band-grid validation and rotation, simulated tune/settle/read acquisition, ATT compensation and failure handling, complete-sweep snapshots, manual band invalidation and sweep-boundary Delay changes. The scanner contains two fixed 256-bin buffers; this reserves approximately 4 KiB on the current 32-bit data layout, excluding detector state and stack. Clang reports a maximum single stack frame of 592 bytes in the percentile estimator; callers must account for nested call depth and interrupt stack in the final RAM budget. All three portable modules compile as freestanding ARM Cortex-M0+ objects with Clang. Neither check validates RF performance, the BK4829 register sequence, LCD rendering, bootloader compatibility, or a flashable application image.
