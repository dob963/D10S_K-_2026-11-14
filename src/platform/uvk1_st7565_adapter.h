#ifndef D10S_UVK1_ST7565_ADAPTER_H
#define D10S_UVK1_ST7565_ADAPTER_H

#include "mono128x64.h"

/*
 * Target-only adapter for the reviewed UV-K1 V3 platform driver.
 * Call BOARD_Init() first; it configures the board GPIO and initializes
 * the ST7565 controller. This adapter only transfers the eight framebuffer
 * pages and deliberately does not duplicate SPI pins or controller setup.
 */
void d10s_uvk1_st7565_flush(const d10s_mono128x64_t *display);

#endif
