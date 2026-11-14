#ifndef D10S_BK4829_BUS_H
#define D10S_BK4829_BUS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool (*set_csn_high)(bool high, void *context);
    bool (*set_clock_high)(bool high, void *context);
    bool (*set_data_output)(bool output, void *context);
    bool (*write_data_high)(bool high, void *context);
    bool (*read_data_high)(bool *high, void *context);
    bool (*delay_ticks)(uint16_t ticks, void *context);
    void *context;
} d10s_bk4829_bus_port_t;

typedef struct {
    uint16_t half_period_ticks;
    uint16_t turnaround_ticks;
} d10s_bk4829_bus_timing_t;

bool d10s_bk4829_write_register(const d10s_bk4829_bus_port_t *port,
                                const d10s_bk4829_bus_timing_t *timing,
                                uint8_t address, uint16_t value);
bool d10s_bk4829_read_register(const d10s_bk4829_bus_port_t *port,
                               const d10s_bk4829_bus_timing_t *timing,
                               uint8_t address, uint16_t *value);

#endif
