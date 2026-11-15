/*
 * Copyright 2025 muzkr
 * Copyright 2023 Dual Tachyon
 *
 * Derived register profile from:
 * armel/uv-k1-k5v3-firmware-custom, App/driver/bk4829.c plus RX-enable
 * semantics in App/radio.c and GPIO numbering in App/driver/bk4819-regs.h,
 * commit 3261ca7883d5b0b60c0905d8766612dee7b4191d.
 *
 * Changes: adapted to the D10S bus API, fail-closed callbacks and
 * explicit profile context. See THIRD_PARTY_NOTICES.md.
 *
 * Licensed under the Apache License, Version 2.0.
 */
#include "bk4829_uvk1_v3_profile.h"

#include <stddef.h>

typedef struct {
    uint8_t address;
    uint16_t value;
} register_write_t;

/*
 * Source-derived BK4829 baseline values. The sequence and repeated writes
 * are preserved from the pinned upstream BK4829_Init implementation.
 */
static const register_write_t baseline[] = {
    {0x00u, 0x8000u}, {0x00u, 0x0000u},
    {0x37u, 0x9D1Fu}, {0x36u, 0x0022u},
    {0x10u, 0x0318u}, {0x11u, 0x033Au}, {0x12u, 0x03DBu},
    {0x13u, 0x03DFu}, {0x14u, 0x0210u}, {0x49u, 0x2AB2u},
    {0x7Bu, 0x73DCu}, {0x7Du, 0xE920u}, {0x48u, 0x33A8u},
    {0x40u, 0x3516u},
    /* Active upstream #if 1 DTMF coefficient table at the pinned revision. */
    {0x09u, 0x006Fu}, {0x09u, 0x106Bu}, {0x09u, 0x2067u},
    {0x09u, 0x3062u}, {0x09u, 0x4050u}, {0x09u, 0x5047u},
    {0x09u, 0x603Au}, {0x09u, 0x702Cu}, {0x09u, 0x8041u},
    {0x09u, 0x9037u}, {0x09u, 0xA025u}, {0x09u, 0xB017u},
    {0x09u, 0xC0E4u}, {0x09u, 0xD0CBu}, {0x09u, 0xE0B5u},
    {0x09u, 0xF09Fu},
    {0x1Cu, 0x07C0u}, {0x1Du, 0xE555u}, {0x1Eu, 0x4C58u},
    {0x1Fu, 0xC65Au}, {0x3Eu, 0x94C6u},
    {0x73u, 0x4691u}, {0x77u, 0x88EFu}, {0x19u, 0x1041u},
    {0x28u, 0x0B40u}, {0x29u, 0xAA00u}, {0x2Au, 0x6600u},
    {0x2Cu, 0x1822u}, {0x2Fu, 0x9890u}, {0x53u, 0x2028u},
    {0x7Eu, 0x303Eu}, {0x46u, 0x600Au}, {0x4Au, 0x5430u},
    {0x07u, 0x61CEu}, {0x33u, 0x9000u}, {0x3Fu, 0x0000u}
};

#define GPIO_STATE_BASE 0x9000u
#define GPIO_RX_ENABLE  0x0040u
#define GPIO_UHF_LNA    0x0008u
#define GPIO_VHF_LNA    0x0004u
#define VHF_LNA_LIMIT_HZ 28000000u

static bool write_reg(d10s_bk4829_uvk1_v3_profile_t *profile,
                      uint8_t address, uint16_t value)
{
    return d10s_bk4829_write_register(&profile->bus, &profile->timing,
                                       address, value);
}

bool d10s_bk4829_uvk1_v3_resume_rx(uint32_t frequency_hz, void *context)
{
    d10s_bk4829_uvk1_v3_profile_t *profile =
        (d10s_bk4829_uvk1_v3_profile_t *)context;
    uint16_t lna_state;
    if (profile == NULL || !profile->bus_valid ||
        !d10s_bk4829_frequency_supported(frequency_hz) ||
        (frequency_hz % 10u) != 0u) return false;

    lna_state = frequency_hz < VHF_LNA_LIMIT_HZ
        ? GPIO_VHF_LNA : GPIO_UHF_LNA;

    /* The adapter programs REG_38/39 before invoking this RX-path callback. */
    /* Match the pinned UV-K1 V3 RX path: GPIO0 RX_ENABLE plus one
     * internal LNA selector (GPIO3 for >=28 MHz, GPIO4 below 28 MHz).
     * GPIO1 PA_ENABLE is never asserted; this project has no TX path. */
    if (!write_reg(profile, 0x33u,
                   (uint16_t)(GPIO_STATE_BASE | GPIO_RX_ENABLE | lna_state)))
        return false;

    profile->current_frequency_hz = frequency_hz;
    profile->rx_mode_configured = true;
    return true;
}

bool d10s_bk4829_uvk1_v3_stop_rx(void *context)
{
    d10s_bk4829_uvk1_v3_profile_t *profile =
        (d10s_bk4829_uvk1_v3_profile_t *)context;
    bool path_off;
    bool receiver_off;
    if (profile == NULL || !profile->bus_valid) return false;

    path_off = write_reg(profile, 0x33u, GPIO_STATE_BASE);
    receiver_off = write_reg(profile, 0x30u, 0x0000u);
    profile->rx_mode_configured = false;
    return path_off && receiver_off;
}

bool d10s_bk4829_uvk1_v3_initialize_rx(
    const d10s_bk4829_bus_port_t *bus,
    const d10s_bk4829_bus_timing_t *timing,
    uint32_t initial_frequency_hz,
    void *context)
{
    d10s_bk4829_uvk1_v3_profile_t *profile =
        (d10s_bk4829_uvk1_v3_profile_t *)context;
    size_t i;

    if (profile == NULL || bus == NULL || timing == NULL ||
        timing->half_period_ticks == 0u || timing->turnaround_ticks == 0u ||
        !d10s_bk4829_frequency_supported(initial_frequency_hz) ||
        (initial_frequency_hz % 10u) != 0u) return false;

    profile->bus_valid = false;
    profile->rx_mode_configured = false;
    profile->current_frequency_hz = 0u;
    profile->bus = *bus;
    profile->timing = *timing;
    profile->bus_valid = true;

    for (i = 0u; i < (sizeof(baseline) / sizeof(baseline[0])); ++i) {
        if (!write_reg(profile, baseline[i].address, baseline[i].value)) {
            (void)d10s_bk4829_uvk1_v3_stop_rx(profile);
            return false;
        }
    }

    /*
     * Read back the receive gain table before enabling RX. REG_13 is used
     * read/write by the source driver; a stuck bus or rejected baseline must
     * fail closed rather than start a meaningless Spectrum scan.
     */
    {
        uint16_t gain_readback = 0u;
        if (!d10s_bk4829_read_register(&profile->bus, &profile->timing,
                                      0x13u, &gain_readback) ||
            (gain_readback & 0x03FFu) != 0x03DFu) {
            (void)d10s_bk4829_uvk1_v3_stop_rx(profile);
            return false;
        }
    }

    /* Source-derived BK4819_RX_TurnOn sequence from the BK4829 driver. */
    if (!write_reg(profile, 0x37u, 0x9F1Fu) ||
        !write_reg(profile, 0x30u, 0x0000u) ||
        !write_reg(profile, 0x30u, 0xBFF1u) ||
        !d10s_bk4829_write_frequency(&profile->bus, &profile->timing,
                                     initial_frequency_hz) ||
        !d10s_bk4829_uvk1_v3_resume_rx(initial_frequency_hz, profile)) {
        (void)d10s_bk4829_uvk1_v3_stop_rx(profile);
        return false;
    }
    return true;
}
