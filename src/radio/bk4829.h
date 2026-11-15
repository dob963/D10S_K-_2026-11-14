#ifndef D10S_BK4829_H
#define D10S_BK4829_H

#include "bk4829_bus.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_BK4829_RSSI_REGISTER 0x67u
#define D10S_BK4829_RSSI_MASK 0x01FFu
#define D10S_BK4829_REG_FREQ_LOW 0x38u
#define D10S_BK4829_REG_FREQ_HIGH 0x39u

/* Datasheet RF operating windows; gaps and out-of-range values are rejected. */
bool d10s_bk4829_frequency_supported(uint32_t frequency_hz);

/* Datasheet frequency word: 10 Hz units. Reject unsupported/non-exact values. */
bool d10s_bk4829_frequency_to_word(uint32_t frequency_hz,
                                   uint32_t *frequency_word);
/* Writes REG_38 then REG_39; caller controls RX mode transitions. */
bool d10s_bk4829_write_frequency(const d10s_bk4829_bus_port_t *port,
                                 const d10s_bk4829_bus_timing_t *timing,
                                 uint32_t frequency_hz);

/* Set one of six BK4829 receive gain steps; levels 0..5 are relative only. */
bool d10s_bk4829_set_rx_gain_level(const d10s_bk4829_bus_port_t *port,
                                   const d10s_bk4829_bus_timing_t *timing,
                                   uint8_t level);

/* Raw 9-bit RSSI only. This function never reports dBm. */
bool d10s_bk4829_read_raw_rssi(const d10s_bk4829_bus_port_t *port,
                               const d10s_bk4829_bus_timing_t *timing,
                               uint16_t *raw_rssi);

#endif
