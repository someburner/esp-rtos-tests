#############################################################
# Flash Options (SDK)
#############################################################
# Flash Cal sector?
# FLASH_CAL_SECT     ?= no
FLASH_CAL_SECT     ?= yes
CAL_SECT_ADDR      ?= 0x3FB000

#Flash esp_init_data_default?
# FLASH_INIT_DEFAULT ?= no
FLASH_INIT_DEFAULT ?= yes

#Flash blank.bin?
# FLASH_BLANK        ?= no
FLASH_BLANK        ?= yes

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
