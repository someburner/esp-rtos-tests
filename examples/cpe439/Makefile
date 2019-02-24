PROGRAM=cpe439
EXTRA_COMPONENTS = extras/paho_mqtt_c extras/hw_timer extras/ws2812 extras/onewire extras/crc

# Create user flags and pass to main program

# Driver
user_CFLAGS += \
	-DBAUD_RATE=$(BAUD_RATE) \
	-DTEMP_UPDATE_MS=$(TEMP_UPDATE_MS) \
	-DPIXEL_COUNT=$(PIXEL_COUNT)

# WiFi
user_CFLAGS += \
	-DWIFI_SSID=$(WIFI_SSID) \
	-DWIFI_PASS=$(WIFI_PASS)

# MQTT
user_CFLAGS += \
	-DMQTT_HOST=$(MQTT_HOST) -DMQTT_PORT=$(MQTT_PORT) \
	-DMQTT_USER=$(MQTT_USER) -DMQTT_PASS=$(MQTT_PASS) \
	-DMQTT_VERSION=$(MQTT_VERSION) -DMQTT_0LEN_CLIENT_ID=$(MQTT_0LEN_CLIENT_ID)

# Tasks
user_CFLAGS += \
	-DWIFI_TASK_PRIO=$(WIFI_TASK_PRIO) \
	-DMQTT_TASK_PRIO=$(MQTT_TASK_PRIO) \
	-DTEMP_PUB_TASK_PRIO=$(TEMP_PUB_TASK_PRIO) \
	-DWS_FADE_TASK_PRIO=$(WS_FADE_TASK_PRIO)

#-D=$()
PROGRAM_CFLAGS += $(user_CFLAGS)

# user_CFLAGS := $(CFLAGS) $(user_CFLAGS)

include ../../common.mk
