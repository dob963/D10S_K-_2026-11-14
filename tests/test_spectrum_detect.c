#include "spectrum_detect.h"

#include <assert.h>
#include <stdio.h>

static void make_noise_sweep(d10s_spectrum_bin_t *bins, size_t count, uint16_t base)
{
    size_t i;
    for (i = 0u; i < count; ++i) {
        bins[i].frequency_hz = 400000000u + (uint32_t)i * 12500u;
        bins[i].rssi = (uint16_t)(base + (i % 3u));
    }
}

static void test_confirmation_and_peak_hold(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    unsigned sweep;

    d10s_spectrum_init(&state, NULL);
    for (sweep = 0u; sweep < 3u; ++sweep) {
        make_noise_sweep(bins, 21u, 100u);
        bins[10].rssi = 160u;
        assert(d10s_spectrum_process(&state, bins, 21u, &result));
        assert(result.candidate_valid);
        assert(result.candidate_streak == (uint8_t)(sweep + 1u));
        assert(result.confirmed == (sweep == 2u));
    }
    assert(result.peak_hold_valid);
    assert(result.peak_hold_level == 160u);
    assert(result.peak_hold_frequency_hz >= bins[10].frequency_hz - 6250u);
    assert(result.peak_hold_frequency_hz <= bins[10].frequency_hz + 6250u);
}

static void test_single_sweep_spike_is_not_confirmed(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];

    d10s_spectrum_init(&state, NULL);
    make_noise_sweep(bins, 21u, 90u);
    bins[8].rssi = 150u;
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    assert(result.candidate_valid);
    assert(!result.confirmed);
    make_noise_sweep(bins, 21u, 90u);
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    assert(!result.candidate_valid);
    assert(!result.confirmed);
    assert(!result.peak_hold_valid);
}

static void test_noise_floor_tracks_background(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    uint16_t first_floor;
    unsigned i;

    d10s_spectrum_init(&state, NULL);
    make_noise_sweep(bins, 21u, 80u);
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    first_floor = result.noise_floor;
    for (i = 0u; i < 8u; ++i) {
        make_noise_sweep(bins, 21u, 120u);
        assert(d10s_spectrum_process(&state, bins, 21u, &result));
    }
    assert(result.noise_floor > first_floor);
    assert(result.noise_floor >= 110u);
}

static void test_one_noisy_sweep_does_not_drag_noise_floor(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    uint16_t baseline;

    d10s_spectrum_init(&state, NULL);
    make_noise_sweep(bins, 21u, 80u);
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    baseline = result.noise_floor;
    make_noise_sweep(bins, 21u, 130u);
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    make_noise_sweep(bins, 21u, 80u);
    assert(d10s_spectrum_process(&state, bins, 21u, &result));
    assert(result.noise_floor == baseline);
}

static void test_nine_bit_rssi_is_preserved(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    unsigned sweep;

    d10s_spectrum_init(&state, NULL);
    for (sweep = 0u; sweep < 3u; ++sweep) {
        make_noise_sweep(bins, 21u, 200u);
        bins[10].rssi = 500u;
        assert(d10s_spectrum_process(&state, bins, 21u, &result));
    }
    assert(result.confirmed);
    assert(result.confirmed_level == 500u);
}

static void test_sub_bin_frequency_refinement(void)
{
    d10s_spectrum_config_t config;
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[9];
    size_t i;

    d10s_spectrum_default_config(&config);
    config.confirm_sweeps = 1u;
    d10s_spectrum_init(&state, &config);
    for (i = 0u; i < 9u; ++i) {
        bins[i].frequency_hz = 145000000u + (uint32_t)i * 12500u;
        bins[i].rssi = 100u;
    }
    bins[3].rssi = 120u;
    bins[4].rssi = 150u;
    bins[5].rssi = 140u;
    assert(d10s_spectrum_process(&state, bins, 9u, &result));
    assert(result.confirmed);
    assert(result.confirmed_frequency_hz > bins[4].frequency_hz);
    assert(result.confirmed_frequency_hz < bins[5].frequency_hz);
}

static void test_rejects_invalid_frequency_grid(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[3] = {
        {100000u, 10u}, {110000u, 20u}, {105000u, 10u}
    };
    d10s_spectrum_init(&state, NULL);
    assert(!d10s_spectrum_process(&state, bins, 3u, &result));
}

static void test_rejects_irregular_frequency_grid(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[4] = {
        {100000u, 10u}, {110000u, 11u}, {125000u, 12u}, {130000u, 11u}
    };
    d10s_spectrum_init(&state, NULL);
    assert(!d10s_spectrum_process(&state, bins, 4u, &result));
}

int main(void)
{
    test_confirmation_and_peak_hold();
    test_single_sweep_spike_is_not_confirmed();
    test_noise_floor_tracks_background();
    test_one_noisy_sweep_does_not_drag_noise_floor();
    test_nine_bit_rssi_is_preserved();
    test_sub_bin_frequency_refinement();
    test_rejects_invalid_frequency_grid();
    test_rejects_irregular_frequency_grid();
    puts("D10S Spectrum detector tests: PASS");
    return 0;
}
