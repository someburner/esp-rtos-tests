# Component makefile for extras/crc

INC_DIRS += $(ROOT)extras/crc

# args for passing into compile rule generation
extras/crc_INC_DIR =  $(ROOT)extras/crc
extras/crc_SRC_DIR =  $(ROOT)extras/crc

$(eval $(call component_compile_rules,extras/crc))
