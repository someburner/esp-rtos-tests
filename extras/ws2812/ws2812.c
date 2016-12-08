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

// os_timer_t ws2812_timer;

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
  { 1, 0, 0 },  // red
  { 0, 1, 0 },  // green
  { 0, 0, 1 },  // blue
  { 1, 1, 0 },  // yellow
  { 1, 0, 1 },  // magenta
  { 0, 1, 1 },  // cyan
  { 1, 1, 1 },  // white
  { 160.0 / 256, 82.0 / 256, 45.0 / 256 },  // sienna
  { 46.0 / 256, 139.0 / 256, 87.0 / 256 },  // sea green
};

/* 40MHz Computation:                                                         *
 * -------------------------------------------------------------------------- *
 * 25ns per clock period                                                      *
 * 2*25 = 50ns per spi bit                                                    *
 * So for a zero-bit to have a 350ns pulse-width:                             *
 *    -> do 7x1-bit = 7x50ns = 350ns                                          *
 *    -> do 16x0-bit = 16x50ns = 800ns                                        *
 *    -> Which means dout = b1111111 0000000000000000 = 0x3FFF000             *
 *       and total bits to write is 16+7 = 23                                 *
 *                                                                            *
 * So for a one-bit to have a 700ns pulse-width:                              *
 *    -> do 14x1-bit = 14x50ns = 700ns                                        *
 *    -> do 12x0-bit = 12x50ns = 600ns                                        *
 *    -> Which means dout = b 11111111111111 000000000000 = 0x3FFF000         *
 *       and total bits to write is 14+12 = 26 bits                           *
*/
void  ws2812_sendByte(uint8_t b)
{
   static uint8_t bit;
   static uint32_t out;
   for ( bit = 0; bit < 8; bit++)
   {
      if (b & 0x80) // is high-order bit set?
      {
         out = 0x3FFF000;
         spi_transaction(HSPIBUS,0, 0, 0, 0, 26,  out, 0, NULL, 0);
     // spi_transaction spi_no,  cmd_bits, cmd_data, addr_bits, addr_data, dout_bits, dout_data, din, din, dummy
     // spi_transaction(SPI_DEV,      0,      0,          0,       0,       26,    out,        0,  NULL,  0);
      }
      else
      {
         out = 0x7F0000;
         spi_transaction(HSPIBUS, 0, 0, 0, 0, 23, out, 0, NULL, 0);
      }
      b <<= 1; // shift next bit into high-order position
      sdk_os_delay_us(1); // seems to be more stable if this is added.
   }
}

// Display a single color on the whole string
void ws2812_sendPixels()
{
    // NeoPixel wants colors in green-then-red-then-blue order
   ws2812_sendByte(ws->g);
   ws2812_sendByte(ws->r);
   ws2812_sendByte(ws->b);
}

void ws2812_sendPixel_params(uint8_t r, uint8_t g, uint8_t b)
{
   ws2812_sendByte(g); // NeoPixel wants colors in green-then-red-then-blue order
   ws2812_sendByte(r);
   ws2812_sendByte(b);
}

// Display a single color on the whole string
void ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b)
{
   uint16_t pixel;
   // ets_intr_lock(); /* These seem to be OK */
   for (pixel = 0; pixel < count; pixel++)
      ws2812_sendPixel_params(r, g, b);
   // ets_intr_unlock(); /* These seem to be OK */
   ws2812_show();  // latch the colors
}

void ws2812_showit_fade(void)
{
   if (ws->cur_pixel < PIXEL_COUNT)
   {
      ws2812_sendPixels();
      ws->cur_pixel++;
   } else {
      ws->cur_pixel = 0;
   }
}

/* Called from Driver_Event_Task */
void  ws2812_doit(void)
{
   ws2812_showit_fade();

   // if (ws->state == WS_STATE_DOIT_ACTIVE)
   // {
   //    ws2812_showit_fade();
   //    // hw_timer_start();
   // }
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
      printf("ws->r=%hhd\n", ws->r);
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
   // ws2812_doit();

   if (fade->in_out)
      fade->cur_m++;
   else
      fade->cur_m--;
}

#if 0
/* Timer callback every 10ms for updating colors. */
static void ws2812_timer_cb(void* arg)
{
   uint32_t anim_type = *(uint32_t *)arg;

   switch (anim_type)
   {
      case WS2812_ANIM_FADE_INOUT:
      {
         ws2812_fade_cb();
      } break;

   }

}
#endif


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
         fade->in_out = true;
         fade->period = 5.0; // seconds
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
   sdk_os_delay_us(9);
   // sdk_os_delay_us(1);
}

void ws2812_clear(void)
{
   /* Set all to 0 */
   ws->r = 0;
   ws->g = 0;
   ws->b = 0;

   ws2812_show(); // in case MOSI went high, latch in whatever-we-sent
   ws2812_sendPixels();
   ws2812_show();
}

void animTask(void *p)
{
   while(1)
   {
      static int test = 0;
      if (test%100 == 0)
         printf(".");

      int anim = ws->cur_anim;
      if (ws->state == WS_STATE_DOIT_ACTIVE)
      {
         switch ( anim )
         {
            case WS2812_ANIM_FADE_INOUT:
            {
               ws2812_fade_cb();
               test++;
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
   spi_clock(HSPIBUS, 2, 2); // prediv==2==40mhz, postdiv==1==40mhz
   spi_tx_byte_order(HSPIBUS, SPI_BYTE_ORDER_HIGH_TO_LOW);
   spi_rx_byte_order(HSPIBUS, SPI_BYTE_ORDER_HIGH_TO_LOW);

   /* Mode 1: CPOL=0, CPHA=1 */
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

   xTaskCreate(&animTask, "animTask", 1024, NULL, 7, NULL);

   return true;
}
