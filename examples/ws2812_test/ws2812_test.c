/**
 * @file   es2812_rainbow.c
 * @author Ondřej Hruška, 2016
 *
 * @brief  Example of a rainbow effect with
 *         WS2812 connected to GPIO2.
 *
 * This demo is in the public domain.
 */

#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp/uart.h" // uart_set_baud
#include <stdio.h> // printf
#include <stdint.h>

#include "ws2812.h"

#define PIXELS 32

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

/** GPIO number used to control the RGBs */
static const uint8_t ws_pin = 13; // MOSI (D7 on NodeMCU)

void colorLock(void *pvParameters)
{
   while(1)
   {
      vTaskDelayMs(1); // Print every second
      ws2812_showColor (PIXELS, 0xB2, 0x22, 0x22);  // firebrick
   }
}

void user_init(void)
{
   uint8_t clock_freq = sdk_system_get_cpu_freq();
   uart_set_baud(0, BAUD_RATE);

   printf("ws2812_test init\n");
   printf("SDK ver: %s\n", sdk_system_get_sdk_version());
   printf("Clock Freq = %huMHz\n", clock_freq);

   // Configure the GPIO
   gpio_enable(ws_pin, GPIO_OUTPUT);

   if (ws2812_init())
   {
      printf("ws2812_init ok\n");
   }

    xTaskCreate(&colorLock, "colorLock", 256, NULL, 7, NULL);
}




//end
