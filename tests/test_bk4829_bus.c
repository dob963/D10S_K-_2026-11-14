#include "bk4829.h"
#include "bk4829_timing.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef enum {
    FAIL_NONE = 0, FAIL_CSN, FAIL_CLOCK, FAIL_DIRECTION,
    FAIL_WRITE, FAIL_READ, FAIL_DELAY
} fail_kind_t;

typedef struct {
    bool csn, clock, output;
    bool input[16]; unsigned input_count, input_index;
    bool written[64]; unsigned written_count;
    uint16_t delays[256]; unsigned delay_count;
    fail_kind_t fail_kind;
} fake_bus_t;

static bool fail_once(fake_bus_t *b, fail_kind_t kind) {
    if (b->fail_kind != kind) return false;
    b->fail_kind = FAIL_NONE;
    return true;
}
static bool csn(bool v, void *c) {
    fake_bus_t *b=c; if (fail_once(b, FAIL_CSN)) return false;
    b->csn=v; return true;
}
static bool clk(bool v, void *c) {
    fake_bus_t *b=c; if (fail_once(b, FAIL_CLOCK)) return false;
    b->clock=v; return true;
}
static bool dir(bool v, void *c) {
    fake_bus_t *b=c; if (fail_once(b, FAIL_DIRECTION)) return false;
    b->output=v; return true;
}
static bool wr(bool v, void *c) {
    fake_bus_t *b=c; if (fail_once(b, FAIL_WRITE)) return false;
    if (!b->csn && b->output) b->written[b->written_count++]=v;
    return true;
}
static bool rd(bool *v, void *c) {
    fake_bus_t *b=c;
    if (fail_once(b, FAIL_READ) || b->input_index>=b->input_count)
        return false;
    *v=b->input[b->input_index++]; return true;
}
static bool delay(uint16_t n, void *c) {
    fake_bus_t *b=c; CHECK(n);
    CHECK(b->delay_count < sizeof(b->delays) / sizeof(b->delays[0]));
    b->delays[b->delay_count++] = n;
    if (fail_once(b, FAIL_DELAY)) return false;
    return true;
}
static void append(bool *o,unsigned *n,uint16_t v,uint8_t width) {
    for(uint8_t i=0;i<width;i++) o[(*n)++]=((v>>(width-1u-i))&1u)!=0u;
}

static void test_each_bus_callback_fails_closed(void)
{
    const d10s_bk4829_bus_timing_t timing = {2u, 3u};
    const fail_kind_t write_failures[] = {
        FAIL_CSN, FAIL_CLOCK, FAIL_DIRECTION, FAIL_WRITE, FAIL_DELAY
    };
    unsigned i;

    for (i = 0u; i < sizeof(write_failures) / sizeof(write_failures[0]); ++i) {
        fake_bus_t b;
        d10s_bk4829_bus_port_t p;
        memset(&b, 0, sizeof(b));
        b.csn = true;
        b.output = true;
        b.fail_kind = write_failures[i];
        p = (d10s_bk4829_bus_port_t){csn, clk, dir, wr, rd, delay, &b};
        CHECK(!d10s_bk4829_write_register(&p, &timing, 0x25u, 0xA55Au));
        CHECK(b.csn && b.clock && b.output);
    }

    {
        fake_bus_t b;
        d10s_bk4829_bus_port_t p;
        uint16_t value = 0xFFFFu;
        memset(&b, 0, sizeof(b));
        b.csn = true;
        b.output = true;
        b.input_count = 16u;
        b.fail_kind = FAIL_READ;
        p = (d10s_bk4829_bus_port_t){csn, clk, dir, wr, rd, delay, &b};
        CHECK(!d10s_bk4829_read_register(&p, &timing, 0x26u, &value));
        CHECK(b.csn && b.clock && b.output);
        CHECK(value == 0xFFFFu);
    }
}

int main(void) {
    const d10s_bk4829_bus_timing_t t={2u,3u};
    fake_bus_t b; d10s_bk4829_bus_port_t p;
    bool expected[24]; unsigned n=0u; uint16_t value=0u,rssi=0u;
    uint32_t word=0u;
    memset(&b,0,sizeof(b)); b.csn=true; b.output=true;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    CHECK(d10s_bk4829_write_register(&p,&t,0x25u,0xA55Au));
    append(expected,&n,0x25u,8u); append(expected,&n,0xA55Au,16u);
    CHECK(b.written_count==n && memcmp(b.written,expected,n)==0);
    CHECK(b.csn && b.clock && b.output);

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.input_count=16u;
    for(unsigned i=0;i<16u;i++) b.input[i]=((0xA1B2u>>(15u-i))&1u)!=0u;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    CHECK(d10s_bk4829_read_register(&p,&t,0x26u,&value));
    n=0u; append(expected,&n,0xA6u,8u);
    CHECK(b.written_count==n && memcmp(b.written,expected,n)==0);
    CHECK(value==0xA1B2u && b.input_index==16u && b.csn && b.clock && b.output);

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.input_count=16u;
    for(unsigned i=0;i<16u;i++) b.input[i]=true;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    CHECK(d10s_bk4829_read_raw_rssi(&p,&t,&rssi) && rssi==511u);
    CHECK(d10s_bk4829_frequency_supported(18000000u));
    CHECK(d10s_bk4829_frequency_supported(580000000u));
    CHECK(d10s_bk4829_frequency_supported(760000000u));
    CHECK(d10s_bk4829_frequency_supported(1160000000u));
    CHECK(!d10s_bk4829_frequency_supported(600000000u));
    CHECK(!d10s_bk4829_frequency_supported(1300000000u));
    CHECK(d10s_bk4829_frequency_to_word(144000000u,&word) && word==14400000u);
    CHECK(!d10s_bk4829_frequency_to_word(144000001u,&word));
    CHECK(!d10s_bk4829_frequency_to_word(144000000u,0));

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.fail_kind=FAIL_DELAY;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    CHECK(!d10s_bk4829_write_register(&p,&t,0x20u,0u));
    CHECK(b.csn && b.clock && b.output);
    CHECK(!d10s_bk4829_write_register(&p,&t,0x80u,0u));
    CHECK(!d10s_bk4829_read_register(&p,&t,0x20u,0));
    {
        d10s_bk4829_bus_timing_t built = {0u, 0u};
        CHECK(!d10s_bk4829_build_timing(0u, &built));
        CHECK(d10s_bk4829_build_timing(1000000u, &built));
        CHECK(built.half_period_ticks == 35u && built.turnaround_ticks == 35u);
        CHECK(d10s_bk4829_build_timing(48000000u, &built));
        CHECK(built.half_period_ticks == 35u && built.turnaround_ticks == 35u);
        CHECK(d10s_bk4829_build_timing(72000000u, &built));
        CHECK(built.half_period_ticks == 35u && built.turnaround_ticks == 35u);
        CHECK(!d10s_bk4829_build_timing(72000001u, &built));

        /* Verify that the bus consumes the generated 35-cycle timing. */
        memset(&b, 0, sizeof(b)); b.csn=true; b.output=true;
        p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
        CHECK(d10s_bk4829_write_register(&p,&built,0x25u,0xA55Au));
        CHECK(b.delay_count > 0u);
        for (unsigned i=0u; i<b.delay_count; ++i)
            CHECK(b.delays[i] == 35u);
    }
    test_each_bus_callback_fails_closed();
    puts("BK4829 bus tests: PASS"); return 0;
}
