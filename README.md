# D10S_K-_2026-11-14

Independent, receive-only Spectrum Analyzer firmware project for the **standard Quansheng UV-K1** (user-reported bootloader 7.03.01; platform family PY32F071 + BK4829). This project does not target UV-K1(8) V3 Mini Kong or UV-K5 V3. Exact PCB revision and physical compatibility still require validation.

## Implemented portable modules

### Spectrum engine
- Integrated coordinator in src/app/spectrum_app.c connects scanner, detector, snapshots, graph renderer, framebuffer and ST7565 controller.
- Adaptive noise floor: per-sweep percentile, rolling five-sweep history and smoothing.
- Relative peak detection, local prominence, three-sweep confirmation, parabolic frequency refinement and confirmed peak hold.
- Multi-band plan: up to eight validated bands with uniform grids and per-band dwell. Initial target spans (144–146 MHz and 430–440 MHz) use 129 points each, checked by a dedicated host test for exact grid divisibility.
- Non-blocking tune/settle/RSSI acquisition; alternate sweeps run in opposite directions to reduce directional bias, while samples are reordered by frequency before detector processing; each sample is tagged with the actual frequency returned by the radio adapter.
- Only complete sweeps are published; manual band and ATT changes invalidate stale snapshots.
- Delay control and optional adaptive Turbo polling, bounded by caller-provided timings.
- Six-step BK4829 receive-gain control is connected through REG_13. The UI identifies the six uncalibrated software stages as `AT0`–`AT5`; the underlying register presets correspond to the source-derived `ATT 0`, `ATT 6`, `ATT 15`, `ATT 27`, `BYP` and `BYP+` settings. The write preserves REG_13 bits 10..15, waits 40 ms before the first post-change sample, and resets detector history. The gain response still requires RF measurement on the actual board; no dBm calibration is claimed.

### Display
- 128x64 Spectrum graph with RSSI trace, threshold line, peak marker, cursor and frequency label.
- 1-bit framebuffer with normal and moderately bold 5x7 text.
- ST7565 page-addressing and eight-page flush controller, with UV-K1 power sequencing, page clearing, reverse-SEG orientation, column offset defaults and active-high backlight on/off.
- PY32F071 GPIO/SPI callbacks and a debounced keypad matrix scanner are implemented against the source-derived standard UV-K1 pin map. Current key map: 1 selects VHF 144–146 MHz; 2 selects UHF 430–440 MHz; 3 toggles adaptive Turbo; 4 cycles Delay through 8/12/16/24 ms; 5 cycles the six BK4829 gain stages; 6/7 lower or raise the adaptive noise-opening margin and 8 restores its default; 9 freezes the last complete sweep without further RF reads and resumes from the first bin; MENU/0 toggles or places the cursor; UP/DOWN move one bin; SIDE1/SIDE2 move eight bins; F toggles normal/moderately bold labels; STAR toggles peak-marker visibility. Timing and gain changes are deferred to safe sweep boundaries, and the first sample after an ATT change uses a 40 ms settling window. Physical key mapping, debounce, display behavior and RF gain response still need radio validation.

### BK4829
- Three-wire register read/write bus, timing calculation, frequency conversion and raw 9-bit REG_67 RSSI.
- Source-derived receive windows: 18–630 MHz and 840–1300 MHz; the 630–840 MHz gap is rejected. These limits follow the upstream K1/V3 RX range implementation, not a claim that every frequency is equally sensitive.
- The pinned upstream RF-path selector uses GPIO4/VHF_LNA only below 28 MHz and GPIO3/UHF_LNA at or above 28 MHz. Consequently, both configured analyzer bands (144–146 MHz and 430–440 MHz) currently select GPIO3; this is intentional source-derived behavior, not proof that the exact radio PCB routes those paths correctly.
- Scanner adapter discards the first RSSI conversion after a tune and returns the following one.
- `d10s_bk4829_uvk1_v3_radio_init()` binds the source-derived profile and scanner adapter into the exact radio-ops interface consumed by the Spectrum app; it still requires a separately verified board GPIO bus.
- Opt-in source-derived standard UV-K1 RX register profile in src/radio/bk4829_uvk1_v3_profile.c. It includes the upstream baseline register sequence, RX-mode setup and VHF/UHF LNA selector, adapted to the D10S bus API. It asserts the source-documented GPIO0 RX_ENABLE output alongside the selected internal LNA path; it never asserts GPIO1 PA_ENABLE and contains no transmit code. Physical RF-path operation remains unverified. Source and license attribution are recorded in THIRD_PARTY_NOTICES.md.

RSSI is raw relative data, **not calibrated dBm**. ATT stages are gain presets rather than calibrated absolute attenuation measurements; no cross-stage RSSI correction is applied. Delay and Turbo timings are software presets, not measured RF calibration values.

## Build and test

Requirements: a C11 compiler and Make, or CMake.

    make test

Equivalent CMake build:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

GitHub Actions executes host tests, Makefile tests and freestanding ARM Cortex-M0+ object compilation.

## Current status — not a flashable firmware

The portable Spectrum application and source-derived standard UV-K1 BK4829 profile are included in the build and host test suites. They are **not yet validated on the user's physical radio**. The profile is based on one pinned upstream K1/K5 V3 firmware revision, using its K1 board implementation; it does not prove that the exact PCB has the same RF path, GPIO routing, LNA or switch wiring. The PY32F071 GPIO/SPI backend, keypad controls, Delay/Turbo controls and six REG_13 receive-gain presets are integrated. ATT changes are applied at a safe sweep boundary and followed by a 40 ms software settling interval; gain response and timing still require measurement on the actual board. Raw RSSI is not calibrated in dBm and no cross-stage RSSI compensation is claimed. The target startup, linker, application link and raw-image structural checks run in CI. CI success is not a hardware boot test: the linked image remains diagnostic and is not released for flashing. The confirmed PCB revision, electrical pin map, RX/AGC/LNA behavior, keypad/display operation, actual gain-step response and bootloader hand-off remain physical release gates.

Do not flash intermediate objects, host test outputs or any file described as a smoke test. No hardware-approved flashable BIN is published in this repository; CI artifacts remain diagnostic release candidates pending physical validation.

The scanner reserves two 256-point buffers (about 4 KiB on a 32-bit ABI) plus the 1 KiB framebuffer, detector state and stack. CI checks linker bounds and per-function stack-usage limits; these static checks do not measure cumulative nested stack use or stack high-water mark on hardware.

The alternating sweep-direction behavior is independently implemented from the documented Armel/F4HWN Spectrum workflow; no upstream Spectrum application source is copied. The upstream manual's hardware warning is retained: BK4819 and BK4829 front-end, AGC and RSSI characteristics are not interchangeable.

## Engineering references

- Architecture contract: docs/architecture.md
- Evidence and release gates: docs/evidence-and-release-gates.md
- Spectrum algorithm and scanner: docs/spectrum-algorithm.md
- Upstream platform review: docs/upstream-review.md
- BK4829 hardware adapter contract: docs/bk4829-hardware-adapter-contract.md
- BK4829 upstream evidence: docs/bk4829-upstream-evidence.md
- BK4829 AFC status (REG_73 bit 4 confirmed; AFCR 0..7 unresolved): docs/bk4829-afc-status.md
- BK4829 UV-K1 V3 profile: docs/bk4829-uvk1-v3-profile.md
- ST7565 backend and target port: docs/st7565-platform-port.md
- Integrated application layer: docs/application-layer.md
- PY32F071 target build, linker, startup and hardware release limits: target/README.md
- Physical standard UV-K1 hardware acceptance checklist: docs/hardware-validation-checklist.md
- Spectrum behavior requirements and source review: docs/reference-behavior.md
- Third-party notices: THIRD_PARTY_NOTICES.md

The UI is an independent advanced Spectrum design. No application source from other firmware projects is copied into the D10S detector, scan planner, scanner, view renderer or framebuffer.
