#include "spectrum_scanner.h"

enum {
    SCANNER_PHASE_STOPPED = 0,
    SCANNER_PHASE_TUNE,
    SCANNER_PHASE_SETTLE,
    SCANNER_PHASE_READ,
    SCANNER_PHASE_FAILED
};

bool d10s_spectrum_scanner_init(d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_band_t *bands,
                                uint8_t band_count,
                                const d10s_spectrum_config_t *config,
                                uint16_t settle_ms)
{
    if (scanner == 0 || settle_ms == 0u) return false;
    if (!d10s_spectrum_plan_init(&scanner->plan, bands, band_count))
        return false;

    d10s_spectrum_init(&scanner->detector, config);
    scanner->settle_started_ms = 0u;
    scanner->current_frequency_hz = 0u;
    scanner->bin_index = 0u;
    scanner->settle_ms = settle_ms;
    scanner->rssi_compensation = 0u;
    scanner->attenuation_level = 0u;
    scanner->attenuation_configured = false;
    scanner->published_generation = 0u;
    scanner->published_band_index = 0u;
    scanner->published_attenuation_level = 0u;
    scanner->published_count = 0u;
    scanner->published_valid = false;
    scanner->phase = SCANNER_PHASE_TUNE;
    return true;
}

bool d10s_spectrum_scanner_set_attenuation(
                                d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_radio_ops_t *radio,
                                uint8_t level,
                                uint16_t measured_raw_rssi_offset)
{
    if (scanner == 0 || radio == 0 || radio->set_attenuation == 0)
        return false;
    if (scanner->phase != SCANNER_PHASE_TUNE || scanner->bin_index != 0u)
        return false;
    if (scanner->attenuation_configured &&
        scanner->attenuation_level == level &&
        scanner->rssi_compensation == measured_raw_rssi_offset)
        return true;
    if (!radio->set_attenuation(radio->context, level)) return false;

    scanner->attenuation_level = level;
    scanner->rssi_compensation = measured_raw_rssi_offset;
    scanner->attenuation_configured = true;
    scanner->published_valid = false;
    d10s_spectrum_reset_tracking(&scanner->detector);
    return true;
}

bool d10s_spectrum_scanner_set_delay(d10s_spectrum_scanner_t *scanner,
                                     uint16_t settle_ms)
{
    if (scanner == 0 || settle_ms == 0u ||
        scanner->phase != SCANNER_PHASE_TUNE || scanner->bin_index != 0u)
        return false;
    scanner->settle_ms = settle_ms;
    return true;
}

bool d10s_spectrum_scanner_select_band(d10s_spectrum_scanner_t *scanner,
                                       uint8_t band_index)
{
    if (scanner == 0 || scanner->phase != SCANNER_PHASE_TUNE ||
        scanner->bin_index != 0u) return false;
    if (!d10s_spectrum_plan_select_band(&scanner->plan, band_index))
        return false;

    scanner->published_valid = false;
    scanner->current_frequency_hz = 0u;
    d10s_spectrum_reset_tracking(&scanner->detector);
    return true;
}

bool d10s_spectrum_scanner_snapshot(const d10s_spectrum_scanner_t *scanner,
                                    const d10s_spectrum_bin_t **bins,
                                    uint16_t *count,
                                    uint8_t *band_index,
                                    uint8_t *attenuation_level,
                                    uint32_t *generation)
{
    if (scanner == 0 || bins == 0 || count == 0 ||
        !scanner->published_valid) return false;
    *bins = scanner->published_bins;
    *count = scanner->published_count;
    if (band_index != 0) *band_index = scanner->published_band_index;
    if (attenuation_level != 0)
        *attenuation_level = scanner->published_attenuation_level;
    if (generation != 0) *generation = scanner->published_generation;
    return true;
}

void d10s_spectrum_scanner_stop(d10s_spectrum_scanner_t *scanner)
{
    if (scanner == 0) return;
    scanner->phase = SCANNER_PHASE_STOPPED;
    scanner->bin_index = 0u;
    d10s_spectrum_reset_tracking(&scanner->detector);
}

d10s_spectrum_scanner_status_t d10s_spectrum_scanner_tick(
                                d10s_spectrum_scanner_t *scanner,
                                const d10s_spectrum_radio_ops_t *radio,
                                uint32_t now_ms,
                                d10s_spectrum_result_t *result,
                                bool *band_changed)
{
    const d10s_spectrum_band_t *band;

    if (band_changed != 0) *band_changed = false;
    if (scanner == 0 || radio == 0 || result == 0 ||
        radio->tune_hz == 0 || radio->read_rssi == 0)
        return D10S_SCANNER_ERROR;

    if (scanner->phase == SCANNER_PHASE_STOPPED) return D10S_SCANNER_IDLE;
    if (scanner->phase == SCANNER_PHASE_FAILED) return D10S_SCANNER_ERROR;

    band = d10s_spectrum_plan_current(&scanner->plan);
    if (band == 0) {
        scanner->phase = SCANNER_PHASE_FAILED;
        return D10S_SCANNER_ERROR;
    }

    switch (scanner->phase) {
    case SCANNER_PHASE_TUNE: {
        uint32_t requested_frequency_hz = 0u;
        uint32_t actual_frequency_hz = 0u;
        if (!d10s_spectrum_plan_frequency(&scanner->plan,
                                          scanner->bin_index,
                                          &requested_frequency_hz) ||
            !radio->tune_hz(radio->context, requested_frequency_hz,
                            &actual_frequency_hz) ||
            actual_frequency_hz < band->start_hz ||
            actual_frequency_hz > band->stop_hz) {
            scanner->phase = SCANNER_PHASE_FAILED;
            return D10S_SCANNER_ERROR;
        }
        scanner->current_frequency_hz = actual_frequency_hz;
        scanner->bins[scanner->bin_index].frequency_hz = actual_frequency_hz;
        scanner->settle_started_ms = now_ms;
        scanner->phase = SCANNER_PHASE_SETTLE;
        return D10S_SCANNER_BUSY;
    }

    case SCANNER_PHASE_SETTLE:
        if ((uint32_t)(now_ms - scanner->settle_started_ms) < scanner->settle_ms)
            return D10S_SCANNER_BUSY;
        scanner->phase = SCANNER_PHASE_READ;
        return D10S_SCANNER_BUSY;

    case SCANNER_PHASE_READ: {
        uint16_t rssi = 0u;
        bool changed;

        if (!radio->read_rssi(radio->context, &rssi) ||
            rssi > D10S_SPECTRUM_MAX_RSSI) {
            scanner->phase = SCANNER_PHASE_FAILED;
            return D10S_SCANNER_ERROR;
        }
        {
            uint32_t compensated = (uint32_t)rssi + scanner->rssi_compensation;
            if (compensated > D10S_SPECTRUM_MAX_RSSI)
                compensated = D10S_SPECTRUM_MAX_RSSI;
            scanner->bins[scanner->bin_index].rssi = (uint16_t)compensated;
        }

        if ((uint16_t)(scanner->bin_index + 1u) < band->bin_count) {
            ++scanner->bin_index;
            scanner->phase = SCANNER_PHASE_TUNE;
            return D10S_SCANNER_BUSY;
        }

        if (!d10s_spectrum_process(&scanner->detector,
                                   scanner->bins,
                                   band->bin_count,
                                   result)) {
            scanner->phase = SCANNER_PHASE_FAILED;
            return D10S_SCANNER_ERROR;
        }

        {
            uint16_t i;
            scanner->published_count = band->bin_count;
            scanner->published_band_index = scanner->plan.active_band;
            scanner->published_attenuation_level = scanner->attenuation_level;
            for (i = 0u; i < band->bin_count; ++i)
                scanner->published_bins[i] = scanner->bins[i];
            ++scanner->published_generation;
            scanner->published_valid = true;
        }

        changed = d10s_spectrum_plan_complete_sweep(&scanner->plan);
        if (changed) d10s_spectrum_reset_tracking(&scanner->detector);

        scanner->bin_index = 0u;
        scanner->phase = SCANNER_PHASE_TUNE;
        if (band_changed != 0) *band_changed = changed;
        return D10S_SCANNER_SWEEP_READY;
    }

    default:
        scanner->phase = SCANNER_PHASE_FAILED;
        return D10S_SCANNER_ERROR;
    }
}
