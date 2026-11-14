#include "scan_bands.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

int main(void)
{
    d10s_spectrum_plan_t plan;
    uint8_t band_index;

    CHECK(D10S_SCAN_BAND_COUNT == 2u);
    CHECK(d10s_spectrum_plan_init(&plan, D10S_SCAN_BANDS,
                                  D10S_SCAN_BAND_COUNT));
    for (band_index = 0u; band_index < D10S_SCAN_BAND_COUNT; ++band_index) {
        const d10s_spectrum_band_t *band = &D10S_SCAN_BANDS[band_index];
        const uint32_t step = (band->stop_hz - band->start_hz) /
                              ((uint32_t)band->bin_count - 1u);
        uint32_t first = 0u;
        uint32_t last = 0u;
        CHECK(d10s_spectrum_band_is_valid(band));
        CHECK(band->bin_count == 129u);
        CHECK(d10s_spectrum_plan_select_band(&plan, band_index) ==
              (band_index != 0u));
        CHECK(d10s_spectrum_plan_frequency(&plan, 0u, &first));
        CHECK(d10s_spectrum_plan_frequency(&plan, 128u, &last));
        CHECK(first == band->start_hz);
        CHECK(last == band->stop_hz);
        CHECK(step != 0u);
    }
    puts("Target scan-band configuration tests: PASS");
    return 0;
}
