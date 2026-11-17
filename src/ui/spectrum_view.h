#ifndef D10S_SPECTRUM_VIEW_H
#define D10S_SPECTRUM_VIEW_H

#include "spectrum_detect.h"
#include "waterfall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define D10S_VIEW_WIDTH 128u
#define D10S_VIEW_HEIGHT 64u
#define D10S_VIEW_MAX_BANDS 8u
#define D10S_VIEW_TEXT_BOLD 0x01u

typedef struct {
    void *context;
    void (*pixel)(void *context, uint8_t x, uint8_t y, bool on);
    void (*text)(void *context, uint8_t x, uint8_t y,
                 const char *value, uint8_t style);
} d10s_spectrum_canvas_t;

typedef struct {
    const d10s_spectrum_bin_t *bins;
    uint16_t count;
    uint8_t band_index;
    uint8_t band_count;
    uint8_t attenuation_level;
    bool turbo_enabled;
    bool hold_enabled;
    uint16_t noise_floor;
    uint16_t open_threshold;
    uint16_t noise_margin;
    bool peak_hold_valid;
    uint32_t peak_hold_frequency_hz;
    const d10s_spectrum_peak_t *peaks;
    uint8_t peak_count;
    bool cursor_valid;
    uint16_t cursor_bin;
    const char *band_label; /* Optional short label; keep it to five characters. */
    bool bold_labels;
    const d10s_waterfall_t *waterfall;
} d10s_spectrum_view_model_t;

bool d10s_spectrum_view_cursor_frequency(
                    const d10s_spectrum_view_model_t *model,
                    uint16_t cursor_bin,
                    uint32_t *frequency_hz);
bool d10s_spectrum_view_render(const d10s_spectrum_canvas_t *canvas,
                               const d10s_spectrum_view_model_t *model);

#endif
