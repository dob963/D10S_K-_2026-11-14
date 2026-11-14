# D10S_K-_2026-11-14

Independent, receive-only Spectrum Analyzer firmware project for the Quansheng UV-K1 / UV-K1(8) V3 target (reported platform: PY32F071 + BK4829). Exact PCB compatibility still needs confirmation.

## Implemented portable modules

### Spectrum engine
- Integrated application coordinator (`src/app/spectrum_app.c`) links the radio scan loop, detector, snapshot, view renderer, framebuffer and ST7565 frame transfer.
- Adaptive noise floor: per-sweep percentile, rolling five-sweep history and IIR smoothing.
- Relative peak detection: noise-relative threshold, local prominence and three-sweep confirmation by default.
- Peak refinement: three-point parabolic interpolation.
- Peak hold: updates only after a confirmed signal.
- Multi-band plan: up to eight validated bands with an exact uniform frequency grid and per-band dwell.
- Non-blocking scanner: tune, settle and RSSI-read phases through a hardware adapter.
- Actual-frequency tracking and stable display snapshots: only complete sweeps are published, with band and ATT metadata.
- Delay control at sweep boundaries and optional Turbo stability polling bounded by caller-supplied timings.
- ATT interface with caller-supplied compensation in raw RSSI units; no invented dB offsets.

### Display
- 128x64 graph renderer: RSSI trace, grid, adaptive threshold, held-peak marker, cursor and frequency label.
- Monochrome framebuffer with normal or moderately bold text.
- Callback-based ST7565 controller: panel command initialization, page addressing, and complete eight-page framebuffer flush from the Spectrum view.
- One presentation entry point now clears the framebuffer, renders a validated Spectrum snapshot, and flushes all eight LCD pages; invalid snapshots are rejected without sending a partial frame.
- Host tests verify command/data ordering, page layout, framebuffer-to-controller transfer and configured column offset. The PY32F071 GPIO/SPI callback implementation and physical panel validation remain outstanding.

### BK4829 primitives
- Three-wire register read/write transport with bus-idle recovery on communication failures.
- Timing calculation from the measured rate of the supplied delay callback; no assumed MCU clock.
- Raw REG_67 RSSI extraction (9 bits, 0..511), with no dBm conversion.
- Frequency-word conversion and REG_38/REG_39 programming in 10 Hz units.
- Datasheet operating windows enforced: 18–580 MHz and 760–1160 MHz. The 580–760 MHz gap and frequencies above 1160 MHz are rejected. These are IC limits, not proof that every frequency is usable with the UV-K1 RF front end.

RSSI is raw relative data, **not calibrated dBm**. No dB values or ATT offsets are invented.

## Build and test

Requirements: a C11 compiler and Make, or CMake.

```sh
make test
```

Equivalent CMake build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

GitHub Actions runs host tests, a Makefile build/test pass, and freestanding ARM Cortex-M0+ compilation of the portable modules.

## Current status — not a flashable firmware

This repository contains a tested portable Spectrum application pipeline, but not a target-ready radio firmware image. It still lacks a verified board-specific BK4829 initialization/RX profile, a PY32F071 GPIO/SPI callback backend, keyboard integration, target startup/linker integration, and a flashable BIN. The complete Spectrum application pipeline is tested with a simulated radio and LCD controller. The BK4829 scanner adapter connects the register bus and RSSI primitives to the Spectrum scanner, but deliberately requires board-supplied receiver initialization, RX-resume and standby callbacks; no complete verified receiver/AGC/LNA profile is present. The ST7565 controller can flush the framebuffer through callbacks, but the physical PY32F071 GPIO/SPI implementation and panel orientation/contrast still require validation. Do not flash test outputs or intermediate objects.

The scanner currently allocates two 256-point buffers (about 4 KiB on a 32-bit ABI), in addition to the 1 KiB display framebuffer, detector state and stack. The combined application RAM budget remains to be proven from the final ARM link map. The detector's percentile routine has a measured 592-byte single stack frame in the current host compiler report. Final RAM and stack usage must be checked from the linked ARM map before release.

## Engineering references

- [Third-party source and license audit](docs/third-party-license-audit.md) — source provenance, license checks, attribution requirements and reuse gates.

- [Architecture contract](docs/architecture.md)
- [Evidence and release gates](docs/evidence-and-release-gates.md)
- [Spectrum algorithm and scanner](docs/spectrum-algorithm.md)
- [Upstream platform review](docs/upstream-review.md)
- [BK4829 hardware adapter contract and bring-up gates](docs/bk4829-hardware-adapter-contract.md)
- [BK4829 upstream RSSI and board evidence](docs/bk4829-upstream-evidence.md)
- [BK4829 RF operating windows](docs/bk4829-rf-limits.md)
- [ST7565 display backend and target port](docs/st7565-platform-port.md)
- [Karina-inspired Spectrum behavior requirements](docs/reference-behavior.md)

The UI direction is the requested Karina-style Spectrum workflow, not the F4HWN Spectrum screen. The reference behavior is documented separately; no Karina or F4HWN Spectrum source has been copied into the portable detector, band planner, or scanner.
