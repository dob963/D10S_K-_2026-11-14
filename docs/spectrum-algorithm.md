# Spectrum detector: first implementation

## Input contract

The detector consumes one completed sweep: an ordered array of frequency/RSSI pairs. Frequencies must be strictly increasing with a constant bin spacing, and the array must contain 3 to 256 bins. Uniform spacing is required for valid three-point frequency interpolation. RSSI is the raw unsigned 9-bit relative measurement (0..511) from the BK4829 receiver path; out-of-range input is rejected and it is not converted to dBm.

The caller must reset tracking when it changes band or frequency grid. The receiver driver remains responsible for tuning, waiting for settling, acquiring RSSI, and recording the actual tuned frequency in every bin.

## Multi-band plan

The portable scan-plan module accepts up to eight caller-defined bands. The caller-owned band array must remain valid for the lifetime of the plan. Each band defines start/stop frequency, bin count and dwell sweeps. The plan validates a positive integer, uniform bin spacing, exposes exact per-bin frequencies, rotates bands after the configured dwell, and resets its dwell counter on manual band selection. The application must call `d10s_spectrum_reset_tracking()` whenever the active band changes so old noise history and held peaks are not carried into a different RF range.

No band limits are hard-coded: final ranges must come from the confirmed hardware variant and the selected scan plan.

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

These values are initial algorithm defaults, not hardware-calibrated constants. They must be tuned using measured sweeps from the exact UV-K1 board and receiver configuration.

The percentile estimator assumes that most bins in a sweep are not occupied by strong signals. A very wideband signal or a sweep dominated by interference can bias the estimate upward. A later hardware integration stage should log representative quiet, noisy, and occupied sweeps before changing this estimator.

## Current validation

The portable C module passes host unit tests for confirmation, single-sweep spike rejection, rolling noise-floor robustness and tracking, preservation of all 9 RSSI bits, sub-bin interpolation, and invalid/irregular frequency-grid rejection. Clang reports a maximum single stack frame of 592 bytes in the percentile estimator; callers should account for its nested call stack and interrupt stack in the final RAM budget. It also compiles as a freestanding ARM Cortex-M0+ object with Clang. Neither check validates RF performance, the BK4829 register sequence, LCD rendering, bootloader compatibility, or a flashable application image.
