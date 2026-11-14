#include "spectrum_core.h"

#include <string.h>

void d10s_noise_floor_init(d10s_noise_floor_t *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

void d10s_noise_floor_add(d10s_noise_floor_t *state, uint8_t rssi)
{
    if (state == NULL) {
        return;
    }

    state->samples[state->next] = rssi;
    state->next = (uint8_t)((state->next + 1u) % D10S_NOISE_WINDOW);
    if (state->count < D10S_NOISE_WINDOW) {
        state->count++;
    }
}

uint8_t d10s_noise_floor_percentile(const d10s_noise_floor_t *state,
                                    uint8_t percentile)
{
    uint8_t sorted[D10S_NOISE_WINDOW];
    uint8_t i;
    uint8_t j;
    uint8_t value;
    size_t index;

    if ((state == NULL) || (state->count == 0u)) {
        return 0u;
    }
    if (percentile > 100u) {
        percentile = 100u;
    }

    memcpy(sorted, state->samples, state->count);
    for (i = 1u; i < state->count; i++) {
        value = sorted[i];
        j = i;
        while ((j > 0u) && (sorted[j - 1u] > value)) {
            sorted[j] = sorted[j - 1u];
            j--;
        }
        sorted[j] = value;
    }

    index = ((size_t)(state->count - 1u) * percentile) / 100u;
    return sorted[index];
}

uint8_t d10s_noise_floor_threshold(const d10s_noise_floor_t *state,
                                   uint8_t margin)
{
    uint16_t threshold;

    threshold = (uint16_t)d10s_noise_floor_percentile(state, 25u) + margin;
    return (threshold > UINT8_MAX) ? UINT8_MAX : (uint8_t)threshold;
}

void d10s_peak_tracker_init(d10s_peak_tracker_t *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

void d10s_peak_tracker_reset(d10s_peak_tracker_t *state)
{
    d10s_peak_tracker_init(state);
}

bool d10s_peak_tracker_update(d10s_peak_tracker_t *state,
                              uint8_t rssi,
                              uint8_t open_threshold,
                              uint32_t frequency_hz)
{
    if (state == NULL) {
        return false;
    }

    if (rssi < open_threshold) {
        d10s_peak_tracker_reset(state);
        return false;
    }

    if ((state->consecutive_passes > 0u) &&
        (state->candidate_hz != frequency_hz)) {
        state->consecutive_passes = 0u;
        state->confirmed = false;
    }

    state->candidate_hz = frequency_hz;
    if (state->consecutive_passes < D10S_PEAK_CONFIRM_PASSES) {
        state->consecutive_passes++;
    }
    state->confirmed =
        (state->consecutive_passes >= D10S_PEAK_CONFIRM_PASSES);

    return state->confirmed;
}
