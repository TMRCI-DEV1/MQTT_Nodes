# Gilberton Turntable Control

## Overview
This Arduino sketch is designed to control the Gilberton Turntable using an ESP32 Node. The turntable can be controlled by entering a 1 or 2-digit track number on a keypad, or by receiving MQTT messages published by JMRI. The sketch also supports calibration of the turntable positions.

## Components
The system utilizes the following components:

- ESP32 Node
- 3x4 membrane matrix keypad
- Serial LCD 2004 20x4 display module with I2C interface
- 16 Channel I2C Interface Electromagnetic Relay Module
- 8 Channel I2C Interface Electromagnetic Relay Module
- STEPPERONLINE CNC stepper motor driver
- Photo-interrupter "homing" sensor
- Reset button
- STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead)

## Libraries Used
The sketch utilizes the following libraries:

**Wire**: This library provides support for the I2C connection used by the ESP32. The library is available at [https://github.com/esp8266/Arduino/tree/master/libraries/Wire].

**WiFi**: This library provides the necessary functions to connect to a WiFi network. It is part of the official ESP32 Arduino core and can be found at [https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi].

**Keypad**: This library provides necessary functions for the 3x4 membrane keypad. The library can be found at [https://github.com/Chris--A/Keypad].

**LiquidCrystal_I2C**: This library allows easy control of the Liquid Crystal display with I2C interface. It can be found at [https://github.com/johnrickman/LiquidCrystal_I2C].

**PCF8575**: This library enables communication with I2C 16-channel relay board based on the PCF8575 IC. It can be found at [https://github.com/xreef/PCF8575_library/tree/master].

**PCF8574**: This library enables communication with I2C 8-channel relay board based on the PCF8575 IC. It can be found at [https://github.com/xreef/PCF8574_library].

**AccelStepper**: This library provides advanced stepper motor control, including acceleration and deceleration. It can be found at [https://github.com/waspinator/AccelStepper].

**PubSubClient**: This library facilitates MQTT communication and allows the ESP32 to subscribe to MQTT topics and publish messages. The library can be found at [https://github.com/knolleary/pubsubclient].

**EEPROM**: This library provides functions for reading and writing to the EEPROM memory of the ESP32. It is part of the official ESP32 Arduino core and can be found at [https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM].

**ArduinoOTA**: This library enables Over-The-Air (OTA) updates for the ESP32. It allows you to upload new firmware to the ESP32 wirelessly. The library is available at [https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA].

## Functionality
The ESP32 Node connects to a WiFi network and subscribes to MQTT messages published by JMRI. The expected MQTT message format is 'Tracknx', where 'n' represents the 2-digit track number (01-23) and 'x' represents 'H' for the head-end or 'T' for the tail-end.

The LCD displays the IP address, the commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname.

The sketch supports a calibration mode, which is used to set the positions of the turntable for each track number and end (head or tail). In calibration mode, the current position of the turntable is stored when a track number and end are entered on the keypad.

When not in calibration mode, the sketch retrieves the stored positions from the EEPROM at startup, and uses them to move the turntable to the correct position when a track number and end are entered on the keypad or received in an MQTT message.

## How-To: 
### Calibration Process
1. Set **CALIBRATION_MODE** to **true** in the sketch and upload it to the ESP32 Node.
2. After the ESP32 Node starts up, the LCD will display a message asking you to confirm that you want to start calibration. Press '1' on the keypad to confirm and start calibration.
3. Manually position the turntable at the head-end position for track 1.
4. Enter '1' on the keypad, followed by '*'. The LCD will display a message confirming that the position has been stored.
5. Repeat steps 3 and 4 for each track number and end. For the tail-end positions, enter the track number followed by '#'.
6. After all positions have been stored, set **CALIBRATION_MODE** to **false** in the sketch and upload it again to the ESP32 Node.

### Operation
1. After the ESP32 Node starts up, it will connect to the WiFi network and subscribe to the MQTT topic for the turntable.
2. To move the turntable to a specific position, enter the track number on the keypad, followed by '*' for the head-end or '#' for the tail-end. The turntable will move to the stored position for that track number and end.

### Troubleshooting
1. **WiFi Connection Issues**: If the ESP32 Node fails to connect to the WiFi network, check that the SSID and password in the sketch match those of your network. Also, ensure that the ESP32 Node is within range of the WiFi router.

2. **Turntable Movement Issues**: If the turntable doesn't move as expected, ensure that the stepper motor is correctly connected and powered. If the problem persists, you may need to recalibrate the turntable positions.

### Contributing
Contributions to this project are welcome. If you have a feature request, bug report, or want to contribute code, please open an issue or pull request on the project's GitHub page.

### License
This project is licensed under the MIT License. See the LICENSE file in the project root for more information.
