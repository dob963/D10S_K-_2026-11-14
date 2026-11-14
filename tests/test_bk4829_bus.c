#include "bk4829.h"
#include "bk4829_timing.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool csn, clock, output;
    bool input[16]; unsigned input_count, input_index;
    bool written[64]; unsigned written_count;
    bool fail_delay;
} fake_bus_t;
static bool csn(bool v, void *c) { ((fake_bus_t *)c)->csn=v; return true; }
static bool clk(bool v, void *c) { ((fake_bus_t *)c)->clock=v; return true; }
static bool dir(bool v, void *c) { ((fake_bus_t *)c)->output=v; return true; }
static bool wr(bool v, void *c) {
    fake_bus_t *b=c; if (!b->csn && b->output) b->written[b->written_count++]=v;
    return true;
}
static bool rd(bool *v, void *c) {
    fake_bus_t *b=c; if (b->input_index>=b->input_count) return false;
    *v=b->input[b->input_index++]; return true;
}
static bool delay(uint16_t n, void *c) {
    fake_bus_t *b=c; assert(n); if (b->fail_delay) { b->fail_delay=false; return false; }
    return true;
}
static void append(bool *o,unsigned *n,uint16_t v,uint8_t width) {
    for(uint8_t i=0;i<width;i++) o[(*n)++]=((v>>(width-1u-i))&1u)!=0u;
}
int main(void) {
    const d10s_bk4829_bus_timing_t t={2u,3u};
    fake_bus_t b; d10s_bk4829_bus_port_t p;
    bool expected[24]; unsigned n=0u; uint16_t value=0u,rssi=0u;
    uint32_t word=0u;
    memset(&b,0,sizeof(b)); b.csn=true; b.output=true;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    assert(d10s_bk4829_write_register(&p,&t,0x25u,0xA55Au));
    append(expected,&n,0x25u,8u); append(expected,&n,0xA55Au,16u);
    assert(b.written_count==n && memcmp(b.written,expected,n)==0);
    assert(b.csn && b.clock && b.output);

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.input_count=16u;
    for(unsigned i=0;i<16u;i++) b.input[i]=((0xA1B2u>>(15u-i))&1u)!=0u;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    assert(d10s_bk4829_read_register(&p,&t,0x26u,&value));
    n=0u; append(expected,&n,0xA6u,8u);
    assert(b.written_count==n && memcmp(b.written,expected,n)==0);
    assert(value==0xA1B2u && b.input_index==16u && b.csn && b.clock && b.output);

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.input_count=16u;
    for(unsigned i=0;i<16u;i++) b.input[i]=true;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    assert(d10s_bk4829_read_raw_rssi(&p,&t,&rssi) && rssi==511u);
    assert(d10s_bk4829_frequency_to_word(144000000u,&word) && word==14400000u);
    assert(!d10s_bk4829_frequency_to_word(144000001u,&word));
    assert(!d10s_bk4829_frequency_to_word(144000000u,0));

    memset(&b,0,sizeof(b)); b.csn=true; b.output=true; b.fail_delay=true;
    p=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&b};
    assert(!d10s_bk4829_write_register(&p,&t,0x20u,0u));
    assert(b.csn && b.clock && b.output);
    assert(!d10s_bk4829_write_register(&p,&t,0x80u,0u));
    assert(!d10s_bk4829_read_register(&p,&t,0x20u,0));
    {
        d10s_bk4829_bus_timing_t built = {0u, 0u};
        assert(!d10s_bk4829_build_timing(0u, &built));
        assert(d10s_bk4829_build_timing(1000000u, &built));
        assert(built.half_period_ticks == 1u && built.turnaround_ticks == 1u);
        assert(d10s_bk4829_build_timing(48000000u, &built));
        assert(built.half_period_ticks == 2u && built.turnaround_ticks == 48u);
        assert(!d10s_bk4829_build_timing(100000000000u, &built));
    }
    puts("BK4829 bus tests: PASS"); return 0;
}
