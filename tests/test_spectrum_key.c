#include "spectrum_key.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void test_single_event_after_three_stable_samples(void)
{
    d10s_spectrum_key_state_t state = {0};
    d10s_spectrum_key_t event = D10S_KEY_NONE;

    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_UP, false, &event));
    CHECK(event == D10S_KEY_NONE);
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_UP, false, &event));
    CHECK(event == D10S_KEY_NONE);
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_UP, false, &event));
    CHECK(event == D10S_KEY_UP);

    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_UP, false, &event));
    CHECK(event == D10S_KEY_NONE);
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_UP, false, &event));
    CHECK(event == D10S_KEY_NONE);
}

static void test_release_then_new_key(void)
{
    d10s_spectrum_key_state_t state = {0};
    d10s_spectrum_key_t event = D10S_KEY_NONE;
    unsigned i;

    for (i = 0u; i < 3u; ++i)
        CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_MENU, false, &event));
    CHECK(event == D10S_KEY_MENU);
    for (i = 0u; i < 3u; ++i)
        CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_NONE, false, &event));
    CHECK(event == D10S_KEY_NONE);
    CHECK(state.pressed == D10S_KEY_NONE);

    for (i = 0u; i < 3u; ++i)
        CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_F, false, &event));
    CHECK(event == D10S_KEY_F);
}

static void test_ambiguous_samples_do_not_change_state(void)
{
    d10s_spectrum_key_state_t state = {0};
    d10s_spectrum_key_t event = D10S_KEY_NONE;
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_4, false, &event));
    CHECK(state.stable_count == 1u);
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_NONE, true, &event));
    CHECK(event == D10S_KEY_NONE && state.candidate == D10S_KEY_4 &&
          state.stable_count == 1u);
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_4, false, &event));
    CHECK(d10s_spectrum_key_debounce(&state, D10S_KEY_4, false, &event));
    CHECK(event == D10S_KEY_4);
}

int main(void)
{
    test_single_event_after_three_stable_samples();
    test_release_then_new_key();
    test_ambiguous_samples_do_not_change_state();
    puts("Spectrum key debounce tests: PASS");
    return 0;
}
