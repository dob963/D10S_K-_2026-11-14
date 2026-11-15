# Upstream source review

## Selected platform reference

Repository: https://github.com/armel/uv-k1-k5v3-firmware-custom

Reviewed revision: 3261ca7883d5b0b60c0905d8766612dee7b4191d. The repository declares Apache-2.0. Any derived code must retain required license and copyright notices.

This source provides a useful PY32F071 UV-K1/UV-K5 V3 platform reference: startup, HAL, linker configuration, ST7565, keyboard, board GPIO and the driver file App/driver/bk4829.c.

## BK4829 findings traced in the pinned source

- The RF source uses historical BK4819_* API and register names while the file and target describe BK4829. This is a project-specific compatibility layer, not a general guarantee for all chips or all boards.
- The RSSI getter reads register 0x67 and masks 0x01FF; D10S preserves this raw 9-bit range and does not claim dBm.
- Frequency tuning writes registers 0x38 and 0x39; the radio interface uses 10 Hz frequency units.
- The upstream BK4819_Init() contains the baseline register sequence now isolated in the optional D10S V3 profile.
- BK4819_RX_TurnOn() writes REG_37=0x9F1F, REG_30=0x0000, then REG_30=0xBFF1.
- BK4819_PickRXFilterPathBasedOnFrequency() selects GPIO4 for VHF LNA below 28 MHz and GPIO3 for UHF LNA at/above 28 MHz. The upstream RX setup asserts GPIO0 (source-named RX_ENABLE), while its TX setup clears GPIO0 and separately controls GPIO1 as PA_ENABLE. D10S follows the RX enable semantics and never asserts GPIO1; the actual RF path still needs board validation.
- The upstream board file maps BK4829 CSN/SCK/SDA to PF9/PB8/PB9 and ST7565 SCK/MOSI/A0/CS to PA5/PA7/PA6/PB2. These are candidate pins for that source's board target only. They are **not verified for the user's actual PCB revision**.

## Spectrum behavior reference: Armel/F4HWN and fagci

The pinned Spectrum implementation (`App/app/spectrum.c` and `spectrum.h`) carries a file-level Apache-2.0 notice for Copyright 2023 fagci. The upstream repository NOTICE identifies the analyzer as work originally developed by fagci and credits the UV-K1/K5 V3 port to muzkr and Armel/F4HWN. The pinned upstream manual documents alternating sweep direction on each full cycle to reduce directional bias, plus AUTO/MANUAL trigger operation. D10S adopts the alternating-direction behavior as an independently implemented scanner feature; it does not copy the upstream Spectrum source. The acquisition order is reversed on alternate sweeps, while each completed snapshot is written back in ascending frequency order before detector processing and display. This preserves the detector's uniform-grid contract and stable cursor indexing. The upstream manual also explicitly warns that BK4819 and BK4829 have different front-end, LNA/PGA, AGC and RSSI scaling; no BK4819 thresholds or calibration values are transplanted into D10S.

## D10S integration

The source-derived register profile is in src/radio/bk4829_uvk1_v3_profile.c. It retains the source baseline and RX-mode setup while adapting writes to the D10S bus API. Frequency tuning is kept in the scanner adapter, so the profile's RX-resume callback selects the internal LNA path without duplicating synthesizer writes. The profile does not claim that GPIO0 or an external RF switch has been enabled.

No automatic assumption is made that a repository name or radio model is proof of identical PCB routing. Confirm actual board revision, MCU marking, pin continuity, RF switch and LNA topology before selecting this profile on hardware.

## Spectrum direction

D10S retains an independent detector and interface. Its layout, scan flow and typography are designed as an original advanced Spectrum interface. No application source from other firmware projects is copied into D10S.

## Evidence limits

The profile compiles and is exercised by a fake-bus register-write test. That proves the D10S code emits the expected words to the bus abstraction. It does not prove RF reception, external RX enable, RF switch state, LNA gain, AGC settling, correct actual PCB GPIO mapping, LCD or bootloader behavior. A complete target image and physical radio validation remain mandatory.
