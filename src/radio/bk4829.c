#include "bk4829.h"

bool d10s_bk4829_frequency_supported(uint32_t frequency_hz)
{
    return (frequency_hz >= 18000000u && frequency_hz <= 580000000u) ||
           (frequency_hz >= 760000000u && frequency_hz <= 1160000000u);
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
