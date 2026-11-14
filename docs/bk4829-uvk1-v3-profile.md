# UV-K1 V3 BK4829 receive profile

## Source and license

This opt-in register profile is derived from armel/uv-k1-k5v3-firmware-custom, file App/driver/bk4829.c, pinned commit 3261ca7883d5b0b60c0905d8766612dee7b4191d. The upstream repository declares Apache-2.0. Copyright and license attribution are retained in the source headers and THIRD_PARTY_NOTICES.md; LICENSES/Apache-2.0.txt contains the license text.

The adapter isolates the baseline register writes and RX-mode sequence in a separate profile and uses the D10S bus API. Spectrum scanning, detection and view code remain independent.

## Traceable sequence

The baseline array preserves the register/value write sequence from the pinned upstream BK4819_Init implementation in the BK4829 driver, including repeated writes to register 0x09.

The receive-mode transition preserves the source BK4819_RX_TurnOn writes:
- REG_37 = 0x9F1F
- REG_30 = 0x0000
- REG_30 = 0xBFF1

The profile then programs the initial frequency through REG_38/REG_39 and selects the corresponding internal LNA path. On later scanner retunes, the scanner adapter owns REG_38/REG_39; the profile resume callback only updates the LNA path. This avoids duplicate synthesizer writes.

The upstream path selector uses GPIO4 for the VHF LNA below 28 MHz and GPIO3 for the UHF LNA at/above 28 MHz. D10S currently emits the selector writes:
- VHF path: REG_33 = 0x9004
- UHF path: REG_33 = 0x9008
- stop/reset path state: REG_33 = 0x9000 and REG_30 = 0x0000

**Important omission by design:** the upstream code also manipulates GPIO0 as an RX-enable output. D10S does not assert that output in this profile because the external RX-enable / RF-switch connection has not been confirmed on the user's PCB. The profile field is named rx_mode_configured, not receiver_on, to avoid implying that an external RF path is active. The RF chip's RX mode and its external antenna/LNA path are separate facts.

## Fail-closed behavior

If a baseline write or RX transition fails, the profile attempts to clear the LNA selector and REG_30. A hardware bus failure may also prevent the stop writes; the profile therefore reports initialization failure and clears rx_mode_configured. This is a software fail-closed attempt, not a guarantee of RF hardware shutdown in the presence of a failed bus.

## Validation performed

The host test records 24-bit register writes made through a fake bus and checks baseline startup, RX-mode setup, REG_38/REG_39 ordering, VHF and UHF LNA selection, stop writes, unsupported-frequency rejection and bus-error handling. CI also compiles the module as a freestanding Cortex-M0+ object.

These tests validate D10S register-write intent only. They do not validate the actual crystal, synthesizer lock, RSSI calibration, AGC, GPIO0 external enable, antenna switch, LNA gain, RF performance, PCB pinout or startup behavior on a physical UV-K1. The profile remains opt-in until those checks are completed.
