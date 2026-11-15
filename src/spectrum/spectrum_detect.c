#include "spectrum_detect.h"

static void clear_bytes(void *memory, size_t length)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t i;
    for (i = 0u; i < length; ++i) bytes[i] = 0u;
}

static uint16_t clamp_u16(uint32_t value)
{
    return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value;
}

static void sort_u16(uint16_t *values, size_t count)
{
    size_t i;
    for (i = 1u; i < count; ++i) {
        const uint16_t value = values[i];
        size_t j = i;
        while (j > 0u && values[j - 1u] > value) {
            values[j] = values[j - 1u];
            --j;
        }
        values[j] = value;
    }
}

static bool estimate_noise_floor(d10s_spectrum_state_t *state,
                                 const d10s_spectrum_bin_t *bins,
                                 size_t count)
{
    uint16_t sorted[D10S_SPECTRUM_MAX_BINS];
    uint16_t history[D10S_NOISE_HISTORY_SWEEPS];
    size_t i;
    size_t percentile_index;
    uint16_t observation;
    uint16_t rolling_observation;

    if (count == 0u || count > D10S_SPECTRUM_MAX_BINS) return false;

    for (i = 0u; i < count; ++i) sorted[i] = bins[i].rssi;
    sort_u16(sorted, count);
    percentile_index = ((count - 1u) * state->config.noise_percentile) / 100u;
    observation = sorted[percentile_index];

    state->noise_history[state->noise_history_next] = observation;
    state->noise_history_next = (uint8_t)((state->noise_history_next + 1u) %
                                          D10S_NOISE_HISTORY_SWEEPS);
    if (state->noise_history_count < D10S_NOISE_HISTORY_SWEEPS)
        ++state->noise_history_count;

    for (i = 0u; i < state->noise_history_count; ++i)
        history[i] = state->noise_history[i];
    sort_u16(history, state->noise_history_count);
    percentile_index = ((state->noise_history_count - 1u) *
                        state->config.noise_history_percentile) / 100u;
    rolling_observation = history[percentile_index];

    if (!state->noise_initialized) {
        state->noise_floor = rolling_observation;
        state->noise_initialized = true;
    } else {
        const uint32_t filtered =
            ((uint32_t)state->noise_floor * 3u + rolling_observation + 2u) / 4u;
        state->noise_floor = clamp_u16(filtered);
    }
    return true;
}

static bool valid_frequency_grid(const d10s_spectrum_bin_t *bins, size_t count)
{
    size_t i;
    uint32_t step;
    if (count < 3u) return false;
    if (bins[1].frequency_hz <= bins[0].frequency_hz) return false;
    step = bins[1].frequency_hz - bins[0].frequency_hz;
    for (i = 2u; i < count; ++i) {
        if (bins[i].frequency_hz <= bins[i - 1u].frequency_hz ||
            bins[i].frequency_hz - bins[i - 1u].frequency_hz != step) return false;
    }
    return true;
}

static uint32_t refined_frequency(const d10s_spectrum_bin_t *bins, size_t index)
{
    const int32_t left = (int32_t)bins[index - 1u].rssi;
    const int32_t center = (int32_t)bins[index].rssi;
    const int32_t right = (int32_t)bins[index + 1u].rssi;
    const int32_t denominator = left - (2 * center) + right;
    const int32_t numerator = left - right;
    int32_t offset_q8 = 0;
    const uint32_t span =
        bins[index + 1u].frequency_hz - bins[index - 1u].frequency_hz;
    int64_t frequency;

    if (denominator < 0) {
        offset_q8 = (128 * numerator) / denominator;
        if (offset_q8 > 128) offset_q8 = 128;
        if (offset_q8 < -128) offset_q8 = -128;
    }
    frequency = (int64_t)bins[index].frequency_hz +
                ((int64_t)span * offset_q8) / 512;
    if (frequency < 0) return 0u;
    if (frequency > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)frequency;
}

static size_t find_peaks(const d10s_spectrum_state_t *state,
                         const d10s_spectrum_bin_t *bins,
                         size_t count,
                         uint16_t threshold,
                         uint16_t *peak_indices)
{
    size_t found = 0u;
    size_t i;

    for (i = 1u; i + 1u < count; ++i) {
        const uint16_t level = bins[i].rssi;
        const uint16_t neighbor =
            bins[i - 1u].rssi > bins[i + 1u].rssi
                ? bins[i - 1u].rssi : bins[i + 1u].rssi;
        const uint16_t prominence =
            level > neighbor ? (uint16_t)(level - neighbor) : 0u;
        size_t position;

        if (level < threshold || level < bins[i - 1u].rssi ||
            level < bins[i + 1u].rssi) continue;
        if (level == bins[i - 1u].rssi && level == bins[i + 1u].rssi) continue;
        if (prominence < state->config.min_peak_prominence) continue;

        /* Keep the strongest bounded set, in descending RSSI order. */
        if (found == D10S_SPECTRUM_MAX_PEAKS &&
            level <= bins[peak_indices[D10S_SPECTRUM_MAX_PEAKS - 1u]].rssi)
            continue;
        position = found;
        if (position > D10S_SPECTRUM_MAX_PEAKS - 1u)
            position = D10S_SPECTRUM_MAX_PEAKS - 1u;
        while (position > 0u &&
               bins[peak_indices[position - 1u]].rssi < level) {
            if (position < D10S_SPECTRUM_MAX_PEAKS)
                peak_indices[position] = peak_indices[position - 1u];
            --position;
        }
        if (position < D10S_SPECTRUM_MAX_PEAKS)
            peak_indices[position] = (uint16_t)i;
        if (found < D10S_SPECTRUM_MAX_PEAKS) ++found;
    }
    return found;
}

static bool bins_within_tolerance(uint16_t a, uint16_t b, uint8_t tolerance)
{
    return a > b ? (uint16_t)(a - b) <= tolerance
                 : (uint16_t)(b - a) <= tolerance;
}

void d10s_spectrum_default_config(d10s_spectrum_config_t *config)
{
    if (config == NULL) return;
    config->noise_percentile = 20u;
    config->noise_history_percentile = 50u;
    config->noise_margin = 12u;
    config->min_peak_prominence = 5u;
    config->confirm_sweeps = 3u;
    config->confirm_tolerance_bins = 1u;
}

void d10s_spectrum_init(d10s_spectrum_state_t *state,
                        const d10s_spectrum_config_t *config)
{
    d10s_spectrum_config_t selected;
    if (state == NULL) return;
    if (config != NULL) {
        selected = *config;
    } else {
        d10s_spectrum_default_config(&selected);
    }
    clear_bytes(state, sizeof(*state));
    state->config = selected;
    if (state->config.noise_percentile > 100u)
        state->config.noise_percentile = 100u;
    if (state->config.noise_history_percentile > 100u)
        state->config.noise_history_percentile = 100u;
    if (state->config.confirm_sweeps == 0u)
        state->config.confirm_sweeps = 1u;
}

void d10s_spectrum_reset_tracking(d10s_spectrum_state_t *state)
{
    size_t i;
    if (state == NULL) return;
    state->noise_initialized = false;
    state->noise_floor = 0u;
    for (i = 0u; i < D10S_NOISE_HISTORY_SWEEPS; ++i)
        state->noise_history[i] = 0u;
    state->noise_history_count = 0u;
    state->noise_history_next = 0u;
    state->pending_valid = false;
    state->pending_bin = 0u;
    state->pending_streak = 0u;
    state->peak_hold_valid = false;
    state->peak_hold_frequency_hz = 0u;
    state->peak_hold_level = 0u;
}

bool d10s_spectrum_process(d10s_spectrum_state_t *state,
                           const d10s_spectrum_bin_t *bins,
                           size_t count,
                           d10s_spectrum_result_t *result)
{
    uint16_t threshold;
    uint16_t peak_indices[D10S_SPECTRUM_MAX_PEAKS];
    d10s_spectrum_peak_t next_tracks[D10S_SPECTRUM_MAX_PEAKS];
    bool old_track_used[D10S_SPECTRUM_MAX_PEAKS] = {false};
    size_t peak_count;
    size_t i;
    size_t j;

    if (state == NULL || bins == NULL || result == NULL ||
        count < 3u || count > D10S_SPECTRUM_MAX_BINS ||
        !valid_frequency_grid(bins, count)) return false;
    for (i = 0u; i < count; ++i) {
        if (bins[i].rssi > D10S_SPECTRUM_MAX_RSSI) return false;
    }

    clear_bytes(result, sizeof(*result));
    clear_bytes(next_tracks, sizeof(next_tracks));
    if (!estimate_noise_floor(state, bins, count)) return false;
    threshold = clamp_u16((uint32_t)state->noise_floor + state->config.noise_margin);
    result->noise_floor = state->noise_floor;
    result->open_threshold = threshold;

    peak_count = find_peaks(state, bins, count, threshold, peak_indices);
    result->peak_count = (uint8_t)peak_count;

    for (i = 0u; i < peak_count; ++i) {
        const uint16_t bin = peak_indices[i];
        size_t matched_track = D10S_SPECTRUM_MAX_PEAKS;
        uint16_t best_distance = UINT16_MAX;
        d10s_spectrum_peak_t *peak = &result->peaks[i];

        for (j = 0u; j < D10S_SPECTRUM_MAX_PEAKS; ++j) {
            uint16_t distance;
            if (!state->tracked_peaks[j].valid || old_track_used[j] ||
                !bins_within_tolerance(bin, state->tracked_peaks[j].bin,
                                       state->config.confirm_tolerance_bins))
                continue;
            distance = bin > state->tracked_peaks[j].bin
                ? (uint16_t)(bin - state->tracked_peaks[j].bin)
                : (uint16_t)(state->tracked_peaks[j].bin - bin);
            if (distance < best_distance) {
                best_distance = distance;
                matched_track = j;
            }
        }

        peak->valid = true;
        peak->bin = bin;
        peak->frequency_hz = refined_frequency(bins, bin);
        peak->level = bins[bin].rssi;
        if (matched_track < D10S_SPECTRUM_MAX_PEAKS) {
            old_track_used[matched_track] = true;
            peak->streak = state->tracked_peaks[matched_track].streak;
            if (peak->streak < UINT8_MAX) ++peak->streak;
        } else {
            peak->streak = 1u;
        }
        peak->confirmed = peak->streak >= state->config.confirm_sweeps;
        next_tracks[i] = *peak;

        if (i == 0u) {
            result->candidate_valid = true;
            result->candidate_frequency_hz = peak->frequency_hz;
            result->candidate_level = peak->level;
            result->candidate_streak = peak->streak;
            result->confirmed = peak->confirmed;
            if (peak->confirmed) {
                result->confirmed_frequency_hz = peak->frequency_hz;
                result->confirmed_level = peak->level;
            }
        }

        if (peak->confirmed &&
            (!state->peak_hold_valid || peak->level >= state->peak_hold_level)) {
            state->peak_hold_valid = true;
            state->peak_hold_frequency_hz = peak->frequency_hz;
            state->peak_hold_level = peak->level;
        }
    }

    for (i = 0u; i < D10S_SPECTRUM_MAX_PEAKS; ++i)
        state->tracked_peaks[i] = next_tracks[i];

    /* Preserve the legacy strongest-candidate tracking fields for callers
       that inspect state directly; confirmation is now per-peak. */
    state->pending_valid = peak_count > 0u;
    state->pending_bin = peak_count > 0u ? peak_indices[0] : 0u;
    state->pending_streak = peak_count > 0u ? result->peaks[0].streak : 0u;

    result->peak_hold_valid = state->peak_hold_valid;
    result->peak_hold_frequency_hz = state->peak_hold_frequency_hz;
    result->peak_hold_level = state->peak_hold_level;
    return true;
}
