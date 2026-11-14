#ifndef D10S_BK4829_H
#define D10S_BK4829_H

#include "bk4829_bus.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_BK4829_RSSI_REGISTER 0x67u
#define D10S_BK4829_RSSI_MASK 0x01FFu

/* Datasheet frequency word: 10 Hz units. Reject non-exact 10 Hz values. */
bool d10s_bk4829_frequency_to_word(uint32_t frequency_hz,
                                   uint32_t *frequency_word);
/* Raw 9-bit RSSI only. This function never reports dBm. */
bool d10s_bk4829_read_raw_rssi(const d10s_bk4829_bus_port_t *port,
                               const d10s_bk4829_bus_timing_t *timing,
                               uint16_t *raw_rssi);

#endif
