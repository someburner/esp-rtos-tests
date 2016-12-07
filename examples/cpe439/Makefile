PROGRAM=cpe439
EXTRA_COMPONENTS = extras/paho_mqtt_c extras/hw_timer extras/onewire extras/crc

# Create user flags and pass to main program
user_CFLAGS += -DBAUD_RATE=$(BAUD_RATE) -DMQTT_HOST=$(MQTT_HOST) \
	-DMQTT_PORT=$(MQTT_PORT) -DMQTT_USER=$(MQTT_USER) -DMQTT_PASS=$(MQTT_PASS) \
	-DMQTT_VERSION=$(MQTT_VERSION) -DMQTT_0LEN_CLIENT_ID=$(MQTT_0LEN_CLIENT_ID) \
	-DTEMP_UPDATE_MS=$(TEMP_UPDATE_MS)

PROGRAM_CFLAGS += $(user_CFLAGS)

# user_CFLAGS := $(CFLAGS) $(user_CFLAGS)

include ../../common.mk
