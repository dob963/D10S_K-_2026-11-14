# D10S_K-_2026-11-14

Independent, receive-only Spectrum Analyzer firmware project for the Quansheng UV-K1 / UV-K1(8) V3 target (reported platform: PY32F071 + BK4829). Exact PCB compatibility still needs confirmation.

## Implemented portable core

The code in src/spectrum/ currently provides:

- Adaptive noise floor: per-sweep percentile, rolling five-sweep history and IIR smoothing.
- Relative peak detection: noise-relative threshold, local prominence and three-sweep confirmation by default.
- Peak refinement: three-point parabolic interpolation.
- Peak hold: updates only after a confirmed signal.
- Multi-band plan: up to eight validated bands with an exact uniform frequency grid and per-band dwell.
- Non-blocking scanner: tune, settle and RSSI-read phases through a hardware adapter.
- Actual-frequency tracking: every point uses the frequency returned by the radio adapter.
- Stable display snapshots: only complete sweeps are published; the displayed band and ATT setting are included with the snapshot.
- Delay control: settling delay can be changed at a sweep boundary.
- ATT interface: manual level change at a sweep boundary, with caller-supplied measured compensation in raw RSSI units.

RSSI is raw relative 9-bit data (0..511), **not calibrated dBm**. No dB values or ATT offsets are invented.

## Build and test the portable core

Requirements: a C11 compiler and Make.

```sh
make test
```

Equivalent CMake build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

GitHub Actions runs the host tests and compiles all three portable C modules as freestanding ARM Cortex-M0+ objects.

## Current status — not a flashable firmware

This repository still does **not** contain a complete radio application, a verified board-specific BK4829 initialization/RX driver, LCD/keyboard integration, the target startup/linker integration, or a flashable BIN. The scanner uses a simulated radio in its host tests. Do not flash test outputs or intermediate objects.

The scanner currently allocates two 256-point buffers (about 4 KiB on the 32-bit ABI, before state and stack). The detector's percentile routine has a measured 592-byte single stack frame in the current host compiler report. Final RAM and stack usage must be checked from the linked ARM map before release.

## Engineering references

- [Architecture contract](docs/architecture.md)
- [Evidence and release gates](docs/evidence-and-release-gates.md)
- [Spectrum algorithm and scanner](docs/spectrum-algorithm.md)
- [Upstream platform review](docs/upstream-review.md)
- [Karina-inspired Spectrum behavior requirements](docs/reference-behavior.md)

The UI direction is the requested Karina-style Spectrum workflow, not the F4HWN Spectrum screen. The reference behavior is documented separately; no Karina or F4HWN Spectrum source has been copied into the portable detector, band planner, or scanner.
