# Third-party notices

## UV-K1 V3 BK4829 register profile

File: `src/radio/bk4829_uvk1_v3_profile.c` and its header.

Derived register defaults and receive/LNA state transitions from:
- Project: `armel/uv-k1-k5v3-firmware-custom`
- File: `App/driver/bk4829.c`
- Pinned commit: `3261ca7883d5b0b60c0905d8766612dee7b4191d`
- Copyright notices in source: Copyright 2025 muzkr; Copyright 2023 Dual Tachyon.
- License: Apache License 2.0, copy included at `LICENSES/Apache-2.0.txt`.

The D10S files are modified adaptations: register writes use the D10S bus API, receiver callbacks are separated from the scanner's frequency write, and profile failures attempt a fail-closed stop. The D10S source files contain a notice identifying these changes.

No Karina or F4HWN Spectrum code is included in the D10S detector, planner, scanner, view, or framebuffer.