#include "st7565.h"
#include "mono128x64.h"

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
    fake_lcd_t *f=c; CHECK(f->selected);
    CHECK(f->count<sizeof(f->bytes));
    f->bytes[f->count]=v; f->modes[f->count]=f->data_mode; ++f->count;
    return true;
}
static bool delay(uint16_t ms, void *c) {
    fake_lcd_t *f=c; CHECK(ms>0u); ++f->delays; return true;
}
static void test_mono_framebuffer_flushes_all_st7565_pages(void)
{
    fake_lcd_t f;
    d10s_st7565_t lcd;
    d10s_st7565_port_t port = {select, mode, write_byte, delay, &f};
    d10s_st7565_config_t config = {4u, 31u, 4u, 7u, false, false,
                                   false, 120u, 40u};
    d10s_mono128x64_t framebuffer;

    memset(&f, 0, sizeof(f));
    d10s_mono128x64_clear(&framebuffer, false);
    d10s_mono128x64_pixel(&framebuffer, 0u, 0u, true);
    d10s_mono128x64_pixel(&framebuffer, 127u, 63u, true);
    CHECK(d10s_st7565_init(&lcd, &port, &config));
    f.count = 0u;
    CHECK(d10s_mono128x64_flush(&framebuffer, &lcd));
    CHECK(f.count == D10S_ST7565_PAGES * 131u);
    CHECK(f.bytes[3u] == 0x01u);
    CHECK(f.bytes[7u * 131u + 3u + 127u] == 0x80u);
    CHECK(!f.selected);
}

int main(void) {
    fake_lcd_t f; d10s_st7565_t lcd;
    d10s_st7565_port_t port={select,mode,write_byte,delay,&f};
    d10s_st7565_config_t config={4u,31u,4u,7u,false,false,false,120u,40u};
    uint8_t page[128]; uint8_t frame[8][128];
    memset(&f,0,sizeof(f)); memset(page,0xA5,sizeof(page));
    memset(frame,0x5A,sizeof(frame));
    CHECK(d10s_st7565_init(&lcd,&port,&config));
    CHECK(lcd.initialized && !f.selected && f.delays==2u);
    f.count=0u; f.select_asserts=0u; f.select_releases=0u;
    CHECK(d10s_st7565_flush_page(&lcd,3u,page));
    CHECK(f.count==131u && f.bytes[0]==0xB3u &&
           f.bytes[1]==0x10u && f.bytes[2]==0x04u);
    CHECK(!f.modes[0] && !f.modes[1] && !f.modes[2]);
    for(unsigned i=3u;i<131u;i++) CHECK(f.modes[i] && f.bytes[i]==0xA5u);
    CHECK(!f.selected);
    f.count=0u;
    CHECK(d10s_st7565_flush_frame(&lcd,(const uint8_t (*)[128])frame));
    CHECK(f.count==8u*131u && !f.selected);
    CHECK(!d10s_st7565_flush_page(&lcd,8u,page));
    test_mono_framebuffer_flushes_all_st7565_pages();
    puts("ST7565 transport tests: PASS"); return 0;
}
