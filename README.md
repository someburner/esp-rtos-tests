# About

**DEVELOP**

Experimental repo for testing FreeRTOS w/ ESP8266. Will gradually morph to
something for a class project. probably not much use to anyone else unless you
want to see how to do some custom rBoot configurations.

Forked from [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos)

```
git clone https://github.com/someburner/esp-rtos-tests.git
```

# Building

**NOTE**:
* The `-C` option to make is the same as `cd <dir> && make`
* The -j flag tells make that it is allowed to spawn the provided amount of
  'threads'. Ideally each thread is executed on its own core/CPU, so your
  multi-core/CPU environment is used to its fullest.

**Example**:

```
make -j4 -C examples/cpe439
make flash -j4 -C examples/cpe439 ESPPORT=/dev/node_mcu
make -j4 -C examples/cpe439 clean

make -j4 -C examples/ws2812_test
make flash -C examples/ws2812_test ESPPORT=/dev/node_mcu

make -j4 -C examples/mqtt_client
make flash -j4 -C examples/mqtt_client ESPPORT=/dev/node_mcu

make -j4 -C examples/onewire_hw_test
make flash -j4 -C examples/onewire_hw_test ESPPORT=/dev/node_mcu

```

**esptool2**:
```
make mkesptool2 -C examples/http_get
```

**rboot**:
```
make rboot -C examples/http_get
```

### Hardware Info

**NodeMCU**:

* "D1" on silkscreen == GPIO5

### Additonal Build Info

The [esp-open-rtos build process wiki page](https://github.com/SuperHouse/esp-open-rtos/wiki/Build-Process)
has in-depth details of the build process.

**Track/untrack local config**:
```
#ignore chagnes
git update-index --assume-unchanged include/ssid_config.h
git update-index --assume-unchanged local.mk

#don't ignore
git update-index --no-assume-unchanged include/ssid_config.h
git update-index --no-assume-unchanged local.mk
```

**Pull in esp-open-rtos patches**:
```
git pull upstream master && git push origin master
```

# Code Structure

* `examples` contains a range of example projects (one per subdirectory). Check them out!
* `include` contains header files from Espressif RTOS SDK, relating to the binary libraries & Xtensa core.
* `core` contains source & headers for low-level ESP8266 functions & peripherals. `core/include/esp` contains useful headers for peripheral access, etc. Minimal to no FreeRTOS dependencies.
* `extras` is a directory that contains optional components that can be added to your project. Most 'extras' components will have a corresponding example in the `examples` directory. Extras include:
   - mbedtls - [mbedTLS](https://tls.mbed.org/) is a TLS/SSL library providing up to date secure connectivity and encryption support.
   - i2c - software i2c driver ([upstream project](https://github.com/kanflo/esp-open-rtos-driver-i2c))
   - rboot-ota - OTA support (over-the-air updates) including a TFTP server for receiving updates ([for rboot by @raburton](http://richard.burtons.org/2015/05/18/rboot-a-new-boot-loader-for-esp8266/))
   - bmp180 driver for digital pressure sensor ([upstream project](https://github.com/Angus71/esp-open-rtos-driver-bmp180))
* `FreeRTOS` contains FreeRTOS implementation, subdirectory structure is the standard FreeRTOS structure. `FreeRTOS/source/portable/esp8266/` contains the ESP8266 port.
* `lwip` contains the lwIP TCP/IP library. See [Third Party Libraries](https://github.com/SuperHouse/esp-open-rtos/wiki/Third-Party-Libraries) wiki page for details.
* `libc` contains the newlib libc. [Libc details here](https://github.com/SuperHouse/esp-open-rtos/wiki/libc-configuration).


# Components
* [FreeRTOS v9.0.0](http://www.freertos.org/)
* [Espressif IOT RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK)
* [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk/)
* [esptool.py](https://github.com/themadinventor/esptool)
* [lwIP](http://lwip.wikia.com/wiki/LwIP_Wiki) v1.4.1, modified via the [esp-lwip project](https://github.com/kadamski/esp-lwip) by @kadamski.
* [newlib](https://github.com/projectgus/newlib-xtensa) v2.2.0, with patches for xtensa support and locking stubs for thread-safe operation on FreeRTOS.

## Binary Components

Binary libraries (inside the `lib` dir) are all supplied by Espressif as part of their RTOS SDK. These parts were MIT Licensed.

As part of the esp-open-rtos build process, all binary SDK symbols are prefixed with `sdk_`. This makes it easier to differentiate binary & open source code, and also prevents namespace conflicts.

Espressif's RTOS SDK provided a "libssl" based on axTLS. This has been replaced with the more up to date mbedTLS library (see below).

Some binary libraries appear to contain unattributed open source code:

* libnet80211.a & libwpa.a appear to be based on FreeBSD net80211/wpa, or forks of them. ([See this issue](https://github.com/SuperHouse/esp-open-rtos/issues/4)).
* libudhcp has been removed from esp-open-rtos. It was released with the Espressif RTOS SDK but udhcp is GPL licensed.






# end
