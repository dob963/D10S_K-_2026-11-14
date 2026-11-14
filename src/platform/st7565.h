#ifndef D10S_ST7565_H
#define D10S_ST7565_H

#include <stdbool.h>
#include <stdint.h>

#define D10S_ST7565_WIDTH 128u
#define D10S_ST7565_PAGES 8u

typedef struct {
    bool (*chip_select)(bool asserted, void *context);
    bool (*data_mode)(bool data, void *context);
    bool (*write_byte)(uint8_t value, void *context);
    bool (*delay_ms)(uint16_t milliseconds, void *context);
    void *context;
} d10s_st7565_port_t;

typedef struct {
    uint8_t column_offset;
    uint8_t contrast;
    uint8_t resistor_ratio;
    uint8_t power_control;
    bool reverse_segment;
    bool reverse_common;
    bool inverse;
    uint16_t reset_delay_ms;
    uint16_t power_delay_ms;
} d10s_st7565_config_t;

typedef struct {
    d10s_st7565_port_t port;
    d10s_st7565_config_t config;
    bool initialized;
} d10s_st7565_t;

bool d10s_st7565_init(d10s_st7565_t *display,
                      const d10s_st7565_port_t *port,
                      const d10s_st7565_config_t *config);
bool d10s_st7565_flush_page(d10s_st7565_t *display, uint8_t page,
                            const uint8_t pixels[D10S_ST7565_WIDTH]);
bool d10s_st7565_flush_frame(d10s_st7565_t *display,
                             const uint8_t framebuffer[D10S_ST7565_PAGES]
                                                      [D10S_ST7565_WIDTH]);

#endif
