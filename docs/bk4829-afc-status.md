# BK4829 AFC status and unresolved register semantics

Status: deliberately not implemented.

## Current initialization

The D10S profile contains a baseline initialization sequence adapted from the pinned Apache-2.0 UV-K1 BK4829 driver revision recorded in `docs/bk4829-upstream-evidence.md`. The value `REG_19 = 0x1041` is retained solely because it occurs in that source-derived initialization sequence.

**No claim is made that REG_19 is an AFC register.** Its function is not established by the available source evidence. It must not be interpreted as AFC merely because it appears in initialization.

## AFC evidence rules

- Do not copy AFC settings from BK4819-based UV-K5 V1/V2 firmware. BK4819 and BK4829 RF/AGC behavior is not interchangeable.
- AFCR values discussed on 4PDA are research leads only; they do not establish register addresses, bit fields, reset values, or safe operating values.
- Do not implement AFC writes until an authoritative BK4829 datasheet/register description or controlled before/after register dumps identify the relevant controls and their semantics.
- Preserve the current receive initialization independently of AFC research. Do not change the baseline sequence based on guesses.

## Explicit implementation TODO

`TODO(AFC)`: identify BK4829 AFC control/status registers, bit meanings, valid ranges, update timing, and interaction with tuning/AGC. Record the evidence and tests before adding any register writes. Until then AFC remains unimplemented and must not be advertised as operational.

## Factory dump status

No verified factory register dump for the exact target radio/PCB is currently recorded in this repository. The existing initialization is therefore source-derived, not factory-dump-confirmed. A dump must be captured from the exact radio and annotated with chip identity, board revision, frequency, mode, and timing before it is used as evidence.
