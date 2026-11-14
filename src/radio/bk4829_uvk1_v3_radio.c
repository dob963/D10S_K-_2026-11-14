#include "bk4829_uvk1_v3_radio.h"

#include <stddef.h>

bool d10s_bk4829_uvk1_v3_radio_init(
    d10s_bk4829_uvk1_v3_radio_t *radio,
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    uint32_t initial_frequency_hz,
    d10s_spectrum_radio_ops_t *radio_ops)
{
    if (radio == NULL || bus == NULL || timing == NULL || radio_ops == NULL)
        return false;

    radio->initialized = false;
    radio->profile.bus_valid = false;
    radio->profile.rx_mode_configured = false;

    if (!d10s_bk4829_scanner_adapter_init(
            &radio->scanner_adapter, bus, timing,
            d10s_bk4829_uvk1_v3_initialize_rx,
            d10s_bk4829_uvk1_v3_resume_rx,
            d10s_bk4829_uvk1_v3_stop_rx,
            initial_frequency_hz, &radio->profile)) {
        d10s_bk4829_scanner_adapter_bind(&radio->scanner_adapter, radio_ops);
        return false;
    }

    d10s_bk4829_scanner_adapter_bind(&radio->scanner_adapter, radio_ops);
    radio->initialized = true;
    return true;
}

void d10s_bk4829_uvk1_v3_radio_stop(d10s_bk4829_uvk1_v3_radio_t *radio)
{
    if (radio == NULL || !radio->initialized) return;
    d10s_bk4829_scanner_adapter_stop(&radio->scanner_adapter);
    radio->initialized = false;
}
