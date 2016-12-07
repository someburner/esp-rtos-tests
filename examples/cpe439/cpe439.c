#include <string.h>
#include <ssid_config.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

QueueHandle_t tempPubQueue;

static int stagger = 0;

void tempTask(void *pvParameters)
{
   while(1)
   {
      vTaskDelayMs(TEMP_UPDATE_MS); // Print every second
      gpio_write(2, (stagger++)%2);

      if (stagger % 2)
         OW_request_new_temp();

      if (!( (stagger+1) % 10))
      {
         uint32_t freeHeap = sdk_system_get_free_heap_size();
         printf("freeHeap:%u\n", freeHeap);
      }
   }
}

// void tempPubRxTask(void *p)
// {
//    QueueHandle_t * tPubQueue = (QueueHandle_t *)p;
//
//    while(1) {
//       xQueueSendToBackFromISR(tsqueue, &now, NULL);
//
//       xQueueReceive(*tPubQueue, &button_ts, portMAX_DELAY);
//       vTaskDelayMs(1234UL); // Print every second
//
//    }
// }

void config_wifi()
{
   static struct sdk_station_config config = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASS,
   };

   printf("config_wifi\n");
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

   mqtt_app_init(&tempPubQueue);

   // gpio_enable(2, GPIO_OUTPUT); //GPIO_INPUT, GPIO_OUT_OPEN_DRAIN
   GPIO.ENABLE_OUT_SET = BIT(2);
   IOMUX_GPIO2 = IOMUX_GPIO2_FUNC_GPIO | IOMUX_PIN_OUTPUT_ENABLE;


   OW_init(&tempPubQueue);
   printf("Request temp ");
   if (OW_request_new_temp())
   {
      printf("OK\n");
      xTaskCreate(tempTask, (signed char *)"tempTask", 512, NULL, OW_TASK_PRIO, NULL);
      // xTaskCreate(tempPubRxTask, (signed char *)"tempPubRxTask", 256, &tempPubQueue, OW_TASK_PRIO, NULL);
   }
   else
   {
      printf("failed!\n");
   }

}
