#ifndef D10S_BK4829_TIMING_H
#define D10S_BK4829_TIMING_H

#include "bk4829_bus.h"

#include <stdbool.h>
#include <stdint.h>

/* delay_tick_hz must be the measured HCLK/SysTick rate (1–72 MHz).
 * The profile uses 35 timer ticks per BK4829 serial edge/turnaround, matching
 * the 35-NOP delay in the pinned UV-K1 V3 reference driver. */
bool d10s_bk4829_build_timing(uint32_t delay_tick_hz,
                              d10s_bk4829_bus_timing_t *timing);

#endif
