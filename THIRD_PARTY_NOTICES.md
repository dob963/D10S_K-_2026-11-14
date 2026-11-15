# Third-party notices

## UV-K1 V3 BK4829 register profile

File: `src/radio/bk4829_uvk1_v3_profile.c` and its header.

Derived register defaults, receive-mode writes, RX_ENABLE/LNA GPIO semantics and GPIO numbering from:
- Project: `armel/uv-k1-k5v3-firmware-custom`
- Files: `App/driver/bk4829.c`, `App/radio.c`, `App/driver/bk4819-regs.h`
- Pinned commit: `3261ca7883d5b0b60c0905d8766612dee7b4191d`
- Copyright notices in source: Copyright 2025 muzkr; Copyright 2023 Dual Tachyon.
- License: Apache License 2.0, copy included at `LICENSES/Apache-2.0.txt`.

The D10S files are modified adaptations: register writes use the D10S bus API, receiver callbacks are separated from the scanner's frequency write, GPIO0 RX_ENABLE and the appropriate LNA selector are asserted for receive, GPIO1 PA_ENABLE is deliberately never asserted, and profile failures attempt a fail-closed stop. The D10S source files contain a notice identifying these changes.

Relevant attribution from the upstream repository's NOTICE: the UV-K1/UV-K5 V3 port is joint work by muzkr and Armel / F4HWN, and the underlying BK4819-compatible driver carries the Dual Tachyon and muzkr copyright notices. Upstream project: https://github.com/armel/uv-k1-k5v3-firmware-custom. Attribution does not imply endorsement.

No Karina or F4HWN Spectrum code is included in the D10S detector, planner, scanner, view, or framebuffer.

## BK4829 receive gain / attenuation ladder

File: `src/radio/bk4829.c`.

The six REG_13 receive-gain presets and their interpretation (ATT 0, -6, -15, -27 dB, BYP, BYP+) are adapted from:
- Project: `armel/uv-k1-k5v3-firmware-custom`
- File: `App/app/foxhunt.c`
- Pinned commit: `3261ca7883d5b0b60c0905d8766612dee7b4191d`
- Copyright: Copyright 2026 Armel F4HWN
- License: Apache License 2.0; the applicable license text is included at `LICENSES/Apache-2.0.txt`.

D10S adaptation: the gain presets are exposed through the standalone BK4829 register API; REG_13 bits 10..15 are preserved by read-modify-write; RF gain changes are queued until a complete sweep boundary; the detector history is reset and the first post-change sample receives a 40 ms settling window. No dBm calibration or absolute RSSI correction is claimed.
\n## Spectrum acquisition behavior reference\n\nD10S independently implements alternating sweep direction (documented in the Armel/F4HWN Spectrum manual) and optional BK4829 REG_63 readiness polling in Turbo (behavior observed in `App/apps/spectrum3d/spectrum3d_app.c`, Copyright 2026 Armel F4HWN, Apache-2.0, pinned commit `3261ca7883d5b0b60c0905d8766612dee7b4191d`). No Spectrum or Spectrum3D application source was copied. The relevant upstream repository is credited in `docs/third-party-license-audit.md`; its Apache-2.0 license and NOTICE were reviewed.\n