#ifndef D10S_WATERFALL_H
#define D10S_WATERFALL_H

#include "spectrum_detect.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_WATERFALL_WIDTH 128u
#define D10S_WATERFALL_HEIGHT 16u
#define D10S_WATERFALL_ROW_BYTES 16u

typedef struct {
    uint8_t rows[D10S_WATERFALL_HEIGHT][D10S_WATERFALL_ROW_BYTES];
    uint8_t head;
    uint8_t count;
} d10s_waterfall_t;

void d10s_waterfall_init(d10s_waterfall_t *waterfall);
/* Aggiunge una sweep; il livello viene convertito in 16 livelli con dithering 4x4. */
bool d10s_waterfall_push(d10s_waterfall_t *waterfall,
                         const d10s_spectrum_bin_t *bins,
                         uint16_t count,
                         uint16_t noise_floor,
                         uint16_t full_scale);
bool d10s_waterfall_pixel(const d10s_waterfall_t *waterfall,
                          uint8_t chronological_row,
                          uint8_t x);

#endif
