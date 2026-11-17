/*
 * The six REG_13 gain presets follow the documented implementation in
 * armel/uv-k1-k5v3-firmware-custom, App/app/foxhunt.c, pinned commit
 * 3261ca7883d5b0b60c0905d8766612dee7b4191d (Apache-2.0).
 * Adapted to the D10S register bus; see THIRD_PARTY_NOTICES.md.
 */
#include "bk4829.h"

bool d10s_bk4829_frequency_supported(uint32_t frequency_hz)
{
    return (frequency_hz >= 18000000u && frequency_hz <= 630000000u) ||
           (frequency_hz >= 840000000u && frequency_hz <= 1300000000u);
}

bool d10s_bk4829_frequency_to_word(uint32_t frequency_hz,
                                   uint32_t *frequency_word)
{
    if (frequency_word == 0 || (frequency_hz % 10u) != 0u ||
        !d10s_bk4829_frequency_supported(frequency_hz)) return false;
    *frequency_word = frequency_hz / 10u;
    return true;
}

bool d10s_bk4829_write_frequency(const d10s_bk4829_bus_port_t *port,
                                 const d10s_bk4829_bus_timing_t *timing,
                                 uint32_t frequency_hz)
{
    uint32_t word;
    if (!d10s_bk4829_frequency_to_word(frequency_hz, &word)) return false;
    if (!d10s_bk4829_write_register(port, timing, D10S_BK4829_REG_FREQ_LOW,
                                    (uint16_t)(word & 0xFFFFu))) return false;
    return d10s_bk4829_write_register(port, timing, D10S_BK4829_REG_FREQ_HIGH,
                                      (uint16_t)(word >> 16));
}

bool d10s_bk4829_set_rx_gain_level(
    const d10s_bk4829_bus_port_t *port,
    const d10s_bk4829_bus_timing_t *timing,
    uint8_t level)
{
    static const uint16_t gain_presets[6] = {
        0x03DFu, /* Level 0: reference receive gain preset */
        0x03DDu, /* Level 1: relative gain preset; not dB-calibrated */
        0x03DBu, /* Level 2: relative gain preset; not dB-calibrated */
        0x03D9u, /* Level 3: relative gain preset; not dB-calibrated */
        0x0379u, /* Level 4: BYP-style reduced-LNA preset */
        0x0139u  /* Level 5: BYP+-style reduced-LNA preset */
    };
    uint16_t current;
    uint16_t updated;
    if (level >= (uint8_t)(sizeof(gain_presets) / sizeof(gain_presets[0])) ||
        !d10s_bk4829_read_register(port, timing, 0x13u, &current))
        return false;
    /* Change only documented gain fields; preserve REG_13 bits 10..15. */
    updated = (uint16_t)((current & (uint16_t)~0x03FFu) |
                         gain_presets[level]);
    return d10s_bk4829_write_register(port, timing, 0x13u, updated);
}

bool d10s_bk4829_read_raw_rssi(const d10s_bk4829_bus_port_t *port,
                               const d10s_bk4829_bus_timing_t *timing,
                               uint16_t *raw_rssi)
{
    uint16_t value;
    if (raw_rssi == 0 ||
        !d10s_bk4829_read_register(port, timing,
                                   D10S_BK4829_RSSI_REGISTER, &value))
        return false;
    *raw_rssi = (uint16_t)(value & D10S_BK4829_RSSI_MASK);
    return true;
}

/*
 * UV-K1/V3 F4HWN source identifies REG_73 bit 4 as AFC disable.
 * Preserve all other register bits. This controls AFC enable/disable only;
 * AFCR width/range (0..7) is not mapped by the available source evidence.
 * TODO: registro AFC/AFCR da confermare. Indizio 4PDA: AFCR 0-7;
 * non associare questi valori a un registro senza una fonte tecnica.
 */
bool d10s_bk4829_set_afc_enabled(const d10s_bk4829_bus_port_t *port,
                                  const d10s_bk4829_bus_timing_t *timing,
                                  bool enabled)
{
    uint16_t value;
    if (!d10s_bk4829_read_register(port, timing, 0x73u, &value))
        return false;
    if (enabled)
        value = (uint16_t)(value & (uint16_t)~0x0010u);
    else
        value = (uint16_t)(value | 0x0010u);
    return d10s_bk4829_write_register(port, timing, 0x73u, value);
}
