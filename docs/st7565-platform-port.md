# ST7565 display backend

## Implemented layers

1. `src/ui/spectrum_view.c` draws the Spectrum graph through pixel/text callbacks.
2. `src/platform/mono128x64.c` provides a 1024-byte, page-oriented monochrome framebuffer and a compact 5x7 font. Bold text is produced by a one-pixel horizontal expansion; it is intentionally a modest weight change.
3. `src/platform/st7565.c` provides controller initialization, page/column addressing and 128-byte page/frame transfer.
4. `d10s_st7565_port_t` is the board boundary. `src/platform/py32f071_uvk1_v3.c` now implements candidate UV-K1 V3 CS, A0/data-mode and SPI1 byte callbacks. The millisecond delay callback remains externally supplied and must be calibrated.

The framebuffer layout is eight contiguous 128-byte pages. Pixel (x,y) is stored at `bytes[(y >> 3) * 128 + x]`, bit `(y & 7)`. The flush operation transfers pages 0 through 7 in order. The panel column offset is configurable. `d10s_st7565_uvk1_v3_default_config()` supplies the source-derived K1 candidate values (column offset 4, contrast 31, resistor ratio 4, reverse SEG, normal COM, normal display); these are not final calibration until checked on the actual LCD.

## Controller configuration

`d10s_st7565_config_t` carries the panel-specific values: column offset, contrast, resistor ratio, power-control bits, segment/common direction, inverse mode and reset/power delays. The K1 helper supplies values observed in the pinned reference source, but they remain candidate settings until verified on the user's panel. The test values validate software sequencing, not optical contrast or final electrical calibration.

## Source reference

The reviewed UV-K1 V3 upstream source is `armel/uv-k1-k5v3-firmware-custom` at commit `3261ca7883d5b0b60c0905d8766612dee7b4191d`. It exposes `ST7565_DrawLine(Column, Line, bitmap, size)` and initializes the controller through its board layer. Its SPI/GPIO mapping belongs to that target source and must not be assumed for another PCB revision without continuity checks. This upstream source declares Apache-2.0; any code later reused from it must retain the required notices and attribution. D10S has not copied the upstream ST7565 implementation into this generic backend.

## Remaining target work

- Confirm the candidate PA5/PA7/PB2/PA6 mapping by continuity on the exact UV-K1 PCB revision.
- Supply and calibrate the external delay callback used for reset/power waits.
- Confirm CS polarity, A0 polarity, reset line handling and SPI mode/clock.
- Confirm whether the panel needs a hardware reset pulse before software reset.
- Verify segment/common orientation, column offset, contrast and bias on the physical LCD.
- Measure full-frame refresh time and ensure it does not starve the scanner.
- Validate startup/exit display behavior on the radio.

Host tests confirm byte ordering and framebuffer mapping only. They do not validate panel optics, electrical timing or the user's actual hardware.
