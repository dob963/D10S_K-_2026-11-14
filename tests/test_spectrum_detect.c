#include "spectrum_detect.h"
#include "spectrum_scanner.h"
#include "spectrum_view.h"
#include "mono128x64.h"
#include "spectrum_plan.h"

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

static void test_rejects_rssi_above_nine_bit_range(void)
{
    d10s_spectrum_state_t state;
    d10s_spectrum_result_t result;
    d10s_spectrum_bin_t bins[3] = {
        {100000u, 100u}, {110000u, 512u}, {120000u, 100u}
    };
    d10s_spectrum_init(&state, NULL);
    CHECK(!d10s_spectrum_process(&state, bins, 3u, &result));
    CHECK(!state.noise_initialized);
}

static void test_band_grid_boundaries(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144100000u, 9u, 2u}
    };
    d10s_spectrum_plan_t plan;
    uint32_t frequency = 0u;

    CHECK(d10s_spectrum_plan_init(&plan, bands, 1u));
    CHECK(d10s_spectrum_plan_frequency(&plan, 0u, &frequency));
    CHECK(frequency == 144000000u);
    CHECK(d10s_spectrum_plan_frequency(&plan, 4u, &frequency));
    CHECK(frequency == 144050000u);
    CHECK(d10s_spectrum_plan_frequency(&plan, 8u, &frequency));
    CHECK(frequency == 144100000u);
    CHECK(!d10s_spectrum_plan_frequency(&plan, 9u, &frequency));
}

static void test_multi_band_dwell_and_rotation(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144070000u, 8u, 2u},
        {430000000u, 430070000u, 8u, 1u}
    };
    d10s_spectrum_plan_t plan;

    CHECK(d10s_spectrum_plan_init(&plan, bands, 2u));
    CHECK(!d10s_spectrum_plan_complete_sweep(&plan));
    CHECK(plan.active_band == 0u);
    CHECK(d10s_spectrum_plan_complete_sweep(&plan));
    CHECK(plan.active_band == 1u);
    CHECK(plan.sweeps_on_band == 0u);
    CHECK(d10s_spectrum_plan_complete_sweep(&plan));
    CHECK(plan.active_band == 0u);
}

static void test_invalid_band_definitions(void)
{
    const d10s_spectrum_band_t too_few = {100u, 200u, 2u, 1u};
    const d10s_spectrum_band_t irregular = {100u, 210u, 4u, 1u};
    const d10s_spectrum_band_t no_dwell = {100u, 200u, 3u, 0u};
    const d10s_spectrum_band_t reversed = {200u, 100u, 3u, 1u};
    d10s_spectrum_plan_t plan;

    CHECK(!d10s_spectrum_band_is_valid(&too_few));
    CHECK(!d10s_spectrum_band_is_valid(&irregular));
    CHECK(!d10s_spectrum_band_is_valid(&no_dwell));
    CHECK(!d10s_spectrum_band_is_valid(&reversed));
    CHECK(!d10s_spectrum_plan_init(&plan, &too_few, 1u));
}

static void test_manual_band_change_resets_dwell(void)
{
    const d10s_spectrum_band_t bands[] = {
        {100u, 200u, 3u, 3u},
        {300u, 400u, 3u, 1u}
    };
    d10s_spectrum_plan_t plan;

    CHECK(d10s_spectrum_plan_init(&plan, bands, 2u));
    CHECK(!d10s_spectrum_plan_complete_sweep(&plan));
    CHECK(plan.sweeps_on_band == 1u);
    CHECK(d10s_spectrum_plan_select_band(&plan, 1u));
    CHECK(plan.sweeps_on_band == 0u);
    CHECK(!d10s_spectrum_plan_select_band(&plan, 1u));
    CHECK(d10s_spectrum_plan_complete_sweep(&plan));
    CHECK(plan.active_band == 0u);
}

typedef struct {
    uint32_t frequency_hz;
    uint32_t first_hz;
    uint32_t step_hz;
    uint32_t tune_calls;
    uint32_t read_calls;
    uint16_t noise_level;
    uint16_t peak_level;
    bool fail_tune;
    bool fail_read;
    bool fail_att;
    uint8_t attenuation_level;
    uint8_t read_jitter;
} fake_radio_t;

static bool fake_radio_tune(void *context, uint32_t requested_hz,
                            uint32_t *actual_hz)
{
    fake_radio_t *radio = (fake_radio_t *)context;
    if (radio->fail_tune || actual_hz == 0) return false;
    radio->frequency_hz = requested_hz;
    *actual_hz = requested_hz;
    ++radio->tune_calls;
    return true;
}

static bool fake_radio_read(void *context, uint16_t *rssi)
{
    fake_radio_t *radio = (fake_radio_t *)context;
    uint32_t index;
    if (radio->fail_read || rssi == 0) return false;
    index = (radio->frequency_hz - radio->first_hz) / radio->step_hz;
    *rssi = index == 2u ? radio->peak_level :
            (uint16_t)(radio->noise_level + (index & 1u));
    if (radio->read_jitter != 0u && (radio->read_calls & 1u) == 0u)
        *rssi = (uint16_t)(*rssi + radio->read_jitter);
    ++radio->read_calls;
    return true;
}

static bool fake_radio_set_att(void *context, uint8_t level)
{
    fake_radio_t *radio = (fake_radio_t *)context;
    if (radio->fail_att) return false;
    radio->attenuation_level = level;
    return true;
}

static d10s_spectrum_scanner_status_t fake_run_until_sweep(
        d10s_spectrum_scanner_t *scanner,
        const d10s_spectrum_radio_ops_t *ops,
        uint32_t *now_ms,
        d10s_spectrum_result_t *result,
        bool *band_changed)
{
    unsigned i;
    d10s_spectrum_scanner_status_t status = D10S_SCANNER_BUSY;
    for (i = 0u; i < 100u; ++i) {
        status = d10s_spectrum_scanner_tick(scanner, ops, *now_ms,
                                             result, band_changed);
        *now_ms += 1u;
        if (status != D10S_SCANNER_BUSY) return status;
    }
    return status;
}

static void test_scanner_acquires_and_confirms_signal(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 2u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 100u, .peak_level = 160u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;
    unsigned sweep;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 3u));
    for (sweep = 0u; sweep < 3u; ++sweep) {
        CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                                   &result, &changed) == D10S_SCANNER_SWEEP_READY);
        CHECK(!changed);
    }
    CHECK(result.confirmed);
    CHECK(result.confirmed_level == 160u);
    CHECK(fake.tune_calls == 15u);
    CHECK(fake.read_calls == 15u);
}

static void test_scanner_resets_detector_on_band_change(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u},
        {430000000u, 430004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 2u, NULL, 1u));
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(changed);
    CHECK(scanner.plan.active_band == 1u);
    CHECK(!scanner.detector.noise_initialized);
    CHECK(!scanner.detector.peak_hold_valid);
}

static void test_scanner_rejects_radio_failures(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u, .fail_tune = true
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 2u));
    CHECK(d10s_spectrum_scanner_tick(&scanner, &ops, 0u,
                                     &result, &changed) == D10S_SCANNER_ERROR);
    CHECK(d10s_spectrum_scanner_tick(&scanner, &ops, 1u,
                                     &result, &changed) == D10S_SCANNER_ERROR);
}

static void test_scanner_att_compensation_and_boundary_rules(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 2u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 100u, .peak_level = 160u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 1u));
    CHECK(d10s_spectrum_scanner_set_attenuation(&scanner, &ops, 2u, 20u));
    CHECK(fake.attenuation_level == 2u);
    CHECK(scanner.attenuation_configured);
    CHECK(scanner.rssi_compensation == 20u);
    CHECK(d10s_spectrum_scanner_tick(&scanner, &ops, 0u,
                                     &result, &changed) == D10S_SCANNER_BUSY);
    CHECK(!d10s_spectrum_scanner_set_attenuation(&scanner, &ops, 1u, 10u));
    CHECK(fake.attenuation_level == 2u);
    now_ms = 1u;
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(result.candidate_valid);
    CHECK(result.candidate_level == 180u);

    CHECK(d10s_spectrum_scanner_set_attenuation(&scanner, &ops, 1u, 10u));
    CHECK(fake.attenuation_level == 1u);
    CHECK(!scanner.detector.noise_initialized);
    CHECK(!scanner.detector.peak_hold_valid);
}

static void test_scanner_att_failure_does_not_change_state(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 2u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 100u, .peak_level = 160u, .fail_att = true
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 1u));
    CHECK(!d10s_spectrum_scanner_set_attenuation(&scanner, &ops, 3u, 15u));
    CHECK(!scanner.attenuation_configured);
    CHECK(scanner.rssi_compensation == 0u);
}

static void test_scanner_publishes_only_complete_sweeps(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u},
        {430000000u, 430004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    const d10s_spectrum_bin_t *snapshot = 0;
    uint16_t count = 0u;
    uint8_t band_index = 0u;
    uint8_t att = 0u;
    uint32_t generation = 0u;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 2u, NULL, 1u));
    CHECK(!d10s_spectrum_scanner_snapshot(&scanner, &snapshot, &count,
                                          &band_index, &att, &generation));
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(changed);
    CHECK(d10s_spectrum_scanner_snapshot(&scanner, &snapshot, &count,
                                         &band_index, &att, &generation));
    CHECK(count == 5u);
    CHECK(band_index == 0u);
    CHECK(att == 0u);
    CHECK(generation == 1u);
    CHECK(snapshot[0].frequency_hz == 144000000u);
    CHECK(snapshot[4].frequency_hz == 144004000u);

    /* The following sweep is in progress: published data must not be overwritten. */
    CHECK(d10s_spectrum_scanner_tick(&scanner, &ops, now_ms,
                                     &result, &changed) == D10S_SCANNER_BUSY);
    CHECK(d10s_spectrum_scanner_snapshot(&scanner, &snapshot, &count,
                                         &band_index, &att, &generation));
    CHECK(generation == 1u);
    CHECK(band_index == 0u);
    CHECK(snapshot[0].frequency_hz == 144000000u);
}

static void test_scanner_manual_band_change_invalidates_snapshot(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u},
        {430000000u, 430004000u, 5u, 1u}
    };
    d10s_spectrum_scanner_t scanner;
    const d10s_spectrum_bin_t *snapshot = 0;
    uint16_t count = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 2u, NULL, 1u));
    scanner.published_valid = true;
    CHECK(d10s_spectrum_scanner_select_band(&scanner, 1u));
    CHECK(scanner.plan.active_band == 1u);
    CHECK(!scanner.detector.noise_initialized);
    CHECK(!d10s_spectrum_scanner_snapshot(&scanner, &snapshot, &count,
                                          0, 0, 0));
    CHECK(!d10s_spectrum_scanner_select_band(&scanner, 1u));
}

static void test_scanner_delay_changes_only_between_sweeps(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 5u));
    CHECK(d10s_spectrum_scanner_set_delay(&scanner, 2u));
    CHECK(scanner.settle_ms == 2u);
    CHECK(d10s_spectrum_scanner_tick(&scanner, &ops, now_ms,
                                     &result, &changed) == D10S_SCANNER_BUSY);
    CHECK(d10s_spectrum_scanner_set_delay(&scanner, 8u));
    CHECK(scanner.settle_ms == 2u);
    CHECK(scanner.pending_delay_valid);
    now_ms += 2u;
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(!scanner.pending_delay_valid);
    CHECK(d10s_spectrum_scanner_set_delay(&scanner, 8u));
    CHECK(scanner.settle_ms == 8u);
    CHECK(!d10s_spectrum_scanner_set_delay(&scanner, 0u));
}

static void test_turbo_exits_early_when_rssi_stabilizes(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 40u));
    CHECK(!d10s_spectrum_scanner_set_turbo(&scanner, true, 40u, 1u, 0u, 2u));
    CHECK(!d10s_spectrum_scanner_set_turbo(&scanner, true, 2u, 0u, 0u, 2u));
    CHECK(!d10s_spectrum_scanner_set_turbo(&scanner, true, 2u, 1u, 0u, 1u));
    CHECK(d10s_spectrum_scanner_set_turbo(&scanner, true, 2u, 1u, 0u, 2u));
    CHECK(!d10s_spectrum_scanner_set_delay(&scanner, 2u));
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(fake.read_calls == 10u);
    CHECK(now_ms < 100u);
    CHECK(scanner.turbo_enabled);
    CHECK(d10s_spectrum_scanner_set_turbo(&scanner, false, 0u, 0u, 0u, 0u));
    CHECK(!scanner.turbo_enabled);
}

static void test_turbo_honors_max_delay_when_rssi_never_stabilizes(void)
{
    const d10s_spectrum_band_t bands[] = {
        {144000000u, 144004000u, 5u, 1u}
    };
    fake_radio_t fake = {
        .first_hz = 144000000u, .step_hz = 1000u,
        .noise_level = 90u, .peak_level = 150u, .read_jitter = 1u
    };
    const d10s_spectrum_radio_ops_t ops = {
        &fake, fake_radio_tune, fake_radio_read, fake_radio_set_att
    };
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_result_t result;
    bool changed = false;
    uint32_t now_ms = 0u;

    CHECK(d10s_spectrum_scanner_init(&scanner, bands, 1u, NULL, 8u));
    CHECK(d10s_spectrum_scanner_set_turbo(&scanner, true, 2u, 1u, 0u, 2u));
    CHECK(fake_run_until_sweep(&scanner, &ops, &now_ms,
                               &result, &changed) == D10S_SCANNER_SWEEP_READY);
    CHECK(fake.read_calls == 20u);
    CHECK(now_ms >= 25u);
}

typedef struct {
    bool pixels[D10S_VIEW_HEIGHT][D10S_VIEW_WIDTH];
    uint32_t pixel_writes;
    uint32_t text_writes;
    bool saw_bold;
    bool saw_frequency;
    bool saw_band;
    bool saw_att;
    bool saw_turbo;
} fake_canvas_t;

static void fake_canvas_pixel(void *context, uint8_t x, uint8_t y, bool on)
{
    fake_canvas_t *canvas = (fake_canvas_t *)context;
    canvas->pixels[y][x] = on;
    ++canvas->pixel_writes;
}

static void fake_canvas_text(void *context, uint8_t x, uint8_t y,
                             const char *value, uint8_t style)
{
    fake_canvas_t *canvas = (fake_canvas_t *)context;
    (void)x;
    (void)y;
    ++canvas->text_writes;
    if ((style & D10S_VIEW_TEXT_BOLD) != 0u) canvas->saw_bold = true;
    if (value != 0 && value[0] == '1' && value[1] == '4' &&
        value[2] == '4' && value[3] == '.' &&
        value[4] == '0' && value[5] == '0' && value[6] == '2' &&
        value[7] == '\0') canvas->saw_frequency = true;
    if (value != 0 && value[0] == 'B' && value[1] == '1' && value[2] == '/')
        canvas->saw_band = true;
    if (value != 0 && value[0] == 'A' && value[1] == '0' &&
        value[2] == '0' && value[3] == '2')
        canvas->saw_att = true;
    if (value != 0 && value[0] == 'T' && value[1] == '\0')
        canvas->saw_turbo = true;
}

static void test_spectrum_view_cursor_uses_published_bin_frequency(void)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 100u}, {144001000u, 110u}, {144002000u, 160u},
        {144003000u, 120u}, {144004000u, 100u}
    };
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 5u, .band_index = 0u, .band_count = 2u,
        .attenuation_level = 2u, .turbo_enabled = true,
        .noise_floor = 100u, .open_threshold = 120u,
        .peak_hold_valid = true, .peak_hold_frequency_hz = 144002000u,
        .cursor_valid = true, .cursor_bin = 2u,
        .band_label = 0, .bold_labels = true
    };
    uint32_t frequency = 0u;

    CHECK(d10s_spectrum_view_cursor_frequency(&model, 2u, &frequency));
    CHECK(frequency == 144002000u);
    CHECK(!d10s_spectrum_view_cursor_frequency(&model, 5u, &frequency));
}

static void test_spectrum_view_draws_graph_and_status_labels(void)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 100u}, {144001000u, 110u}, {144002000u, 160u},
        {144003000u, 120u}, {144004000u, 100u}
    };
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 5u, .band_index = 0u, .band_count = 2u,
        .attenuation_level = 2u, .turbo_enabled = true,
        .noise_floor = 100u, .open_threshold = 120u,
        .peak_hold_valid = true, .peak_hold_frequency_hz = 144002000u,
        .cursor_valid = true, .cursor_bin = 2u,
        .band_label = 0, .bold_labels = true
    };
    fake_canvas_t fake = {0};
    const d10s_spectrum_canvas_t canvas = {
        &fake, fake_canvas_pixel, fake_canvas_text
    };

    CHECK(d10s_spectrum_view_render(&canvas, &model));
    CHECK(fake.pixel_writes > 100u);
    CHECK(fake.text_writes >= 5u);
    CHECK(fake.saw_bold);
    CHECK(fake.saw_frequency);
    CHECK(fake.saw_band);
    CHECK(fake.saw_att);
    CHECK(fake.saw_turbo);
}

static void test_spectrum_view_rejects_invalid_snapshot(void)
{
    const d10s_spectrum_bin_t bins[] = {
        {100000u, 10u}, {110000u, 20u}, {120000u, 10u}
    };
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 3u, .band_index = 1u, .band_count = 1u,
        .noise_floor = 10u, .open_threshold = 20u
    };
    fake_canvas_t fake = {0};
    const d10s_spectrum_canvas_t canvas = {
        &fake, fake_canvas_pixel, fake_canvas_text
    };

    CHECK(!d10s_spectrum_view_render(&canvas, &model));
    CHECK(fake.pixel_writes == 0u);
}

static void test_spectrum_view_rejects_invalid_rssi_and_band_count(void)
{
    const d10s_spectrum_bin_t bad_rssi[] = {
        {100000u, 10u}, {110000u, 512u}, {120000u, 10u}
    };
    const d10s_spectrum_bin_t good_bins[] = {
        {100000u, 10u}, {110000u, 20u}, {120000u, 10u}
    };
    d10s_spectrum_view_model_t model = {
        .bins = bad_rssi, .count = 3u, .band_index = 0u, .band_count = 1u,
        .noise_floor = 10u, .open_threshold = 20u
    };
    fake_canvas_t fake = {0};
    const d10s_spectrum_canvas_t canvas = {
        &fake, fake_canvas_pixel, fake_canvas_text
    };

    CHECK(!d10s_spectrum_view_render(&canvas, &model));
    model.bins = good_bins;
    model.band_count = 9u;
    CHECK(!d10s_spectrum_view_render(&canvas, &model));
    CHECK(fake.pixel_writes == 0u);
}

static uint32_t count_framebuffer_bits(const d10s_mono128x64_t *display)
{
    uint16_t i;
    uint32_t bits = 0u;
    for (i = 0u; i < D10S_MONO128X64_BUFFER_BYTES; ++i) {
        uint8_t value = display->bytes[i];
        while (value != 0u) {
            bits += (uint32_t)(value & 1u);
            value >>= 1;
        }
    }
    return bits;
}

static void test_mono128x64_pixel_layout_and_page_bounds(void)
{
    d10s_mono128x64_t display;
    d10s_mono128x64_clear(&display, false);
    d10s_mono128x64_pixel(&display, 0u, 0u, true);
    d10s_mono128x64_pixel(&display, 127u, 63u, true);
    CHECK(display.bytes[0] == 0x01u);
    CHECK(display.bytes[1023] == 0x80u);
    CHECK(d10s_mono128x64_page(&display, 0u) == &display.bytes[0]);
    CHECK(d10s_mono128x64_page(&display, 7u) == &display.bytes[896]);
    CHECK(d10s_mono128x64_page(&display, 8u) == 0);
    d10s_mono128x64_clear(&display, true);
    CHECK(count_framebuffer_bits(&display) == 8192u);
    d10s_mono128x64_clear(&display, false);
    CHECK(count_framebuffer_bits(&display) == 0u);
}

static void test_mono128x64_canvas_text_and_bold_style(void)
{
    d10s_mono128x64_t regular;
    d10s_mono128x64_t bold;
    d10s_spectrum_canvas_t regular_canvas;
    d10s_spectrum_canvas_t bold_canvas;

    d10s_mono128x64_clear(&regular, false);
    d10s_mono128x64_clear(&bold, false);
    d10s_mono128x64_bind(&regular, &regular_canvas);
    d10s_mono128x64_bind(&bold, &bold_canvas);
    CHECK(regular_canvas.pixel != 0);
    CHECK(regular_canvas.text != 0);
    regular_canvas.text(regular_canvas.context, 0u, 0u, "D10S", 0u);
    bold_canvas.text(bold_canvas.context, 0u, 0u, "D10S",
                     D10S_VIEW_TEXT_BOLD);
    CHECK(count_framebuffer_bits(&regular) > 0u);
    CHECK(count_framebuffer_bits(&bold) > count_framebuffer_bits(&regular));
    CHECK(d10s_mono128x64_page(&bold, 0u)[0] != 0u);
}

static void test_mono128x64_framebuffer_with_spectrum_renderer(void)
{
    const d10s_spectrum_bin_t bins[] = {
        {144000000u, 100u}, {144001000u, 110u}, {144002000u, 160u},
        {144003000u, 120u}, {144004000u, 100u}
    };
    const d10s_spectrum_view_model_t model = {
        .bins = bins, .count = 5u, .band_index = 0u, .band_count = 2u,
        .attenuation_level = 2u, .turbo_enabled = true,
        .noise_floor = 100u, .open_threshold = 120u,
        .peak_hold_valid = true, .peak_hold_frequency_hz = 144002000u,
        .cursor_valid = true, .cursor_bin = 2u,
        .band_label = 0, .bold_labels = true
    };
    d10s_mono128x64_t display;
    d10s_spectrum_canvas_t canvas;

    d10s_mono128x64_clear(&display, false);
    d10s_mono128x64_bind(&display, &canvas);
    CHECK(d10s_spectrum_view_render(&canvas, &model));
    CHECK(count_framebuffer_bits(&display) > 250u);
    CHECK(count_framebuffer_bits(&display) < 8192u);
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
    test_rejects_rssi_above_nine_bit_range();
    test_scanner_acquires_and_confirms_signal();
    test_scanner_resets_detector_on_band_change();
    test_scanner_rejects_radio_failures();
    test_scanner_att_compensation_and_boundary_rules();
    test_scanner_att_failure_does_not_change_state();
    test_scanner_publishes_only_complete_sweeps();
    test_scanner_manual_band_change_invalidates_snapshot();
    test_scanner_delay_changes_only_between_sweeps();
    test_turbo_exits_early_when_rssi_stabilizes();
    test_turbo_honors_max_delay_when_rssi_never_stabilizes();
    test_spectrum_view_cursor_uses_published_bin_frequency();
    test_spectrum_view_draws_graph_and_status_labels();
    test_spectrum_view_rejects_invalid_snapshot();
    test_spectrum_view_rejects_invalid_rssi_and_band_count();
    test_mono128x64_pixel_layout_and_page_bounds();
    test_mono128x64_canvas_text_and_bold_style();
    test_mono128x64_framebuffer_with_spectrum_renderer();
    test_band_grid_boundaries();
    test_multi_band_dwell_and_rotation();
    test_invalid_band_definitions();
    test_manual_band_change_resets_dwell();
    puts("D10S Spectrum detector tests: PASS");
    return 0;
}
