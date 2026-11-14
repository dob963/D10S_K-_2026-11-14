#ifndef D10S_BK4829_TIMING_H
#define D10S_BK4829_TIMING_H

#include "bk4829_bus.h"

#include <stdbool.h>
#include <stdint.h>

/* delay_tick_hz must be the measured rate of the actual delay callback. */
bool d10s_bk4829_build_timing(uint32_t delay_tick_hz,
                              d10s_bk4829_bus_timing_t *timing);

#endif
