#include "st7565.h"

#include <stddef.h>

static bool command(d10s_st7565_t *d, uint8_t value)
{
    return d->port.data_mode(false, d->port.context) &&
           d->port.write_byte(value, d->port.context);
}

static bool send_command_frame(d10s_st7565_t *d,
                               const uint8_t *commands, uint8_t count)
{
    uint8_t i;
    bool ok = true;
    if (!d->port.chip_select(true, d->port.context)) return false;
    for (i = 0u; i < count && ok; ++i) ok = command(d, commands[i]);
    if (!d->port.chip_select(false, d->port.context)) ok = false;
    return ok;
}

void d10s_st7565_uvk1_v3_default_config(d10s_st7565_config_t *c)
{
    if (c == NULL) return;
    c->column_offset = 4u;
    c->contrast = 31u;
    c->resistor_ratio = 4u;
    c->power_control = 7u;
    c->reverse_segment = true;
    c->reverse_common = false;
    c->inverse = false;
    c->reset_delay_ms = 120u;
    c->power_delay_ms = 40u;
}

bool d10s_st7565_init(d10s_st7565_t *d,
                      const d10s_st7565_port_t *p,
                      const d10s_st7565_config_t *c)
{
    static const uint8_t blank_page[D10S_ST7565_WIDTH] = {0};
    uint8_t commands[8];
    uint8_t page;
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

    /* K1 display reset is software-only on the reviewed board lineage. */
    if (!d->port.chip_select(false, d->port.context) ||
        !d->port.chip_select(true, d->port.context)) return false;
    if (!command(d, 0xAEu) ||
        !d->port.chip_select(false, d->port.context)) return false;

    if (!d->port.chip_select(true, d->port.context)) return false;
    if (!command(d, 0xE2u) ||
        !d->port.chip_select(false, d->port.context) ||
        !d->port.delay_ms(c->reset_delay_ms, d->port.context)) return false;

    commands[0] = 0xA2u; /* 1/9 bias */
    commands[1] = (uint8_t)(0xC0u | (c->reverse_common ? 0x08u : 0u));
    commands[2] = (uint8_t)(0xA0u | (c->reverse_segment ? 0x01u : 0u));
    commands[3] = (uint8_t)(0xA6u | (c->inverse ? 0x01u : 0u));
    commands[4] = 0xA4u; /* normal display */
    commands[5] = (uint8_t)(0x20u | c->resistor_ratio);
    commands[6] = 0x81u; /* electronic volume */
    commands[7] = c->contrast;
    if (!send_command_frame(d, commands, 8u)) return false;

    /* Follow the board-proven staged booster/regulator/follower startup. */
    commands[0] = 0x2Bu; /* VB=0, VR=1, VF=1 */
    if (!send_command_frame(d, commands, 1u) ||
        !d->port.delay_ms(1u, d->port.context)) return false;
    commands[0] = 0x2Eu; /* VB=1, VR=1, VF=0 */
    if (!send_command_frame(d, commands, 1u) ||
        !d->port.delay_ms(1u, d->port.context)) return false;
    commands[0] = (uint8_t)(0x28u | c->power_control);
    for (page = 0u; page < 4u; ++page)
        if (!send_command_frame(d, commands, 1u)) return false;
    if (!d->port.delay_ms(c->power_delay_ms, d->port.context)) return false;

    commands[0] = 0x40u; /* start line 0 */
    if (!send_command_frame(d, commands, 1u)) return false;

    /* Clear all eight pages while the panel is still disabled. */
    d->initialized = true;
    for (page = 0u; page < D10S_ST7565_PAGES; ++page) {
        if (!d10s_st7565_flush_page(d, page, blank_page)) {
            d->initialized = false;
            return false;
        }
    }

    commands[0] = 0xAFu; /* display on */
    if (!send_command_frame(d, commands, 1u)) {
        d->initialized = false;
        return false;
    }
    return true;
}

bool d10s_st7565_flush_page(d10s_st7565_t *d, uint8_t page,
                            const uint8_t pixels[D10S_ST7565_WIDTH])
{
    uint16_t i;
    bool ok = true;
    if (d == NULL || !d->initialized || pixels == NULL ||
        page >= D10S_ST7565_PAGES) return false;
    if (!d->port.chip_select(true, d->port.context)) return false;
    ok = command(d, (uint8_t)(0xB0u | page));
    ok = command(d, (uint8_t)(0x10u |
                   ((d->config.column_offset >> 4) & 0x0Fu))) && ok;
    ok = command(d, (uint8_t)(d->config.column_offset & 0x0Fu)) && ok;
    if (!d->port.data_mode(true, d->port.context)) ok = false;
    for (i = 0u; i < D10S_ST7565_WIDTH && ok; ++i)
        ok = d->port.write_byte(pixels[i], d->port.context);
    if (!d->port.chip_select(false, d->port.context)) ok = false;
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
