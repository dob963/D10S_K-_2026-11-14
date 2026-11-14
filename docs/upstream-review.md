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
- BK4819_RX_TurnOn() writes the RX register sequence REG_37=0x9F1F, REG_30=0x0000, then REG_30=0xBFF1.
- BK4819_PickRXFilterPathBasedOnFrequency() selects a VHF path below 28 MHz and UHF path otherwise using BK4829 GPIO output bits; the upstream mapping uses GPIO4 for VHF LNA, GPIO3 for UHF LNA and GPIO0 for RX enable.
- The upstream board file maps BK4829 CSN/SCK/SDA to PF9/PB8/PB9 and ST7565 SCK/MOSI/A0/CS to PA5/PA7/PA6/PB2. These are candidate pins for that source's board target only. They are **not verified for the user's actual PCB revision**.

## D10S integration

The source-derived register profile is in src/radio/bk4829_uvk1_v3_profile.c. It retains the source baseline sequence and RX setup while adapting writes to the D10S bus API. Frequency tuning is kept in the scanner adapter, so the profile's RX-resume callback selects LNA and enables RX without duplicating the synthesizer writes.

No automatic assumption is made that a repository name or radio model is proof of identical PCB routing. Confirm actual board revision, MCU marking, pin continuity, RF switch and LNA topology before selecting this profile on hardware.

## Spectrum direction

D10S retains an independent detector and interface. The chosen visual/behavioral direction is Karina-inspired; F4HWN's Spectrum screen is not the selected UI. No Karina or F4HWN Spectrum source code is copied into D10S.

## Evidence limits

The profile compiles and is exercised by a fake-bus register-write test. That proves the D10S code emits the expected words to the bus abstraction. It does not prove RF reception, RF switch state, LNA gain, AGC settling, correct actual PCB GPIO mapping, LCD or bootloader behavior. A complete target image and physical radio validation remain mandatory.
