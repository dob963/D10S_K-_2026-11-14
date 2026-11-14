#include "spectrum_key.h"

bool d10s_spectrum_key_debounce(d10s_spectrum_key_state_t *state,
                                d10s_spectrum_key_t sampled,
                                bool ambiguous,
                                d10s_spectrum_key_t *event)
{
    if (state == 0 || event == 0 || sampled > D10S_KEY_SIDE1)
        return false;

    *event = D10S_KEY_NONE;
    if (ambiguous) return true;

    if (sampled != state->candidate) {
        state->candidate = sampled;
        state->stable_count = 1u;
    } else if (state->stable_count < 3u) {
        ++state->stable_count;
    }

    if (state->stable_count >= 3u && state->pressed != state->candidate) {
        state->pressed = state->candidate;
        if (state->pressed != D10S_KEY_NONE)
            *event = state->pressed;
    }
    return true;
}
