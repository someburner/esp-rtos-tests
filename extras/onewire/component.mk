# Component makefile for extras/onewire

# expected anyone using onewire driver includes it as 'onewire/onewire.h'
INC_DIRS += $(onewire_ROOT)..
INC_DIRS += $(ROOT)extras/crc

# args for passing into compile rule generation
onewire_INC_DIR = $(onewire_ROOT)
onewire_SRC_DIR = $(onewire_ROOT)

onewire_CFLAGS += \
	-DEN_TEMP_SENSOR=$(EN_TEMP_SENSOR) -DTEMP_SENSOR_PIN=$(TEMP_SENSOR_PIN) \
	-DOW_TASK_PRIO=$(OW_TASK_PRIO)

$(eval $(call component_compile_rules,onewire))
