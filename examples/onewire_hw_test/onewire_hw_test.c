/* Very basic example to test the pwm library
 * Hook up an LED to pin14 and you should see the intensity change
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Javier Cardona (https://github.com/jcard0na)
 * BSD Licensed as described in the file LICENSE
 */
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"


#include "maxim28.h"


/* use this to tell above timer which seq it was armed from */
static int onewire_timer_arg = DS_SEQ_INVALID;

/* Onewire driver declaration */
static onewire_driver_t onewire_driver;
static onewire_driver_t * one_driver = &onewire_driver;

/* Temperature data */
static Temperature latest_temp;

#define SENSOR_GPIO 5 //GPIO5

void task1(void *pvParameters)
{
    printf("Hello from ow_task!\r\n");
    uint32_t const init_count = 0;
    while(1) {
        vTaskDelay(100);
        uint32_t count = getTestCount();
        printf("test count = %lu\n", count);
    }
}

void user_init(void)
{
   uart_set_baud(0, BAUD_RATE);

   printf("onewire_hw_test\n");
   printf("SDK version:%s\n", sdk_system_get_sdk_version());

   memset(one_driver, 0, sizeof(onewire_driver));
   memset(&latest_temp, 0, sizeof(latest_temp));

   // gpio_set_pullup(SENSOR_GPIO, true, true);

   /* Init hw timer */
   printf("ow_hw_init()\n");
   ow_hw_init();
   ow_hw_start();



   xTaskCreate(task1, (signed char *)"ow_task", 256, NULL, 2, NULL);
}
