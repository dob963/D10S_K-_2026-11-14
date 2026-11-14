# D10S Spectrum Analyzer — architecture contract

Target: Quansheng UV-K1 / UV-K1(8) V3, subject to exact board-revision confirmation.
Expected platform: Puya PY32F071 Cortex-M0+, BK4829 RF front end, ST7565 128x64 LCD.
Bootloader reported for the target unit: 7.03.01 (must be re-read before any release).

## Scope
- Receive-only spectrum analyzer.
- No transmit, beacon, messenger, or general-purpose transceiver features.
- Independent project identity: D10S.
- UI direction: Karina-style spectrum presentation and scan workflow; do not reproduce the F4HWN spectrum UI.
- Typography: clear, moderately bold frequency and scale labels; avoid excessive weight.

## Functional design
1. Multi-band sweep scheduler with per-band limits and independent sweep state.
2. Relative RSSI acquisition; no dBm label until calibration points are measured on the target radio.
3. Adaptive noise estimate based on a rolling percentile/statistical window, not the minimum sample of a sweep.
4. Candidate peak gate with multi-pass confirmation; one isolated sample cannot open a signal.
5. Local frequency refinement around a confirmed candidate.
6. Peak hold and stable display update.
7. ATT state tracked explicitly; attenuation changes must not silently shift the reported level. Any compensation is measured in raw RSSI units.
8. Completed sweeps are published as stable, band-tagged snapshots; no partial buffer is exposed to the display.
9. Delay can be changed only at a sweep boundary. Turbo remains a separate adaptive acquisition policy, not a blind reduction of settling time.
10. Optional blacklist and signal-memory handling, kept separate from the acquisition loop.
11. EXP/cursor frequency derived from the displayed snapshot's stored frequency data and band tag.

## Required bug protections
- First memory slot must preserve its channel/name metadata.
- Changing a band range while a peak is held must not corrupt the active frequency or range.
- EXP cursor must report the correct frequency during multi-band scanning.

## Layer boundaries
- platform/: clock, GPIO, SPI, timing and display bus.
- radio/: BK4829 bus, verified RX setup, tuning, RSSI and attenuation.
- spectrum/: scheduler, noise estimator, peak confirmation/refinement and peak hold.
- ui/: Karina-inspired layout, display primitives, cursor and settings.
- tests/: host-side deterministic tests for frequency grids, detector and state transitions.

## Release gates
A host test or successful cross-compile is not evidence of RF reception or safe flashing. A flashable artifact requires a complete target startup, a verified board-specific BK4829 profile, a valid K1 image layout and DFU/package validation. Hardware validation is still required for RF, LCD, timing, LNA switching and bootloader transfer.
