#ifndef D10S_BK4829_UVK1_V3_RADIO_H
#define D10S_BK4829_UVK1_V3_RADIO_H

#include "bk4829_uvk1_v3_profile.h"

typedef struct {
    d10s_bk4829_uvk1_v3_profile_t profile;
    d10s_bk4829_scanner_adapter_t scanner_adapter;
    bool initialized;
} d10s_bk4829_uvk1_v3_radio_t;

/*
 * Builds the exact radio-ops object expected by d10s_spectrum_app_init().
 * Bus callbacks must come from a separately verified board GPIO port.
 */
bool d10s_bk4829_uvk1_v3_radio_init(
    d10s_bk4829_uvk1_v3_radio_t *radio,
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    uint32_t initial_frequency_hz,
    d10s_spectrum_radio_ops_t *radio_ops);
void d10s_bk4829_uvk1_v3_radio_stop(d10s_bk4829_uvk1_v3_radio_t *radio);

#endif
