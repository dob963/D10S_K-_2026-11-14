#ifndef D10S_BK4829_SCANNER_ADAPTER_H
#define D10S_BK4829_SCANNER_ADAPTER_H

#include "bk4829.h"
#include "spectrum_scanner.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*d10s_bk4829_rx_init_fn)(
    const d10s_bk4829_bus_port_t *port,
    const d10s_bk4829_bus_timing_t *timing,
    uint32_t initial_frequency_hz,
    void *context);
typedef bool (*d10s_bk4829_rx_resume_fn)(uint32_t frequency_hz,
                                         void *context);
typedef bool (*d10s_bk4829_rx_stop_fn)(void *context);

typedef struct {
    d10s_bk4829_bus_port_t bus;
    d10s_bk4829_bus_timing_t timing;
    d10s_bk4829_rx_resume_fn resume_rx;
    d10s_bk4829_rx_stop_fn stop_rx;
    void *context;
    bool ready;
    bool discard_first_rssi;
} d10s_bk4829_scanner_adapter_t;

/*
 * Initialization, RX-mode transitions and fail-safe standby are supplied by
 * a board-specific verified profile. This adapter owns synthesizer writes
 * and RSSI reads; it does not invent BK4829 startup or RF-switch register data.
 */
bool d10s_bk4829_scanner_adapter_init(
    d10s_bk4829_scanner_adapter_t *adapter,
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    d10s_bk4829_rx_init_fn initialize_rx,
    d10s_bk4829_rx_resume_fn resume_rx,
    d10s_bk4829_rx_stop_fn stop_rx,
    uint32_t initial_frequency_hz,
    void *context);
void d10s_bk4829_scanner_adapter_stop(
    d10s_bk4829_scanner_adapter_t *adapter);
void d10s_bk4829_scanner_adapter_bind(
    d10s_bk4829_scanner_adapter_t *adapter,
    d10s_spectrum_radio_ops_t *radio_ops);

#endif
