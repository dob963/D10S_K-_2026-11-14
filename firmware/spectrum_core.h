#ifndef D10S_SPECTRUM_CORE_H
#define D10S_SPECTRUM_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define D10S_NOISE_WINDOW 32u
#define D10S_PEAK_CONFIRM_PASSES 2u

typedef struct {
    uint8_t samples[D10S_NOISE_WINDOW];
    uint8_t count;
    uint8_t next;
} d10s_noise_floor_t;

typedef struct {
    uint8_t consecutive_passes;
    bool confirmed;
    uint32_t candidate_hz;
} d10s_peak_tracker_t;

void d10s_noise_floor_init(d10s_noise_floor_t *state);
void d10s_noise_floor_add(d10s_noise_floor_t *state, uint8_t rssi);
uint8_t d10s_noise_floor_percentile(const d10s_noise_floor_t *state,
                                    uint8_t percentile);
uint8_t d10s_noise_floor_threshold(const d10s_noise_floor_t *state,
                                   uint8_t margin);

void d10s_peak_tracker_init(d10s_peak_tracker_t *state);
bool d10s_peak_tracker_update(d10s_peak_tracker_t *state,
                              uint8_t rssi,
                              uint8_t open_threshold,
                              uint32_t frequency_hz);
void d10s_peak_tracker_reset(d10s_peak_tracker_t *state);

#endif
