#include "uvk1_st7565_adapter.h"

#include "driver/st7565.h"

void d10s_uvk1_st7565_flush(const d10s_mono128x64_t *display)
{
    uint8_t page;
    if (display == 0) return;

    for (page = 0u; page < D10S_MONO128X64_PAGE_COUNT; ++page) {
        const uint8_t *page_data = d10s_mono128x64_page(display, page);
        if (page_data != 0)
            ST7565_DrawLine(0u, page, page_data, D10S_MONO128X64_PAGE_BYTES);
    }
}
