#ifndef D10S_SPECTRUM_SCANNER_H
#define D10S_SPECTRUM_SCANNER_H

#include "spectrum_detect.h"
#include "spectrum_plan.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *context;
    bool (*tune_hz)(void *context, uint32_t requested_hz,
                    uint32_t *actual_hz);
    bool (*read_rssi)(void *context, uint16_t *rssi);
    bool (*set_attenuation)(void *context, uint8_t level);
} d10s_spectrum_radio_ops_t;

typedef enum {
    D10S_SCANNER_IDLE = 0,
    D10S_SCANNER_BUSY,
    D10S_SCANNER_SWEEP_READY,
    D10S_SCANNER_ERROR
} d10s_spectrum_scanner_status_t;

typedef struct {
    d10s_spectrum_plan_t plan;
    d10s_spectrum_state_t detector;
    d10s_spectrum_bin_t bins[D10S_SPECTRUM_MAX_BINS];
    uint32_t settle_started_ms;
    uint32_t current_frequency_hz;
    uint16_t bin_index;
    uint16_t settle_ms;
    uint16_t rssi_compensation;
    uint8_t attenuation_level;
    uint8_t phase;
    bool attenuation_configured;
} d10s_spectrum_scanner_t;

/* Bands are caller-owned and must outlive the scanner instance. */
bool d10s_spectrum_scanner_init(d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_band_t *bands,
                                uint8_t band_count,
                                const d10s_spectrum_config_t *config,
                                uint16_t settle_ms);
void d10s_spectrum_scanner_stop(d10s_spectrum_scanner_t *scanner);
/* Offset is a measured correction in raw RSSI units, never dB. */
bool d10s_spectrum_scanner_set_attenuation(
                                d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_radio_ops_t *radio,
                                uint8_t level,
                                uint16_t measured_raw_rssi_offset);
d10s_spectrum_scanner_status_t d10s_spectrum_scanner_tick(
                                d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_radio_ops_t *radio,
                                uint32_t now_ms,
                                d10s_spectrum_result_t *result,
                                bool *band_changed);

#endif
