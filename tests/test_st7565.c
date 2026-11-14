#include "st7565.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    bool selected, data_mode;
    uint8_t bytes[1100];
    bool modes[1100];
    unsigned count, delays, select_asserts, select_releases;
} fake_lcd_t;
static bool select(bool asserted, void *c) {
    fake_lcd_t *f=c; f->selected=asserted;
    if(asserted) ++f->select_asserts; else ++f->select_releases;
    return true;
}
static bool mode(bool is_data, void *c) {
    fake_lcd_t *f=c; f->data_mode=is_data; return true;
}
static bool write_byte(uint8_t v, void *c) {
    fake_lcd_t *f=c; assert(f->selected);
    assert(f->count<sizeof(f->bytes));
    f->bytes[f->count]=v; f->modes[f->count]=f->data_mode; ++f->count;
    return true;
}
static bool delay(uint16_t ms, void *c) {
    fake_lcd_t *f=c; assert(ms>0u); ++f->delays; return true;
}
int main(void) {
    fake_lcd_t f; d10s_st7565_t lcd;
    d10s_st7565_port_t port={select,mode,write_byte,delay,&f};
    d10s_st7565_config_t config={4u,31u,4u,7u,false,false,false,120u,40u};
    uint8_t page[128]; uint8_t frame[8][128];
    memset(&f,0,sizeof(f)); memset(page,0xA5,sizeof(page));
    memset(frame,0x5A,sizeof(frame));
    assert(d10s_st7565_init(&lcd,&port,&config));
    assert(lcd.initialized && !f.selected && f.delays==2u);
    f.count=0u; f.select_asserts=0u; f.select_releases=0u;
    assert(d10s_st7565_flush_page(&lcd,3u,page));
    assert(f.count==131u && f.bytes[0]==0xB3u &&
           f.bytes[1]==0x10u && f.bytes[2]==0x04u);
    assert(!f.modes[0] && !f.modes[1] && !f.modes[2]);
    for(unsigned i=3u;i<131u;i++) assert(f.modes[i] && f.bytes[i]==0xA5u);
    assert(!f.selected);
    f.count=0u;
    assert(d10s_st7565_flush_frame(&lcd,(const uint8_t (*)[128])frame));
    assert(f.count==8u*131u && !f.selected);
    assert(!d10s_st7565_flush_page(&lcd,8u,page));
    puts("ST7565 transport tests: PASS"); return 0;
}
