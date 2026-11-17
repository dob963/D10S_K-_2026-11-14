# BK4829 upstream driver evidence review

Review date: 2026-11-14

## Source inspected

- Upstream project: https://github.com/armel/uv-k1-k5v3-firmware-custom
- Reviewed source revision: `3261ca7883d5b0b60c0905d8766612dee7b4191d`
- Files inspected:
  - `App/driver/bk4829.c`
  - `App/driver/bk4819.h`
  - `App/app/spectrum.c`
  - `App/app/scanner.c`
  - `App/board.h`

The upstream file named `bk4829.c` uses legacy `BK4819_*` function names. This is a naming/API compatibility detail, not evidence that a BK4819 register table can be transplanted without review.

## Evidence useful to D10S

1. The upstream BK4829 implementation bit-bangs its three-wire interface using CSN=PF9, SCL=PB8 and SDA=PB9. These are source-derived pins for that upstream target configuration only. They are not yet verified against the exact PCB revision in the user's radio.
2. Its RSSI getter reads register `0x67` and masks to nine bits (`0x01FF`). The portable D10S core already preserves the 0..511 raw range.
3. Upstream Spectrum and scanner acquisition paths discard one RSSI read after tuning and use the following read. This is an important AGC/settling behavior to reproduce in the hardware adapter and then measure on the target.
4. The upstream BK4829 source has its own initialization sequence and register writes. These must be reviewed as a complete, chip- and board-specific sequence; individual values must not be lifted into D10S without their ordering, preconditions, and RF path context.
5. `App/board.h` exposes generic board initialization functions but does not by itself establish the full RF switch/LNA mapping.
6. `App/driver/bk4819-regs.h` defines `afcDisableRegSpec` as register `0x73`, bit 4, mask 1. `App/radio.c` sets this field when modulation is not FM; `App/driver/bk4829.c` and RAW audio applications set it to disable AFC. This is direct source evidence for AFC enable/disable, not for AFCR width 0..7.
7. The active `#if 1` DTMF coefficient array in `App/driver/bk4829.c` is `{111,107,103,98,80,71,58,44,65,55,37,23,228,203,181,159}`; the alternative `#else` table is not the active init branch.

## D10S implementation decision

- Keep the portable scanner independent of chip registers.
- Implement the hardware adapter so a tune operation performs the verified RF/receiver setup, waits the configured settle interval, discards the first RSSI sample where the upstream behavior is applicable, and returns the subsequent raw 9-bit RSSI sample.
- Treat PF9/PB8/PB9 as a candidate mapping only until the exact standard UV-K1 PCB and continuity/pin mapping are confirmed.
- Do not convert raw RSSI to dBm. Do not claim ATT compensation is calibrated until measurements across attenuation settings and bands exist.
- D10S sets REG_73 bit 4 to disable AFC during Spectrum sweeps, using the source-confirmed control field. AFCR width 0..7 remains unresolved.
- Do not claim receiver initialization is complete until startup, RX enable, RF switch, LNA/ATT, frequency tuning and RSSI have been checked on the actual radio.

## Licensing

The reviewed upstream source carries Apache-2.0 notices. Any future code reuse must retain required copyright/license notices and be recorded in a third-party attribution file. This note records source observations; it does not copy upstream implementation code.
