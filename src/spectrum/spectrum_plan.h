#ifndef D10S_SPECTRUM_PLAN_H
#define D10S_SPECTRUM_PLAN_H

#include <stdbool.h>
#include <stdint.h>

#define D10S_SPECTRUM_MAX_BANDS 8u
#define D10S_SPECTRUM_MIN_BINS 3u
#define D10S_SPECTRUM_MAX_BINS 256u

typedef struct {
    uint32_t start_hz;
    uint32_t stop_hz;
    uint16_t bin_count;
    uint8_t dwell_sweeps;
} d10s_spectrum_band_t;

typedef struct {
    const d10s_spectrum_band_t *bands;
    uint8_t band_count;
    uint8_t active_band;
    uint8_t sweeps_on_band;
} d10s_spectrum_plan_t;

bool d10s_spectrum_band_is_valid(const d10s_spectrum_band_t *band);
bool d10s_spectrum_plan_init(d10s_spectrum_plan_t *plan,
                             const d10s_spectrum_band_t *bands,
                             uint8_t band_count);
const d10s_spectrum_band_t *d10s_spectrum_plan_current(
                             const d10s_spectrum_plan_t *plan);
bool d10s_spectrum_plan_frequency(const d10s_spectrum_plan_t *plan,
                                  uint16_t bin_index,
                                  uint32_t *frequency_hz);
/* Returns true only when the completed sweep changes the active band. */
bool d10s_spectrum_plan_complete_sweep(d10s_spectrum_plan_t *plan);
bool d10s_spectrum_plan_select_band(d10s_spectrum_plan_t *plan,
                                    uint8_t band_index);

#endif
