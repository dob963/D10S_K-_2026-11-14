#ifndef D10S_PY32F071_UVK1_V3_H
#define D10S_PY32F071_UVK1_V3_H

#include "bk4829_bus.h"
#include "st7565.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*d10s_py32_delay_ms_fn)(uint16_t milliseconds, void *context);
typedef bool (*d10s_py32_delay_ticks_fn)(uint16_t ticks, void *context);

typedef struct {
    d10s_py32_delay_ms_fn delay_ms;
    d10s_py32_delay_ticks_fn delay_ticks;
    void *delay_context;
    bool gpio_ready;
    bool spi_ready;
} d10s_py32f071_uvk1_v3_t;

/*
 * Pin mapping follows the pinned UV-K1 V3 platform reference and remains
 * subject to continuity checks on the actual PCB revision.
 * Delay callbacks are mandatory; their timing must be measured/calibrated.
 */
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
