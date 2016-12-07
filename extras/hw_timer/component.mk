# Component makefile for extras/pwm

INC_DIRS += $(ROOT)extras/hw_timer $(ROOT)extras/onewire $(ROOT)extras/ws2812

# args for passing into compile rule generation
extras/hw_timer_INC_DIR =  $(ROOT)extras/hw_timer
extras/hw_timer_SRC_DIR =  $(ROOT)extras/hw_timer

$(eval $(call component_compile_rules,extras/hw_timer))
