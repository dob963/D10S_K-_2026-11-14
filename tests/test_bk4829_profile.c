#include "bk4829_uvk1_v3_radio.h"

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
    uint16_t read_value;
    uint8_t read_bit_count;
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
        bus->read_bit_count = 0u;
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
    fake_bus_t *bus = (fake_bus_t *)context;
    if (high == NULL || bus->read_bit_count >= 16u) return false;
    *high = ((bus->read_value >> (15u - bus->read_bit_count)) & 1u) != 0u;
    ++bus->read_bit_count;
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

static uint8_t write_index(const fake_bus_t *bus,
                            uint8_t address, uint16_t value)
{
    uint8_t i;
    for (i = 0u; i < bus->register_count; ++i) {
        if (bus->writes[i].address == address &&
            bus->writes[i].value == value) return i;
    }
    return 0xFFu;
}

static bool gpio1_pa_enable_seen(const fake_bus_t *bus)
{
    uint8_t i;
    for (i = 0u; i < bus->register_count; ++i) {
        if (bus->writes[i].address == 0x33u &&
            (bus->writes[i].value & 0x0020u) != 0u) return true;
    }
    return false;
}

static uint16_t last_gpio_state(const fake_bus_t *bus)
{
    uint8_t i = bus->register_count;
    while (i != 0u) {
        --i;
        if (bus->writes[i].address == 0x33u)
            return bus->writes[i].value;
    }
    return 0xFFFFu;
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

    fake.read_value = 0x03DFu;
    CHECK(d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, initial_hz,
                                            &profile));
    CHECK(profile.bus_valid);
    CHECK(profile.rx_mode_configured);
    CHECK(profile.current_frequency_hz == initial_hz);
    CHECK(fake.register_count > 50u);
    CHECK(fake.writes[0].address == 0x00u);
    CHECK(fake.writes[0].value == 0x8000u);
    CHECK(fake.writes[1].address == 0x00u);
    CHECK(fake.writes[1].value == 0x0000u);
    CHECK(write_seen(&fake, 0x37u, 0x9D1Fu));
    /* Spectrum init disables AFC through documented REG_73 bit 4. */
    CHECK(write_seen(&fake, 0x73u, 0x03CFu));
    /* Active upstream #if 1 DTMF coefficient table, exact register words. */
    CHECK(write_seen(&fake, 0x09u, 0x006Fu));
    CHECK(write_seen(&fake, 0x09u, 0x106Bu));
    CHECK(write_seen(&fake, 0x09u, 0x2067u));
    CHECK(write_seen(&fake, 0x09u, 0x3062u));
    CHECK(write_seen(&fake, 0x09u, 0x4050u));
    CHECK(write_seen(&fake, 0x09u, 0x5047u));
    CHECK(write_seen(&fake, 0x09u, 0x603Au));
    CHECK(write_seen(&fake, 0x09u, 0x702Cu));
    CHECK(write_seen(&fake, 0x09u, 0x8041u));
    CHECK(write_seen(&fake, 0x09u, 0x9037u));
    CHECK(write_seen(&fake, 0x09u, 0xA025u));
    CHECK(write_seen(&fake, 0x09u, 0xB017u));
    CHECK(write_seen(&fake, 0x09u, 0xC0E4u));
    CHECK(write_seen(&fake, 0x09u, 0xD0CBu));
    CHECK(write_seen(&fake, 0x09u, 0xE0B5u));
    CHECK(write_seen(&fake, 0x09u, 0xF09Fu));
    CHECK(write_seen(&fake, 0x37u, 0x9F1Fu));
    CHECK(write_seen(&fake, 0x30u, 0x0000u));
    CHECK(write_seen(&fake, 0x30u, 0xBFF1u));
    CHECK(write_seen(&fake, 0x38u, (uint16_t)word));
    CHECK(write_seen(&fake, 0x39u, (uint16_t)(word >> 16)));
    /* Cold-start tuning must precede the RX power-up sequence. */
    CHECK(write_index(&fake, 0x38u, (uint16_t)word) <
          write_index(&fake, 0x37u, 0x9F1Fu));
    CHECK(write_index(&fake, 0x39u, (uint16_t)(word >> 16)) <
          write_index(&fake, 0x30u, 0x0000u));
    CHECK(write_index(&fake, 0x30u, 0x0000u) <
          write_index(&fake, 0x30u, 0xBFF1u));
    CHECK(write_index(&fake, 0x30u, 0xBFF1u) <
          write_index(&fake, 0x33u, 0x9044u));
    CHECK(!write_seen(&fake, 0x33u, 0x9008u));
    CHECK(write_seen(&fake, 0x33u, 0x9044u));
    CHECK(last_gpio_state(&fake) == 0x9044u);
    CHECK(!gpio1_pa_enable_seen(&fake));

    /* The upstream frequency argument is in 10-Hz units. D10S uses Hz,
     * therefore the source boundary 28,000,000 becomes 280,000,000 Hz.
     * VHF (144 MHz) selects GPIO4; UHF (433 MHz) selects GPIO3. */
    CHECK(d10s_bk4829_uvk1_v3_resume_rx(144000000u, &profile));
    CHECK(last_gpio_state(&fake) == 0x9044u);
    CHECK(d10s_bk4829_uvk1_v3_resume_rx(433000000u, &profile));
    CHECK(last_gpio_state(&fake) == 0x9048u);

    before = fake.register_count;
    CHECK(d10s_bk4829_uvk1_v3_resume_rx(27000000u, &profile));
    CHECK(profile.current_frequency_hz == 27000000u);
    CHECK(last_gpio_state(&fake) == 0x9044u);
    CHECK(!write_seen(&fake, 0x33u, 0x9004u));
    CHECK(write_seen(&fake, 0x33u, 0x9044u));
    CHECK(!gpio1_pa_enable_seen(&fake));
    CHECK(fake.register_count == before + 1u);

    before = fake.register_count;
    CHECK(!d10s_bk4829_uvk1_v3_resume_rx(700000000u, &profile));
    CHECK(fake.register_count == before);
}

static void test_uvk1_profile_stops_rx_and_lna(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_profile_t profile = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};

    fake.read_value = 0x03DFu;
    CHECK(d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, 433000000u,
                                            &profile));
    CHECK(d10s_bk4829_uvk1_v3_stop_rx(&profile));
    CHECK(!profile.rx_mode_configured);
    CHECK(write_seen(&fake, 0x33u, 0x9000u));
    CHECK(write_seen(&fake, 0x30u, 0x0000u));
    CHECK(!gpio1_pa_enable_seen(&fake));
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
    CHECK(!profile.rx_mode_configured);
    CHECK(profile.bus_valid);
}

static void test_uvk1_profile_rejects_bad_gain_readback(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_profile_t profile = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};

    fake.read_value = 0u; /* Simulate a nonresponsive or incorrect REG_13. */
    CHECK(!d10s_bk4829_uvk1_v3_initialize_rx(&port, &timing, 145000000u,
                                             &profile));
    CHECK(!profile.rx_mode_configured);
    CHECK(write_seen(&fake, 0x33u, 0x9000u));
    CHECK(!write_seen(&fake, 0x33u, 0x9044u));
}

static void test_radio_wrapper_binds_profile_to_scanner_ops(void)
{
    fake_bus_t fake = {0};
    d10s_bk4829_uvk1_v3_radio_t radio = {0};
    d10s_spectrum_radio_ops_t ops = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};
    uint32_t actual_hz = 0u;

    fake.read_value = 0x03DFu;
    CHECK(d10s_bk4829_uvk1_v3_radio_init(&radio, &port, &timing,
                                          145000000u, &ops));
    CHECK(radio.initialized);
    CHECK(radio.profile.rx_mode_configured);
    CHECK(ops.context == &radio.scanner_adapter);
    CHECK(ops.tune_hz != 0);
    CHECK(ops.read_rssi != 0);
    CHECK(ops.set_attenuation != 0);
    fake.read_value = (uint16_t)(0xFC00u | 0x03DFu);
    CHECK(ops.set_attenuation(ops.context, 0u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x03DFu)));
    CHECK(ops.set_attenuation(ops.context, 1u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x03DDu)));
    CHECK(ops.set_attenuation(ops.context, 2u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x03DBu)));
    CHECK(ops.set_attenuation(ops.context, 3u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x03D9u)));
    CHECK(ops.set_attenuation(ops.context, 4u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x0379u)));
    CHECK(ops.set_attenuation(ops.context, 5u));
    CHECK(write_seen(&fake, 0x13u, (uint16_t)(0xFC00u | 0x0139u)));
    CHECK(!ops.set_attenuation(ops.context, 6u));

    CHECK(ops.tune_hz(ops.context, 145125000u, &actual_hz));
    CHECK(actual_hz == 145125000u);
    CHECK(radio.profile.current_frequency_hz == 145125000u);
    CHECK(write_seen(&fake, 0x38u, (uint16_t)(145125000u / 10u)));
    CHECK(!write_seen(&fake, 0x33u, 0x9008u));

    d10s_bk4829_uvk1_v3_radio_stop(&radio);
    CHECK(!radio.initialized);
    CHECK(!radio.profile.rx_mode_configured);
    CHECK(write_seen(&fake, 0x33u, 0x9000u));
    CHECK(write_seen(&fake, 0x30u, 0x0000u));
    CHECK(!gpio1_pa_enable_seen(&fake));
}

static void test_afc_enable_disable_preserves_other_reg73_bits(void)
{
    fake_bus_t fake = {0};
    const d10s_bk4829_bus_port_t port = make_port(&fake);
    const d10s_bk4829_bus_timing_t timing = {2u, 48u};
    const uint16_t original = 0xA5C5u;

    fake.read_value = original;
    CHECK(d10s_bk4829_set_afc_enabled(&port, &timing, false));
    CHECK(write_seen(&fake, 0x73u, (uint16_t)(original | 0x0010u)));

    fake.register_count = 0u;
    fake.read_value = (uint16_t)(original | 0x0010u);
    CHECK(d10s_bk4829_set_afc_enabled(&port, &timing, true));
    CHECK(write_seen(&fake, 0x73u,
                     (uint16_t)((original | 0x0010u) & (uint16_t)~0x0010u)));
}

int main(void)
{
    test_uvk1_profile_initializes_and_selects_lna_path();
    test_uvk1_profile_stops_rx_and_lna();
    test_uvk1_profile_fails_closed_on_bus_error();
    test_uvk1_profile_rejects_bad_gain_readback();
    test_radio_wrapper_binds_profile_to_scanner_ops();
    test_afc_enable_disable_preserves_other_reg73_bits();
    puts("BK4829 UV-K1 V3 profile tests: PASS");
    return 0;
}
