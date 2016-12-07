#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

static int onoff = 1;

void tempTask(void *pvParameters)
{
   while(1)
   {
      vTaskDelayMs(TEMP_UPDATE_MS); // Print every second
      gpio_write(2, onoff);
      onoff = !onoff;
      OW_request_new_temp();
   }
}

void task1(void *pvParameters)
{
   while(1) {
      vTaskDelayMs(1234UL); // Print every second
   }
}

void user_init(void)
{
   uint8_t clock_freq = sdk_system_get_cpu_freq();
   uart_set_baud(0, BAUD_RATE);

   printf("cpe439 project init\n");
   printf("SDK ver: %s\n", sdk_system_get_sdk_version());
   printf("Clock Freq = %huMHz\n", clock_freq);

   mqtt_app_init();

   // gpio_enable(2, GPIO_OUTPUT); //GPIO_INPUT, GPIO_OUT_OPEN_DRAIN
   GPIO.ENABLE_OUT_SET = BIT(2);
   IOMUX_GPIO2 = IOMUX_GPIO2_FUNC_GPIO | IOMUX_PIN_OUTPUT_ENABLE;

   OW_init();
   printf("Request temp ");
   if (OW_request_new_temp())
   {
      printf("OK\n");
      xTaskCreate(tempTask, (signed char *)"tempTask", 256, NULL, OW_TASK_PRIO, NULL);
   }
   else
   {
      printf("failed!\n");
   }

   xTaskCreate(task1, (signed char *)"task1", 256, NULL, 0, NULL);
}
