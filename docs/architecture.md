# D10S Spectrum Analyzer — architecture contract

Target: standard Quansheng UV-K1 only (user-reported bootloader 7.03.01). UV-K1(8) V3 Mini Kong and UV-K5 V3 are excluded; exact PCB revision and MCU marking require physical confirmation.
Expected platform: Puya PY32F071 Cortex-M0+, BK4829 RF front end, ST7565 128x64 LCD.
Bootloader reported for the target unit: 7.03.01 (must be re-read before any release).

## Scope
- Receive-only spectrum analyzer.
- No transmit, beacon, messenger, or general-purpose transceiver features.
- Independent project identity: D10S.
- Primary design: original D10S Spectrum interface, with a compact 128x64 graph, clear frequency/readout hierarchy, operator controls and moderately bold labels.
- Implement the Spectrum workflow independently. Do not copy proprietary application code or reproduce third-party assets. Retain all required notices for any separately licensed code actually reused.
- Supporting firmware projects, including F4HWN/GOGUFW and UV-K1/K5 V3 work, are technical references only for platform drivers, RF handling, timing, memory and test techniques. D10S uses an independently implemented Spectrum analyzer; no Karina firmware is the target or a code base.
- Typography: clear, moderately bold frequency and scale labels; avoid excessive weight.

## Functional design
1. Multi-band sweep scheduler with per-band limits and independent sweep state, with independent per-band state and limits.
2. Relative RSSI acquisition; no dBm label until calibration points are measured on the target radio.
3. Adaptive noise estimate based on a rolling percentile/statistical window, not the minimum sample of a sweep.
4. Candidate peak gate with multi-pass confirmation; one isolated sample cannot open a signal.
5. Local frequency refinement around a confirmed candidate.
6. Peak hold and stable display update.
7. ATT state tracked explicitly; attenuation changes must not silently shift the reported level. Any compensation is measured in raw RSSI units.
8. Completed sweeps are published as stable, band-tagged snapshots; no partial buffer is exposed to the display.
9. Delay can be changed only at a sweep boundary. Turbo uses optional RSSI-stability polling bounded by a caller-configured maximum delay; it stays disabled until hardware timings are measured. Expose the speed/sensitivity trade-off through the Delay and Turbo controls.
10. The current release is Spectrum-only: channel memories, memory-slot metadata, blacklist management and general transceiver functions are explicitly out of scope unless required to reproduce a documented Spectrum scan feature.
11. EXP/cursor frequency derived from the displayed snapshot's stored frequency data and band tag.
12. Provide multi-band scanning, operator-adjustable Delay/Turbo, adjustable signal-opening threshold, band-specific frequency labels, transient-spike filtering, confirmed signal capture/hold and inspection. Treat exact timing and thresholds as hardware-calibration items, not copied constants.

## Required Spectrum bug protections
- Changing the active band while a peak is held must reset detector tracking without corrupting the next band's frequency grid.
- EXP/cursor frequency must come from the displayed snapshot's stored frequency values, including during automatic multi-band rotation.
- A Turbo setting must not silently trade away weak-signal detection: its faster mode and Delay state must be apparent and reversible.

## Layer boundaries
- platform/: clock, GPIO, SPI, timing and display bus.
- radio/: BK4829 bus, verified RX setup, tuning, RSSI and attenuation.
- spectrum/: scheduler, noise estimator, peak confirmation/refinement and peak hold.
- ui/: original D10S layout and operator workflow, independently implemented display primitives, cursor and settings. The portable 128x64 graph renderer, framebuffer and callback-based ST7565 page transport are implemented and host-tested; the PY32F071 GPIO/SPI backend is implemented against a source-derived candidate pin map, while physical panel validation and final visual comparison remain outstanding.
- tests/: host-side deterministic tests for frequency grids, detector and state transitions.

## Release gates
A host test or successful cross-compile is not evidence of RF reception or safe flashing. A flashable artifact requires a complete target startup, a verified board-specific BK4829 profile, a valid K1 image layout and DFU/package validation. Hardware validation is still required for RF, LCD, timing, LNA switching and bootloader transfer. The visual acceptance gate requires review of the D10S output on the physical 128x64 display.
