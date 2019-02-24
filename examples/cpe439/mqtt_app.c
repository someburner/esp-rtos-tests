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
#include "cpe439.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

#if MQTT_0LEN_CLIENT_ID==1
#define MQTT_CLEANSESSION
static char mqtt_client_id[2] = { 0, 0 };
#else
#error "MQTTv31 not supported."
#endif

#define TEMP_DATA_PUBLEN 7

SemaphoreHandle_t wifi_alive;
QueueHandle_t publish_queue;
static QueueHandle_t * rgbQueueHandle = NULL;


#define RGB_MSG_LEN 18
#define RGB_MSG_LEN_MIN 10

static void temp_pub_task(void *p)
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
         if (msg)
            free(msg);
         msg = NULL;
      }
      vTaskDelayMs(132);
      // taskYIELD();
   }
}

static char rgb_keys[4] =  { 'r', 'g', 'b', '~'};
uint8_t rgb_out[3] = { 0, 0, 0 };

// Msg Format: r:RRRg:GGGb:BBB~
// Replace RRR, GGG, BBB with 0-255
// mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:255g:0b:0~'
// mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:0g:255b:0~'
// mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:0g:0b:255~'

static void  topic_received(mqtt_message_data_t *md)
{
   int i, parse_loc;
   bool contParse = true;

   uint8_t * rgb_colors = NULL;
   mqtt_message_t *message = md->message;
   printf("Received: ");
   // if (md->topic->lenstring.len != RGB_MSG_LEN)
   // {
   //    printf("unkown msg len = %d\n", md->topic->lenstring.len);
   //    return;
   // }
   if (((int)message->payloadlen > RGB_MSG_LEN) || ( (int)message->payloadlen < RGB_MSG_LEN_MIN ) )
   {
      printf("unkown msg len = %d\n", (int)message->payloadlen);
      return;
   }

   parse_loc = 0;

   uint8_t rgb_read[3];

   for( i = 0; i < (int)message->payloadlen; )
   {
      if (i >= (int)message->payloadlen)
         goto parse_err;

      char * next_ch = (char*) &(((char*)message->payload)[ i ]);

      printf("1 %c\n", *next_ch);
      if (*next_ch == '~')
      {
         goto done;
      }

      if ( (!next_ch) || (*next_ch != rgb_keys[parse_loc]) || (*(next_ch+1) != ':') )
         goto parse_err;

      printf("2\n");
      int j;
      int okChars = 0;
      char tmpBuf[4] = { 0, 0, 0, 0};

      for ( j = 0; j < 4; j++)
      {
         char * next_num = (char*) &(((char*)message->payload)[ i+2+j ]);
         if ((*next_num < '0') || (*next_num > '9'))
         {
            if ( (parse_loc <= 3) && (*next_num == rgb_keys[parse_loc+1])  )
            {
               if (parse_loc == 3)
                  goto done;
               parse_loc++;
               printf("2\n");
               break;
            }

            if (j == 0)
            {
               goto parse_err;
            }
         }
         else
         {
            printf("3\n");
            okChars++;
            tmpBuf[j] = *next_num;
         }
      }

      if (okChars)
      {
         int x = atoi(tmpBuf);
         rgb_read[parse_loc-1] = (uint8_t) x;
         printf("found %d\n", x);
      }

      if (parse_loc >= 3)
      {
         printf("done!\n");
         goto done;
      } else {
         i+=okChars + 2;
      }
   }
done:
   rgb_out[0] = rgb_read[0];
   rgb_out[1] = rgb_read[1];
   rgb_out[2] = rgb_read[2];

   /* QueueHandle_t	xQueue, const void* pvItemToQueue, TickType_t xTicksToWait */
   xQueueSendToBack(*rgbQueueHandle, &rgb_out, (TickType_t)0);
   // rgb_colors = (uint8_t *) malloc(sizeof(uint8_t)*12);
   return;

parse_err:
   printf("parse err\n");
   return;
}

static void  mqtt_task(void *pvParameters)
{
   int ret = 0;
   struct mqtt_network network;
   mqtt_client_t client = mqtt_client_default;

   uint8_t mqtt_buf[100];
   uint8_t mqtt_readbuf[100];
   mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

   mqtt_network_new( &network );

   while(1)
   {
      xSemaphoreTake(wifi_alive, portMAX_DELAY);
      printf("%s: started\n\r", __func__);
      printf("%s: (Re)connecting to MQTT server %s ... ",__func__, MAKE_STRING(MQTT_HOST));
      ret = mqtt_network_connect(&network, MAKE_STRING(MQTT_HOST), MQTT_PORT);
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
      // data.username.cstring   = MQTT_USER;
      // data.password.cstring   = MQTT_PASS;
      // data.username.cstring   = "";
      // data.password.cstring   = "";
      data.username.cstring   = NULL;
      data.password.cstring   = NULL;
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
      mqtt_subscribe(&client, "/cpe439/rgb", MQTT_QOS1, topic_received);
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
            ret = mqtt_publish(&client, "/cpe439/temp", &message);
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

void mqtt_app_init(QueueHandle_t * pubTempQueue, QueueHandle_t * rgbQueue)
{
   if (rgbQueue)
      rgbQueueHandle = rgbQueue;

   printf("mqtt_app_init\n");

   vSemaphoreCreateBinary(wifi_alive);
   publish_queue = xQueueCreate(3, TEMP_DATA_PUBLEN);
   xTaskCreate(&wifi_task, "wifi_task",  256, NULL, WIFI_TASK_PRIO, NULL);
   xTaskCreate(&temp_pub_task, "temp_pub_task", 512, pubTempQueue, TEMP_PUB_TASK_PRIO, NULL);

   xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, MQTT_TASK_PRIO, NULL);
}
