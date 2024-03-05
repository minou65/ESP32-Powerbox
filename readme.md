# PVPowerbox

## Table of contents
- [PVPowerbox](#pvpowerbox)
  - [Table of contents](#table-of-contents)
  - [Desription](#desription)
  - [Librarys](#librarys)
  - [Settings](#settings)
    - [Inverter](#inverter)
      - [IPAddress](#ipaddress)
      - [Port](#port)
      - [Register Input power](#register-input-power)
      - [Data Length](#data-length)
      - [Gain](#gain)
      - [Interval (seconds)](#interval-seconds)
    - [Relay](#relay)
      - [Designation](#designation)
      - [Power (W)](#power-w)
      - [GPIO](#gpio)
    - [Shelly](#shelly)
      - [Designation](#designation-1)
      - [Power (W)](#power-w-1)
      - [URL on](#url-on)
      - [URL off](#url-off)
      - [minimum on time (minutes)](#minimum-on-time-minutes)
      - [Do not enable before](#do-not-enable-before)
    - [WiFi](#wifi)
      - [Default Password](#default-password)
      - [Default IP address](#default-ip-address)
      - [OTA](#ota)
      - [Configuration options](#configuration-options)
  - [Blinking codes](#blinking-codes)
  - [Reset](#reset)

## Desription

## Librarys
[ModbusClientTCP]https://github.com/eModbus/eModbus)
[AsyncTCP](https://github.com/dvarrel/AsyncTCP)

## Settings
### Inverter
#### IPAddress
IP address of the inverter

#### Port


#### Register Input power
Register where the Input power is sored. See inverter documentation

#### Data Length
How many bytes should be read. See inverter documentation.

#### Gain
The gain setting is used to adjust the output power. See inverter documentation.

#### Interval (seconds)
This setting specifies the time interval in seconds between each polling of the inverter.

### Relay
#### Designation
Description of the output

#### Power (W)
What power from the photovoltaic system is necessary for the output to be activated

#### GPIO
Default GPIO's are 
- Relay 1 = 1
- Relay 2 = 2
- Relay 3 = 3
- Relay 4 = 4

### Shelly
#### Designation
Description of the output

#### Power (W)
What power from the photovoltaic system is necessary for the output to be activated

#### URL on
URL to turn on Shelly

#### URL off
URL to turn off Shelly

#### minimum on time (minutes)
Switch-off delay when the power falls below the power setting

#### Do not enable before
Here you can define a time. The Shelly will not be switched on before this time

### WiFi

#### Default Password
When not connected to an AP the default password is 123456789

#### Default IP address
When in AP mode, the default IP address is 192.168.4.1

#### OTA 
OTA is enabled, use default IP address or if connected to a AP the correct address.
Port is the default port.

#### Configuration options
After the first boot, there are some values needs to be set up.
These items are maked with __*__ (star) in the list below.

You can set up the following values in the configuration page:

-  __Thing name__ - Please change the name of the device to
a name you think describes it the most. It is advised to
incorporate a location here in case you are planning to
set up multiple devices in the same area. You should only use
english letters, and the "_" underscore character. Thus, must not
use Space, dots, etc. E.g. `lamp_livingroom` __*__
- __AP password__ - This password is used, when you want to
access the device later on. You must provide a password with at least 8,
at most 32 characters.
You are free to use any characters, further more you are
encouraged to pick a password at least 12 characters long containing
at least 3 character classes. __*__
- __WiFi SSID__ - The name of the WiFi network you want the device
to connect to. __*__
- __WiFi password__ - The password of the network above. Note, that
unsecured passwords are not supported in your protection. __*__

## Blinking codes
Prevoius chapters were mentioned blinking patterns, now here is a
table summarize the menaning of the blink codes.

- __Rapid blinking__ (mostly on, interrupted by short off periods) -
Entered Access Point mode. This means the device create an own WiFi
network around it. You can connect to the device with your smartphone
(or WiFi capable computer).
- __Alternating on/off blinking__ - Trying to connect the configured
WiFi network.
- __Mostly off with occasional short flash__ - The device is online.

## Reset
When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
password to buld an AP. (E.g. in case of lost password)

Reset pin is D3 / IO17