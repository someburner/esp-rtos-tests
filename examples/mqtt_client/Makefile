PROGRAM=mqtt_client
EXTRA_COMPONENTS = extras/paho_mqtt_c

# Create user flags and pass to main program
user_CFLAGS += -DBAUD_RATE=$(BAUD_RATE) -DMQTT_HOST=$(MQTT_HOST) \
	-DMQTT_PORT=$(MQTT_PORT) -DMQTT_USER=$(MQTT_USER) -DMQTT_PASS=$(MQTT_PASS)

PROGRAM_CFLAGS += $(user_CFLAGS)

# user_CFLAGS := $(CFLAGS) $(user_CFLAGS)

include ../../common.mk
