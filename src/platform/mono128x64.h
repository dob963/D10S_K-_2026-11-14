#ifndef D10S_MONO128X64_H
#define D10S_MONO128X64_H

#include "spectrum_view.h"
#include "st7565.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_MONO128X64_PAGE_COUNT 8u
#define D10S_MONO128X64_PAGE_BYTES 128u
#define D10S_MONO128X64_BUFFER_BYTES 1024u

typedef struct {
    uint8_t bytes[D10S_MONO128X64_BUFFER_BYTES];
} d10s_mono128x64_t;

void d10s_mono128x64_clear(d10s_mono128x64_t *display, bool on);
void d10s_mono128x64_pixel(void *context, uint8_t x, uint8_t y, bool on);
void d10s_mono128x64_text(void *context, uint8_t x, uint8_t y,
                          const char *value, uint8_t style);
void d10s_mono128x64_bind(d10s_mono128x64_t *display,
                          d10s_spectrum_canvas_t *canvas);
const uint8_t *d10s_mono128x64_page(const d10s_mono128x64_t *display,
                                    uint8_t page);
bool d10s_mono128x64_flush(const d10s_mono128x64_t *display,
                           d10s_st7565_t *controller);

#endif
