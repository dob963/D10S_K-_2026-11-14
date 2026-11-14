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