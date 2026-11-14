#include "spectrum_plan.h"

bool d10s_spectrum_band_is_valid(const d10s_spectrum_band_t *band)
{
    uint32_t span;
    uint32_t intervals;

    if (band == 0 || band->start_hz == 0u ||
        band->stop_hz <= band->start_hz ||
        band->bin_count < D10S_SPECTRUM_MIN_BINS ||
        band->bin_count > D10S_SPECTRUM_MAX_BINS ||
        band->dwell_sweeps == 0u) return false;

    span = band->stop_hz - band->start_hz;
    intervals = (uint32_t)band->bin_count - 1u;
    return (span % intervals) == 0u && (span / intervals) != 0u;
}

bool d10s_spectrum_plan_init(d10s_spectrum_plan_t *plan,
                             const d10s_spectrum_band_t *bands,
                             uint8_t band_count)
{
    uint8_t i;
    if (plan == 0 || bands == 0 || band_count == 0u ||
        band_count > D10S_SPECTRUM_MAX_BANDS) return false;

    for (i = 0u; i < band_count; ++i) {
        if (!d10s_spectrum_band_is_valid(&bands[i])) return false;
    }

    plan->bands = bands;
    plan->band_count = band_count;
    plan->active_band = 0u;
    plan->sweeps_on_band = 0u;
    return true;
}

const d10s_spectrum_band_t *d10s_spectrum_plan_current(
                             const d10s_spectrum_plan_t *plan)
{
    if (plan == 0 || plan->bands == 0 ||
        plan->band_count == 0u || plan->active_band >= plan->band_count)
        return 0;
    return &plan->bands[plan->active_band];
}

bool d10s_spectrum_plan_frequency(const d10s_spectrum_plan_t *plan,
                                  uint16_t bin_index,
                                  uint32_t *frequency_hz)
{
    const d10s_spectrum_band_t *band;
    uint32_t step;
    uint64_t frequency;

    if (frequency_hz == 0) return false;
    band = d10s_spectrum_plan_current(plan);
    if (band == 0 || bin_index >= band->bin_count) return false;

    step = (band->stop_hz - band->start_hz) /
           ((uint32_t)band->bin_count - 1u);
    frequency = (uint64_t)band->start_hz + (uint64_t)step * bin_index;
    if (frequency > UINT32_MAX) return false;
    *frequency_hz = (uint32_t)frequency;
    return true;
}

bool d10s_spectrum_plan_complete_sweep(d10s_spectrum_plan_t *plan)
{
    const d10s_spectrum_band_t *band = d10s_spectrum_plan_current(plan);
    if (band == 0) return false;

    if (plan->sweeps_on_band < UINT8_MAX) ++plan->sweeps_on_band;
    if (plan->sweeps_on_band < band->dwell_sweeps) return false;

    plan->sweeps_on_band = 0u;
    plan->active_band = (uint8_t)((plan->active_band + 1u) % plan->band_count);
    return true;
}

bool d10s_spectrum_plan_select_band(d10s_spectrum_plan_t *plan,
                                    uint8_t band_index)
{
    if (plan == 0 || plan->bands == 0 ||
        band_index >= plan->band_count) return false;
    if (plan->active_band == band_index) return false;

    plan->active_band = band_index;
    plan->sweeps_on_band = 0u;
    return true;
}
