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
        {144000000u, 144004000u, 5u, 1u}
    };
    fake_hardware_t hw = {0};
    const d10s_st7565_port_t lcd_port = {
        lcd_select, lcd_mode, lcd_write, lcd_delay, &hw
    };
    const d10s_st7565_config_t lcd_config = {
        4u, 31u, 4u, 7u, false, false, false, 1u, 1u
    };
    const d10s_spectrum_radio_ops_t radio = {
        &hw, radio_tune, radio_read, NULL
    };
    d10s_st7565_t display;
    d10s_spectrum_app_t app;
    uint32_t now_ms;
    uint32_t updated_frames = 0u;

    hw.first_hz = 144000000u;
    CHECK(d10s_st7565_init(&display, &lcd_port, &lcd_config));
    hw.bytes_written = 0u;
    CHECK(d10s_spectrum_app_init(&app, bands, 1u, &radio, &display,
                                 NULL, 1u));
    CHECK(d10s_spectrum_app_set_cursor(&app, true, 2u));
    CHECK(d10s_spectrum_app_set_bold_labels(&app, true));

    for (now_ms = 0u; now_ms < 45u; ++now_ms) {
        const d10s_spectrum_app_status_t status =
            d10s_spectrum_app_tick(&app, now_ms);
        CHECK(status != D10S_APP_ERROR);
        if (status == D10S_APP_FRAME_UPDATED) ++updated_frames;
    }

    CHECK(updated_frames == 3u);
    CHECK(hw.tune_calls == 15u);
    CHECK(hw.read_calls == 15u);
    CHECK(hw.bytes_written == 3u * 8u * 131u);
    CHECK(!hw.selected);
    CHECK(app.last_result.confirmed);
    CHECK(app.last_result.confirmed_level == 180u);
    CHECK(app.framebuffer.bytes[1] != 0u);
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
        &hw, radio_tune, radio_read, NULL
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
