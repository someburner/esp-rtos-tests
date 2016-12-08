#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"

#include "../onewire/maxim28.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

static int onoff = 1;

void task1(void *pvParameters)
{
   while(1) {
      vTaskDelayMs(2000UL); // Print every second
      /* Should be close to 1000 */
      // printf("test count = %lu\n", count);
      gpio_write(2, onoff);
      onoff = !onoff;

      OW_request_new_temp();
   }
}

void user_init(void)
{
   uint8_t clock_freq = sdk_system_get_cpu_freq();
   uart_set_baud(0, BAUD_RATE);

   printf("onewire_hw_test\n");
   printf("SDK ver: %s\n", sdk_system_get_sdk_version());
   printf("Clock Freq = %huMHz\n", clock_freq);

   // gpio_enable(2, GPIO_OUTPUT); //GPIO_INPUT, GPIO_OUT_OPEN_DRAIN
   GPIO.ENABLE_OUT_SET = BIT(2);
   IOMUX_GPIO2 = IOMUX_GPIO2_FUNC_GPIO | IOMUX_PIN_OUTPUT_ENABLE;

   OW_init();
   printf("Request temp ");
   if (OW_request_new_temp())
      printf("OK\n");
   else
      printf("failed!\n");


   xTaskCreate(task1, (signed char *)"hw_timer_task", 256, NULL, 2, NULL);
}
