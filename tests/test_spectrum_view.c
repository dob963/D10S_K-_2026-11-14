#include "spectrum_view.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char attenuation[8];
    uint8_t attenuation_style;
    unsigned text_calls;
} capture_t;

static void pixel(void *context, uint8_t x, uint8_t y, bool on)
{
    (void)context; (void)x; (void)y; (void)on;
}

static void text(void *context, uint8_t x, uint8_t y,
                 const char *value, uint8_t style)
{
    capture_t *capture = (capture_t *)context;
    ++capture->text_calls;
    if (x == 91u && y == 1u) {
        (void)strncpy(capture->attenuation, value,
                      sizeof(capture->attenuation) - 1u);
        capture->attenuation_style = style;
    }
}

static void check_stage(uint8_t level, const char *expected)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 90u}, {144001000u, 100u}, {144002000u, 110u}
    };
    capture_t capture = {{0}, 0u, 0u};
    const d10s_spectrum_canvas_t canvas = {&capture, pixel, text};
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 3u, .band_index = 0u, .band_count = 2u,
        .attenuation_level = level, .noise_floor = 90u,
        .open_threshold = 102u, .noise_margin = 12u,
        .bold_labels = true
    };

    assert(d10s_spectrum_view_render(&canvas, &model));
    assert(strcmp(capture.attenuation, expected) == 0);
    assert(capture.attenuation_style == D10S_VIEW_TEXT_BOLD);
    assert(capture.text_calls >= 3u);
}


static void test_spectrum_view_marks_each_confirmed_peak(void)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 100u}, {144001000u, 140u}, {144002000u, 100u},
        {144003000u, 150u}, {144004000u, 100u}
    };
    const d10s_spectrum_peak_t peaks[] = {
        {true, 144001000u, 140u, 1u, 3u, true},
        {true, 144003000u, 150u, 3u, 3u, true}
    };
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 5u, .band_index = 0u, .band_count = 1u,
        .noise_floor = 100u, .open_threshold = 120u,
        .peaks = peaks, .peak_count = 2u
    };
    fake_canvas_t fake = {0};
    const d10s_spectrum_canvas_t canvas = {
        &fake, fake_canvas_pixel, fake_canvas_text
    };

    CHECK(d10s_spectrum_view_render(&canvas, &model));
    CHECK(fake.pixels[12u][31u]);
    CHECK(fake.pixels[12u][95u]);
}

int main(void)
{
    check_stage(0u, "AT0");
    check_stage(5u, "AT5");
    return 0;
}
