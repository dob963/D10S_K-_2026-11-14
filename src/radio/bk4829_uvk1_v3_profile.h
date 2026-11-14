/*
 * Copyright 2025 muzkr
 * Copyright 2023 Dual Tachyon
 *
 * Derived register profile from:
 * armel/uv-k1-k5v3-firmware-custom, App/driver/bk4829.c,
 * commit 3261ca7883d5b0b60c0905d8766612dee7b4191d.
 *
 * Changes: adapted to the D10S bus API, fail-closed callbacks and
 * explicit profile context. See THIRD_PARTY_NOTICES.md.
 *
 * Licensed under the Apache License, Version 2.0.
 */
#ifndef D10S_BK4829_UVK1_V3_PROFILE_H
#define D10S_BK4829_UVK1_V3_PROFILE_H

#include "bk4829_scanner_adapter.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    d10s_bk4829_bus_port_t bus;
    d10s_bk4829_bus_timing_t timing;
    uint32_t current_frequency_hz;
    bool bus_valid;
    bool receiver_on;
} d10s_bk4829_uvk1_v3_profile_t;

/*
 * Source-derived UV-K1 V3 reference initialization. This is opt-in and not
 * hardware-validated on every PCB revision. Caller must provide verified bus
 * callbacks/timing and confirm the RF switch/LNA wiring before use.
 */
bool d10s_bk4829_uvk1_v3_initialize_rx(
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    uint32_t initial_frequency_hz,
    void *context);
bool d10s_bk4829_uvk1_v3_resume_rx(uint32_t frequency_hz, void *context);
bool d10s_bk4829_uvk1_v3_stop_rx(void *context);

#endif
