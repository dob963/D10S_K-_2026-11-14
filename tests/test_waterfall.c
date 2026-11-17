#include "waterfall.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    d10s_waterfall_t waterfall;
    d10s_spectrum_bin_t bins[128];
    unsigned x;
    unsigned lit = 0u;

    for (x = 0u; x < 128u; ++x) {
        bins[x].frequency_hz = 145000000u + x * 12500u;
        bins[x].rssi = 200u;
    }
    d10s_waterfall_init(&waterfall);
    assert(waterfall.count == 0u);
    assert(d10s_waterfall_push(&waterfall, bins, 128u, 100u, 200u));
    assert(waterfall.count == 1u);
    for (x = 0u; x < 128u; ++x)
        if (d10s_waterfall_pixel(&waterfall, 0u, (uint8_t)x)) ++lit;
    assert(lit == 128u);

    for (unsigned sweep = 0u; sweep < 18u; ++sweep)
        assert(d10s_waterfall_push(&waterfall, bins, 128u, 100u, 200u));
    assert(waterfall.count == D10S_WATERFALL_HEIGHT);
    assert(d10s_waterfall_pixel(&waterfall, 15u, 0u));
    bins[0].rssi = 100u;
    assert(d10s_waterfall_push(&waterfall, bins, 128u, 100u, 200u));
    assert(!d10s_waterfall_pixel(&waterfall, 15u, 0u));
    assert(!d10s_waterfall_push(&waterfall, bins, 1u, 100u, 200u));
    puts("Waterfall 4x4 tests: PASS");
    return 0;
}
