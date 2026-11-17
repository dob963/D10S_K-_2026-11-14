#include "waterfall.h"

#include <stddef.h>

static const uint8_t bayer4[4][4] = {
    {0u, 8u, 2u, 10u},
    {12u, 4u, 14u, 6u},
    {3u, 11u, 1u, 9u},
    {15u, 7u, 13u, 5u}
};

void d10s_waterfall_init(d10s_waterfall_t *w)
{
    if (w == NULL) return;
    for (uint8_t y = 0u; y < D10S_WATERFALL_HEIGHT; ++y)
        for (uint8_t b = 0u; b < D10S_WATERFALL_ROW_BYTES; ++b)
            w->rows[y][b] = 0u;
    w->head = 0u;
    w->count = 0u;
}

bool d10s_waterfall_push(d10s_waterfall_t *w,
                         const d10s_spectrum_bin_t *bins,
                         uint16_t count,
                         uint16_t noise_floor,
                         uint16_t full_scale)
{
    uint8_t x;
    if (w == NULL || bins == NULL || count < 2u ||
        count > D10S_SPECTRUM_MAX_BINS) return false;
    if (full_scale <= noise_floor) {
        if (noise_floor >= D10S_SPECTRUM_MAX_RSSI) return false;
        full_scale = (uint16_t)(noise_floor + 1u);
    }

    for (x = 0u; x < D10S_WATERFALL_WIDTH; ++x) {
        uint32_t first = ((uint32_t)x * count) / D10S_WATERFALL_WIDTH;
        uint32_t last = ((uint32_t)(x + 1u) * count) /
                        D10S_WATERFALL_WIDTH;
        uint16_t level = 0u;
        uint8_t intensity;
        if (last <= first) last = first + 1u;
        if (last > count) last = count;
        for (uint32_t i = first; i < last; ++i)
            if (bins[i].rssi > level) level = bins[i].rssi;

        if (level <= noise_floor) intensity = 0u;
        else if (level >= full_scale) intensity = 15u;
        else intensity = (uint8_t)(((uint32_t)(level - noise_floor) * 15u) /
                                    (full_scale - noise_floor));

        {
            const uint8_t threshold = bayer4[w->head & 3u][x & 3u];
            const uint8_t mask = (uint8_t)(1u << (x & 7u));
            if (intensity > threshold)
                w->rows[w->head][x >> 3] |= mask;
            else
                w->rows[w->head][x >> 3] &= (uint8_t)~mask;
        }
    }

    w->head = (uint8_t)((w->head + 1u) % D10S_WATERFALL_HEIGHT);
    if (w->count < D10S_WATERFALL_HEIGHT) ++w->count;
    return true;
}

bool d10s_waterfall_pixel(const d10s_waterfall_t *w,
                          uint8_t chronological_row,
                          uint8_t x)
{
    uint8_t oldest;
    uint8_t physical;
    if (w == NULL || chronological_row >= w->count ||
        x >= D10S_WATERFALL_WIDTH) return false;
    oldest = (uint8_t)((w->head + D10S_WATERFALL_HEIGHT - w->count) %
                       D10S_WATERFALL_HEIGHT);
    physical = (uint8_t)((oldest + chronological_row) %
                         D10S_WATERFALL_HEIGHT);
    return (w->rows[physical][x >> 3] & (uint8_t)(1u << (x & 7u))) != 0u;
}
