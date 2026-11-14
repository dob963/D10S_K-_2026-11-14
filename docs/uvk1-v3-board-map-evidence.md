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
