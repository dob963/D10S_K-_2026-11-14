#ifndef D10S_SPECTRUM_DETECT_H
#define D10S_SPECTRUM_DETECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define D10S_SPECTRUM_MAX_BINS 256u
#define D10S_SPECTRUM_MAX_RSSI 511u
#define D10S_NOISE_HISTORY_SWEEPS 5u

typedef struct {
    uint32_t frequency_hz;
    uint16_t rssi;
} d10s_spectrum_bin_t;

typedef struct {
    uint8_t noise_percentile;
    uint8_t noise_history_percentile;
    uint16_t noise_margin;
    uint16_t min_peak_prominence;
    uint8_t confirm_sweeps;
    uint8_t confirm_tolerance_bins;
} d10s_spectrum_config_t;

typedef struct {
    uint16_t noise_floor;
    uint16_t open_threshold;
    bool candidate_valid;
    uint32_t candidate_frequency_hz;
    uint16_t candidate_level;
    uint8_t candidate_streak;
    bool confirmed;
    uint32_t confirmed_frequency_hz;
    uint16_t confirmed_level;
    bool peak_hold_valid;
    uint32_t peak_hold_frequency_hz;
    uint16_t peak_hold_level;
} d10s_spectrum_result_t;

typedef struct {
    d10s_spectrum_config_t config;
    bool noise_initialized;
    uint16_t noise_floor;
    uint16_t noise_history[D10S_NOISE_HISTORY_SWEEPS];
    uint8_t noise_history_count;
    uint8_t noise_history_next;
    bool pending_valid;
    uint16_t pending_bin;
    uint8_t pending_streak;
    bool peak_hold_valid;
    uint32_t peak_hold_frequency_hz;
    uint16_t peak_hold_level;
} d10s_spectrum_state_t;

void d10s_spectrum_default_config(d10s_spectrum_config_t *config);
void d10s_spectrum_init(d10s_spectrum_state_t *state,
                        const d10s_spectrum_config_t *config);
void d10s_spectrum_reset_tracking(d10s_spectrum_state_t *state);
bool d10s_spectrum_process(d10s_spectrum_state_t *state,
                           const d10s_spectrum_bin_t *bins,
                           size_t count,
                           d10s_spectrum_result_t *result);

#endif
