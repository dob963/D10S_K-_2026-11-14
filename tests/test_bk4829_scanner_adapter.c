#include "bk4829_scanner_adapter.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct {
    bool csn, clock, output;
    bool input[128];
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
    fake_t *f=c; CHECK(p && t); ++f->init_calls; f->initial_hz=hz; return true;
}
static bool resume(uint32_t hz, void *c) {
    fake_t *f=c; ++f->resume_calls; f->resumed_hz=hz; return true;
}
static bool stop_rx(void *c) { ++((fake_t *)c)->stop_calls; return true; }
static void append_bits(fake_t *f, uint16_t value) {
    for(unsigned i=0u;i<16u;i++)
        f->input[f->input_count++]=((value>>(15u-i))&1u)!=0u;
}

static bool initialize_fail(const d10s_bk4829_bus_port_t *p,
                            const d10s_bk4829_bus_timing_t *t,
                            uint32_t hz, void *c)
{
    (void)p; (void)t; (void)hz; (void)c;
    return false;
}

static void test_failed_receiver_initialization_stays_not_ready(void)
{
    fake_t f;
    d10s_bk4829_scanner_adapter_t adapter;
    d10s_bk4829_bus_port_t bus;
    const d10s_bk4829_bus_timing_t timing = {2u, 3u};

    memset(&f, 0, sizeof(f));
    bus = (d10s_bk4829_bus_port_t){csn, clk, dir, wr, rd, delay, &f};
    CHECK(!d10s_bk4829_scanner_adapter_init(&adapter, &bus, &timing,
                                             initialize_fail, resume, stop_rx,
                                             144000000u, &f));
    CHECK(!adapter.ready);
    CHECK(f.stop_calls == 1u);
}

int main(void) {
    fake_t f; d10s_bk4829_scanner_adapter_t adapter;
    d10s_spectrum_radio_ops_t ops={0};
    d10s_bk4829_bus_port_t bus;
    const d10s_bk4829_bus_timing_t timing={2u,3u};
    uint32_t actual=0u; uint16_t rssi=0u;
    memset(&f,0,sizeof(f)); f.csn=true; f.output=true;
    bus=(d10s_bk4829_bus_port_t){csn,clk,dir,wr,rd,delay,&f};
    CHECK(d10s_bk4829_scanner_adapter_init(&adapter,&bus,&timing,
                                             initialize,resume,stop_rx,
                                             144000000u,&f));
    CHECK(f.init_calls==1u && f.initial_hz==144000000u);
    d10s_bk4829_scanner_adapter_bind(&adapter,&ops);
    CHECK(ops.tune_hz && ops.read_rssi && ops.set_attenuation!=NULL && ops.rssi_ready!=NULL);
    CHECK(!ops.tune_hz(ops.context,600000000u,&actual));
    CHECK(ops.tune_hz(ops.context,144025000u,&actual));
    CHECK(actual==144025000u && f.resume_calls==1u &&
           f.resumed_hz==144025000u);

    {
        bool ready = true;
        append_bits(&f,0x00C8u); /* REG_63 low byte 200: unsettled */
        CHECK(ops.rssi_ready(ops.context,&ready) && !ready);
        append_bits(&f,0x00C7u); /* below the upstream 200 threshold */
        CHECK(ops.rssi_ready(ops.context,&ready) && ready);
    }
    append_bits(&f,0x0123u); /* discard first post-tune sample */
    append_bits(&f,0x0156u); /* return second sample */
    CHECK(ops.read_rssi(ops.context,&rssi) && rssi==0x0156u);
    append_bits(&f,0x0100u); /* no extra discard before next tune */
    CHECK(ops.read_rssi(ops.context,&rssi) && rssi==0x0100u);

    d10s_bk4829_scanner_adapter_stop(&adapter);
    CHECK(f.stop_calls==1u);
    CHECK(!ops.read_rssi(ops.context,&rssi));
    test_failed_receiver_initialization_stays_not_ready();
    puts("BK4829 scanner adapter tests: PASS"); return 0;
}
