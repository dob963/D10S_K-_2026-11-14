#include "mono128x64.h"

#include <stddef.h>

static const uint8_t *glyph_for(char character)
{
    static const uint8_t digits[10][7] = {
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}
    };
    static const uint8_t letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
    };
    static const uint8_t dot[7] = {0,0,0,0,0,12,12};
    static const uint8_t slash[7] = {1,2,2,4,8,8,16};
    static const uint8_t dash[7] = {0,0,0,31,0,0,0};
    static const uint8_t colon[7] = {0,12,12,0,12,12,0};
    static const uint8_t blank[7] = {0,0,0,0,0,0,0};

    if (character >= '0' && character <= '9')
        return digits[(unsigned)(character - '0')];
    if (character >= 'A' && character <= 'Z')
        return letters[(unsigned)(character - 'A')];
    if (character >= 'a' && character <= 'z')
        return letters[(unsigned)(character - 'a')];
    if (character == '.') return dot;
    if (character == '/') return slash;
    if (character == '-') return dash;
    if (character == ':') return colon;
    return blank;
}

void d10s_mono128x64_clear(d10s_mono128x64_t *display, bool on)
{
    uint16_t i;
    if (display == 0) return;
    for (i = 0u; i < D10S_MONO128X64_BUFFER_BYTES; ++i)
        display->bytes[i] = on ? 0xFFu : 0u;
}

void d10s_mono128x64_pixel(void *context, uint8_t x, uint8_t y, bool on)
{
    d10s_mono128x64_t *display = (d10s_mono128x64_t *)context;
    uint16_t offset;
    uint8_t mask;
    if (display == 0 || x >= 128u || y >= 64u) return;
    offset = (uint16_t)((uint16_t)(y >> 3) * D10S_MONO128X64_PAGE_BYTES + x);
    mask = (uint8_t)(1u << (y & 7u));
    if (on) display->bytes[offset] |= mask;
    else display->bytes[offset] &= (uint8_t)~mask;
}

void d10s_mono128x64_text(void *context, uint8_t x, uint8_t y,
                          const char *value, uint8_t style)
{
    uint16_t cursor_x = x;
    size_t index;
    bool bold = (style & D10S_VIEW_TEXT_BOLD) != 0u;
    if (context == 0 || value == 0) return;

    for (index = 0u; value[index] != '\0' && cursor_x < 128u; ++index) {
        const uint8_t *glyph = glyph_for(value[index]);
        uint8_t row;
        for (row = 0u; row < 7u; ++row) {
            uint8_t column;
            for (column = 0u; column < 5u; ++column) {
                if ((glyph[row] & (uint8_t)(1u << (4u - column))) != 0u) {
                    d10s_mono128x64_pixel(context, (uint8_t)(cursor_x + column),
                                          (uint8_t)(y + row), true);
                    if (bold)
                        d10s_mono128x64_pixel(context,
                                              (uint8_t)(cursor_x + column + 1u),
                                              (uint8_t)(y + row), true);
                }
            }
        }
        cursor_x = (uint16_t)(cursor_x + (bold ? 7u : 6u));
    }
}

void d10s_mono128x64_bind(d10s_mono128x64_t *display,
                          d10s_spectrum_canvas_t *canvas)
{
    if (display == 0 || canvas == 0) return;
    canvas->context = display;
    canvas->pixel = d10s_mono128x64_pixel;
    canvas->text = d10s_mono128x64_text;
}

const uint8_t *d10s_mono128x64_page(const d10s_mono128x64_t *display,
                                    uint8_t page)
{
    if (display == 0 || page >= D10S_MONO128X64_PAGE_COUNT) return 0;
    return &display->bytes[(uint16_t)page * D10S_MONO128X64_PAGE_BYTES];
}

bool d10s_mono128x64_flush(const d10s_mono128x64_t *display,
                           d10s_st7565_t *controller)
{
    if (display == 0 || controller == 0) return false;
    return d10s_st7565_flush_frame(
        controller,
        (const uint8_t (*)[D10S_MONO128X64_PAGE_BYTES])display->bytes);
}
