#ifndef D10S_PY32F071_UVK1_V3_H
#define D10S_PY32F071_UVK1_V3_H

#include "bk4829_bus.h"
#include "bk4829_timing.h"
#include "st7565.h"
#include "spectrum_key.h"

#include <stdbool.h>
#include <stdint.h>

#define D10S_PY32F071_UVK1_V3_INITIALIZER {0}

typedef bool (*d10s_py32_delay_ms_fn)(uint16_t milliseconds, void *context);
typedef bool (*d10s_py32_delay_ticks_fn)(uint16_t ticks, void *context);

typedef struct {
    d10s_py32_delay_ms_fn delay_ms;
    d10s_py32_delay_ticks_fn delay_ticks;
    void *delay_ms_context;
    void *delay_ticks_context;
    bool gpio_ready;
    bool spi_ready;
    bool systick_ready;
    bool keypad_ready;
    d10s_spectrum_key_state_t key_state;
    uint32_t core_clock_hz;
    uint32_t systick_last;
    uint32_t milliseconds;
    uint64_t cycle_remainder;
} d10s_py32f071_uvk1_v3_t;

/*
 * Pin mapping follows the pinned UV-K1 V3 platform reference and remains
 * subject to continuity checks on the actual PCB revision.
 * Either supply calibrated delay callbacks or initialize the built-in SysTick
 * backend with the measured core clock before requesting the _systick ports.
 */
bool d10s_py32f071_uvk1_v3_clock_from_registers(
    uint32_t rcc_cr, uint32_t rcc_icscr, uint32_t rcc_cfgr,
    uint32_t rcc_pllcfgr, uint32_t hse_clock_hz,
    uint32_t *core_clock_hz);
bool d10s_py32f071_uvk1_v3_systick_init(
    d10s_py32f071_uvk1_v3_t *board,
    uint32_t measured_core_clock_hz);
bool d10s_py32f071_uvk1_v3_systick_init_from_rcc(
    d10s_py32f071_uvk1_v3_t *board,
    uint32_t hse_clock_hz);
uint32_t d10s_py32f071_uvk1_v3_millis(
    d10s_py32f071_uvk1_v3_t *board);
bool d10s_py32f071_uvk1_v3_set_backlight(
    d10s_py32f071_uvk1_v3_t *board,
    bool enabled);
bool d10s_py32f071_uvk1_v3_keypad_init(
    d10s_py32f071_uvk1_v3_t *board);
bool d10s_py32f071_uvk1_v3_keypad_poll(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_spectrum_key_t *key);
bool d10s_py32f071_uvk1_v3_build_bk4829_timing(
    const d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_timing_t *timing);

bool d10s_py32f071_uvk1_v3_display_port_init_systick(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_st7565_port_t *port);
bool d10s_py32f071_uvk1_v3_radio_bus_port_init_systick(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_port_t *port);

bool d10s_py32f071_uvk1_v3_display_port_init(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_st7565_port_t *port,
    d10s_py32_delay_ms_fn delay_ms,
    void *delay_context);

bool d10s_py32f071_uvk1_v3_radio_bus_port_init(
    d10s_py32f071_uvk1_v3_t *board,
    d10s_bk4829_bus_port_t *port,
    d10s_py32_delay_ticks_fn delay_ticks,
    void *delay_context);

#endif
