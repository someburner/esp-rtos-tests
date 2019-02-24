# About

Class project using FreeRTOS w/ ESP8266. Forked from [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos)

# Hardware

* WS2812B LED strip
* NodeMCU (ESP8266 / ESP12E)
* MAX31820 temperature sensor
* 4.7K ohm resistor (for MAX31820)
* 220 ohm resistor (for WS2812B)
* Some wires and micro-USB cable

# Building

**NOTE**: Must be on linux to build.

1. esp-open-sdk

[esp-open-sdk](https://github.com/pfalcon/esp-open-sdk) is required to build
this project. Head to that link for build instructions. Standlone build is fine.
Some of the required dependencies for esp-open-sdk are also needed in this
project, namely:

* `python3`
* `python3-pip`
* `pyserial` -> `pip3 install pyserial`
* `gcc`, `make`

2. Setup

Clone this repository, create a `.local` folder in project root, copy over
example settings, and make edits.

```sh
git clone https://github.com/someburner/esp-rtos-tests.git
cd esp-rtos-tests
mkdir -p .local
cp settings.example.mk .local/settings.mk
```

In particular, `OPENSDK_ROOT`, `WIFI_SSID`, `WIFI_PASS`, and `PIXEL_COUNT` will
need to be changed. `PIXEL_COUNT` should be set to the number of LEDs on your
WS2812B LED strip.

3. Bootloader

Once `OPENSDK_ROOT` is set, you can build the bootloader and the project.
You may skip building esptool2 and bootloader (rboot) as their binaries are
included in this repo. But commands to re-generate are included for reference.

```sh
# esptool2, required to build bootloader and generate binaries
make mkesptool2 -C examples/cpe439

# bootloader
make -C bootloader/rboot

# clean project
make -C examples/cpe439 clean

# build project
make -C examples/cpe439
```

# Hardware connections

Before flashing, make sure hardware is connected. The definitions in
`settings.example.mk` are set to be hooked up as follows:

### WS2812B

* WS2812B 3.3v -> NodeMCU 3.3v
* WS2812B Data -> 220 ohm -> NodeMCU D7
* WS2812B GND -> NodeMCU GND

### MAX31820

* MAX31820 3.3v -> NodeMCU 3.3v
* MAX31820 Data -> NodeMCU D1
* MAX31820 Data -> 4.7K Ohm -> NodeMCU 3.3v

# Flashing

To flash the project, connect NodeMCU to USB and run:

```sh
# flash project
make flash -C examples/cpe439 ESPPORT=/dev/ttyUSB0

# if you get permission errors, try this:
sudo chown $(whoami):$(whoami) /dev/ttyUSB0
```

**NOTE**: `/dev/ttyUSB0` must correspond to the serial port of NodeMCU. Usually
it will be `/dev/ttyUSB0`.

# Run / Usage

## UART

With pyserial you can watch UART output with the following command:

```sh
python3 -m serial.tools.miniterm --eol CRLF --exit-char 003 /dev/ttyUSB0 500000 --raw -q
```

**protip**: Add a line like this in your `~/.bashrc`

```
alias nodemcu='python3 -m serial.tools.miniterm --eol CRLF --exit-char 003 /dev/ttyUSB0 500000 --raw -q'
```

Then you can bring up serial with just `nodemcu`.

## MQTT

Assuming hardware is connected correctly, you can interact with the device via
MQTT.

**NOTE**: Commands assuming mosquitto pub/sub clients are installed. To install:

```sh
sudo apt-get install mosquitto-clients
```

### Listening for temperature updates

On init, the device immediately begins polling the temperature sensor. These
should be logging out on serial every couple seconds. If they aren't, check
your connections. Once the device is connected to WiFi and MQTT, it will begin
publishing messages on the `/cpe439/temp` topic.

To watch these updates come in remotely:

```sh
mosquitto_sub -h test.mosquitto.org -t /cpe439/temp
```

### Changing WS2812B colors

A simple message format is used to send RGB values to the device. Commands
should take this form:

```
r:RRRg:GGGb:BBB~
```

Replace RRR, GGG, BBB with 0-255. For example, these 3 commands will switch the
colors to red, green, and blue respectively:

```sh
# red
mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:255g:0b:0~'
# green
mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:0g:255b:0~'
# blue
mosquitto_pub -h test.mosquitto.org -t /cpe439/rgb -m 'r:0g:0b:255~'
```

<br>
<br>

## Additonal Build Info

The [esp-open-rtos build process wiki page](https://github.com/SuperHouse/esp-open-rtos/wiki/Build-Process)
has in-depth details of the build process.

## Code Structure

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

## Components
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
