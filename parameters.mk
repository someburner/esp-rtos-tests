# Parameters for the esp-open-rtos make process
#
# You can edit this file to change parameters, but a better option is
# to create a local.mk file and add overrides there. The local.mk file
# can be in the root directory, or the program directory alongside the
# Makefile, or both.
#
-include $(ROOT)local.mk
-include local.mk

PROJ_ROOT = /home/jeffrey/ESP8266/esp-freertos-7.5.2/

#############################################################
# Flash Options (SDK)
#############################################################
# Flash Cal sector?
# FLASH_CAL_SECT  ?= no
FLASH_CAL_SECT ?= yes
CAL_SECT_ADDR   ?= 0x3FB000

#Flash esp_init_data_default?
# FLASH_INIT_DEFAULT  ?= no
FLASH_INIT_DEFAULT ?= yes

#Flash blank.bin?
FLASH_BLANK    ?= no
# FLASH_BLANK    ?= yes

#############################################################
# Flash Options (Programs)
#############################################################
# Choose which rom to boot if flashing rconf
ROM_TO_BOOT        ?= 0

# Choose which roms to flash
FLASH_RBOOT        ?= yes
FLASH_RCONF        ?= yes
FLASH_ROM0         ?= yes
FLASH_ROM1         ?= no
FLASH_ROM2         ?= no
FLASH_ROM3         ?= no

# Choose to flash a blank to sector 1.
# Causes rBoot to recreate conf w defaults.
FLASH_RCONF_BLANK  ?= no

# Specify names of each rom
RBOOT_NAME         ?= rboot.bin
RCONF_NAME         ?= rconf.bin
#############################################################

# Flash size in megabits
# Valid values are same as for esptool.py - 2,4,8,16,32
FLASH_SIZE ?= 16

# Flash mode, valid values are same as for esptool.py - qio,qout,dio.dout
FLASH_MODE ?= qio

# Flash speed in MHz, valid values are same as for esptool.py - 80, 40, 26, 20
FLASH_SPEED ?= 80

# Output directories to store intermediate compiled files
# relative to the program directory
BUILD_DIR ?= $(PROGRAM_DIR)build/
FIRMWARE_DIR ?= $(PROGRAM_DIR)firmware/

# esptool.py from https://github.com/themadinventor/esptool
# ESPTOOL ?= esptool.py
ESPTOOL ?= $(PROJ_ROOT)utils/esptool.py
# serial port settings for esptool.py
ESPPORT ?= /dev/ftdi_esp
# ESPBAUD ?= 115200

# Esptool2
ESPTOOL2 ?= $(PROJ_ROOT)utils/esptool2
ESPTOOL2_SRC_DIR = $(PROJ_ROOT)utils/esptool_2/src

#############################################################
# Esptool flash options:
# Configured for esp-12e (4MByte Windbond)
#############################################################
ESPBAUD   ?= 460800 # 115200, 230400, 460800
ET_FM     ?= qio  # qio, dio, qout, dout
ET_FS     ?= 32m  # 32Mbit flash size in esptool flash command
ET_FF     ?= 80m  # 80Mhz flash speed in esptool flash command
# ET_BLANK1 ?= 0x1FE000 # where to flash blank.bin to erase wireless settings
# ESP_INIT1 ?= 0x1FC000 # flash init data provided by espressif
ET_BLANK ?= 0x3FE000 # where to flash blank.bin to erase wireless settings
ESP_INIT ?= 0x3FC000 # flash init data provided by espressif

# firmware tool arguments
ESPTOOL_ARGS=-fs $(ET_FS) -fm $(ET_FM) -ff $(ET_FF)

#rBoot Options
RBOOT_E2_SECTS       ?= .text .data .rodata
RBOOT_E2_USER_ARGS   ?= -bin -boot2 -iromchksum -4096 -$(ET_FM) -80

# rBoot setup
RBOOT_SRC_DIR = $(PROJ_ROOT)bootloader/rboot
RBOOT=bin/rboot.bin
COMMON=liblibc.a libjsmn.a libjson.a liblwip.a libmqtt.a libspiffs.a libutil.a \
   libnewcrypto.a libplatform.a librboot.a
LIBMAIN_SRC = $(SDK_ROOT)lib/libmain.a
LIBMAIN_DST = $(PROJ_ROOT)sdk-overrides/lib/libmain2.a

# set this to 0 if you don't need floating point support in printf/scanf
# this will save approx 14.5KB flash space and 448 bytes of statically allocated
# data RAM
#
# NB: Setting the value to 0 requires a recent esptool.py (Feb 2016 / commit ebf02c9)
PRINTF_SCANF_FLOAT_SUPPORT ?= 1

FLAVOR ?= release # or debug

# Compiler names, etc. assume gdb
#CROSS ?= xtensa-lx106-elf-
CROSS ?= /home/jeffrey/esp-open-sdk-1.5.4.1/xtensa-lx106-elf/bin/xtensa-lx106-elf-
SDK_ROOT ?= /home/jeffrey/esp-open-sdk-1.5.4.1/sdk/

# Path to the filteroutput.py tool
FILTEROUTPUT ?= $(ROOT)/utils/filteroutput.py

AR = $(CROSS)ar
CC = $(CROSS)gcc
CPP = $(CROSS)cpp
LD = $(CROSS)gcc
NM = $(CROSS)nm
C++ = $(CROSS)g++
SIZE = $(CROSS)size
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

# Source components to compile and link. Each of these are subdirectories
# of the root, with a 'component.mk' file.
COMPONENTS     ?= $(EXTRA_COMPONENTS) FreeRTOS lwip core open_esplibs

# binary esp-iot-rtos SDK libraries to link. These are pre-processed prior to linking.
SDK_LIBS		?= main net80211 phy pp wpa

# open source libraries linked in
LIBS ?= hal gcc c

# set to 0 if you want to use the toolchain libc instead of esp-open-rtos newlib
OWN_LIBC ?= 1

# Note: you will need a recent esp
ENTRY_SYMBOL ?= call_user_start

# Set this to zero if you don't want individual function & data sections
# (some code may be slightly slower, linking will be slighty slower,
# but compiled code size will come down a small amount.)
SPLIT_SECTIONS ?= 1

# Set this to 1 to have all compiler warnings treated as errors (and stop the
# build).  This is recommended whenever you are working on code which will be
# submitted back to the main project, as all submitted code will be expected to
# compile without warnings to be accepted.
WARNINGS_AS_ERRORS ?= 0

# Common flags for both C & C++_
C_CXX_FLAGS ?= -Wall -Wl,-EL -nostdlib $(EXTRA_C_CXX_FLAGS)
# Flags for C only
CFLAGS		?= $(C_CXX_FLAGS) -std=gnu99 $(EXTRA_CFLAGS)
# Flags for C++ only
CXXFLAGS	?= $(C_CXX_FLAGS) -fno-exceptions -fno-rtti $(EXTRA_CXXFLAGS)

# these aren't all technically preprocesor args, but used by all 3 of C, C++, assembler
CPPFLAGS	+= -mlongcalls -mtext-section-literals

LDFLAGS		= -nostdlib -Wl,--no-check-sections -L$(BUILD_DIR)sdklib -L$(ROOT)lib -u $(ENTRY_SYMBOL) -Wl,-static -Wl,-Map=$(BUILD_DIR)$(PROGRAM).map $(EXTRA_LDFLAGS)

ifeq ($(WARNINGS_AS_ERRORS),1)
    C_CXX_FLAGS += -Werror
endif

ifeq ($(SPLIT_SECTIONS),1)
  C_CXX_FLAGS += -ffunction-sections -fdata-sections
  LDFLAGS += -Wl,-gc-sections
endif

ifeq ($(FLAVOR),debug)
    C_CXX_FLAGS += -g -O0
    LDFLAGS += -g -O0
else ifeq ($(FLAVOR),sdklike)
    # These are flags intended to produce object code as similar as possible to
    # the output of the compiler used to build the SDK libs (for comparison of
    # disassemblies when coding replacement routines).  It is not normally
    # intended to be used otherwise.
    CFLAGS += -O2 -Os -fno-inline -fno-ipa-cp -fno-toplevel-reorder -fno-caller-saves -fconserve-stack
    LDFLAGS += -O2
else
    C_CXX_FLAGS += -g -O2
    LDFLAGS += -g -O2
endif

GITSHORTREV=\"$(shell cd $(ROOT); git rev-parse --short -q HEAD 2> /dev/null)\"
ifeq ($(GITSHORTREV),\"\")
  GITSHORTREV="\"(nogit)\"" # (same length as a short git hash)
endif
CPPFLAGS += -DGITSHORTREV=$(GITSHORTREV)

LINKER_SCRIPTS += $(ROOT)ld/program.ld $(ROOT)ld/rom.ld

# rboot firmware binary paths for flashing
RBOOT_BIN = $(ROOT)bootloader/firmware/rboot.bin
RBOOT_PREBUILT_BIN = $(ROOT)bootloader/firmware_prebuilt/rboot.bin
RBOOT_CONF = $(ROOT)bootloader/firmware_prebuilt/blank_config.bin

# if a custom bootloader hasn't been compiled, use the
# prebuilt binary from the source tree
ifeq (,$(wildcard $(RBOOT_BIN)))
RBOOT_BIN=$(RBOOT_PREBUILT_BIN)
endif
