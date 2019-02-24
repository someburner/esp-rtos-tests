#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "ws2812.h"
#include "cpe439.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

QueueHandle_t tempPubQueue;
QueueHandle_t rgbRxQueue;

static int stagger = 0;
static uint8_t cur_color[3] = {0, 255, 0};

void tempTask(void *pvParameters)
{
   mqtt_app_init(&tempPubQueue, &rgbRxQueue);

   /* Wait until we have joined AP and are assigned an IP */
   while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP)
      vTaskDelayMs(100);

   GPIO.ENABLE_OUT_SET = BIT(2);
   IOMUX_GPIO2 = IOMUX_GPIO2_FUNC_GPIO | IOMUX_PIN_OUTPUT_ENABLE;

   OW_init(&tempPubQueue);
   ws2812_init();

   printf("Request temp ");
   if (OW_request_new_temp())
   {
      printf("OK\n");
   }
   else
   {
      printf("failed!\n");
   }

   while(1)
   {
      gpio_write(2, (stagger++)%2);

      if (stagger % 2)
      {
         OW_request_new_temp();
      }

      if (!( (stagger+1) % 10))
      {
         uint32_t freeHeap = sdk_system_get_free_heap_size();
         printf("freeHeap:%u\n", freeHeap);
      }
      vTaskDelayMs(2000); // Print every second
      // vTaskDelayMs(TEMP_UPDATE_MS); // Print every second
      taskYIELD();
   }
}

void colorLock(void *pvParameters)
{
   while(1)
   {
      uint8_t msg[3];
      while (xQueueReceive(rgbRxQueue, (void *)msg, 0) == pdTRUE)
      {
         printf("rgb queue rx: r:%dg:%db:%d\n", msg[0], msg[1], msg[2] );
         cur_color[0] =  msg[0];
         cur_color[1] =  msg[1];
         cur_color[2] =  msg[2];
         // vTaskDelayMs(123);
      }
      // firebrick
      ws2812_showColor(PIXEL_COUNT, cur_color[0], cur_color[1], cur_color[2]);
      vTaskDelayMs(597);
   }
}

void config_wifi()
{
   static struct sdk_station_config config = {
      .ssid = MAKE_STRING(WIFI_SSID),
      .password = MAKE_STRING(WIFI_PASS),
   };

   printf("config_wifi\n");
   printf("SSID: %s\n", config.ssid);
   printf("PASS: %s\n", config.password);
   sdk_wifi_set_opmode(STATION_MODE);
   sdk_wifi_station_set_config(&config);
}

void user_init(void)
{
   uint8_t clock_freq = sdk_system_get_cpu_freq();
   uart_set_baud(0, BAUD_RATE);
   config_wifi();

   printf("cpe439 project init\n");
   printf("SDK ver: %s\n", sdk_system_get_sdk_version());
   printf("Clock Freq = %huMHz\n", clock_freq);

   // "+22.43\0"
   tempPubQueue = xQueueCreate(10, sizeof(char*));

   rgbRxQueue = xQueueCreate(10, sizeof(uint8_t)*3);

   xTaskCreate(&colorLock, "colorLock", 256, NULL, WS_FADE_TASK_PRIO, NULL);

   xTaskCreate(tempTask, (signed char *)"tempTask", 512, NULL, TEMP_PUB_TASK_PRIO, NULL);

#if 0
   if (ws2812_init())
      printf("ws2812_init ok\n");
   ws2812_anim_init(WS2812_ANIM_FADE_INOUT);
#endif




}
