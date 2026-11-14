#include "bk4829_bus.h"

static bool valid(const d10s_bk4829_bus_port_t *p,
                  const d10s_bk4829_bus_timing_t *t)
{
    return p != 0 && t != 0 && t->half_period_ticks != 0u &&
        t->turnaround_ticks != 0u && p->set_csn_high != 0 &&
        p->set_clock_high != 0 && p->set_data_output != 0 &&
        p->write_data_high != 0 && p->read_data_high != 0 &&
        p->delay_ticks != 0;
}

static bool idle(const d10s_bk4829_bus_port_t *p)
{
    bool ok = p->set_csn_high(true, p->context);
    ok = p->set_clock_high(true, p->context) && ok;
    ok = p->set_data_output(true, p->context) && ok;
    ok = p->write_data_high(true, p->context) && ok;
    return ok;
}

static void recover(const d10s_bk4829_bus_port_t *p)
{
    (void)p->set_csn_high(true, p->context);
    (void)p->set_clock_high(true, p->context);
    (void)p->set_data_output(true, p->context);
    (void)p->write_data_high(true, p->context);
}

static bool begin(const d10s_bk4829_bus_port_t *p,
                  const d10s_bk4829_bus_timing_t *t)
{
    if (!idle(p) || !p->set_clock_high(false, p->context) ||
        !p->set_csn_high(false, p->context) ||
        !p->delay_ticks(t->half_period_ticks, p->context)) {
        recover(p);
        return false;
    }
    return true;
}

static bool write_bit(const d10s_bk4829_bus_port_t *p,
                      const d10s_bk4829_bus_timing_t *t, bool bit)
{
    if (!p->write_data_high(bit, p->context) ||
        !p->delay_ticks(t->half_period_ticks, p->context) ||
        !p->set_clock_high(true, p->context) ||
        !p->delay_ticks(t->half_period_ticks, p->context) ||
        !p->set_clock_high(false, p->context) ||
        !p->delay_ticks(t->half_period_ticks, p->context)) return false;
    return true;
}

static bool write_bits(const d10s_bk4829_bus_port_t *p,
                       const d10s_bk4829_bus_timing_t *t,
                       uint16_t value, uint8_t count)
{
    uint8_t i;
    for (i = 0u; i < count; ++i) {
        uint8_t shift = (uint8_t)(count - 1u - i);
        if (!write_bit(p, t, ((value >> shift) & 1u) != 0u)) return false;
    }
    return true;
}

static bool end(const d10s_bk4829_bus_port_t *p,
                const d10s_bk4829_bus_timing_t *t)
{
    if (!p->set_csn_high(true, p->context) ||
        !p->delay_ticks(t->half_period_ticks, p->context) ||
        !p->set_clock_high(true, p->context) ||
        !p->set_data_output(true, p->context) ||
        !p->write_data_high(true, p->context)) {
        recover(p);
        return false;
    }
    return true;
}

bool d10s_bk4829_write_register(const d10s_bk4829_bus_port_t *p,
                                const d10s_bk4829_bus_timing_t *t,
                                uint8_t address, uint16_t value)
{
    if (!valid(p, t) || address > 0x7Fu || !begin(p, t)) return false;
    if (!write_bits(p, t, address, 8u) ||
        !write_bits(p, t, value, 16u)) {
        recover(p);
        return false;
    }
    return end(p, t);
}

bool d10s_bk4829_read_register(const d10s_bk4829_bus_port_t *p,
                               const d10s_bk4829_bus_timing_t *t,
                               uint8_t address, uint16_t *value)
{
    uint16_t result = 0u;
    uint8_t i;
    if (!valid(p, t) || address > 0x7Fu || value == 0 ||
        !begin(p, t)) return false;
    if (!write_bits(p, t, (uint16_t)(address | 0x80u), 8u) ||
        !p->set_data_output(false, p->context) ||
        !p->delay_ticks(t->turnaround_ticks, p->context)) {
        recover(p);
        return false;
    }
    for (i = 0u; i < 16u; ++i) {
        bool bit = false;
        if (!p->read_data_high(&bit, p->context) ||
            !p->delay_ticks(t->half_period_ticks, p->context) ||
            !p->set_clock_high(true, p->context) ||
            !p->delay_ticks(t->half_period_ticks, p->context) ||
            !p->set_clock_high(false, p->context) ||
            !p->delay_ticks(t->half_period_ticks, p->context)) {
            recover(p);
            return false;
        }
        result = (uint16_t)((result << 1) | (bit ? 1u : 0u));
    }
    if (!end(p, t)) return false;
    *value = result;
    return true;
}
