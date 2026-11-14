#include "bk4829_scanner_adapter.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool csn, clock, output;
    bool input[64];
    unsigned input_count, input_index;
    unsigned init_calls, resume_calls, stop_calls;
    uint32_t initial_hz, resumed_hz;
} fake_t;

static bool csn(bool v, void *c) { ((fake_t *)c)->csn=v; return true; }
static bool clk(bool v, void *c) { ((fake_t *)c)->clock=v; return true; }
static bool dir(bool v, void *c) { ((fake_t *)c)->output=v; return true; }
static bool wr(bool v, void *c) { (void)v; (void)c; return true; }
static bool rd(bool *v, void *c) {
    fake_t *f=c;
    if(f->input_index>=f->input_count) return false;
    *v=f->input[f->input_index++]; return true;
}
static bool delay(uint16_t n, void *c) { (void)c; return n!=0u; }
static bool initialize(const d10s_bk4829_bus_port_t *p,
                       const d10s_bk4829_bus_timing_t *t,
                       uint32_t hz, void *c) {
    fake_t *f=c; assert(p && t); ++f->init_calls; f->initial_hz=hz; return true;
}
static bool resume(uint32_t hz, void *c) {
    fake_t *f=c; ++f->resume_calls; f->resumed_hz=hz; return true;
}
static void append_bits(fake_t *f, uint16_t value) {
    for(unsigned i=0u;i<16u;i++)
        f->input[f->input_count++]=((value>>(15u-i))&1u)!=0u;
}
int main(void) {
    fake_t f; d10s_bk4829_scanner_adapter_t adapter;
    d10s_spectrum_radio_ops_t ops={0};
    d10s_bk4829_bus_port_t bus;
    const d10s_bk4829_bus_timing_t timing={2u,3u};
    uint32_t actual=0u; uint16_t rssi=0u;
    memset(&f,0,sizeof(f)); f.csn=true; f.output=true;
    bus=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&f};
    assert(d10s_bk4829_scanner_adapter_init(&adapter,&bus,&timing,
                                             initialize,resume,
                                             144000000u,&f));
    assert(f.init_calls==1u && f.initial_hz==144000000u);
    d10s_bk4829_scanner_adapter_bind(&adapter,&ops);
    assert(ops.tune_hz && ops.read_rssi && ops.set_attenuation==NULL);
    assert(!ops.tune_hz(ops.context,600000000u,&actual));
    assert(ops.tune_hz(ops.context,144025000u,&actual));
    assert(actual==144025000u && f.resume_calls==1u &&
           f.resumed_hz==144025000u);

    append_bits(&f,0x0123u); /* discarded first post-tune sample */
    append_bits(&f,0x0156u); /* returned sample */
    assert(ops.read_rssi(ops.context,&rssi) && rssi==0x0156u);
    append_bits(&f,0x0100u); /* no second discard without another tune */
    assert(ops.read_rssi(ops.context,&rssi) && rssi==0x0100u);

    d10s_bk4829_scanner_adapter_stop(&adapter);
    assert(!ops.read_rssi(ops.context,&rssi));
    puts("BK4829 scanner adapter tests: PASS"); return 0;
}
