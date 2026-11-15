#include "py32f071_uvk1_v3.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void test_hsi_clock_and_divider(void)
{
    uint32_t hz = 0u;
    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, (1u << 13), 0u, 0u, 24000000u, &hz));
    CHECK(hz == 8000000u);

    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        (1u << 11), (4u << 13), 0u, 0u, 24000000u, &hz));
    CHECK(hz == 12000000u);
}

static void test_hse_and_pll_clock(void)
{
    uint32_t hz = 0u;
    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, 0u, 0x08u, 0u, 24000000u, &hz));
    CHECK(hz == 24000000u);

    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, (4u << 13), 0x10u, (2u | (1u << 2)), 24000000u, &hz));
    CHECK(hz == 72000000u);

    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, 0u, 0x10u, 3u, 24000000u, &hz));
    CHECK(hz == 48000000u);
}

static void test_ahb_prescaler_and_invalid_sources(void)
{
    uint32_t hz = 0u;
    CHECK(d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, (4u << 13), (8u << 8), 0u, 24000000u, &hz));
    CHECK(hz == 12000000u);

    CHECK(!d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, 0u, 0x08u, 0u, 0u, &hz));
    CHECK(!d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, 0u, 0x10u, 0u, 24000000u, &hz));
    CHECK(!d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, 0u, 0x18u, 0u, 24000000u, &hz));

    /* PLLMUL encodings 10 and 11 are reserved by the Puya reference manual. */
    CHECK(!d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, (4u << 13), 0x10u, (2u | (2u << 2)), 24000000u, &hz));
    CHECK(!d10s_py32f071_uvk1_v3_clock_from_registers(
        0u, (4u << 13), 0x10u, (2u | (3u << 2)), 24000000u, &hz));
}

int main(void)
{
    test_hsi_clock_and_divider();
    test_hse_and_pll_clock();
    test_ahb_prescaler_and_invalid_sources();
    puts("PY32F071 clock calculation tests: PASS");
    return 0;
}
