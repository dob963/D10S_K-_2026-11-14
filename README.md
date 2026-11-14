# D10S_K-_2026-11-14

Independent, receive-only Spectrum Analyzer firmware project for the Quansheng UV-K1 / UV-K1(8) V3 target (reported platform: PY32F071 + BK4829). Exact PCB revision and compatibility remain to be confirmed.

## Implemented portable modules

### Spectrum engine
- Integrated coordinator in src/app/spectrum_app.c connects scanner, detector, snapshots, graph renderer, framebuffer and ST7565 controller.
- Adaptive noise floor: per-sweep percentile, rolling five-sweep history and smoothing.
- Relative peak detection, local prominence, three-sweep confirmation, parabolic frequency refinement and confirmed peak hold.
- Multi-band plan: up to eight validated bands with uniform grids and per-band dwell.
- Non-blocking tune/settle/RSSI acquisition; each sample is tagged with the actual frequency returned by the radio adapter.
- Only complete sweeps are published; manual band and ATT changes invalidate stale snapshots.
- Delay control and optional adaptive Turbo polling, bounded by caller-provided timings.
- ATT callback with measured correction in raw RSSI units only.

### Display
- 128x64 Spectrum graph with RSSI trace, threshold line, peak marker, cursor and frequency label.
- 1-bit framebuffer with normal and moderately bold 5x7 text.
- ST7565 page-addressing and eight-page flush controller.
- Physical PY32F071 GPIO/SPI callbacks, keypad integration, orientation and contrast validation remain outstanding.

### BK4829
- Three-wire register read/write bus, timing calculation, frequency conversion and raw 9-bit REG_67 RSSI.
- Scanner adapter discards the first RSSI conversion after a tune and returns the following one.
- Opt-in source-derived UV-K1 V3 RX register profile in src/radio/bk4829_uvk1_v3_profile.c. It includes the upstream baseline register sequence, RX enable sequence and VHF/UHF LNA GPIO selection, adapted to the D10S bus API. Source and license attribution are recorded in THIRD_PARTY_NOTICES.md.

RSSI is raw relative data, **not calibrated dBm**. ATT offsets, Delay and Turbo parameters are not claimed calibrated.

## Build and test

Requirements: a C11 compiler and Make, or CMake.

    make test

Equivalent CMake build:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

GitHub Actions executes host tests, Makefile tests and freestanding ARM Cortex-M0+ object compilation.

## Current status — not a flashable firmware

The portable Spectrum application and source-derived UV-K1 V3 BK4829 profile are now in the build and host test suites. They are **not yet validated on the user's physical radio**. The profile is an opt-in reference based on one upstream V3 source revision; it is not proof that the exact radio PCB has the same RF path, GPIO routing, LNA or switch wiring. The board-specific GPIO/SPI implementation, confirmed PCB revision, hardware RX/AGC/LNA measurements, keypad integration, target startup/linker integration and complete linked firmware image are still missing.

Do not flash intermediate objects, host test outputs or any file described as a smoke test. No flashable BIN is published in this repository.

The scanner reserves two 256-point buffers (about 4 KiB on a 32-bit ABI) plus the 1 KiB framebuffer, detector state and stack. The final RAM/stack budget must be proven from the linked ARM map.

## Engineering references

- Architecture contract: docs/architecture.md
- Evidence and release gates: docs/evidence-and-release-gates.md
- Spectrum algorithm and scanner: docs/spectrum-algorithm.md
- Upstream platform review: docs/upstream-review.md
- BK4829 hardware adapter contract: docs/bk4829-hardware-adapter-contract.md
- BK4829 upstream evidence: docs/bk4829-upstream-evidence.md
- BK4829 UV-K1 V3 profile: docs/bk4829-uvk1-v3-profile.md
- ST7565 backend and target port: docs/st7565-platform-port.md
- Integrated application layer: docs/application-layer.md
- Karina-inspired Spectrum behavior: docs/reference-behavior.md
- Third-party notices: THIRD_PARTY_NOTICES.md

The UI follows the requested Karina-inspired Spectrum workflow, not the F4HWN Spectrum screen. No Karina or F4HWN Spectrum source is copied into the D10S detector, scan planner, scanner, view renderer or framebuffer.
