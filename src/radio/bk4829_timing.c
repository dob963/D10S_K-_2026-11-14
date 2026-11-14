#include "bk4829_timing.h"

#include <limits.h>

static uint64_t ceil_div(uint64_t n, uint64_t d)
{
    return n / d + ((n % d) != 0u ? 1u : 0u);
}

bool d10s_bk4829_build_timing(uint32_t delay_tick_hz,
                              d10s_bk4829_bus_timing_t *timing)
{
    uint64_t half;
    uint64_t turnaround;
    if (delay_tick_hz == 0u || timing == 0) return false;
    half = ceil_div((uint64_t)delay_tick_hz * 25u, 1000000000u);
    turnaround = ceil_div(delay_tick_hz, 1000000u);
    if (half == 0u) half = 1u;
    if (turnaround == 0u) turnaround = 1u;
    if (half > UINT16_MAX || turnaround > UINT16_MAX) return false;
    timing->half_period_ticks = (uint16_t)half;
    timing->turnaround_ticks = (uint16_t)turnaround;
    return true;
}
