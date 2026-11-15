# Spectrum behavior reference and D10S requirements

## D10S Spectrum behavior specification

D10S is an independent Spectrum analyzer design for the standard Quansheng UV-K1 only. UV-K1(8) V3 Mini Kong and UV-K5 V3 are excluded targets. The behavior below is the D10S project specification; no external firmware application or screen layout is a required dependency.

All timing, thresholds, frequency grids and RF settings are D10S-specific provisional values. They require measurement and validation on the target radio.

## Supporting technical references

Other UV-K1/K5 V3 firmware projects may be consulted for platform drivers, RF register handling, timing, memory use, build systems and test methods. They are not the primary Spectrum UI or behavior target. Every code reuse requires file-level license and copyright review; where provenance or compatibility is unclear, implement the behavior independently.

## D10S operator-facing behavior

- Scan several user-selected bands in one run, rather than only one continuous VFO range.
- Provide a Delay control: reducing the delay increases speed but can reduce sensitivity.
- Provide a Turbo mode for faster scanning, with an explicit risk of missing weak or short signals.
- Let the operator adjust the signal-opening threshold relative to the background noise.
- Support band-specific scan ranges and preserve correct frequency labels while bands rotate.
- Filter transient spikes, including VHF-related display/RF artifacts.
- Provide signal capture/hold and a way to stop and inspect a captured signal.
- Use a clear 128x64 graph, readable frequency and scale labels, threshold line, cursor and confirmed peak marker. Typography is moderately bold, not excessively heavy.

Maintainer notes report a change from static waits toward adaptive delays in the July 2026 development line, later applied permanently in September 2026. The notes also mention a VHF spike filter in August 2026. D10S will validate these behaviors independently; it will not copy old menu values or assume their timings are suitable for every board.

## D10S implementation mapping

| Reference behavior | D10S design |
| --- | --- |
| Multi-band scan | spectrum_plan.c: up to eight validated bands, exact uniform bin grids and per-band dwell |
| Delay | d10s_spectrum_scanner_set_delay(): changes settling delay only at a sweep boundary |
| Turbo | Optional adaptive RSSI-stability polling with caller-supplied minimum delay, poll interval, stability tolerance and maximum Delay; disabled by default and still requires real-radio tuning |
| Noise/squelch bar | Adaptive rolling percentile plus a configurable relative margin, not a fixed absolute threshold |
| Peak filtering | Local-maximum prominence and confirmation over consecutive completed sweeps |
| Frequency precision | Store the actual frequency reported by the radio adapter for each sample |
| Band change while holding a peak | Reset detector state on automatic band rotation; manual selection invalidates the published snapshot and clears old peak/noise state |
| Stable display data | Publish only completed sweep snapshots; partial scans stay in the working buffer |
| ATT | Apply only caller-supplied measured raw-RSSI offset; no invented dB calibration |

## Regression cases retained

1. The first saved memory entry must display its stored channel name.
2. Changing a band's limits while a peak is held must not leave a shifted frequency range or stale captured frequency.
3. EXP cursor frequency during multi-band scanning must use the frequency grid belonging to the displayed snapshot, never the currently active band's range by assumption.

These memory-slot cases are historical notes and are not part of the current Spectrum-only release unless a future UI integration explicitly adds a memory browser.

## Visual acceptance

The D10S renderer is an original implementation. The current renderer provides a high-contrast graph, threshold line, confirmed peak marker, cursor and frequency label. Final contrast, spacing and legibility must be checked on the physical 128x64 LCD.

The D10S detector, planner, scanner, view renderer and framebuffer are independently implemented.
