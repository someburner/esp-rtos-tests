/* ========================================================================== *
 *                       WS2812 Driver Implementations                        *
 *            Drives WS2812 LED strip connected to MISO using HSPI.           *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
 /* Basic concepts derived from:
  + http://www.gammon.com.au/forum/?id=13357
  + https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
*/
#include "espressif/esp_common.h" // sdk_os_delay_us
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

#include "esp/spi2.h"
#include "hw_timer.h"
#include "ws2812.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

#define REFRESH_RATE 1000UL
#define INITIAL_BRIGHTNESS 128

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Obtain callback timer div from a desired period (in secs) and # of         *
 * callbacks per "period", and make sure it's at least 1ms                    */
#define MS_DIV_PER_CB(desired_per, cbs_per_per)  (( (uint32_t)(1000UL * (desired_per/cbs_per_per)) ) | 1UL)

#define MOD_BRIGHTNESS(pixel, level) \
   (pixel) = ( (pixel * level) >> 8 );

#define UPDATE_BRIGHTNESS() \
   if (ws->brightness) \
   { \
      MOD_BRIGHTNESS(ws->r, ws->brightness); \
      MOD_BRIGHTNESS(ws->g, ws->brightness); \
      MOD_BRIGHTNESS(ws->b, ws->brightness); \
   }

static ws2812_driver_t ws_driver;
static ws2812_driver_t * ws = &ws_driver;

static ws2812_fade_inout_t fade_st;
static ws2812_fade_inout_t * fade = &fade_st;

// float patterns [] [3] = {
//   { 0.5, 0, 0 },  // red
//   { 0, 0.5, 0 },  // green
//   { 0, 0, 0.5 },  // blue
//   { 0.5, 0.5, 0 },  // yellow
//   { 0.5, 0, 0.5 },  // magenta
//   { 0, 0.5, 0.5 },  // cyan
//   { 0.5, 0.5, 0.5 },  // white
//   { 160.0 / 512, 82.0 / 512, 45.0 / 512 },  // sienna
//   { 46.0 / 512, 139.0 / 512, 87.0 / 512 },  // sea green
// };

  // { 320.0 / 512, 164.0 / 512, 90.0 / 512 },  // sienna
  // { 92.0 / 512, 278.0 / 512, 87.0 / 174 },  // sea green

float patterns [] [3] = {
  { 0.5, 0, 0 },  // red
  { 0, 1, 0 },  // green
  { 0, 0, 1 },  // blue
  { 1, 1, 0 },  // yellow
  { 1, 0, 1 },  // magenta
  { 0, 1, 1 },  // cyan
  { 1, 1, 1 },  // white
  { 160.0 / 256, 82.0 / 256, 45.0 / 256 },  // sienna
  { 46.0 / 256, 139.0 / 256, 87.0 / 256 },  // sea green
};

/*
 * t_HIGH + T_LOW =1.25µs±150n
 * | LOW-ORDER (0-code)         |
 * |:--------------------------:|
 * |  On/Off | Min | Typ | Max  |
 * |  1-bit  | 200 | 350 | 500  |
 * |  0-bit  | 650 | 800 | 5000 |
 * | HIGH-ORDER (1-code)        |
 * |:--------------------------:|
 * |  On/Off | Min | Typ | Max  |
 * |  1-bit  | 550 | 700 | 5500 |
 * |  0-bit  | 450 | 600 | 5500 |
 *
 * ---> LATCH TIME: > 6000ns <---
40MHz Computation:                                                         *
 * -------------------------------------------------------------------------- *
 * 25ns per clock period                                                      *
 * 2*25 = 50ns per spi bit                                                    *
 * So for a zero-bit to have ~350ns pulse-width:                              *
 *    -> do 6x1-bit = 6x50ns = 300ns                                          *
 *    -> do 18x0-bit = 18x50ns = 900ns                                       *
 *    -> Which means dout = b11111111 00000000 0000000000000000 = 0xFF000000   *
 *    -> Which means dout = 111111 0000000000000000  = 0x003FC0000   *
 * 111111
 18
 11111111 11111111 11
 0xFFFFC0
 *       and total bits to write is 8+24 = 24                                 *
 *                                                                            *
 * So for a one-bit to have ~700ns pulse-width:                               *
 *    -> do 14x1-bit = 15x50ns = 700ns                                        *
 *    -> do 12x0-bit = 6x50ns = 900ns                                        *
 *    -> Which means dout = b 11111111111111 000000000000000000 = 0x7FFF0000   *
 *       and total bits to write is 14+12 = 26 bits                           *
 * Typical 700/350ns 1/0-bit
 * do 4x0-bit = 400ns
 * do 7x1-bit = 700ns
 * ---- LOW-ORDER ---
 * Typical 600/350ns 1/0-bit
 * do 8x1-bit = 400ns
 * do 7x0-bit = 700ns
 *
*/
void IRAM ws2812_sendByte(uint8_t b)
{
   static uint8_t bit;
   static uint32_t out;
   for ( bit = 0; bit < 8; bit++)
   {
      if (b & 0x80) // is high-order bit set?
      {
         //8, c, e
         out = 0xFFFFc0;
         spi_transaction(24,  out, 0);
      }
      else
      {
         /* 23/0x007F0000 - 350/800ns 1/0bit */
         /* 22/0x003F8000 - 350/800ns 1/0bit */
         out = 0xFE000000;
         spi_transaction(32,  out, 0);
      }
      b <<= 1; // shift next bit into high-order position
   }
}

// Display a single color on the whole string
// void IRAM ws2812_sendPixels()
void ws2812_sendPixels()
{
   // NeoPixel wants colors in green-then-red-then-blue order
   // WS2812B wants it RGB
   ws2812_sendByte(ws->r);
   ws2812_sendByte(ws->g);
   ws2812_sendByte(ws->b);
}

void IRAM ws2812_sendPixel_params(uint8_t r, uint8_t g, uint8_t b)
{
   // NeoPixel wants colors in green-then-red-then-blue order
   // WS2812B wants it RGB
   ws2812_sendByte(r);
   ws2812_sendByte(g);
   ws2812_sendByte(b);
}

// Display a single color on the whole string
void IRAM ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b)
{
   uint16_t pixel;
   for (pixel = 0; pixel < count; pixel++)
      ws2812_sendPixel_params(r, g, b);
   ws2812_show();  // latch the colors
}

void ws2812_showit_fade(void)
{
   static uint32_t intr_restore;
   sdk_os_delay_us(1);
   intr_restore = _xt_disable_interrupts();
   if (ws->cur_pixel < PIXEL_COUNT)
   {
      // _xt_isr_mask( (1<<INUM_TIMER_FRC1) | (1<<INUM_TICK) | (1<<INUM_SOFT) | (1<<INUM_TIMER_FRC2)  );
      ws2812_sendPixels();
      ws->cur_pixel++;
      // _xt_isr_unmask( (1<<INUM_TIMER_FRC1) | (1<<INUM_TICK) | (1<<INUM_SOFT) | (1<<INUM_TIMER_FRC2) );
   } else {
      ws->cur_pixel = 0;
   }
   _xt_restore_interrupts(intr_restore);
   sdk_os_delay_us(1);
}

/* Called from Driver_Event_Task */
void  ws2812_doit(void)
{
   if (ws->state == WS_STATE_DOIT_ACTIVE)
   {
      ws2812_showit_fade();
   }
}

/* Callback specifically for fade animation */
void ws2812_fade_cb(void)
{
   if (fade->cur_pattern >= (ARRAY_SIZE(patterns)))
   {
      fade->cur_pattern = 0;
   }

   if ( ( fade->in_out) && ( fade->cur_m >= 255 ) )
   {
      // printf("ws->r=%hhd\n", ws->r);
      fade->in_out = false;
   }

   if ( !(fade->in_out) && (fade->cur_m == 0) )
   {
      fade->cur_pattern++;
      fade->in_out = true;
   }

   /* Freshly populate rgb */
   ws->cur_pixel = 0;
   ws->r = patterns[fade->cur_pattern][0] * fade->cur_m;
   ws->g = patterns[fade->cur_pattern][1] * fade->cur_m;
   ws->b = patterns[fade->cur_pattern][2] * fade->cur_m;
   // UPDATE_BRIGHTNESS();

   if (fade->in_out)
      fade->cur_m++;
   else
      fade->cur_m--;
}

/* Initialize/start a preset animation */
void ws2812_anim_init(uint8_t anim_type)
{
   // os_timer_disarm(&ws2812_timer);
   // hw_timer_stop();

   switch (anim_type)
   {
      case WS2812_ANIM_FADE_INOUT:
      {
         ws->cur_anim = WS2812_ANIM_FADE_INOUT;
         fade->cur_m = 0;
         fade->cur_pattern = 0;
         fade->in_out = false;
         fade->period = 1.0; // seconds
         ws->state = WS_STATE_DOIT_LOADED;
         printf("fade inout anim init\n");
      } break;

      default:
         printf("Invalid anim_type\n");
         ws2812_clear();
         return;
   }

   if (ws->state == WS_STATE_DOIT_LOADED)
   {
      printf("Ihw_timer_start\n");
      ws->state = WS_STATE_DOIT_ACTIVE;
      hw_timer_init();
      hw_timer_start();
   }
}

void ws2812_set_brightness(uint8_t b)
{
   ws->brightness = b;
}

/* Stop currently running animation, if any. */
void ws2812_anim_stop(void)
{
   ws2812_clear();
   ws->state = WS2812_ANIM_INVALID;
   ws->cur_anim = WS_STATE_DOIT_STOPPED;
}

/* Used for init only since hw_timer delay is 10us, so about the same. */
void ws2812_show(void)
{
   sdk_os_delay_us(6);
}

void ws2812_clear(void)
{
   /* Set all to 0 */
   ws->r = 0;
   ws->g = 0;
   ws->b = 0;
   ws2812_sendPixels();
}

void animTask(void *p)
{
   while(1)
   {
      int anim = ws->cur_anim;
      if (ws->state == WS_STATE_DOIT_ACTIVE)
      {
         switch ( anim )
         {
            case WS2812_ANIM_FADE_INOUT:
            {
               // ws2812_fade_cb();
            } break;

            //ws2812_showColor (PIXELS, 0xB2, 0x22, 0x22);  // firebrick
         }
      }
      // vTaskDelayMs(MS_DIV_PER_CB(fade->period, 255));
      vTaskDelayMs(15);
   }
}

bool ws2812_spi_init(void)
{
   spi_init_gpio(HSPIBUS, SPI_CLK_USE_DIV);
   spi_clock(HSPIBUS, 2, 2); // prediv==2==40mhz, postdiv==2==20mhz
   spi_tx_byte_order(HSPIBUS, SPI_BYTE_ORDER_HIGH_TO_LOW);
   spi_rx_byte_order(HSPIBUS, SPI_BYTE_ORDER_HIGH_TO_LOW);

   /* Mode 0: CPOL=0, CPHA=0 */
   /* uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol */
   spi_mode(HSPIBUS, 0, 0);

   SET_PERI_REG_MASK(SPI_USER(HSPIBUS), SPI_CS_SETUP|SPI_CS_HOLD);
   CLEAR_PERI_REG_MASK(SPI_USER(HSPIBUS), SPI_FLASH_MODE);

   return true;
}

bool ws2812_init(void)
{
   if (ws2812_spi_init())
   {
      printf("ws2812_spi_init ok\n");
      ws2812_clear();
   }

   ws->brightness = INITIAL_BRIGHTNESS;

   // driver_event_register_ws(ws);

   xTaskCreate(&animTask, "animTask", 1024, NULL, 0, NULL);

   return true;
}
