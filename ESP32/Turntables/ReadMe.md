# Turntable Control

## Overview

This ESP32 sketch is designed to control a Turntable using an ESP32 Node. The turntable can be controlled by entering a 1 or 2-digit track number on a keypad, or by receiving MQTT messages published by JMRI. The sketch also supports calibration of the turntable positions.

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

- Wire: [https://github.com/esp8266/Arduino/tree/master/libraries/Wire](https://github.com/esp8266/Arduino/tree/master/libraries/Wire)
- WiFi: [https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- Keypad: [https://github.com/Chris--A/Keypad](https://github.com/Chris--A/Keypad)
- LiquidCrystal_I2C: [https://github.com/johnrickman/LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C)
- PCF8575: [https://github.com/xreef/PCF8575_library/tree/master](https://github.com/xreef/PCF8575_library/tree/master)
- PCF8574: [https://github.com/xreef/PCF8574_library](https://github.com/xreef/PCF8574_library)
- AccelStepper: [https://github.com/waspinator/AccelStepper](https://github.com/waspinator/AccelStepper)
- PubSubClient: [https://github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
- EEPROM: [https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM](https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM)
- ArduinoOTA: [https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)

## Functionality

The ESP32 Node connects to a WiFi network and subscribes to MQTT messages published by JMRI. The expected MQTT message format is 'Tracknx', where 'n' represents the 2-digit track number (01-23) and 'x' represents 'H' for the head-end or 'T' for the tail-end.

The LCD displays the IP address, the commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname.

The sketch supports a calibration mode, which is used to set the positions of the turntable for each track number and end (head or tail). In calibration mode, the current position of the turntable is stored when a track number and end are entered on the keypad.

When not in calibration mode, the sketch retrieves the stored positions from the EEPROM at startup and uses them to move the turntable to the correct position when a track number and end are entered on the keypad or received in an MQTT message.

The sketch also includes an emergency stop feature, which can be activated by pressing the '9' key on the keypad three times in a row. When the emergency stop is activated, the turntable movement is immediately halted.

## How to Use

### Calibration Process

1. Set the `CALIBRATION_MODE` constant to `true` in the sketch and upload it to the ESP32 Node.
2. After the ESP32 Node starts up, the LCD will display a message asking you to confirm that you want to start calibration. Press '1' on the keypad to confirm and start calibration.
3. Manually position the turntable at the head-end position for track 1 using the '4' (CCW) and '6' (CW) keys. Single key press moves the stepper by X steps. A held down key press moves the stepper continuously until key is depressed.
4. Enter '1' on the keypad, followed by '*'. The LCD will display a message confirming that the position has been stored.
5. Repeat steps 3 and 4 for each track number and end. For the tail-end positions, enter the track number followed by '#'.
6. After all positions have been stored, set the `CALIBRATION_MODE` constant to `false` in the sketch and upload it again to the ESP32 Node.

### Operation

1. After the ESP32 Node starts up, it will connect to the WiFi network and subscribe to the MQTT topic for the turntable.
2. To move the turntable to a specific position, enter the track number on the keypad, followed by '*' for the head-end or '#' for the tail-end. The turntable will move to the stored position for that track number and end.
3. To activate the emergency stop, press the '9' key on the keypad three times in a row. The turntable movement will be immediately halted.

### Troubleshooting

1. **WiFi Connection Issues**: If the ESP32 Node fails to connect to the WiFi network, check that the SSID and password in the sketch match those of your network. Also, ensure that the ESP32 Node is within range of the WiFi router.
2. **Turntable Movement Issues**: If the turntable doesn't move as expected, ensure that the stepper motor is correctly connected and powered. If the problem persists, you may need to recalibrate the turntable positions.

## Constants

The sketch defines several constants:

- `STEPS_PER_REV`: The number of microsteps per revolution for the stepper motor.
- `EEPROM_POSITION_ADDRESS`: The EEPROM address for storing the turntable positions.
- `EEPROM_HEADS_ADDRESS`: The EEPROM address for storing the track head positions.
- `EEPROM_TAILS_ADDRESS`: The EEPROM address for storing the track tail positions.
- `HOMING_SENSOR_PIN`: The pin number for the homing sensor.
- `RESET_BUTTON_PIN`: The pin number for the reset button.
- `MQTT_TOPIC`: The MQTT topic to subscribe to for turntable commands.
- `TRACK_NUMBERS`: An array of track numbers.
- `STEPPER_SPEED`: The speed of the stepper motor.
- `DELAY_TIMES`: An array of delay times.
- `EEPROM_SIZE`: The size of the EEPROM memory.
- `RELAY_BOARD1_I2C_ADDRESS`: The I2C address of the first relay board.
- `RELAY_BOARD2_I2C_ADDRESS`: The I2C address of the second relay board.
- `KEYPAD_ROW_PINS`: An array of row pins for the keypad.
- `KEYPAD_COLUMN_PINS`: An array of column pins for the keypad.
- `CALIBRATION_MODE`: A boolean value indicating whether calibration mode is active.
- `CONFIRM_YES`: The character for confirming actions.
- `CONFIRM_NO`: The character for canceling actions.
- `STEP_MOVE_SINGLE_KEYPRESS`: The number of steps to move for a single keypress.
- `STEP_MOVE_HELD_KEYPRESS`: The number of steps to move for a held keypress.

## Variables

The sketch also defines several variables:

- `emergencyStopCounter`: A counter for the emergency stop feature.
- `currentPosition`: The current position of the turntable.
- `trackHeads`: An array of track head positions.
- `trackTails`: An array of track tail positions.
- `ssid`: The SSID of the WiFi network.
- `password`: The password of the WiFi network.
- `mqtt_broker`: The MQTT broker address.
- `espClient`: The WiFi client instance.
- `client`: The MQTT client instance.
- `stepper`: The stepper motor instance.
- `lcd`: The LCD instance.
- `relayBoard1`: The first relay board instance.
- `relayBoard2`: The second relay board instance.
- `keypad`: The keypad instance.

## Functions

The sketch includes several functions:

- `initializeKeysArray()`: This function initializes the keys array with the layout defined in KEYPAD_LAYOUT. This array is used to create the keypad instance.

- `connectToWiFi()`: This function attempts to connect the ESP32 to the WiFi network using the provided SSID and password. It will keep trying to connect until a successful connection is established.

- `connectToMQTT()`: This function attempts to connect the ESP32 to the MQTT broker. It will keep trying to connect until a successful connection is established. Once connected, it subscribes to the MQTT topic defined in MQTT_TOPIC.

- `setup()`: This is the Arduino setup function, which is called once when the program starts. It initializes the Serial and Wire communications, connects to the WiFi network, initializes the EEPROM, prints the IP address to the serial monitor, sets the hostname, initializes OTA updates, sets up the MQTT client, moves to the home position at startup, initializes the relay boards, initializes the LCD and prints the IP address, displays the calibration mode message, initializes the stepper, and initializes the keys array.

- `loop()`: This is the Arduino main loop function, which is called repeatedly in a loop. It handles MQTT messages and keypad inputs, checks for emergency stop, checks for keypad input, checks for reset button press, and runs the stepper.

- `callback(char* topic, byte* payload, unsigned int length)`: This is the MQTT callback function, which is called when a subscribed topic receives a message. It extracts the track number and end number from the MQTT message, calculates the target position, and moves to the target position.

- `calculateTargetPosition(int trackNumber, int endNumber)`: This function calculates the target position based on the track number and end number. In calibration mode, the target position is the track number itself. Otherwise, it retrieves the corresponding head or tail position.

- `controlRelays(int trackNumber)`: This function controls the track power relays. It turns off all relays, then turns on the relay corresponding to the selected track.

- `moveToTargetPosition(int targetPosition)`:  This function moves the turntable to the target position. It turns off the turntable bridge track power before starting the move, moves the turntable to the target position, updates the current position after moving, turns on the track power for the target position after the move is complete, and turns the turntable bridge track power back on once the turntable has completed its move. If not in calibration mode, it also saves the current position to EEPROM.

### Contributing

Contributions to this project are welcome. If you have a feature request, bug report, or want to contribute code, please open an issue or pull request on the project's GitHub page.

### License

This work is licensed under a
[Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[![CC BY-SA 4.0][cc-by-sa-image]][cc-by-sa]

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-image]: https://licensebuttons.net/l/by-sa/4.0/88x31.png
[cc-by-sa-shield]: https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg
