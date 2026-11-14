# D10S_K-_2026-11-14

Independent, receive-only Spectrum Analyzer firmware project for the Quansheng UV-K1 / UV-K1(8) V3 platform (PY32F071 + BK4829).

## Current state

The first portable Spectrum-processing module is implemented in `src/spectrum/`. It provides:

- rolling percentile-based adaptive noise-floor tracking on raw 9-bit RSSI;
- local-maximum detection with a relative threshold and prominence test;
- confirmation across consecutive sweeps;
- parabolic sub-bin peak-frequency refinement;
- peak hold until the caller resets tracking.

RSSI values are relative raw measurements, **not calibrated dBm**.

## Build and test the portable module

Requirements: a C11 compiler and Make.

```sh
make test
```

The detector is also freestanding-compilable for ARM Cortex-M0+; this object-only check does not build a radio firmware image.

## Important status

This repository does **not** yet contain a complete application, a verified BK4829 receiver startup sequence, a board-specific LCD/keyboard port, or a flashable BIN. Do not flash intermediate test objects or host-test outputs.

See [architecture](docs/architecture.md), [evidence and release gates](docs/evidence-and-release-gates.md), [Spectrum algorithm](docs/spectrum-algorithm.md), and [upstream platform review](docs/upstream-review.md).
