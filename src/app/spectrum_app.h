#ifndef D10S_SPECTRUM_APP_H
#define D10S_SPECTRUM_APP_H

#include "mono128x64.h"
#include "spectrum_scanner.h"
#include "st7565.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    D10S_APP_IDLE = 0,
    D10S_APP_BUSY,
    D10S_APP_FRAME_UPDATED,
    D10S_APP_ERROR
} d10s_spectrum_app_status_t;

typedef struct {
    d10s_spectrum_scanner_t scanner;
    d10s_spectrum_radio_ops_t radio;
    d10s_mono128x64_t framebuffer;
    d10s_st7565_t *display;
    uint16_t cursor_bin;
    bool cursor_enabled;
    bool bold_labels;
    bool initialized;
    d10s_spectrum_result_t last_result;
} d10s_spectrum_app_t;

bool d10s_spectrum_app_init(d10s_spectrum_app_t *app,
                            const d10s_spectrum_band_t *bands,
                            uint8_t band_count,
                            const d10s_spectrum_radio_ops_t *radio,
                            d10s_st7565_t *display,
                            const d10s_spectrum_config_t *detector_config,
                            uint16_t settle_ms);
d10s_spectrum_app_status_t d10s_spectrum_app_tick(
                            d10s_spectrum_app_t *app,
                            uint32_t now_ms);
bool d10s_spectrum_app_set_cursor(d10s_spectrum_app_t *app,
                                  bool enabled,
                                  uint16_t bin_index);
bool d10s_spectrum_app_set_bold_labels(d10s_spectrum_app_t *app,
                                       bool enabled);
bool d10s_spectrum_app_set_delay(d10s_spectrum_app_t *app,
                                 uint16_t settle_ms);
bool d10s_spectrum_app_set_turbo(d10s_spectrum_app_t *app,
                                 bool enabled,
                                 uint16_t min_settle_ms,
                                 uint16_t poll_interval_ms,
                                 uint8_t stable_delta,
                                 uint8_t stable_samples);
bool d10s_spectrum_app_set_attenuation(d10s_spectrum_app_t *app,
                                       uint8_t level,
                                       uint16_t measured_raw_rssi_offset);
bool d10s_spectrum_app_select_band(d10s_spectrum_app_t *app,
                                   uint8_t band_index);
void d10s_spectrum_app_stop(d10s_spectrum_app_t *app);

#endif
