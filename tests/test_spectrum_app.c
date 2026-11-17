#include "spectrum_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct {
    bool selected;
    bool data_mode;
    uint32_t bytes_written;
    uint32_t frames_written;
    uint32_t tune_calls;
    uint32_t read_calls;
    uint32_t frequency_hz;
    uint32_t first_hz;
    uint8_t attenuation_level;
} fake_hardware_t;

static bool lcd_select(bool asserted, void *context)
{
    ((fake_hardware_t *)context)->selected = asserted;
    return true;
}

static bool lcd_mode(bool data, void *context)
{
    ((fake_hardware_t *)context)->data_mode = data;
    return true;
}

static bool lcd_write(uint8_t value, void *context)
{
    fake_hardware_t *hw = (fake_hardware_t *)context;
    (void)value;
    CHECK(hw->selected);
    ++hw->bytes_written;
    return true;
}

static bool lcd_delay(uint16_t milliseconds, void *context)
{
    (void)context;
    return milliseconds != 0u;
}

static bool radio_tune(void *context, uint32_t requested_hz,
                       uint32_t *actual_hz)
{
    fake_hardware_t *hw = (fake_hardware_t *)context;
    if (actual_hz == NULL) return false;
    hw->frequency_hz = requested_hz;
    *actual_hz = requested_hz;
    ++hw->tune_calls;
    return true;
}

static bool radio_set_attenuation(void *context, uint8_t level)
{
    fake_hardware_t *hw = (fake_hardware_t *)context;
    if (level > 5u) return false;
    hw->attenuation_level = level;
    return true;
}

static bool radio_read(void *context, uint16_t *rssi)
{
    fake_hardware_t *hw = (fake_hardware_t *)context;
    uint32_t index;
    if (rssi == NULL) return false;
    index = (hw->frequency_hz - hw->first_hz) / 1000u;
    *rssi = index == 2u ? 180u : (uint16_t)(100u + (index & 1u));
    ++hw->read_calls;
    return true;
}

static void test_complete_spectrum_application_pipeline(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 10u},
        {145000000u, 145004000u, 5u, 10u}
    };
    fake_hardware_t hw = {0};
    const d10s_st7565_port_t lcd_port = {
        lcd_select, lcd_mode, lcd_write, lcd_delay, &hw
    };
    const d10s_st7565_config_t lcd_config = {
        4u, 31u, 4u, 7u, false, false, false, 1u, 1u
    };
    const d10s_spectrum_radio_ops_t radio = {
        &hw, radio_tune, radio_read, radio_set_attenuation, NULL
    };
    d10s_st7565_t display;
    d10s_spectrum_app_t app;
    uint32_t now_ms;
    uint32_t updated_frames = 0u;

    hw.first_hz = 144000000u;
    CHECK(d10s_st7565_init(&display, &lcd_port, &lcd_config));
    hw.bytes_written = 0u;
    CHECK(d10s_spectrum_app_init(&app, bands, 2u, &radio, &display,
                                 NULL, 1u));
    CHECK(d10s_spectrum_app_set_bold_labels(&app, true));
    CHECK(!d10s_spectrum_app_handle_key(&app, D10S_KEY_9));

    for (now_ms = 0u; now_ms < 45u; ++now_ms) {
        const d10s_spectrum_app_status_t status =
            d10s_spectrum_app_tick(&app, now_ms);
        CHECK(status != D10S_APP_ERROR);
        if (status == D10S_APP_FRAME_UPDATED) ++updated_frames;
    }

    CHECK(updated_frames == 1u);
    CHECK(hw.tune_calls == 15u);
    CHECK(hw.read_calls == 15u);
    CHECK(hw.bytes_written == 1u * 8u * 131u);
    CHECK(!hw.selected);
    CHECK(app.last_result.confirmed);
    CHECK(app.last_result.confirmed_level == 180u);
    CHECK(app.framebuffer.bytes[1] != 0u);
    CHECK(app.scanner.published_count == 5u);

    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_MENU));
    CHECK(app.cursor_enabled && app.cursor_bin == 2u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_UP));
    CHECK(app.cursor_bin == 1u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_SIDE1));
    CHECK(app.cursor_bin == 3u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_SIDE2));
    CHECK(app.cursor_bin == 1u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_DOWN));
    CHECK(app.cursor_bin == 2u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_F));
    CHECK(!app.bold_labels);
    CHECK(app.peak_hold_visible);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_STAR));
    CHECK(!app.peak_hold_visible);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_STAR));
    CHECK(app.peak_hold_visible);

    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_2));
    CHECK(app.pending_band_valid && app.pending_band_index == 1u);
    CHECK(d10s_spectrum_app_tick(&app, now_ms) != D10S_APP_ERROR);
    CHECK(!app.pending_band_valid && app.scanner.plan.active_band == 1u);

    /* Complete one sweep on the newly selected band before pausing. */
    {
        unsigned warm_tick;
        for (warm_tick = 1u; warm_tick <= 30u; ++warm_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + warm_tick) !=
                  D10S_APP_ERROR);
        now_ms += 30u;
    }

    /* Freeze the last complete sweep for inspection, without more RF scans. */
    {
        const uint32_t tune_before_pause = hw.tune_calls;
        const uint32_t read_before_pause = hw.read_calls;
        const uint32_t bytes_before_pause = hw.bytes_written;
        unsigned idle_tick;
        CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_9));
        CHECK(app.paused && app.pause_frame_pending);
        CHECK(d10s_spectrum_app_tick(&app, now_ms) == D10S_APP_FRAME_UPDATED);
        CHECK(app.paused && !app.pause_frame_pending);
        CHECK((app.framebuffer.bytes[70u] & (1u << 1)) != 0u);
        CHECK(hw.bytes_written == bytes_before_pause + 8u * 131u);

        for (idle_tick = 1u; idle_tick <= 10u; ++idle_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + idle_tick) ==
                  D10S_APP_IDLE);
        CHECK(hw.tune_calls == tune_before_pause);
        CHECK(hw.read_calls == read_before_pause);
        CHECK(hw.bytes_written == bytes_before_pause + 8u * 131u);

        CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_STAR));
        CHECK(app.pause_frame_pending);
        CHECK(d10s_spectrum_app_tick(&app, now_ms + 11u) ==
              D10S_APP_FRAME_UPDATED);
        CHECK(!app.pause_frame_pending && app.paused);
        CHECK(hw.tune_calls == tune_before_pause);
        CHECK(hw.read_calls == read_before_pause);

        {
            const uint16_t threshold_before = app.last_result.open_threshold;
            CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_7));
            CHECK(app.pause_frame_pending);
            CHECK(app.last_result.open_threshold == threshold_before + 2u);
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 12u) ==
                  D10S_APP_FRAME_UPDATED);
            CHECK(!app.pause_frame_pending && app.paused);
            CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_8));
            CHECK(app.last_result.open_threshold == threshold_before);
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 13u) ==
                  D10S_APP_FRAME_UPDATED);
        }

        CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_9));
        CHECK(!app.paused);
        CHECK(app.scanner.bin_index == 0u);
        CHECK(d10s_spectrum_app_tick(&app, now_ms + 100u) != D10S_APP_ERROR);
        CHECK(hw.tune_calls == tune_before_pause + 1u);
        CHECK(d10s_spectrum_app_tick(&app, now_ms + 101u) != D10S_APP_ERROR);
        CHECK(hw.read_calls == read_before_pause);
        CHECK(d10s_spectrum_app_tick(&app, now_ms + 102u) != D10S_APP_ERROR);
        CHECK(hw.read_calls > read_before_pause);
    }

    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_EXIT));
    CHECK(!app.cursor_enabled);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_0));
    CHECK(app.cursor_enabled && app.cursor_bin == 2u);

    /* Turbo and Delay are user-commandable and commit only at sweep boundaries. */
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_4));
    CHECK(app.scanner.pending_delay_valid);
    CHECK(app.scanner.pending_settle_ms == 12u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_3));
    CHECK(app.scanner.pending_turbo_valid);
    CHECK(app.scanner.pending_turbo_enabled);
    {
        unsigned config_tick;
        for (config_tick = 1u; config_tick <= 200u &&
             (app.scanner.pending_delay_valid ||
              app.scanner.pending_turbo_valid); ++config_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 200u + config_tick) !=
                  D10S_APP_ERROR);
    }
    CHECK(!app.scanner.pending_delay_valid);
    CHECK(!app.scanner.pending_turbo_valid);
    CHECK(app.scanner.settle_ms == 12u);
    CHECK(app.scanner.turbo_enabled);

    /* Delay key cycles 8 -> 12 -> 16 -> 24 -> 8 ms. */
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_4));
    CHECK(app.scanner.pending_settle_ms == 16u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_4));
    CHECK(app.scanner.pending_settle_ms == 24u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_4));
    CHECK(app.scanner.pending_settle_ms == 8u);
    {
        unsigned delay_tick;
        for (delay_tick = 1u; delay_tick <= 200u &&
             app.scanner.pending_delay_valid; ++delay_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 300u + delay_tick) !=
                  D10S_APP_ERROR);
    }
    CHECK(!app.scanner.pending_delay_valid);
    CHECK(app.scanner.settle_ms == 8u);

    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_3));
    CHECK(app.scanner.pending_turbo_valid);
    CHECK(!app.scanner.pending_turbo_enabled);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_valid ||
          app.scanner.attenuation_level == 1u);
    if (app.scanner.pending_attenuation_valid)
        CHECK(app.scanner.pending_attenuation_level == 1u);
    {
        unsigned att_tick;
        for (att_tick = 1u; att_tick <= 200u &&
             app.scanner.pending_attenuation_valid; ++att_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 500u + att_tick) !=
                  D10S_APP_ERROR);
    }
    CHECK(!app.scanner.pending_attenuation_valid);
    CHECK(app.scanner.attenuation_level == 1u);
    CHECK(hw.attenuation_level == 1u);

    /* The key cycles all six gain presets and wraps to ATT 0. */
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_level == 2u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_level == 3u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_level == 4u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_level == 5u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_5));
    CHECK(app.scanner.pending_attenuation_level == 0u);
    {
        unsigned att_tick;
        for (att_tick = 1u; att_tick <= 200u &&
             app.scanner.pending_attenuation_valid; ++att_tick)
            CHECK(d10s_spectrum_app_tick(&app, now_ms + 700u + att_tick) !=
                  D10S_APP_ERROR);
    }
    CHECK(!app.scanner.pending_attenuation_valid);
    CHECK(app.scanner.attenuation_level == 0u);
    CHECK(hw.attenuation_level == 0u);
    CHECK(app.scanner.settle_ms == 8u);

    /* Keys 6/7 adjust the adaptive opening margin; 8 restores default. */
    CHECK(app.scanner.detector.config.noise_margin == 12u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_6));
    CHECK(app.scanner.detector.config.noise_margin == 10u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_7));
    CHECK(app.scanner.detector.config.noise_margin == 12u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_7));
    CHECK(app.scanner.detector.config.noise_margin == 14u);
    CHECK(d10s_spectrum_app_handle_key(&app, D10S_KEY_8));
    CHECK(app.scanner.detector.config.noise_margin == 12u);
    CHECK(!d10s_spectrum_app_set_noise_margin(&app, 1u));
    CHECK(!d10s_spectrum_app_set_noise_margin(&app, 65u));

    d10s_spectrum_app_stop(&app);
    CHECK(d10s_spectrum_app_tick(&app, now_ms) == D10S_APP_ERROR);
}

static void test_application_rejects_uninitialized_display(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144002000u, 3u, 1u}
    };
    fake_hardware_t hw = {0};
    const d10s_spectrum_radio_ops_t radio = {
        &hw, radio_tune, radio_read, NULL, NULL
    };
    d10s_st7565_t display = {0};
    d10s_spectrum_app_t app;

    CHECK(!d10s_spectrum_app_init(&app, bands, 1u, &radio, &display,
                                  NULL, 1u));
}

int main(void)
{
    test_complete_spectrum_application_pipeline();
    test_application_rejects_uninitialized_display();
    puts("Spectrum application integration tests: PASS");
    return 0;
}
