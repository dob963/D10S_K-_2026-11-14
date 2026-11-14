#include "bk4829_scanner_adapter.h"

#include <stddef.h>

static void fail_closed(d10s_bk4829_scanner_adapter_t *a)
{
    bool was_ready;
    if (a == NULL) return;
    was_ready = a->ready;
    a->ready = false;
    a->discard_first_rssi = false;
    if (was_ready && a->stop_rx != NULL) (void)a->stop_rx(a->context);
}

static bool tune_hz(void *context, uint32_t requested_hz,
                    uint32_t *actual_hz)
{
    d10s_bk4829_scanner_adapter_t *a =
        (d10s_bk4829_scanner_adapter_t *)context;
    if (a == NULL || !a->ready || actual_hz == NULL ||
        !d10s_bk4829_frequency_supported(requested_hz) ||
        (requested_hz % 10u) != 0u) return false;

    if (!d10s_bk4829_write_frequency(&a->bus, &a->timing, requested_hz) ||
        !a->resume_rx(requested_hz, a->context)) {
        fail_closed(a);
        return false;
    }
    *actual_hz = requested_hz;
    a->discard_first_rssi = true;
    return true;
}

static bool read_rssi(void *context, uint16_t *rssi)
{
    d10s_bk4829_scanner_adapter_t *a =
        (d10s_bk4829_scanner_adapter_t *)context;
    uint16_t discarded;
    if (a == NULL || !a->ready || rssi == NULL) return false;
    if (a->discard_first_rssi) {
        if (!d10s_bk4829_read_raw_rssi(&a->bus, &a->timing, &discarded)) {
            fail_closed(a);
            return false;
        }
        a->discard_first_rssi = false;
    }
    if (!d10s_bk4829_read_raw_rssi(&a->bus, &a->timing, rssi)) {
        fail_closed(a);
        return false;
    }
    return true;
}

bool d10s_bk4829_scanner_adapter_init(
    d10s_bk4829_scanner_adapter_t *a,
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    d10s_bk4829_rx_init_fn initialize_rx,
    d10s_bk4829_rx_resume_fn resume_rx,
    d10s_bk4829_rx_stop_fn stop_rx,
    uint32_t initial_frequency_hz,
    void *context)
{
    if (a == NULL) return false;
    a->bus = (d10s_bk4829_bus_port_t){0};\n    a->timing = (d10s_bk4829_bus_timing_t){0};\n    a->resume_rx = NULL;\n    a->stop_rx = NULL;\n    a->context = NULL;\n    a->ready = false;\n    a->discard_first_rssi = false;
    if (bus == NULL || timing == NULL || initialize_rx == NULL ||
        resume_rx == NULL || stop_rx == NULL ||
        !d10s_bk4829_frequency_supported(initial_frequency_hz) ||
        (initial_frequency_hz % 10u) != 0u ||
        timing->half_period_ticks == 0u ||
        timing->turnaround_ticks == 0u) return false;

    a->bus = *bus;
    a->timing = *timing;
    a->resume_rx = resume_rx;
    a->stop_rx = stop_rx;
    a->context = context;
    if (!initialize_rx(&a->bus, &a->timing, initial_frequency_hz, context)) {
        (void)stop_rx(context);
        return false;
    }
    a->ready = true;
    return true;
}

void d10s_bk4829_scanner_adapter_stop(
    d10s_bk4829_scanner_adapter_t *a)
{
    if (a == NULL) return;
    fail_closed(a);
}

void d10s_bk4829_scanner_adapter_bind(
    d10s_bk4829_scanner_adapter_t *a,
    d10s_spectrum_radio_ops_t *ops)
{
    if (a == NULL || ops == NULL) return;
    ops->context = a;
    ops->tune_hz = tune_hz;
    ops->read_rssi = read_rssi;
    ops->set_attenuation = NULL;
}
