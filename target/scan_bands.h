#ifndef D10S_TARGET_SCAN_BANDS_H
#define D10S_TARGET_SCAN_BANDS_H

#include "spectrum_plan.h"

/*
 * Initial receive-only sweep spans. Each range uses 129 points (128 exact
 * intervals), so the generated frequency grid is exactly uniform in Hz.
 */
static const d10s_spectrum_band_t D10S_SCAN_BANDS[] = {
    {144000000u, 146000000u, 129u, 1u},
    {430000000u, 440000000u, 129u, 1u}
};

#define D10S_SCAN_BAND_COUNT \
    ((uint8_t)(sizeof(D10S_SCAN_BANDS) / sizeof(D10S_SCAN_BANDS[0])))

#endif
