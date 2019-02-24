#
# Project Settings
#

OPENSDK_ROOT ?= /path/to/esp-open-sdk

# Serial baud rate
BAUD_RATE ?= 500000

# WiFi creds
WIFI_SSID ?=YourWiFiSSID
WIFI_PASS ?=YourWiFiPassword

# Task Priorities
WS_FADE_TASK_PRIO  ?= 3
WIFI_TASK_PRIO     ?= 1
MQTT_TASK_PRIO     ?= 2
OW_TASK_PRIO       ?= 6
TEMP_PUB_TASK_PRIO ?= 7

# MQTT settings
MQTT_HOST ?= test.mosquitto.org
MQTT_PORT ?= 1883
# set to 1 so the broker will automatically generate a unique client ID
MQTT_0LEN_CLIENT_ID ?= 1

# Temperature Sensor
EN_TEMP_SENSOR  ?= 1
# pin 5 [GPIO5] (D1 on NodeMCU)
TEMP_SENSOR_PIN ?= 5

# WS2812B settings
PIXEL_COUNT ?= 29
# WS2812_PIN ?= 13
# pin 13 [MOSI] (D7 on NodeMCU) NOTE: must be this pin
