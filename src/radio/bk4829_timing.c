#include "bk4829_timing.h"

#define D10S_BK4829_EDGE_DELAY_CYCLES 35u

bool d10s_bk4829_build_timing(uint32_t delay_tick_hz,
                              d10s_bk4829_bus_timing_t *timing)
{
    /*
     * The pinned UV-K1 V3 driver uses a 35-NOP SHORT_DELAY between BK4829
     * serial edges and after the SDA direction turnaround. The PY32 SysTick
     * backend counts HCLK cycles, so 35 ticks preserve that cycle-based delay
     * regardless of whether the bootloader left HCLK at 8, 48 or 72 MHz.
     */
    if (delay_tick_hz < 1000000u || delay_tick_hz > 72000000u ||
        timing == 0) return false;

    timing->half_period_ticks = D10S_BK4829_EDGE_DELAY_CYCLES;
    timing->turnaround_ticks = D10S_BK4829_EDGE_DELAY_CYCLES;
    return true;
}
