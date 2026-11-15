# D10S UV-K1 V3 hardware validation checklist

This is the remaining physical acceptance checklist for the source-derived PY32F071/BK4829/ST7565 bring-up image. No item below is marked passed: the repository has no access to the user's physical radio.

**Safety gate:** do not flash the intermediate image. Preserve a verified stock firmware backup and a tested bootloader recovery method before any future target-radio test. The D10S application is receive-only and contains no TX code.

## Software verification already completed

- [x] Host tests cover independent multi-peak confirmation (up to eight candidates), strongest-eight bounding, peak tracking through dominant-peak changes, loss/reacquisition, band-reset clearing and detection at both band edges.
- [x] CI run [43704790473](https://github.com/dob963/D10S_K-_2026-11-14/actions/runs/43704790473) passed warning-as-error compilation, all 10 CMake tests (including RSSI percentile bucket boundaries), Cortex-M0+ cross-compilation, full PY32F071 link/layout checks, static cumulative stack analysis and Makefile tests. Current static estimate is 904/1,536 bytes including a 40-byte exception reserve; the 256-byte `__aeabi_idiv` allowance is manually assigned, not measured, and the estimate is not a hardware high-water mark.

These checks do not substitute for the physical tests below.

## 1. Identify the exact target

- [ ] Photograph/read the MCU top marking and package.
- [ ] Record the exact PCB revision and any RF daughterboard/revision marking.
- [ ] Read back and record the installed bootloader version.
- [ ] Confirm the actual display-controller marking and connector/revision.
- [ ] Reconcile those observations with the PY32F071xB / BK4829 / ST7565 assumptions in `docs/uvk1-v3-board-map-evidence.md`.

## 2. Verify board pins before enabling the target port

With the radio unpowered, check continuity from the MCU to each device pin. Record resistance/continuity and the observed pin direction for:

| Signal | Candidate MCU pin | Required check |
|---|---|---|
| BK4829 CSN | PF9 | Active-low chip select continuity |
| BK4829 SCL | PB8 | Serial clock continuity |
| BK4829 SDA | PB9 | Bidirectional data continuity |
| ST7565 SCK | PA5 | SPI clock continuity |
| ST7565 MOSI | PA7 | SPI data continuity |
| ST7565 A0 / D-C | PA6 | Command/data select continuity |
| ST7565 CS | PB2 | Active-low panel chip select |
| LCD backlight | PF8 | Polarity and driver path |

- [ ] Confirm that keypad rows PB15..PB12 and columns PB6..PB3 match the board.
- [ ] Confirm PF8 backlight polarity and that the pin can be driven at the intended logic level.
- [ ] Confirm the candidate GPIO mappings do not conflict with another board function.

Stop and revise the board port if any pin differs. Do not probe an unknown pin by blindly toggling it on a powered radio.

## 3. Verify LCD and keypad independently

- [ ] Confirm ST7565 software-reset and staged power-up using a logic analyzer.
- [ ] Confirm SPI1 mode, bit order, clock rate, CS polarity and A0 polarity.
- [ ] Check full-page clearing, page/column addressing, segment/common orientation and contrast.
- [ ] Confirm the first boot does not leave the panel blank or inverted.
- [ ] Exercise every matrix key and verify one event per press, correct key identity and no phantom/multiple-key events.
- [ ] Confirm key 9 freezes the last complete graph and the H marker appears; pressing 9 again restarts the current band at bin zero.

## 4. Verify the BK4829 receive path

Use a conducted RF source through a suitable attenuator/coupler, with no radiated test transmission.

- [ ] Capture CSN/SCL/SDA and verify register write/read framing.
- [ ] Confirm each serial edge and turnaround delay against the 35-HCLK-cycle implementation and the measured HCLK.
- [ ] Confirm baseline register writes and RX transition against the pinned UV-K1 V3 source sequence.
- [ ] Verify REG_33 contains GPIO0 RX_ENABLE plus the correct GPIO3/GPIO4 LNA selection for each tested frequency.
- [ ] Confirm GPIO1 PA_ENABLE is never set.
- [ ] Confirm the external receive/RF-switch path actually changes state; source code alone does not prove PCB routing.
- [ ] Verify PLL settling with known signals; the current adapter returns the programmed frequency, not a measured lock result.
- [ ] Measure AGC settling and compare the first and second REG_67 RSSI conversions after each tune.
- [ ] Verify useful reception on both VHF and UHF paths with known in-band signals.

## 5. Calibrate the analyzer before enabling faster modes

- [ ] Record raw REG_67 RSSI against known conducted input levels on each band.
- [ ] Measure noise floor and repeatability with input terminated and with known signals.
- [ ] Check peak confirmation, false-peak rejection and parabolic refinement against a known frequency source.
- [ ] Measure sweep time at the 8 ms starting dwell.
- [ ] Delay and Turbo are exposed on keys 4 and 3 respectively; measure sweep time and detection probability for weak/short signals before choosing final defaults.
- [x] Six REG_13 receive-gain presets are integrated and software-tested: ATT 0, ATT -6, ATT -15, ATT -27, BYP and BYP+.
- [ ] Verify electrically that REG_13 controls the expected RF gain path on this exact PCB; the preset names are source-derived, not measured attenuation values.
- [ ] Measure each preset's raw REG_67 response with the same conducted signal, on both bands; determine whether BYP/BYP+ alter LNA behavior as expected.
- [ ] Do not apply cross-stage RSSI compensation or label the stages as calibrated dB until those measurements exist.
- [ ] Confirm the 40 ms post-change settling interval is sufficient using repeated RSSI captures; adjust only from measured evidence.
- [ ] Do not label raw RSSI as dBm without a documented multi-point calibration.

## 6. Validate bootloader/update and recovery last

- [ ] Confirm exact application origin and bootloader hand-off against the actual radio.
- [ ] Confirm the accepted DFU/raw-image format and page-programming behavior for that bootloader revision.
- [ ] Verify a stock backup and a tested recovery path before any application transfer.
- [ ] Only after every previous gate passes, perform a controlled flash test and verify cold boot, normal scan, reset and recovery.

## Evidence to attach to the release record

Record the PCB/MCU/bootloader identifiers, continuity map, logic-analyzer captures, LCD/keypad results, RF source levels/frequencies, RSSI/noise measurements, sweep timing, DFU result, and the exact firmware commit and SHA-256. A successful host test, ARM link or structural image check is not a substitute for these measurements.
