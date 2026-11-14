# Spectrum detector: first implementation

## Input contract

The detector consumes one completed sweep: an ordered array of frequency/RSSI pairs. Frequencies must be strictly increasing and the array must contain 3 to 256 bins. RSSI is an unsigned relative measurement from the receiver path; it is not converted to dBm.

The caller must reset tracking when it changes band or frequency grid. The receiver driver remains responsible for tuning, waiting for settling, acquiring RSSI, and recording the actual tuned frequency in every bin.

## Processing path

1. Copy and sort the sweep RSSI values using bounded insertion sort.
2. Select the configured percentile (default 20th) as the current noise observation.
3. Smooth the noise-floor estimate with a 3:1 previous/current IIR update.
4. Set the opening threshold to noise floor plus a configurable margin.
5. Search interior bins for the strongest local maximum meeting the threshold and prominence requirement.
6. Require the candidate to remain within the configured bin tolerance for consecutive sweeps (default 3).
7. Refine the peak frequency with a three-point parabolic interpolation.
8. Update peak hold only after confirmation; it persists until tracking is reset.

## Defaults and limits

- Percentile: 20
- Noise margin: 12 raw RSSI units
- Minimum local prominence: 5 raw RSSI units
- Confirmations: 3 consecutive sweeps
- Frequency movement tolerance: 1 bin
- Maximum bins: 256

These values are initial algorithm defaults, not hardware-calibrated constants. They must be tuned using measured sweeps from the exact UV-K1 board and receiver configuration.

The percentile estimator assumes that most bins in a sweep are not occupied by strong signals. A very wideband signal or a sweep dominated by interference can bias the estimate upward. A later hardware integration stage should log representative quiet, noisy, and occupied sweeps before changing this estimator.

## Current validation

The portable C module passes host unit tests for confirmation, single-sweep spike rejection, background noise-floor tracking, sub-bin interpolation, and invalid frequency-grid rejection. It also compiles as a freestanding ARM Cortex-M0+ object with Clang. Neither check validates RF performance, the BK4829 register sequence, LCD rendering, bootloader compatibility, or a flashable application image.
