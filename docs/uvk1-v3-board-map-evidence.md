# UV-K1 V3 board map and display evidence

Status: source-derived board map; physical radio verification remains open.

## Pinned reference

Reference repository: https://github.com/armel/uv-k1-k5v3-firmware-custom

Reviewed revision: `3261ca7883d5b0b60c0905d8766612dee7b4191d`

Relevant reference files:
- `App/board.c`
- `App/driver/bk4829.c`
- `App/driver/st7565.c`
- `Core/py32f071xb.ld`
- `Core/startup_py32f071xx.s`
- `Core/Src/system_py32f071.c`

The repository is Apache-2.0 for the reviewed BK4829 source component; file-level notices and the relevant upstream NOTICE attribution are recorded in `THIRD_PARTY_NOTICES.md`. The Puya linker/startup/device support files have separate BSD-3-Clause notices and must be audited separately before importing any of them. No vendor startup, linker, or HAL implementation has been copied into D10S by this document.

## Source-derived pin map

| Function | MCU pin / peripheral | Evidence in pinned source | D10S status |
|---|---|---|---|
| BK4829 chip select | PF9 | `bk4829.c` PIN_CSN; `board.c` GPIO init | Candidate mapping |
| BK4829 serial clock | PB8 | `bk4829.c` PIN_SCL; `board.c` GPIO init | Candidate mapping |
| BK4829 serial data | PB9 | `bk4829.c` PIN_SDA; `board.c` GPIO init | Candidate mapping |
| ST7565 SCK | PA5 / SPI1 | `st7565.c` SPI init | Candidate mapping |
| ST7565 MOSI | PA7 / SPI1 | `st7565.c` SPI init | Candidate mapping |
| ST7565 chip select | PB2 | `st7565.c` PIN_CS; `board.c` GPIO init | Candidate mapping |
| ST7565 A0 / D-C | PA6 | `st7565.c` PIN_A0; `board.c` GPIO init | Candidate mapping |
| LCD backlight | PF8, active-high | `board.c` GPIO init; `backlight.c` sets the pin for ON | Candidate mapping; D10S provides simple on/off, not the upstream PWM fade/DMA driver |
| Keypad rows | PB15..PB12, input pull-up | `board.c` GPIO init | Candidate mapping |
| Keypad columns | PB6..PB3, output | `board.c` GPIO init | Candidate mapping |
| PTT input | PB10, input pull-up | `board.c` GPIO init | Not used; D10S remains receive-only |

The BK4829 interface is bit-banged three-wire I/O. The LCD uses SPI1 in master mode, 8-bit, MSB-first, CPOL high, second-edge phase, software NSS and a /64 prescaler in the reference driver. The D10S keypad scanner reads the PB15..PB12 rows against PB6..PB3 columns, samples each column three times with 10 microsecond spacing, rejects ambiguous multi-key states, and maps the numeric/navigation keys independently. PTT is intentionally not scanned as an action.

## ST7565 behavior recorded for D10S

The reviewed K1 driver states that the panel has no usable hardware reset pin and uses software reset. Its initialization sequence:
1. Display off, software reset, 120 ms wait.
2. 1/9 bias, normal COM direction, reverse SEG direction, normal display, resistor ratio 5.0, electronic volume 31.
3. Staged power circuit: 0x2B, 0x2E, then 0x2F four times, with short waits.
4. Start line zero; clear all eight pages while display is off; then display on.
5. Pixel writes start at column offset 4.

D10S now implements this startup order in its own callback-based ST7565 driver and exposes `d10s_st7565_uvk1_v3_default_config()` for the recorded K1 settings. `src/platform/py32f071_uvk1_v3.c` provides candidate GPIO/SPI callbacks for the mapped pins. It accepts externally calibrated millisecond/tick delay functions or can use its built-in SysTick backend. The core clock can be derived from RCC state when the board's HSE frequency is supplied, or passed in as an independently measured value. Host tests validate command order, power-up waits, page clearing, column offset and full-frame writes. This is a software-level conformance test, not a physical LCD test.

## MCU and memory-layout lead

The pinned reference linker script identifies PY32F071xB with 128 KiB flash and 16 KiB SRAM, application flash origin `0x08002800`, and an application region of 118 KiB. This is a source-derived target contract, not yet accepted as the exact MCU fitted to the user's radio. The exact package/marking and PCB revision must be confirmed before the D10S linker script or pin map is treated as final.

## Release blockers

- Confirm the physical MCU marking and PCB revision on the actual UV-K1 V3.
- Check continuity of PF9/PB8/PB9 to the BK4829 and PA5/PA7/PB2/PA6 to the ST7565.
- Verify the actual LCD SPI timing, contrast, orientation and backlight polarity.
- Verify the RF switch and LNA path on VHF and UHF using known signals.
- Verify the bootloader's application offset and DFU transfer against the actual radio.
- Complete a target link, inspect ELF/map/stack/RAM, then perform hardware tests.

Until those checks pass, these assignments are documented candidates only. No BIN built from this map is approved for flashing.


## Bootloader clock and HSE source evidence (2026-11-15)

The pinned UV-K1 reference source at revision `3261ca7883d5b0b60c0905d8766612dee7b4191d` provides two relevant facts:
- `Core/Src/system_py32f071.c` defines `HSE_VALUE` as `24000000U` unless overridden.
- `Core/Src/main.c` comments that the bootloader has already configured the clock and sets the software `SystemCoreClock` value to 48 MHz.

D10S therefore supplies 24 MHz to its RCC decoder. This supports the HSE/PLL-HSE calculation path and avoids rejecting that source configuration merely because the HSE argument was zero. It does **not** prove the oscillator value or the bootloader RCC state on the user's individual radio; the assumption remains source-derived and must be checked against the exact PCB/bootloader before release. D10S does not reprogram the clock tree.

The clock unit test explicitly covers the source-derived 24 MHz HSE ×2 = 48 MHz bootloader configuration. Other unsupported or out-of-range RCC states remain fail-closed.

## Official PY32F07X clock-register cross-check (2026-11-15)

Reference: Puya, *PY32F07X Reference Manual*, revision 0.2, sections 8.1 and 8.5.1–8.5.4 (official publisher copy: https://www.puyasemi.com/download_path/%E7%94%A8%E6%88%B7%E6%89%8B%E5%86%8C/MCU%20%E5%BE%AE%E5%A4%84%E7%90%86%E5%99%A8/PY32F07X_Reference_Manual_V0.2.pdf).

Register-field comparison against `src/platform/py32f071_uvk1_v3.c`:

| Field | Reference-manual definition | D10S interpretation | Review |
|---|---|---|---|
| RCC_CR.HSIDIV, bits 13:11 | Divide HSI SYS by 1, 2, 4, 8, 16, 32, 64 or 128 | `(rcc_cr >> 11) & 7`, then divide | Match |
| HSI-to-PLL clock path | Figure 8-1 shows HSI RC branching separately to PLL and through HSIDIV to HSISYS | PLL input uses selected HSI frequency directly; HSIDIV is applied only on direct HSI/HSISYS path | Match; regression tests cover HSIDIV=128 with HSI→PLL x2/x3 |
| RCC_ICSCR.HSI_FS, bits 15:13 | 000=4 MHz, 001=8 MHz, 010=16 MHz, 011=22.12 MHz, 100=24 MHz, >=101=4 MHz | Eight-entry table implements those five values and 4 MHz for remaining codes | Match |
| RCC_CFGR.SWS, bits 5:3 | 000=HSISYS, 001=HSE, 010=PLL, 011=LSI, 100=LSE | Switch cases 0x00, 0x08, 0x10, 0x18, 0x20 | Match |
| RCC_CFGR.HPRE, bits 11:8 | 0xxx=/1, 1000=/2, 1001=/4, 1010=/8, 1011=/16, 1100=/64, 1101=/128, 1110=/256, 1111=/512 | Shift table {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9} | Match |
| RCC_PLLCFGR.PLLSRC, bits 1:0 | 10=HSI, 11=HSE; 00=no clock, 01=reserved | Accepts 2 for HSI and 3 for HSE; rejects others | Match |
| RCC_PLLCFGR.PLLMUL, bits 3:2 | 00=x2, 01=x3; other encodings reserved | Explicit decoder accepts only 00 and 01; returns failure for 10 and 11 | Match; reserved encodings are rejected |
| PLL input clock | HSI/HSE: both x2 and x3 require 16–24 MHz | Enforces 16 MHz minimum and 24 MHz maximum for both multipliers | Match; input range enforced |

### Follow-up hardening completed

The source decoder rejects reserved PLLMUL encodings 2 and 3 and enforces the PLL reference-input range in section 8.1.5 of the PY32F07X Reference Manual V0.2: 16–24 MHz for both x2 and x3. A review found that the previous x2 lower bound was incorrectly permissive; the implementation and unit test were corrected to reject 12–15.999999 MHz for either multiplier. Tests cover valid x2/x3 source combinations, reserved multiplier/source encodings, and the 16 MHz lower and 24 MHz upper boundaries for both multipliers. The correction is pending CI.

The reference manual describes LSI as 32 kHz and LSE as 32.768 kHz. D10S currently uses 32.768 kHz for both low-speed switch cases; both are below the function's 1 MHz accepted-HCLK floor and therefore cause initialization to fail rather than being used as a supported operating clock. Keep this limitation explicit if low-speed SYSCLK support is ever added.

This review validates register-field decoding against the official manual. It does not prove the radio's live RCC register values, the MCU marking, or physical oscillator accuracy.


## ST7565 resistor-ratio encoding cross-check (2026-11-15)

The D10S default `resistor_ratio = 4` is intentional: the ST7565 command is `0x20 | RR[2:0]`, so code 4 sends `0x24`. The ST7565-0A datasheet Table 11 maps binary `100` to the nominal (1 + Rb/Ra) ratio 5.0. This matches the pinned UV-K1 lineage setting described as 5.0; there is no code correction required. The value is specific to the ST7565-0A table and should not be generalized to the ST7565-0B external-VRS column.

Source: Sitronix ST7565 datasheet, Table 11: https://www.orientdisplay.com/pdf/ST7565.pdf. This is a register-encoding cross-check, not a measurement of the LCD on the physical radio.


## Independent public hardware corroboration (2026-11-15)

The candidate platform has now been cross-checked against sources independent of the D10S source tree:

1. **F4HWN UV-K1 teardown** — https://www.youtube.com/watch?v=e1Crl61yyE4 (published 2025-10-15). The teardown description identifies PY32F071, PY25Q16HB (2 MiB), BK4829 and BK1080. This corroborates the major component family, but the video description alone does not establish every PCB net or pin.
2. **UV-K1 teardown/photo thread** — https://www.hellocq.net/forum/read.php?tid=379537. The post includes front/back main-PCB photos and close-ups, and identifies PY32F071C1BT6 in LQFP48, BK4829 in QFN32, BK1080 and PY25Q16HB. It states that UV-K1 and UV-K5 V3 share the MCU family. These are third-party teardown observations, not a manufacturer schematic.
3. **UV-K1 technical overview** — https://f1lvt.com/files/351f-UV-K5__UV-K1.306.pdf. It describes the PY32F071 and illustrates the opened radio. Useful for model/component corroboration, not a netlist.
4. **FCC filing** — https://fccid.io/XBPUVK1. The filing page lists internal-photo exhibits for FCC ID XBPUV-K1. The filing is a useful primary regulatory source for the submitted product, but the page summary does not by itself identify the MCU pin routing.
5. **Independent firmware implementation** — https://github.com/fagci/k1-scaner. Its README identifies PY32F071, BK4829 and ST7565 128x64; its board and driver sources independently publish candidate mappings for BK4829 PF9/PB8/PB9, ST7565 PA5/PA7/PB2/PA6, backlight PF8, and keypad PB12..PB15/PB3..PB6. This corroborates the software lineage mapping, not continuity on every hardware revision.

### Consolidated confidence

| Fact | Current confidence | Limitation |
|---|---|---|
| Standard UV-K1 platform uses PY32F071 family | High | Confirm package marking on the exact unit before release |
| PY32F071C1BT6 LQFP48 is reported on UV-K1 teardown | High, teardown corroborated | Not yet inspected on user's physical unit |
| BK4829 is the RF transceiver on UV-K1 | High | RF path and external switch/LNA routing remain unverified |
| BK1080 is the broadcast-FM receiver | High | Not used by the receive-only Spectrum path |
| PY25Q16HB is 2 MiB external SPI flash | High | D10S does not yet rely on external-flash features |
| ST7565 128x64 is the panel/controller used by the referenced K1 software lineage | High for software lineage | Actual panel marking/revision should still be checked |
| PF9/PB8/PB9 and PA5/PA7/PB2/PA6 are candidate signal pins | Medium-high, two independent firmware sources | No public schematic/netlist or continuity measurement yet |
| Keypad PB12..PB15/PB3..PB6 and backlight PF8 | Medium-high, source corroborated | Front-panel FFC/revision differences remain possible |
| Exact bootloader and application offset on the target unit | Unverified | Read from the actual unit before any flash |

### Model and bootloader distinction — do not collapse variants

The public 4PDA hardware discussion at https://4pda.to/forum/index.php?showtopic=1071343&st=16320 distinguishes UV-K1 / UV-K5 V3 using PY32F071 and bootloader family 7.xx from older V1/V2 hardware. The Quansheng Toolkit firmware catalogue at https://github.com/knoinok/quansheng_toolkit/blob/main/firmwares/README.md lists stock UV-K1 7.03.01 separately from UV-K5 V3 7.00.11 and explicitly identifies the target MCU/target family. UV Studio's current manual https://armel.github.io/manual/pages/UV-Studio.html also treats UV-K1 and UV-K5 V3 as separate model selections and documents the DFU entry sequence (power off, hold PTT, power on).

The earlier D10S working assumption of bootloader 7.03.01 is consistent with the published standard UV-K1 catalogue entry, but remains a user-reported value until read from the actual radio. Do not infer that UV-K1(8) / Mini Kong, UV-K5 V3 and standard UV-K1 are interchangeable merely because they share PY32F071.

### Legal/provenance boundary for this research

These sources were consulted for factual hardware observations and cross-checking only. No code, images, schematic artwork, or documentation text has been imported from the teardown/forum/Fagci sources into D10S. Fagci's README states Apache 2.0, but the repository's root LICENSE file was not found during this check; therefore its code remains excluded from reuse pending verification of the exact license and notices. Public photos and forum posts are not automatically licensed for republication.


## FCC internal-photo exhibit metadata (2026-11-15)

The FCC ID XBPUV-K1 filing's internal-photo exhibit is indexed as document ID `8384105` (description `Iiternal photos`), application `M0Um3S1ptYL6g71b5OoUEg==`, submitted and made available 2025-06-13. The indexed exhibit metadata reports a 10-page PDF, 3,287,221 bytes, SHA-256 `9bba2e94514b232fb3f2d39c6d4dbb6ceb3e2b393898d3ad234fb592b411a587`, and no short- or long-term confidentiality. Source landing page: https://fccid.io/XBPUV-K1/Internal-Photos/Iiternal-photos-8384105 ; FCC filing index: https://fccid.io/XBPUVK1.

**Evidence limit:** the exhibit's existence and metadata are confirmed, but the PDF image pages have not yet been visually inspected in this review. Therefore this filing currently adds provenance for official internal photographs only; it does not upgrade any MCU marking, PCB revision, pin assignment, RF route, or connector claim. The 2025 FCC submission is for model UV-K1, but must not automatically be treated as a photograph of every later V3 / UV-K1(8) production revision. No exhibit image is copied into D10S.


### HSI divider versus PLL source — clock-tree clarification (2026-11-15)

The official PY32F07X Reference Manual V0.2, Figure 8-1 (section 8.2), draws the HSI RC output as a branch to the PLL and a separate branch through HSIDIV to HSISYS. Therefore RCC_CR.HSIDIV must be applied when HSI is the direct system-clock source, but must **not** be applied to the HSI reference frequency when PLLSRC selects HSI. The decoder's existing PLL path is correct to use the selected HSI frequency without dividing it. Regression tests now set HSIDIV to its maximum divide-by-128 value and confirm that HSI→PLL x2 and x3 still calculate from the undivided 24 MHz and 16 MHz HSI selections, respectively.

Reference: Puya, *PY32F07X Reference Manual*, V0.2, Figure 8-1 and sections 8.1.5, 8.2, 8.5.1–8.5.4: https://www.puyasemi.com/download_path/%E7%94%A8%E6%88%B7%E6%89%8B%E5%86%8C/MCU%20%E5%BE%AE%E5%A4%84%E7%90%86%E5%99%A8/PY32F07X-E_Reference_Manual_V0.2.pdf

This confirms the register-decoding model only; it does not prove the live clock configuration on a particular radio.
