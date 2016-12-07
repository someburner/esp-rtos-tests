#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include <semphr.h>

#if MQTT_0LEN_CLIENT_ID==1
#define MQTT_CLEANSESSION
#endif

SemaphoreHandle_t wifi_alive;
QueueHandle_t publish_queue;
#define TEMP_DATA_PUBLEN 7
#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

static void  beat_task(void *p)
{
   QueueHandle_t *tempQueueHandle = (QueueHandle_t *)p;
   // TickType_t xLastWakeTime = xTaskGetTickCount();

   while (1)
   {
      char * msg;
      if (!(xQueueReceive(*tempQueueHandle, &msg, portMAX_DELAY)))
      {
         printf("xqfailrx\n");
      }
      else
      {
         printf("xQueueReceive %s\n", msg);
         if (xQueueSend(publish_queue, msg, 0) == pdFALSE)
            printf("xqfailtx\n");
         else
         {
            printf("xQueueSend ok\n");
         }
         free(msg);
         msg = NULL;
      }

      taskYIELD();

      // printf("Sent!\r\n");
      // vTaskDelayMs(553);
   }
}

static void  topic_received(mqtt_message_data_t *md)
{
    int i;
    mqtt_message_t *message = md->message;
    printf("Received: ");
    for( i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[ i ]);

    printf(" = ");
    for( i = 0; i < (int)message->payloadlen; ++i)
        printf("%c", ((char *)(message->payload))[i]);

    printf("\r\n");
}

#if MQTT_0LEN_CLIENT_ID==0
static const char *  get_my_id(void)
{
    // Use MAC address for Station as unique ID
    static char my_id[13];
    static bool my_id_done = false;
    int8_t i;
    uint8_t x;
    if (my_id_done)
        return my_id;
    if (!sdk_wifi_get_macaddr(STATION_IF, (uint8_t *)my_id))
        return NULL;
    for (i = 5; i >= 0; --i)
    {
        x = my_id[i] & 0x0F;
        if (x > 9) x += 7;
        my_id[i * 2 + 1] = x + '0';
        x = my_id[i] >> 4;
        if (x > 9) x += 7;
        my_id[i * 2] = x + '0';
    }
    my_id[12] = '\0';
    my_id_done = true;
    return my_id;
}
#endif

static void  mqtt_task(void *pvParameters)
{
   int ret         = 0;
   struct mqtt_network network;
   mqtt_client_t client   = mqtt_client_default;

#if MQTT_0LEN_CLIENT_ID==1
   char mqtt_client_id[2] = { 0, 0 };
#else
   char mqtt_client_id[20];
#endif
   uint8_t mqtt_buf[100];
   uint8_t mqtt_readbuf[100];
   mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

   mqtt_network_new( &network );
#if MQTT_0LEN_CLIENT_ID==0
   memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
   strcpy(mqtt_client_id, "ESP-");
   strcat(mqtt_client_id, get_my_id());
#endif

   while(1)
   {
      xSemaphoreTake(wifi_alive, portMAX_DELAY);
      printf("%s: started\n\r", __func__);
      printf("%s: (Re)connecting to MQTT server %s ... ",__func__,
      MQTT_HOST);
      ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
      if( ret )
      {
         printf("error: %d\n\r", ret);
         taskYIELD();
         continue;
      }
      printf("done\n\r");
      mqtt_client_new(&client, &network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

      data.willFlag       = 0;
      data.MQTTVersion    = 4;
      data.clientID.cstring   = mqtt_client_id;
      data.username.cstring   = MQTT_USER;
      data.password.cstring   = MQTT_PASS;
      data.keepAliveInterval  = 10;
   #ifdef MQTT_CLEANSESSION
      data.cleansession   = 1;
   #else
      data.cleansession   = 0;
   #endif
      printf("Send MQTT connect ... ");
      ret = mqtt_connect(&client, &data);

      if(ret)
      {
         printf("error: %d\n\r", ret);
         mqtt_network_disconnect(&network);
         taskYIELD();
         continue;
      }
      printf("done\r\n");
      mqtt_subscribe(&client, "/esptopic", MQTT_QOS1, topic_received);
      xQueueReset(publish_queue);

      while(1)
      {
         char msg[TEMP_DATA_PUBLEN - 1] = "\0";
         while (xQueueReceive(publish_queue, (void *)msg, 0) == pdTRUE)
         {
            printf("got message to publish\r\n");
            mqtt_message_t message;
            message.payload = msg;
            message.payloadlen = TEMP_DATA_PUBLEN;
            message.dup = 0;
            message.qos = MQTT_QOS1;
            message.retained = 0;
            ret = mqtt_publish(&client, "/beat", &message);
            if (ret != MQTT_SUCCESS )
            {
               printf("error while publishing message: %d\n", ret );
               break;
            }
         }
         ret = mqtt_yield(&client, 1000);
         if (ret == MQTT_DISCONNECTED)
            break;
      }

      printf("Connection dropped, request restart\n\r");
      mqtt_network_disconnect(&network);
      taskYIELD();
   }
}

static void  wifi_task(void *pvParameters)
{
    uint8_t status  = 0;
    uint8_t retries = 10;

    while(1)
    {
        while ((status != STATION_GOT_IP) && (retries)){
            status = sdk_wifi_station_get_connect_status();
            printf("%s: status = %d\n\r", __func__, status );
            if( status == STATION_WRONG_PASSWORD ){
                printf("WiFi: wrong password\n\r");
                break;
            } else if( status == STATION_NO_AP_FOUND ) {
                printf("WiFi: AP not found\n\r");
                break;
            } else if( status == STATION_CONNECT_FAIL ) {
                printf("WiFi: connection failed\r\n");
                break;
            }
            vTaskDelay( 1000 / portTICK_PERIOD_MS );
            --retries;
        }
        if (status == STATION_GOT_IP) {
            printf("WiFi: Connected\n\r");
            xSemaphoreGive( wifi_alive );
            taskYIELD();
        }

        while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP) {
            xSemaphoreGive( wifi_alive );
            taskYIELD();
        }
        printf("WiFi: disconnected\n\r");
        sdk_wifi_station_disconnect();
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

void mqtt_app_init(QueueHandle_t * pubTempQueue)
{
   printf("mqtt_app_init\n");

   vSemaphoreCreateBinary(wifi_alive);
   publish_queue = xQueueCreate(3, TEMP_DATA_PUBLEN);
   xTaskCreate(&wifi_task, "wifi_task",  256, NULL, WIFI_TASK_PRIO, NULL);
   xTaskCreate(&beat_task, "beat_task", 512, pubTempQueue, BEAT_TASK_PRIO, NULL);
   // xTaskCreate(&beat_task, "beat_task", 512, pubTempQueue, 7, NULL);

   xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, MQTT_TASK_PRIO, NULL);
}
