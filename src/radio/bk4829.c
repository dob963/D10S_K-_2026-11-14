#include "bk4829.h"

bool d10s_bk4829_frequency_to_word(uint32_t frequency_hz,
                                   uint32_t *frequency_word)
{
    if (frequency_word == 0 || (frequency_hz % 10u) != 0u) return false;
    *frequency_word = frequency_hz / 10u;
    return true;
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
