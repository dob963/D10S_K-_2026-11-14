#include "spectrum_view.h"

#define GRAPH_LEFT 0u
#define GRAPH_RIGHT 127u
#define GRAPH_TOP 12u
#define GRAPH_BOTTOM 52u
#define GRAPH_HEIGHT (GRAPH_BOTTOM - GRAPH_TOP)

static void put_pixel(const d10s_spectrum_canvas_t *canvas,
                      int x, int y, bool on)
{
    if (x < 0 || x >= (int)D10S_VIEW_WIDTH ||
        y < 0 || y >= (int)D10S_VIEW_HEIGHT) return;
    canvas->pixel(canvas->context, (uint8_t)x, (uint8_t)y, on);
}

static void draw_line(const d10s_spectrum_canvas_t *canvas,
                      int x0, int y0, int x1, int y1, bool on)
{
    const int dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        int twice_error;
        put_pixel(canvas, x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void draw_dashed_vertical(const d10s_spectrum_canvas_t *canvas,
                                 uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1; ++y) {
        if (((y - y0) & 1u) == 0u) put_pixel(canvas, x, y, true);
    }
}

static void draw_dashed_horizontal(const d10s_spectrum_canvas_t *canvas,
                                   uint8_t y, uint8_t x0, uint8_t x1)
{
    uint8_t x;
    for (x = x0; x <= x1; ++x) {
        if (((x - x0) & 1u) == 0u) put_pixel(canvas, x, y, true);
    }
}

static uint16_t maximum_u16(uint16_t a, uint16_t b)
{
    return a > b ? a : b;
}

static uint8_t map_level_to_y(uint16_t value,
                              uint16_t min_level,
                              uint16_t max_level)
{
    uint32_t scaled;
    if (value <= min_level) return GRAPH_BOTTOM;
    if (value >= max_level) return GRAPH_TOP;
    scaled = ((uint32_t)(value - min_level) * GRAPH_HEIGHT) /
             (uint32_t)(max_level - min_level);
    return (uint8_t)(GRAPH_BOTTOM - scaled);
}

static uint8_t map_index_to_x(uint16_t index, uint16_t count)
{
    if (count <= 1u) return GRAPH_LEFT;
    return (uint8_t)(GRAPH_LEFT +
           ((uint32_t)index * (GRAPH_RIGHT - GRAPH_LEFT)) / (count - 1u));
}

static uint16_t nearest_bin_for_frequency(const d10s_spectrum_view_model_t *model,
                                          uint32_t frequency_hz)
{
    uint16_t best = 0u;
    uint32_t best_distance = UINT32_MAX;
    uint16_t i;
    for (i = 0u; i < model->count; ++i) {
        const uint32_t value = model->bins[i].frequency_hz;
        const uint32_t distance = value > frequency_hz
            ? value - frequency_hz : frequency_hz - value;
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

static void append_digit(char *buffer, uint8_t *length, uint8_t digit)
{
    buffer[*length] = (char)('0' + digit);
    ++(*length);
}

static void format_frequency(uint32_t frequency_hz, char *buffer)
{
    uint32_t mhz = frequency_hz / 1000000u;
    uint32_t khz = (frequency_hz % 1000000u) / 1000u;
    char digits[10];
    uint8_t count = 0u;
    uint8_t i;
    uint8_t output = 0u;

    do {
        digits[count] = (char)('0' + (mhz % 10u));
        ++count;
        mhz /= 10u;
    } while (mhz != 0u && count < (uint8_t)sizeof(digits));

    for (i = 0u; i < count; ++i) buffer[output++] = digits[count - i - 1u];
    buffer[output++] = '.';
    append_digit(buffer, &output, (uint8_t)(khz / 100u));
    append_digit(buffer, &output, (uint8_t)((khz / 10u) % 10u));
    append_digit(buffer, &output, (uint8_t)(khz % 10u));
    buffer[output] = '\0';
}

static void draw_header(const d10s_spectrum_canvas_t *canvas,
                        const d10s_spectrum_view_model_t *model)
{
    char label[8] = "B0/0";
    char att[5] = "A000";
    const uint8_t style = model->bold_labels ? D10S_VIEW_TEXT_BOLD : 0u;

    label[1] = (char)('1' + model->band_index);
    label[3] = (char)('0' + (model->band_count % 10u));
    att[1] = (char)('0' + ((model->attenuation_level / 100u) % 10u));
    att[2] = (char)('0' + ((model->attenuation_level / 10u) % 10u));
    att[3] = (char)('0' + (model->attenuation_level % 10u));

    if (canvas->text == 0) return;
    canvas->text(canvas->context, 0u, 1u, "D10S", style);
    canvas->text(canvas->context, 34u, 1u,
                 model->band_label != 0 ? model->band_label : label, style);
    canvas->text(canvas->context, 91u, 1u, att, style);
    if (model->turbo_enabled)
        canvas->text(canvas->context, 111u, 1u, "T", style);
}

bool d10s_spectrum_view_cursor_frequency(
                    const d10s_spectrum_view_model_t *model,
                    uint16_t cursor_bin,
                    uint32_t *frequency_hz)
{
    if (model == 0 || model->bins == 0 || frequency_hz == 0 ||
        model->count < 3u || cursor_bin >= model->count) return false;
    *frequency_hz = model->bins[cursor_bin].frequency_hz;
    return true;
}

bool d10s_spectrum_view_render(const d10s_spectrum_canvas_t *canvas,
                               const d10s_spectrum_view_model_t *model)
{
    uint16_t min_level;
    uint16_t max_level;
    uint16_t i;
    uint8_t previous_y = GRAPH_BOTTOM;
    uint16_t previous_x = 0u;
    uint16_t peak_bin = 0u;
    bool peak_x_valid = false;
    uint8_t style;
    uint32_t cursor_frequency_hz = 0u;
    char frequency_text[16];

    if (canvas == 0 || canvas->pixel == 0 || model == 0 ||
        model->bins == 0 || model->count < 3u ||
        model->count > D10S_SPECTRUM_MAX_BINS ||
        model->band_count == 0u || model->band_count > D10S_VIEW_MAX_BANDS ||
        model->band_index >= model->band_count ||
        model->noise_floor > D10S_SPECTRUM_MAX_RSSI ||
        model->open_threshold > D10S_SPECTRUM_MAX_RSSI) return false;
    if (model->cursor_valid && model->cursor_bin >= model->count) return false;

    for (i = 0u; i < model->count; ++i) {
        if (model->bins[i].rssi > D10S_SPECTRUM_MAX_RSSI) return false;
        if (i > 0u && model->bins[i].frequency_hz <=
                      model->bins[i - 1u].frequency_hz) return false;
    }

    min_level = model->noise_floor > 12u
        ? (uint16_t)(model->noise_floor - 12u) : 0u;
    max_level = maximum_u16(model->open_threshold, model->noise_floor);
    max_level = maximum_u16(max_level, (uint16_t)(min_level + 24u));
    for (i = 0u; i < model->count; ++i)
        max_level = maximum_u16(max_level, model->bins[i].rssi);
    max_level = maximum_u16(max_level,
                            (uint16_t)(model->open_threshold + 8u));
    if (max_level > D10S_SPECTRUM_MAX_RSSI)
        max_level = D10S_SPECTRUM_MAX_RSSI;
    if (max_level <= min_level)
        max_level = (uint16_t)(min_level + 1u);

    draw_header(canvas, model);

    /* Horizontal reference grid and a light baseline. */
    draw_dashed_horizontal(canvas, 22u, GRAPH_LEFT, GRAPH_RIGHT);
    draw_dashed_horizontal(canvas, 32u, GRAPH_LEFT, GRAPH_RIGHT);
    draw_dashed_horizontal(canvas, 42u, GRAPH_LEFT, GRAPH_RIGHT);
    draw_line(canvas, GRAPH_LEFT, GRAPH_BOTTOM, GRAPH_RIGHT,
              GRAPH_BOTTOM, true);

    /* Collapse 3..256 bins onto 128 columns without losing narrow peaks. */
    for (i = 0u; i < D10S_VIEW_WIDTH; ++i) {
        uint32_t first = ((uint32_t)i * (model->count - 1u)) /
                         (D10S_VIEW_WIDTH - 1u);
        uint32_t last = ((uint32_t)(i + 1u) * (model->count - 1u)) /
                        (D10S_VIEW_WIDTH - 1u);
        uint16_t j;
        uint16_t level = 0u;
        uint8_t y;
        if (last >= model->count) last = model->count - 1u;
        for (j = (uint16_t)first; j <= (uint16_t)last; ++j)
            level = maximum_u16(level, model->bins[j].rssi);
        y = map_level_to_y(level, min_level, max_level);
        if (i == 0u) {
            previous_y = y;
            previous_x = 0u;
        } else {
            draw_line(canvas, (int)previous_x, previous_y,
                      (int)i, y, true);
            previous_y = y;
            previous_x = i;
        }
    }

    /* Threshold uses the same relative RSSI scale as the sweep graph. */
    {
        const uint8_t threshold_y =
            map_level_to_y(model->open_threshold, min_level, max_level);
        draw_dashed_horizontal(canvas, threshold_y, GRAPH_LEFT, GRAPH_RIGHT);
    }

    if (model->peak_hold_valid) {
        peak_bin = nearest_bin_for_frequency(model,
                                              model->peak_hold_frequency_hz);
        peak_x_valid = true;
        draw_dashed_vertical(canvas, map_index_to_x(peak_bin, model->count),
                             GRAPH_TOP, GRAPH_BOTTOM);
    }
    if (model->cursor_valid) {
        const uint8_t cursor_x = map_index_to_x(model->cursor_bin,
                                                model->count);
        draw_line(canvas, cursor_x, GRAPH_TOP, cursor_x, GRAPH_BOTTOM, true);
        if (canvas->text != 0 &&
            d10s_spectrum_view_cursor_frequency(model, model->cursor_bin,
                                                &cursor_frequency_hz)) {
            /* The text is formatted from the selected snapshot's stored bin. */
            format_frequency(cursor_frequency_hz, frequency_text);
            canvas->text(canvas->context, 0u, 56u, frequency_text,
                         model->bold_labels ? D10S_VIEW_TEXT_BOLD : 0u);
        }
    } else if (peak_x_valid && canvas->text != 0) {
        format_frequency(model->peak_hold_frequency_hz, frequency_text);
        canvas->text(canvas->context, 0u, 56u, frequency_text,
                     model->bold_labels ? D10S_VIEW_TEXT_BOLD : 0u);
    }

    style = model->bold_labels ? D10S_VIEW_TEXT_BOLD : 0u;
    if (canvas->text != 0)
        canvas->text(canvas->context, 91u, 56u, "RSSI", style);
    return true;
}
