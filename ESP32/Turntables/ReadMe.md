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

### Calibration Preparation

1. Ensure that the CALIBRATION_MODE constant in the sketch is set to true. This will allow the system to enter calibration mode.
2. Upload the sketch to the ESP32 Node.

### Calibration Process

1. After the sketch is uploaded and the ESP32 Node is powered on, the LCD will display a message indicating that the system is in calibration mode. You will be prompted to press '1' to confirm and start the calibration process or '3' to cancel.
2. Press '1' on the keypad to confirm and start the calibration process. The LCD will display a message indicating that calibration has started.
3. Now, you will manually move the turntable to each track's head and tail positions and store these positions. To do this, use the '4' and '6' keys on the keypad to move the turntable counter-clockwise and clockwise, respectively. A single key press will move the turntable by a fixed number of steps (defined by `STEP_MOVE_SINGLE_KEYPRESS`), while holding the key will move the turntable continuously (defined by `STEP_MOVE_HELD_KEYPRESS`).
4. Once the turntable is positioned at the head-end of a track, enter the track number on the keypad (1-23), and then press the '*' key. This will store the current position as the head-end position for the entered track number.
5. Similarly, once the turntable is positioned at the tail-end of a track, enter the track number on the keypad (1-23), and then press the '#' key. This will store the current position as the tail-end position for the entered track number.
6. Repeat steps 3-5 for each track on the turntable.
7. After all track positions have been stored, set the `CALIBRATION_MODE` constant in the sketch back to `false` and re-upload the sketch to the ESP32 Node. This will exit calibration mode and allow the system to operate normally, using the stored track positions.

### Calibration Notes

- During calibration, the system will not respond to MQTT messages. It will only respond to keypad inputs.
- The stored track positions are saved in the ESP32's EEPROM, so they will persist even if the ESP32 is powered off or reset.
- If you need to recalibrate the system in the future, simply set `CALIBRATION_MODE` back to `true` and repeat the calibration process.
- If you trigger an emergency stop during calibration (by pressing the '9' key three times consecutively), the system will stop moving the turntable and display an emergency stop message. After this, you can continue the calibration process.
- If you press the reset button during calibration, the system will trigger a homing sequence instead of restarting. The turntable will move to the home position, and the current position will be set to zero. After this, you can continue the calibration process.

### Operation Preparation

1. Ensure that the `CALIBRATION_MODE` constant in the sketch is set to `false`. This will allow the system to operate normally.
2. Upload the sketch to the ESP32 Node.

### Operation Process

1. After the sketch is uploaded and the ESP32 Node is powered on, the LCD will display the IP address of the ESP32 Node. This IP address is used for Over-The-Air (OTA) updates.
2. The system is now ready to receive commands either from the keypad or via MQTT messages.

### Keypad Operation

1. To move the turntable to a specific track, enter the track number (1-23) on the keypad.
2. After entering the track number, press the '*' key to move the turntable to the head-end of the track, or press the '#' key to move the turntable to the tail-end of the track.
3. The turntable will then move to the commanded position, and the LCD will display the commanded track number and the head or tail position.

### MQTT Operation

1. The system is subscribed to MQTT messages published by JMRI. The expected MQTT message format is 'Tracknx', where 'n' represents the 2-digit track number (01-23) and 'x' represents 'H' for the head-end or 'T' for the tail-end.
2. When an MQTT message is received, the system will extract the track number and end position from the message, calculate the target position, and move the turntable to the target position.

### Emergency Stop

1. If you need to stop the turntable immediately, press the '9' key three times consecutively. This will trigger an emergency stop, and the system will stop moving the turntable and display an emergency stop message on the LCD.

### Reset Button

1. If you press the reset button, the system will trigger a homing sequence instead of restarting. The turntable will move to the home position, and the current position will be set to zero.

### Operation Notes

- The system will not respond to keypad inputs while it is moving the turntable. You must wait until the turntable has finished moving before entering a new command.
- The system will save the current position of the turntable in the ESP32's EEPROM whenever it completes a move. This allows the system to remember its position even if it is powered off or reset.
- The system supports OTA updates. You can update the sketch on the ESP32 Node over the WiFi network using the Arduino IDE. The IP address for OTA updates is displayed on the LCD when the system starts up.

## Troubleshooting Guide for Turntable Control

If you're experiencing issues with the Turntable Control system, you can refer to the following troubleshooting guide to help identify and resolve common problems.

### The turntable is not moving

- Check the power supply to the ESP32 Node, stepper motor, and relay boards. Ensure they are properly connected and powered.
- Verify the wiring connections between the ESP32 Node and the stepper motor. Make sure the step and direction pins are correctly connected.
- Check the stepper motor driver settings, especially the microstepping settings. Ensure they match the `STEPS_PER_REV` constant in the sketch.

### The turntable is not moving to the correct position

- Ensure the turntable has been properly calibrated. If not, switch to calibration mode and follow the calibration guide.
- Check the EEPROM data. If the data is corrupted, you may need to clear the EEPROM and recalibrate the turntable.

### The LCD is not displaying the correct information

- Check the I2C connection between the ESP32 Node and the LCD. Ensure the SDA and SCL pins are correctly connected.
- Verify the I2C address of the LCD. Ensure it matches the address specified in the `LiquidCrystal_I2C lcd(0x27, 20, 4);` line of the sketch.

### The keypad is not working

- Check the wiring connections between the ESP32 Node and the keypad. Ensure the row and column pins are correctly connected.
- Test each key on the keypad. If a specific key is not working, there may be a problem with the keypad itself.

### The system is not receiving MQTT messages

- Check the WiFi connection. Ensure the ESP32 Node is connected to the WiFi network and the MQTT broker.
- Check the MQTT topic. Ensure the topic matches the one specified in the `const char* MQTT_TOPIC = "TMRCI/output/("location")/turntable/#";` line of the sketch.
- Check the MQTT message format. Ensure the messages are in the 'Tracknx' format, where 'n' is the 2-digit track number and 'x' is 'H' for the head-end or 'T' for the tail-end.

### The emergency stop is not working

- Ensure you are pressing the '9' key three times consecutively to trigger the emergency stop.

### The reset button is not working

- Check the wiring connection between the ESP32 Node and the reset button. Ensure the button is correctly connected to the `RESET_BUTTON_PIN`.
- Test the reset button. If it is not working, there may be a problem with the button itself.

### OTA updates are not working

- Check the WiFi connection. Ensure the ESP32 Node is connected to the WiFi network.
- Check the IP address of the ESP32 Node. Ensure it matches the one displayed on the LCD.
- Check the OTA password. Ensure it matches the one specified in the `ArduinoOTA.setPassword("TMRCI");` line of the sketch.

Remember, when troubleshooting, always start with the simplest and most obvious solutions first. Check all connections and settings, and ensure all components are functioning correctly. If you're still having trouble, you may need to consult with a more experienced member.

## Constants

The sketch defines several constants:

- `STEPS_PER_REV`: The number of microsteps per revolution for the stepper motor.
- `CURRENT_POSITION_EEPROM_ADDRESS`: The EEPROM address for storing the turntable positions.
- `EEPROM_HEADS_ADDRESS`: The EEPROM address for storing the track head positions.
- `EEPROM_TAILS_ADDRESS`: The EEPROM address for storing the track tail positions.
- `HOMING_SENSOR_PIN`: The pin number for the homing sensor.
- `RESET_BUTTON_PIN`: The pin number for the reset button.
- `MQTT_TOPIC`: The MQTT topic to subscribe to for turntable commands.
- `TRACK_NUMBERS`: An array of track numbers.
- `STEPPER_SPEED`: The speed of the stepper motor.
- `DELAY_TIMES`: An array of delay times.
- `EEPROM_TOTAL_SIZE_BYTES`: The size of the EEPROM memory.
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

- `setup()`: This is the ESP32 setup function, which is called once when the program starts. It initializes the Serial and Wire communications, connects to the WiFi network, initializes the EEPROM, prints the IP address to the serial monitor, sets the hostname, initializes OTA updates, sets up the MQTT client, moves to the home position at startup, initializes the relay boards, initializes the LCD and prints the IP address, displays the calibration mode message, initializes the stepper, and initializes the keys array.

- `loop()`: This is the ESP32 main loop function, which is called repeatedly in a loop. It handles MQTT messages and keypad inputs, checks for emergency stop, checks for keypad input, checks for reset button press, and runs the stepper.

- `callback(char* topic, byte* payload, unsigned int length)`: This is the MQTT callback function, which is called when a subscribed topic receives a message. It extracts the track number and end number from the MQTT message, calculates the target position, and moves to the target position.

- `calculateTargetPosition(int trackNumber, int endNumber)`: This function calculates the target position based on the track number and end number. In calibration mode, the target position is the track number itself. Otherwise, it retrieves the corresponding head or tail position.

- `controlRelays(int trackNumber)`: This function controls the track power relays. It turns off all relays, then turns on the relay corresponding to the selected track.

- `moveToTargetPosition(int targetPosition)`:  This function moves the turntable to the target position. It turns off the turntable bridge track power before starting the move, moves the turntable to the target position, updates the current position after moving, turns on the track power for the target position after the move is complete, and turns the turntable bridge track power back on once the turntable has completed its move. If not in calibration mode, it also saves the current position to EEPROM.

- `writeToEEPROMWithVerification(int address, const T& value)`: This function writes data to the EEPROM at the specified address, while performing error checking to ensure data integrity. It reads the original value from the EEPROM, writes the new value to the EEPROM, commits the changes, reads the value that was written to the EEPROM, and verifies the data integrity by comparing the original value with the read value. If the verification fails, it prints an error message indicating an EEPROM write error.

- `readFromEEPROMWithVerification(int address, T& value)`: This function reads data from the EEPROM at the specified address, while performing error checking to ensure data integrity. It reads the original value from the EEPROM, copies the original value to the `value` variable, reads the value from the EEPROM again, and verifies the data integrity by comparing the original value with the read value. If the verification fails, it prints an error message indicating an EEPROM read error and returns `false`. If the verification succeeds, it returns `true`.

### Contributing

Contributions to this project are welcome. If you have a feature request, bug report, or want to contribute code, please open an issue or pull request on the project's GitHub page.

**Author**: Thomas Seitz, TMRCI Layout Systems Engineer

### License

This work is licensed under a
[Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[![CC BY-SA 4.0][cc-by-sa-image]][cc-by-sa]

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-image]: https://licensebuttons.net/l/by-sa/4.0/88x31.png
