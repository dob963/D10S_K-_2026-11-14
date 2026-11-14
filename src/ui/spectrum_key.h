#ifndef D10S_SPECTRUM_KEY_H
#define D10S_SPECTRUM_KEY_H

typedef enum {
    D10S_KEY_NONE = 0,
    D10S_KEY_0, D10S_KEY_1, D10S_KEY_2, D10S_KEY_3, D10S_KEY_4,
    D10S_KEY_5, D10S_KEY_6, D10S_KEY_7, D10S_KEY_8, D10S_KEY_9,
    D10S_KEY_MENU, D10S_KEY_UP, D10S_KEY_DOWN, D10S_KEY_EXIT,
    D10S_KEY_STAR, D10S_KEY_F, D10S_KEY_SIDE2, D10S_KEY_SIDE1
} d10s_spectrum_key_t;

typedef struct {
    d10s_spectrum_key_t candidate;
    d10s_spectrum_key_t pressed;
    uint8_t stable_count;
} d10s_spectrum_key_state_t;

/* Emits one key event after three equal scans; held keys do not auto-repeat. */
bool d10s_spectrum_key_debounce(d10s_spectrum_key_state_t *state,
                                d10s_spectrum_key_t sampled,
                                bool ambiguous,
                                d10s_spectrum_key_t *event);

#endif
