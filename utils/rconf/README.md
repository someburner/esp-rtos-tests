# rconf
A config generator for [rBoot](https://github.com/raburton/rboot) written in C.
Used for creating custom config sectors for your ESP8266 bootloader.

## Usage
--------
By default, rBoot assumes 2 roms, the first at the first available memory
location, and the second at half your SPI Flash chip size (i.e. just past the
2MB area if your chip is 4MB). This should be okay for most, but if you
want to take full advantage of rBoot's ability to have 2 or more roms on a
single MB of flash, and have, say, 2 roms that are 1MB, 2 roms that are
512KB, and 1MB for SPIFFS, then it gets trickier and you have to modify your
config sector.

Of course, you *could* (and should) write some code in your project to switch to
whatever rom you want, but this requires some kind of input to decide which rom
to switch to. For developing and debugging, I find it's much easier/faster to
use this tool to generate the config sector, and then flash it along with
whatever rom(s) you're working on. This way, if you're only working on your
512KB roms on the first MB, you set your "rom to boot" to 0/1, and then if you
want to switch to rom 2, you can just change the "rom to boot" and flash the new
config.

## Running
----------

Before compiling, go into rconf.c and change the #defines to match your
project's setup. A couple notes:

* ROM_COUNT must == the number of rom locations in ROM_LOCATIONS
* If you have BOOT_CONFIG_CHKSUM comented in your project, comment it out here
  also
* I have changed the `rboot_config` struct slightly, such that:
```
uint8_t padding[2];
```
is now:
```
uint8_t last_main_rom;
uint8_t last_secondary_rom;
```

Basically I just use 2 of the previously unused bytes in the rboot struct to
keep track of which 'type' of rom I previously used. For my project I have 2
types. Feel free to ignore these (the config file will work this same, as these
bytes are ignored in the vanilla rBoot anyways). Or you can use them like I do,
or use them for some other purpose.


Once you have it all set up, you can compile by simply running:
```
make
```

This will compile the program, and run it once (if the compilaton suceeded). You
can use the `rconf` to generate the same config again if needed. This is
redundant right now, but in the future I'll probably add the ability to pass
arguments.
