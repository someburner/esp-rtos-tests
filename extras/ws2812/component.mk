# Component makefile for extras/ws2812

INC_DIRS += $(ws2812_ROOT)

# args for passing into compile rule generation
ws2812_SRC_DIR =  $(ws2812_ROOT)

ws2812_CFLAGS += \
	-DWS_FADE_TASK_PRIO=$(WS_FADE_TASK_PRIO) \
	-DPIXEL_COUNT=$(PIXEL_COUNT)

$(eval $(call component_compile_rules,ws2812))
