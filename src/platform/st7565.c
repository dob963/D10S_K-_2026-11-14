#include "st7565.h"

#include <stddef.h>

static bool command(d10s_st7565_t *d, uint8_t value)
{
    return d->port.data_mode(false, d->port.context) &&
           d->port.write_byte(value, d->port.context);
}

static bool data(d10s_st7565_t *d, uint8_t value)
{
    return d->port.data_mode(true, d->port.context) &&
           d->port.write_byte(value, d->port.context);
}

bool d10s_st7565_init(d10s_st7565_t *d,
                      const d10s_st7565_port_t *p,
                      const d10s_st7565_config_t *c)
{
    bool ok = true;
    if (d == NULL || p == NULL || c == NULL ||
        p->chip_select == NULL || p->data_mode == NULL ||
        p->write_byte == NULL || p->delay_ms == NULL ||
        c->column_offset > 15u || c->contrast > 63u ||
        c->resistor_ratio > 7u || c->power_control > 7u ||
        c->reset_delay_ms == 0u || c->power_delay_ms == 0u)
        return false;

    d->initialized = false;
    d->port = *p;
    d->config = *c;
    if (!d->port.chip_select(true, d->port.context)) return false;

    ok = command(d, 0xAEu) && ok; /* display off */
    ok = d->port.chip_select(true, d->port.context) && ok;
    if (!ok) return false;
    if (!d->port.chip_select(false, d->port.context)) return false;

    ok = command(d, 0xE2u) && ok; /* software reset */
    if (!d->port.chip_select(true, d->port.context) || !ok) return false;
    if (!d->port.delay_ms(c->reset_delay_ms, d->port.context)) return false;
    if (!d->port.chip_select(false, d->port.context)) return false;

    ok = command(d, 0xA2u) && ok; /* 1/9 bias */
    ok = command(d, (uint8_t)(0xA0u | (c->reverse_segment ? 1u : 0u))) && ok;
    ok = command(d, (uint8_t)(0xC0u | (c->reverse_common ? 8u : 0u))) && ok;
    ok = command(d, (uint8_t)(0xA6u | (c->inverse ? 1u : 0u))) && ok;
    ok = command(d, 0xA4u) && ok; /* normal display */
    ok = command(d, (uint8_t)(0x20u | c->resistor_ratio)) && ok;
    ok = command(d, 0x81u) && ok;
    ok = command(d, c->contrast) && ok;
    ok = command(d, (uint8_t)(0x28u | c->power_control)) && ok;
    if (!d->port.chip_select(true, d->port.context) || !ok) return false;
    if (!d->port.delay_ms(c->power_delay_ms, d->port.context)) return false;
    if (!d->port.chip_select(false, d->port.context)) return false;
    ok = command(d, 0x40u) && ok; /* start line 0 */
    ok = command(d, 0xAFu) && ok; /* display on */
    if (!d->port.chip_select(true, d->port.context) || !ok) return false;

    d->initialized = true;
    return true;
}

bool d10s_st7565_flush_page(d10s_st7565_t *d, uint8_t page,
                            const uint8_t pixels[D10S_ST7565_WIDTH])
{
    uint16_t i;
    bool ok = true;
    if (d == NULL || !d->initialized || pixels == NULL ||
        page >= D10S_ST7565_PAGES) return false;
    if (!d->port.chip_select(false, d->port.context)) return false;
    ok = command(d, (uint8_t)(0xB0u | page)) && ok;
    ok = command(d, (uint8_t)(0x10u | ((d->config.column_offset >> 4) & 0x0Fu))) && ok;
    ok = command(d, (uint8_t)(d->config.column_offset & 0x0Fu)) && ok;
    if (!d->port.data_mode(true, d->port.context)) ok = false;
    for (i = 0u; i < D10S_ST7565_WIDTH && ok; ++i)
        ok = d->port.write_byte(pixels[i], d->port.context);
    if (!d->port.chip_select(true, d->port.context)) ok = false;
    return ok;
}

bool d10s_st7565_flush_frame(d10s_st7565_t *d,
                             const uint8_t framebuffer[D10S_ST7565_PAGES]
                                                      [D10S_ST7565_WIDTH])
{
    uint8_t page;
    if (d == NULL || framebuffer == NULL || !d->initialized) return false;
    for (page = 0u; page < D10S_ST7565_PAGES; ++page)
        if (!d10s_st7565_flush_page(d, page, framebuffer[page])) return false;
    return true;
}
