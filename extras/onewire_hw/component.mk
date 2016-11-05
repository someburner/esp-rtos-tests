# Component makefile for extras/pwm

INC_DIRS += $(ROOT)extras/onewire_hw

# args for passing into compile rule generation
extras/onewire_hw_INC_DIR =  $(ROOT)extras/onewire_hw
extras/onewire_hw_SRC_DIR =  $(ROOT)extras/onewire_hw

$(eval $(call component_compile_rules,extras/onewire_hw))
