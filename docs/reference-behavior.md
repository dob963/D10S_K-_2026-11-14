# Spectrum behavior reference and D10S requirements

## Reviewed reference

Primary behavioral reference: [autumn1rain/karina-v3-k1](https://github.com/autumn1rain/karina-v3-k1), the public Karina firmware repository for UV-K5 V3, UV-K1 and UV-K1(8). Its repository declares Apache-2.0. The public tree reviewed on 2026-11-14 contains documentation and release material, but not the application source files for the Spectrum implementation; therefore D10S does not copy or vendor Karina source code.

Supplementary maintainer notes: [Dim0s firmware pack — Karina information](https://github.com/Dim0s/quansheng-UV-K5v3-K1-firmware-pack). These notes describe the Spectrum behavior and known limitations. They are treated as implementation requirements to verify, not proof that the same timing values are optimal on the target radio.

## Observed Spectrum behavior to preserve

- Scan several user-selected bands in one run, rather than only one continuous VFO range.
- Provide a Delay control: reducing the delay increases speed but can reduce sensitivity.
- Provide a Turbo mode for faster scanning, with an explicit risk of missing weak or short signals.
- Let the operator adjust the signal-opening threshold relative to the background noise.
- Support band-specific scan ranges and preserve correct frequency labels while bands rotate.
- Filter transient spikes, including VHF-related display/RF artifacts.
- Provide signal capture/hold and a way to stop and inspect a captured signal.

Maintainer notes report a change from static waits toward adaptive delays in the July 2026 development line, later applied permanently in September 2026. The notes also mention a VHF spike filter in August 2026. D10S will validate these behaviors independently; it will not copy old menu values or assume their timings are suitable for every board.

## D10S implementation mapping

| Reference behavior | D10S design |
| --- | --- |
| Multi-band scan | spectrum_plan.c: up to eight validated bands, exact uniform bin grids and per-band dwell |
| Delay | d10s_spectrum_scanner_set_delay(): changes settling delay only at a sweep boundary |
| Turbo | Not enabled by a hard-coded shortcut. A separate adaptive acquisition policy must be measured against real receiver RSSI before use |
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

These three UI/memory cases are not yet implemented in the portable scanner module. They are explicit requirements for the later display and memory integration stage.

## Visual direction

D10S is an independent project. Its Spectrum screen should follow the requested Karina-like visual behavior: a high-contrast graph, legible frequency and RSSI labels, moderately bold typography, a clear noise threshold line, a visible peak/cursor, and a simple multi-band status. F4HWN's Spectrum screen is not the selected visual reference.

No Karina or F4HWN source code has been copied into the D10S portable detector, scan planner, or scanner state machine.
