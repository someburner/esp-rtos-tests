/* ========================================================================== *
 *                               WS2812 Header                                *
 *  Contains presets and struct definitions for driving the WS2812 via HSPI.  *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#ifndef _WS2812__h
#define _WS2812__h

#include <stdint.h>
#include <stddef.h> // size_t

#include "espressif/esp_common.h" // sdk_os_delay_us
#include "esp/gpio.h"

#define WS2812_ANIM_INVALID    0
#define WS2812_ANIM_FADE_INOUT 1
#define WS2812_ANIM_COLOR_ONLY 2
#define WS2812_ANIM_MAX        3

typedef enum {
   WS_STATE_INVALID = 0,
   WS_STATE_DOIT_LOADED,
   WS_STATE_DOIT_STOPPED,
   WS_STATE_DOIT_ACTIVE
} WS_STATE_T;

typedef void (*ws2812_cb_t)(void);

typedef struct
{
   uint8_t cur_m;
   uint8_t cur_pattern;
   uint8_t in_out; // bool
   uint8_t padding;
   float   period;
} ws2812_fade_inout_t;

typedef struct
{
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t brightness;

   uint16_t cur_pixel;
   uint16_t cur_anim;

   // os_timer_t timer;
   uint32_t state;

   uint32_t timer_arg;

} ws2812_driver_t;

void IRAM ws2812_sendByte(uint8_t b);
// void IRAM ws2812_sendPixels(void);
void  ws2812_sendPixels(void);

void ws2812_fade_cb(void);
void ws2812_sendPixel_params(uint8_t r, uint8_t g, uint8_t b);
void ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b);

#if 0
void ws2812_colorTransition(  uint8_t from_r, uint8_t from_g, uint8_t from_b,
                              uint8_t to_r,   uint8_t to_g,   uint8_t to_b);
#endif

void ws2812_doit(void);

void ws2812_anim_init(uint8_t anim_type);
void ws2812_showit_fade(void);
void ws2812_anim_stop(void);

void ws2812_set_brightness(uint8_t brightness);

void ws2812_show(void);
void ws2812_clear(void);

bool ws2812_spi_init(void);
bool ws2812_init(void);




#endif
/* End */
