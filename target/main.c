#include "spectrum_app.h"
#include "py32f071_uvk1_v3.h"
#include "bk4829_uvk1_v3_radio.h"
#include "scan_bands.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_INITIAL_FREQUENCY_HZ 144000000u
#define D10S_SCAN_SETTLE_MS 8u

static d10s_py32f071_uvk1_v3_t board =
    D10S_PY32F071_UVK1_V3_INITIALIZER;
static d10s_st7565_t display;
static d10s_bk4829_uvk1_v3_radio_t radio;
static d10s_spectrum_app_t app;
static d10s_st7565_config_t display_config;
static d10s_st7565_port_t display_port;
static d10s_bk4829_bus_port_t radio_port;
static d10s_bk4829_bus_timing_t radio_timing;
static d10s_spectrum_radio_ops_t radio_ops;
static d10s_spectrum_config_t detector_config;

__attribute__((noreturn))
static void fail_closed(void)
{
    d10s_bk4829_uvk1_v3_radio_stop(&radio);
    (void)d10s_py32f071_uvk1_v3_set_backlight(&board, false);
    for (;;) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    d10s_spectrum_key_t key;
    d10s_spectrum_app_status_t app_status;

    /*
     * Do not force a new clock tree: derive HCLK from the bootloader's current
     * RCC state. HSE is intentionally unspecified; HSI/PLL-HSI paths work
     * without assuming a crystal value.
     */
    if (!d10s_py32f071_uvk1_v3_systick_init_from_rcc(&board, 0u))
        fail_closed();

    if (!d10s_py32f071_uvk1_v3_display_port_init_systick(&board,
                                                         &display_port))
        fail_closed();
    d10s_st7565_uvk1_v3_default_config(&display_config);
    if (!d10s_st7565_init(&display, &display_port, &display_config))
        fail_closed();
    if (!d10s_py32f071_uvk1_v3_set_backlight(&board, true))
        fail_closed();

    if (!d10s_py32f071_uvk1_v3_keypad_init(&board))
        fail_closed();

    if (!d10s_py32f071_uvk1_v3_radio_bus_port_init_systick(&board,
                                                            &radio_port) ||
        !d10s_py32f071_uvk1_v3_build_bk4829_timing(&board, &radio_timing))
        fail_closed();

    if (!d10s_bk4829_uvk1_v3_radio_init(&radio, &radio_port, &radio_timing,
                                         D10S_INITIAL_FREQUENCY_HZ,
                                         &radio_ops))
        fail_closed();

    d10s_spectrum_default_config(&detector_config);
    if (!d10s_spectrum_app_init(&app, D10S_SCAN_BANDS,
                                D10S_SCAN_BAND_COUNT,
                                &radio_ops, &display, &detector_config,
                                D10S_SCAN_SETTLE_MS))
        fail_closed();

    for (;;) {
        if (!d10s_py32f071_uvk1_v3_keypad_poll(&board, &key))
            fail_closed();
        if (key != D10S_KEY_NONE)
            (void)d10s_spectrum_app_handle_key(&app, key);

        app_status = d10s_spectrum_app_tick(
            &app, d10s_py32f071_uvk1_v3_millis(&board));
        if (app_status == D10S_APP_ERROR)
            fail_closed();
    }
}
