# BK4829 AFC status and unresolved AFCR semantics

Review update: 2026-11-17.

## Confirmed AFC enable/disable control

The pinned UV-K1/UV-K5 V3 F4HWN source defines:

- Register: `0x73`
- Bit: `4`
- Mask: `0x0010`
- Bit value 1: AFC disabled
- Bit value 0: AFC enabled

Evidence in upstream revision `3261ca7883d5b0b60c0905d8766612dee7b4191d`:
- `App/driver/bk4819-regs.h`: `afcDisableRegSpec = {"AFC Disable", 0x73, 4, 1, 1}`.
- `App/radio.c`: sets the disable field when modulation is not FM.
- `App/driver/bk4829.c`: explicitly disables AFC for RAW receive.
- `App/apps/afscope/afscope_app.c` and `afspectro_app.c`: label REG_73 bit 4 as AFC disabled.

The driver retains legacy `BK4819_*` function names, but this is the UV-K1/V3 source's BK4829 implementation. This evidence is target-specific and is not copied from UV-K5 V1/V2 BK4819 firmware.

## D10S implementation

`d10s_bk4829_set_afc_enabled()` now performs a read-modify-write on REG_73, preserving every bit except bit 4. The Spectrum RF profile sets AFC disabled during frequency sweeps so the receiver's AFC does not pull the tuned frequency while bins are being measured.

This is an evidence-based AFC ON/OFF control. It does not claim calibrated AFC correction or an adjustable AFCR width.

## AFCR 0..7 remains unresolved

The 4PDA AFCR 0..7 discussion is a lead about a user-facing setting, not a verified register map. The available upstream source evidence identifies only the REG_73 AFC disable bit; it does not identify an AFCR 0..7 register field, its units, reset value, or safe values.

Do not write guessed AFCR values. Keep AFCR adjustment unimplemented until the field is found in authoritative BK4829 documentation, a verified source implementation, or controlled register captures.

## REG_19

The value `REG_19 = 0x1041` remains in the source-derived initialization sequence because the upstream BK4829 init writes it. It is not identified as AFC and is not used by the D10S AFC helper.

## Factory firmware versus factory register dump

The upstream repository contains `archive/quansheng.k1.stock.firmware.v7.03.01.bin` (Git blob SHA `7beb185168f0e04989320ff4589b9b8399b7bbdf`; 71,268 bytes). This is a stock firmware image, not a live register dump. Its first vector words decode to initial SP `0x200031A0` and Reset vector `0x080028D5` (Thumb), consistent with an application vector table based at `0x08002800`. This is a useful image-layout reference for the D10S target, but it does not by itself prove the values present in a particular radio at runtime.

No verified factory register dump for the exact radio/PCB is currently recorded in D10S. Do not redistribute or incorporate the stock binary into D10S; its presence in an upstream archive does not establish a license to redistribute it.

## Remaining hardware evidence

- Confirm actual PCB revision and BK4829 bus pin mapping against the radio.
- Capture or otherwise validate the receiver register state on the target hardware.
- Measure frequency settling, RSSI stability and RF path/LNA switching across the supported bands.
- Keep AFCR 0..7 marked unresolved until its actual register field is identified.
