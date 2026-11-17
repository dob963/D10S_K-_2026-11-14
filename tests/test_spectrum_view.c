#include "spectrum_view.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char attenuation[8];
    char center[16];
    char span[8];
    char rbw[8];
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
    if (y == 56u && x == 0u)
        (void)strncpy(capture->center, value, sizeof(capture->center) - 1u);
    if (y == 56u && x == 50u)
        (void)strncpy(capture->span, value, sizeof(capture->span) - 1u);
    if (y == 56u && x == 85u)
        (void)strncpy(capture->rbw, value, sizeof(capture->rbw) - 1u);
}

static void check_stage(uint8_t level, const char *expected)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 90u}, {144001000u, 100u}, {144002000u, 110u}
    };
    capture_t capture = {{0}, {0}, {0}, {0}, 0u, 0u};
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
    assert(capture.text_calls >= 6u);
    assert(strcmp(capture.center, "144.001") == 0);
    assert(strcmp(capture.span, "S2K") == 0);
    assert(strcmp(capture.rbw, "R1K") == 0);
}



int main(void)
{
    check_stage(0u, "AT0");
    check_stage(5u, "AT5");
    return 0;
}
