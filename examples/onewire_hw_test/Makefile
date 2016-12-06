# Simple makefile for simple example
PROGRAM=onewire_hw_test
EXTRA_COMPONENTS = extras/hw_timer extras/onewire extras/crc

# Create user flags and pass to main program
user_CFLAGS += -DBAUD_RATE=$(BAUD_RATE)
PROGRAM_CFLAGS += $(user_CFLAGS)

include ../../common.mk
