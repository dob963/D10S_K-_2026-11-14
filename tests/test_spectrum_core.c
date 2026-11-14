#include "spectrum_core.h"

#include <assert.h>
#include <stdio.h>

static void test_noise_percentile(void)
{
    d10s_noise_floor_t noise;
    d10s_noise_floor_init(&noise);
    assert(d10s_noise_floor_percentile(&noise, 25u) == 0u);

    d10s_noise_floor_add(&noise, 40u);
    d10s_noise_floor_add(&noise, 10u);
    d10s_noise_floor_add(&noise, 30u);
    d10s_noise_floor_add(&noise, 20u);
    assert(d10s_noise_floor_percentile(&noise, 25u) == 10u);
    assert(d10s_noise_floor_threshold(&noise, 12u) == 22u);
}

static void test_noise_window_rollover(void)
{
    d10s_noise_floor_t noise;
    unsigned i;
    d10s_noise_floor_init(&noise);

    for (i = 0u; i < D10S_NOISE_WINDOW; i++) {
        d10s_noise_floor_add(&noise, 20u);
    }
    d10s_noise_floor_add(&noise, 80u);
    assert(noise.count == D10S_NOISE_WINDOW);
    assert(d10s_noise_floor_percentile(&noise, 25u) == 20u);
}

static void test_peak_requires_two_passes(void)
{
    d10s_peak_tracker_t peak;
    d10s_peak_tracker_init(&peak);

    assert(!d10s_peak_tracker_update(&peak, 70u, 50u, 145500000u));
    assert(d10s_peak_tracker_update(&peak, 72u, 50u, 145500000u));
    assert(!d10s_peak_tracker_update(&peak, 71u, 50u, 145600000u));
    assert(!d10s_peak_tracker_update(&peak, 20u, 50u, 145600000u));
    assert(peak.consecutive_passes == 0u);
}

int main(void)
{
    test_noise_percentile();
    test_noise_window_rollover();
    test_peak_requires_two_passes();
    puts("D10S Spectrum core tests: PASS");
    return 0;
}
