#include "bk4829_uvk1_v3_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct {
    bool csn_high;
    bool clock_high;
    bool data_output;
    bool data_high;
    bool active;
    uint32_t bit_count;
    uint32_t shift;
    uint32_t delay_calls;
    uint32_t fail_delay_at;
    uint8_t register_count;
    struct {
        uint8_t address;
        uint16_t value;
    } writes[160];
} fake_bus_t;

static bool fake_csn(bool high, void *context)
{
    fake_bus_t *bus = (fake_bus_t *)context;
    if (!high) {
        bus->active = true;
        bus->bit_count = 0u;
        bus->shift = 0u;
    } else if (bus->active) {
        if (bus->bit_count == 24u &&
            bus->register_count < (uint8_t)(sizeof(bus->writes) / sizeof(bus->writes[0]))) {
            const uint32_t word = bus->shift;
            bus->writes[bus->register_count].address =
                (uint8_t)((word >> 16) & 0xFFu);
            bus->writes[bus->register_count].value = (uint16_t)word;
            ++bus->register_count;
        }
        bus->active = false;
    }
    bus->csn_high = high;
    return true;
}

static bool fake_clock(bool high, void *context)
{
    ((fake_bus_t *)context)->clock_high = high;
    return true;
}

static bool fake_data_output(bool output, void *context)
{
    ((fake_bus_t *)context)->data_output = output;
    return true;
}

static bool fake_write_data(bool high, void *context)
{
    fake_bus_t *bus = (fake_bus_t *)context;
    bus->data_high = high;
    if (bus->active) {
        bus->shift = (bus->shift << 1) | (high ? 1u : 0u);
        ++bus->bit_count;
    }
    return true;
}

static bool fake_read_data(bool *high, void *context)
{
    (void)context;
    if (high == NULL) return false;
    *high = false;
    return true;
}

static bool fake_delay(uint16_t ticks, void *context)
{
    fake_bus_t *bus = (fake_bus_t *)context;
    (void)ticks;
    ++bus->delay_calls;
    return bus->fail_delay_at == 0u ||
           bus->delay_calls < bus->fail_delay_at;
}

static d10s_bk4829_bus_port_t make_port(fake_bus_t *fake)
{
    const d10s_bk4829_bus_port_t port = {
        fake_csn, fake_clock, fake_data_output, fake_write_data,
        fake_read_data, fake_delay, fake
    };
    return port;
}

static bool write_seen(const fake_bus_t *bus, uint8_t address, uint16_t value)
{
    uint8_t i;
    for (i = 0u; i < bus->register_count; ++i) {
        if (bus->writes[i].address == address &&
            bus->writes[i].value == value) return true;
    }
    return false;
}

static void test_uvk1_profile_initializes_and_selects_lna_path(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_profile_t profile = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};
    const uint32_t initial_hz = 145000000u;
    const uint32_t word = initial_hz / 10u;
    uint32_t before;

    CHECK(d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, initial_hz,
                                            &profile));
    CHECK(profile.bus_valid);
    CHECK(profile.receiver_on);
    CHECK(profile.current_frequency_hz == initial_hz);
    CHECK(fake.register_count > 50u);
    CHECK(fake.writes[0].address == 0x00u);
    CHECK(fake.writes[0].value == 0x8000u);
    CHECK(fake.writes[1].address == 0x00u);
    CHECK(fake.writes[1].value == 0x0000u);
    CHECK(write_seen(&fake, 0x37u, 0x9D1Fu));
    CHECK(write_seen(&fake, 0x37u, 0x9F1Fu));
    CHECK(write_seen(&fake, 0x30u, 0x0000u));
    CHECK(write_seen(&fake, 0x30u, 0xBFF1u));
    CHECK(write_seen(&fake, 0x38u, (uint16_t)word));
    CHECK(write_seen(&fake, 0x39u, (uint16_t)(word >> 16)));
    CHECK(write_seen(&fake, 0x33u, 0x9008u));
    CHECK(!write_seen(&fake, 0x33u, 0x9048u));

    before = fake.register_count;
    CHECK(d10s_bk4829_uvk1_v3_resume_rx(27000000u, &profile));
    CHECK(profile.current_frequency_hz == 27000000u);
    CHECK(write_seen(&fake, 0x33u, 0x9004u));
    CHECK(!write_seen(&fake, 0x33u, 0x9044u));
    CHECK(fake.register_count == before + 1u);

    before = fake.register_count;
    CHECK(!d10s_bk4829_uvk1_v3_resume_rx(600000000u, &profile));
    CHECK(fake.register_count == before);
}

static void test_uvk1_profile_stops_rx_and_lna(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_profile_t profile = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};

    CHECK(d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, 433000000u,
                                            &profile));
    CHECK(d10s_bk4829_uvk1_v3_stop_rx(&profile));
    CHECK(!profile.receiver_on);
    CHECK(write_seen(&fake, 0x33u, 0x9000u));
    CHECK(write_seen(&fake, 0x30u, 0x0000u));
}

static void test_uvk1_profile_fails_closed_on_bus_error(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_profile_t profile = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};

    fake.fail_delay_at = 5u;
    CHECK(!d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, 145000000u,
                                             &profile));
    CHECK(!profile.receiver_on);
    CHECK(profile.bus_valid);
}

int main(void)
{
    test_uvk1_profile_initializes_and_enables_rx_path();
    test_uvk1_profile_stops_rx_and_lna();
    test_uvk1_profile_fails_closed_on_bus_error();
    puts("BK4829 UV-K1 V3 profile tests: PASS");
    return 0;
}
