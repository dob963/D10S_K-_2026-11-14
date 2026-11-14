#include "spectrum_app.h"

#include <stddef.h>

bool d10s_spectrum_app_init(d10s_spectrum_app_t *app,
                            const d10s_spectrum_band_t *bands,
                            uint8_t band_count,
                            const d10s_spectrum_radio_ops_t *radio,
                            d10s_st7565_t *display,
                            const d10s_spectrum_config_t *detector_config,
                            uint16_t settle_ms)
{
    if (app == NULL || radio == NULL || display == NULL ||
        !display->initialized || radio->tune_hz == NULL ||
        radio->read_rssi == NULL) return false;

    app->initialized = false;
    app->pending_band_valid = false;
    app->pending_band_index = 0u;
    if (!d10s_spectrum_scanner_init(&app->scanner, bands, band_count,
                                    detector_config, settle_ms)) return false;
    app->radio = *radio;
    app->display = display;
    app->cursor_bin = 0u;
    app->cursor_enabled = false;
    app->bold_labels = true;
    app->peak_hold_visible = true;
    app->paused = false;
    app->pause_frame_pending = false;
    app->last_result = (d10s_spectrum_result_t){0};
    d10s_mono128x64_clear(&app->framebuffer, false);
    app->initialized = true;
    return true;
}

d10s_spectrum_app_status_t d10s_spectrum_app_tick(
                            d10s_spectrum_app_t *app,
                            uint32_t now_ms)
{
    d10s_spectrum_result_t result;
    const d10s_spectrum_bin_t *snapshot = NULL;
    uint16_t count = 0u;
    uint8_t band_index = 0u;
    uint8_t attenuation_level = 0u;
    uint32_t generation = 0u;
    bool band_changed = false;
    d10s_spectrum_scanner_status_t status;
    d10s_spectrum_canvas_t canvas;
    d10s_spectrum_view_model_t model;

    if (app == NULL || !app->initialized || app->display == NULL)
        return D10S_APP_ERROR;

    if (app->paused) {
        if (!app->pause_frame_pending) return D10S_APP_IDLE;
        result = app->last_result;
        status = D10S_SCANNER_SWEEP_READY;
    } else {
        if (app->pending_band_valid) {
            if (app->scanner.plan.active_band == app->pending_band_index ||
                d10s_spectrum_scanner_select_band(&app->scanner,
                                                   app->pending_band_index))
                app->pending_band_valid = false;
        }

        status = d10s_spectrum_scanner_tick(&app->scanner, &app->radio,
                                            now_ms, &result, &band_changed);
    }
    (void)band_changed;
    if (status == D10S_SCANNER_IDLE) return D10S_APP_IDLE;
    if (status == D10S_SCANNER_ERROR) return D10S_APP_ERROR;
    if (status != D10S_SCANNER_SWEEP_READY) return D10S_APP_BUSY;

    if (!d10s_spectrum_scanner_snapshot(&app->scanner, &snapshot, &count,
                                        &band_index, &attenuation_level,
                                        &generation) ||
        snapshot == NULL || count < 3u) return D10S_APP_ERROR;
    (void)generation;

    model = (d10s_spectrum_view_model_t){
        .bins = snapshot,
        .count = count,
        .band_index = band_index,
        .band_count = app->scanner.plan.band_count,
        .attenuation_level = attenuation_level,
        .turbo_enabled = app->scanner.turbo_enabled,
        .hold_enabled = app->paused,
        .noise_floor = result.noise_floor,
        .open_threshold = result.open_threshold,
        .peak_hold_valid = app->peak_hold_visible && result.peak_hold_valid,
        .peak_hold_frequency_hz = result.peak_hold_frequency_hz,
        .cursor_valid = app->cursor_enabled && app->cursor_bin < count,
        .cursor_bin = app->cursor_bin,
        .band_label = NULL,
        .bold_labels = app->bold_labels
    };

    d10s_mono128x64_clear(&app->framebuffer, false);
    d10s_mono128x64_bind(&app->framebuffer, &canvas);
    if (!d10s_spectrum_view_render(&canvas, &model) ||
        !d10s_mono128x64_flush(&app->framebuffer, app->display))
        return D10S_APP_ERROR;

    app->last_result = result;
    app->pause_frame_pending = false;
    return D10S_APP_FRAME_UPDATED;
}


static bool set_cursor_position(d10s_spectrum_app_t *app,
                                uint16_t count, uint16_t index)
{
    if (app == NULL || count < 3u) return false;
    app->cursor_bin = (uint16_t)(index % count);
    app->cursor_enabled = true;
    if (app->paused) app->pause_frame_pending = true;
    return true;
}

bool d10s_spectrum_app_handle_key(d10s_spectrum_app_t *app,
                                  d10s_spectrum_key_t key)
{
    uint16_t count;
    uint16_t cursor;
    if (app == NULL || !app->initialized) return false;

    count = app->scanner.published_count;
    switch (key) {
    case D10S_KEY_MENU:
        if (app->cursor_enabled) {
            app->cursor_enabled = false;
            if (app->paused) app->pause_frame_pending = true;
            return true;
        }
        return set_cursor_position(app, count, (uint16_t)(count / 2u));

    case D10S_KEY_EXIT:
        app->cursor_enabled = false;
        if (app->paused) app->pause_frame_pending = true;
        return true;

    case D10S_KEY_0:
        return set_cursor_position(app, count, (uint16_t)(count / 2u));

    case D10S_KEY_UP:
    case D10S_KEY_DOWN:
    case D10S_KEY_SIDE1:
    case D10S_KEY_SIDE2:
        if (count < 3u) return false;
        cursor = app->cursor_enabled ? app->cursor_bin
                                     : (uint16_t)(count / 2u);
        if (key == D10S_KEY_UP)
            cursor = (uint16_t)((cursor + count - 1u) % count);
        else if (key == D10S_KEY_DOWN)
            cursor = (uint16_t)((cursor + 1u) % count);
        else if (key == D10S_KEY_SIDE1)
            cursor = (uint16_t)((cursor + count - (8u % count)) % count);
        else
            cursor = (uint16_t)((cursor + (8u % count)) % count);
        return set_cursor_position(app, count, cursor);

    case D10S_KEY_F:
        return d10s_spectrum_app_set_bold_labels(app, !app->bold_labels);

    case D10S_KEY_STAR:
        app->peak_hold_visible = !app->peak_hold_visible;
        if (app->paused) app->pause_frame_pending = true;
        return true;

    case D10S_KEY_9:
        if (!app->paused && !app->scanner.published_valid) return false;
        app->paused = !app->paused;
        app->pause_frame_pending = app->paused;
        return true;

    case D10S_KEY_1:
    case D10S_KEY_2:
    case D10S_KEY_3:
    case D10S_KEY_4:
    case D10S_KEY_5:
    case D10S_KEY_6:
    case D10S_KEY_7:
    case D10S_KEY_8:
        app->pending_band_index = (uint8_t)(key - D10S_KEY_1);
        if (app->pending_band_index >= app->scanner.plan.band_count)
            return false;
        app->pending_band_valid =
            app->pending_band_index != app->scanner.plan.active_band;
        return true;

    default:
        return false;
    }
}

bool d10s_spectrum_app_set_cursor(d10s_spectrum_app_t *app,
                                  bool enabled,
                                  uint16_t bin_index)
{
    if (app == NULL || !app->initialized ||
        bin_index >= D10S_SPECTRUM_MAX_BINS) return false;
    app->cursor_enabled = enabled;
    app->cursor_bin = bin_index;
    if (app->paused) app->pause_frame_pending = true;
    return true;
}

bool d10s_spectrum_app_set_bold_labels(d10s_spectrum_app_t *app,
                                       bool enabled)
{
    if (app == NULL || !app->initialized) return false;
    app->bold_labels = enabled;
    if (app->paused) app->pause_frame_pending = true;
    return true;
}

bool d10s_spectrum_app_set_delay(d10s_spectrum_app_t *app,
                                 uint16_t settle_ms)
{
    return app != NULL && app->initialized &&
           d10s_spectrum_scanner_set_delay(&app->scanner, settle_ms);
}

bool d10s_spectrum_app_set_turbo(d10s_spectrum_app_t *app,
                                 bool enabled,
                                 uint16_t min_settle_ms,
                                 uint16_t poll_interval_ms,
                                 uint8_t stable_delta,
                                 uint8_t stable_samples)
{
    return app != NULL && app->initialized &&
           d10s_spectrum_scanner_set_turbo(&app->scanner, enabled,
                                            min_settle_ms, poll_interval_ms,
                                            stable_delta, stable_samples);
}

bool d10s_spectrum_app_set_attenuation(d10s_spectrum_app_t *app,
                                       uint8_t level,
                                       uint16_t measured_raw_rssi_offset)
{
    return app != NULL && app->initialized &&
           d10s_spectrum_scanner_set_attenuation(&app->scanner, &app->radio,
                                                  level,
                                                  measured_raw_rssi_offset);
}

bool d10s_spectrum_app_select_band(d10s_spectrum_app_t *app,
                                   uint8_t band_index)
{
    return app != NULL && app->initialized &&
           d10s_spectrum_scanner_select_band(&app->scanner, band_index);
}

void d10s_spectrum_app_stop(d10s_spectrum_app_t *app)
{
    if (app == NULL || !app->initialized) return;
    d10s_spectrum_scanner_stop(&app->scanner);
    app->pending_band_valid = false;
    app->paused = false;
    app->pause_frame_pending = false;
    app->initialized = false;
}
