#include "spectrum_detect.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

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
        CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
        CHECK(result.candidate_valid);
        CHECK(result.candidate_streak == (uint8_t)(sweep + 1u));
        CHECK(result.confirmed == (sweep == 2u));
    }
    CHECK(result.peak_hold_valid);
    CHECK(result.peak_hold_level == 160u);
    CHECK(result.peak_hold_frequency_hz >= bins[10].frequency_hz - 6250u);
    CHECK(result.peak_hold_frequency_hz <= bins[10].frequency_hz + 6250u);
}

static void test_single_sweep_spike_is_not_confirmed(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];

    d10s_spectrum_init(&state, NULL);
    make_noise_sweep(bins, 21u, 90u);
    bins[8].rssi = 150u;
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    CHECK(result.candidate_valid);
    CHECK(!result.confirmed);
    make_noise_sweep(bins, 21u, 90u);
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    CHECK(!result.candidate_valid);
    CHECK(!result.confirmed);
    CHECK(!result.peak_hold_valid);
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
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    first_floor = result.noise_floor;
    for (i = 0u; i < 8u; ++i) {
        make_noise_sweep(bins, 21u, 120u);
        CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    }
    CHECK(result.noise_floor > first_floor);
    CHECK(result.noise_floor >= 110u);
}

static void test_one_noisy_sweep_does_not_drag_noise_floor(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    uint16_t baseline;

    d10s_spectrum_init(&state, NULL);
    make_noise_sweep(bins, 21u, 80u);
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    baseline = result.noise_floor;
    make_noise_sweep(bins, 21u, 130u);
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    make_noise_sweep(bins, 21u, 80u);
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    CHECK(result.noise_floor == baseline);
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
        CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    }
    CHECK(result.confirmed);
    CHECK(result.confirmed_level == 500u);
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
    CHECK(d10s_spectrum_process(&state, bins, 9u, &result));
    CHECK(result.confirmed);
    CHECK(result.confirmed_frequency_hz > bins[4].frequency_hz);
    CHECK(result.confirmed_frequency_hz < bins[5].frequency_hz);
}

static void test_rejects_invalid_frequency_grid(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[3] = {
        {100000u, 10u}, {110000u, 20u}, {105000u, 10u}
    };
    d10s_spectrum_init(&state, NULL);
    CHECK(!d10s_spectrum_process(&state, bins, 3u, &result));
}

static void test_band_reset_clears_old_noise_and_peak(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[21];
    unsigned sweep;

    d10s_spectrum_init(&state, NULL);
    for (sweep = 0u; sweep < 3u; ++sweep) {
        make_noise_sweep(bins, 21u, 80u);
        bins[10].rssi = 160u;
        CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    }
    CHECK(result.peak_hold_valid);
    d10s_spectrum_reset_tracking(&state);
    make_noise_sweep(bins, 21u, 120u);
    CHECK(d10s_spectrum_process(&state, bins, 21u, &result));
    CHECK(result.noise_floor >= 120u);
    CHECK(!result.peak_hold_valid);
    CHECK(!result.confirmed);
}

static void test_rejects_irregular_frequency_grid(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[4] = {
        {100000u, 10u}, {110000u, 11u}, {125000u, 12u}, {130000u, 11u}
    };
    d10s_spectrum_init(&state, NULL);
    CHECK(!d10s_spectrum_process(&state, bins, 4u, &result));
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
    test_band_reset_clears_old_noise_and_peak();
    puts("D10S Spectrum detector tests: PASS");
    return 0;
}
