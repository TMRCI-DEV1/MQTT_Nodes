# Turntable Control

## Overview

This ESP32 sketch is designed to control a turntable using an ESP32 Node. The turntable can be controlled by entering a 1 or 2-digit track number on a keypad or by receiving MQTT messages published by JMRI. The sketch also supports calibration of the turntable positions.

## Components

The system utilizes the following components:

- ESP32 Node: The main microcontroller responsible for controlling the turntable.
- Input Devices:
  - DIYables 3x4 membrane matrix keypad: Allows users to enter track numbers and control the turntable manually.
- Output Devices:
  - GeeekPi IIC I2C TWI Serial LCD 2004 20x4 Display Module with I2C Interface: Displays system information, track numbers, and positions.
  - KRIDA Electronics 16 Channel Electromagnetic Relay Module with I2C Interface: Controls power to the tracks.
  - KRIDA Electronics 8 Channel Electromagnetic Relay Module with I2C Interface: Controls power to the tracks.
- Stepper Motor Components:
  - STEPPERONLINE DM320T 2-Phase digital Stepper Drive: Drives the stepper motor.
  - TT Electronics Photologic Slotted Optical Switch OPB492T11Z "homing" sensor: Detects the home position of the turntable.
  - STEPPERONLINE stepper motor (Nema 17 Bipolar 0.9deg 46Ncm (65.1oz.in) 2A 42x42x48mm 4 Wires): Rotates the turntable.

## Libraries Used

The sketch utilizes the following libraries:

- Wire: [https://github.com/esp8266/Arduino/tree/master/libraries/Wire](https://github.com/esp8266/Arduino/tree/master/libraries/Wire) - Provides I2C communication for the peripherals.
- WiFi: [https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi) - Enables WiFi connectivity for the ESP32 Node.
- Keypad: [https://github.com/Chris--A/Keypad](https://github.com/Chris--A/Keypad) - Handles input from the keypad.
- LiquidCrystal_I2C: [https://github.com/johnrickman/LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C) - Controls the LCD display.
- PCF8575: [https://github.com/xreef/PCF8575_library/tree/master](https://github.com/xreef/PCF8575_library/tree/master) - Handles I2C communication with the relay modules.
- PCF8574: [https://github.com/xreef/PCF8574_library](https://github.com/xreef/PCF8574_library) - Handles I2C communication with the relay modules.
- AccelStepper: [https://github.com/waspinator/AccelStepper](https://github.com/waspinator/AccelStepper) - Controls the stepper motor movement.
- PubSubClient: [https://github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient) - Enables MQTT communication with JMRI.
- EEPROM: [https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM](https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM) - Allows data to be stored in the ESP32's EEPROM.
- ArduinoOTA: [https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA) - Enables Over-The-Air (OTA) updates for the ESP32 Node.

## Functionality

The ESP32 Node connects to a WiFi network and subscribes to MQTT messages published by JMRI. The expected MQTT message format is 'Tracknx', where 'n' represents the 2-digit track number (01-23) and 'x' represents 'H' for the head-end or 'T' for the tail-end.

The turntable supports bidirectional movement, allowing it to move to both the head and tail positions. The LCD displays the IP address, the commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname.

The sketch supports a calibration mode, which is used to set the positions of the turntable for each track number and end (head or tail). In calibration mode, the current position of the turntable is stored when a track number and end are entered on the keypad.

When not in calibration mode, the sketch retrieves the stored positions from the EEPROM at startup and uses them to move the turntable to the correct position when a track number and end are entered on the keypad or received in an MQTT message.

The sketch also includes an emergency stop feature, which can be activated by pressing the '9' key on the keypad three times in a row. When the emergency stop is activated, the turntable movement is immediately halted.

## How to Use

### Calibration Preparation

1. Uncomment the `//#define calibrationMode` line in the sketch to enable calibration mode. This will allow the system to enter calibration mode.
2. Upload the sketch to the ESP32 Node.

### Calibration Process

1. After uploading the sketch and powering on the ESP32 Node, the LCD will display a message indicating that the system is in calibration mode.
2. Manually move the turntable to each track's head and tail positions using the '4' and '6' keys on the keypad. Each key press will move the turntable by a fixed number of steps (defined by `STEP_MOVE_SINGLE_KEYPRESS`). Holding down a key will move the turntable continuously (defined by `STEP_MOVE_HELD_KEYPRESS`).
3. Once the turntable is positioned at the head-end of a track, enter the track number on the keypad (1 to `NUMBER_OF_TRACKS`), and then press the '*' key. This will store the current position as the head-end position for the entered track number.
4. Similarly, once the turntable is positioned at the tail-end of a track, enter the track number on the keypad (1 to `NUMBER_OF_TRACKS`), and then press the '#' key. This will store the current position as the tail-end position for the entered track number.
5. Repeat steps 2-4 for each track on the turntable.
6. After storing all track positions, comment out the `//#define calibrationMode` line in the sketch to disable calibration mode.
7. Upload the modified sketch to the ESP32 Node to exit calibration mode and allow the system to operate using the stored track positions.

### Operation Preparation

1. Upload the modified sketch (with calibration mode disabled) to the ESP32 Node.

### Operation Process

1. After uploading the sketch and powering on the ESP32 Node, the LCD will display the IP address of the ESP32 Node. This IP address is used for Over-The-Air (OTA) updates.
2. Ensure that the turntable is in the home position.
3. The system is now ready to receive commands either from the keypad or via MQTT messages.

### Keypad Operation

1. To move the turntable to a specific track, enter the track number (1 to `NUMBER_OF_TRACKS`) on the keypad.
2. After entering the track number, press the '*' key to move the turntable to the head-end of the track or press the '#' key to move the turntable to the tail-end of the track.
3. The turntable will move to the commanded position, and the LCD will display the commanded track number and the head or tail position.

### MQTT Operation

1. The system is subscribed to MQTT messages published by JMRI. The expected MQTT message format is 'Tracknx', where 'n' represents the 2-digit track number (01 to `NUMBER_OF_TRACKS`) and 'x' represents 'H' for the head-end or 'T' for the tail-end.
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

- `VERSION_NUMBER`: A constant string that represents the version number of the sketch.
- `ROW_NUM`: The number of rows for the keypad.
- `COLUMN_NUM`: The number of columns for the keypad.
- `KEYPAD_ROW_PINS`: An array of row pins for the keypad.
- `KEYPAD_COLUMN_PINS`: An array of column pins for the keypad.
- `STEPS_PER_REV`: The number of microsteps per revolution for the stepper motor.
- `STEPPER_SPEED`: The speed of the stepper motor.
- `RELAY_BOARD1_ADDRESS`: The address of the first relay board.
- `RELAY_BOARD2_ADDRESS`: The address of the second relay board.
- `LCD_ADDRESS`: The address of the LCD display.
- `LCD_COLUMNS`: The number of columns for the LCD display.
- `LCD_ROWS`: The number of rows for the LCD display.
- `HOMING_SENSOR_PIN`: The pin number for the homing sensor.
- `RESET_BUTTON_PIN`: The pin number for the reset button.
- `mqtt_port`: The MQTT broker port.
- `ssid`: The WiFi network SSID.
- `password`: The WiFi network password.
- `mqtt_broker`: The MQTT broker address.
- `CURRENT_POSITION_EEPROM_ADDRESS`: The EEPROM address for storing the current position.
- `EEPROM_TRACK_HEADS_ADDRESS`: The EEPROM address for storing the track heads.
- `EEPROM_TOTAL_SIZE_BYTES`: The total size of the EEPROM memory.
- `CONFIRM_YES`: The character for confirming actions.
- `CONFIRM_NO`: The character for canceling actions.
- `STEP_MOVE_SINGLE_KEYPRESS`: The number of steps to move for a single keypress.
- `STEP_MOVE_HELD_KEYPRESS`: The number of steps to move for a held keypress.

## Variables

The sketch also defines several variables:

- `MQTT_TOPIC`: A string variable that represents the MQTT topic.
- `NUMBER_OF_TRACKS`: An integer variable that represents the number of tracks.
- `TRACK_NUMBERS`: A pointer to an integer array that stores the track numbers.
- `gilbertonTrackNumbers`: An integer array that stores the track numbers in the Gilbertonlocation.
- `pittsburghTrackNumbers`: An integer array that stores the track numbers in the Pittsburgh location.
- `hobokenTrackNumbers`: An integer array that stores the track numbers in the Hoboken location.
- `trackHeads`: A dynamically allocated integer array that stores the head positions of each track.
- `trackTails`: A dynamically allocated integer array that stores the tail positions of each track.
- `keypadTrackNumber`: A character array that stores the track number entered on the keypad.
- `currentPosition`: An integer variable that represents the current position of the turntable.
- `emergencyStop`: A boolean variable that indicates whether an emergency stop is triggered.
- `calibrationMode`: A boolean variable that indicates whether the calibration mode is enabled.
- `ssid`: A string variable that represents the WiFi network SSID.
- `password`: A string variable that represents the WiFi network password.
- `mqtt_broker`: A string variable that represents the MQTT broker address.
- `mqtt_port`: An integer variable that represents the MQTT broker port.
- `relayBoard1`: An instance of the "RelayBoard" class.
- `relayBoard2`: An instance of the "RelayBoard" class.
- `stepper`: An instance of the "StepperMotor" class.
- `lcd`: An instance of the "LiquidCrystal_I2C" class.
- `client`: An instance of the "WiFiClient" class.
- `keypad`: An instance of the "Keypad" class.

## Function Prototypes

The sketch includes the following function prototypes:

- `callback`: A function that handles the MQTT message callback.
- `calculateTargetPosition`: A function that calculates the target position based on the track number.
- `controlRelays`: A function that controls the relays for track selection.
- `moveToTargetPosition`: A function that moves the turntable to the target position.

## Functions

The sketch includes several functions:

- `getEEPROMTrackTailsAddress()`: This function calculates and returns the EEPROM address for storing track tail positions.

- `writeToEEPROMWithVerification(int address, const T & value)`: This function writes data to EEPROM with error checking. It uses a template to allow for writing different data types to EEPROM.

- `readFromEEPROMWithVerification(int address, T & value)`: This function reads data from EEPROM with error checking. It uses a template to allow for reading different data types from EEPROM.

- `connectToWiFi()`: This function connects the ESP32 to the WiFi network.

- `connectToMQTT()`: This function connects the ESP32 to the MQTT broker and subscribes to the MQTT topic.

- `setup()`: This is the setup function for the ESP32. It initializes the system and connects to WiFi and MQTT.

- `loop()`: This is the main loop function for the ESP32. It handles MQTT messages and keypad inputs.

- `callback(char * topic, byte * payload, unsigned int length)`: This is the MQTT callback function. It handles incoming MQTT messages.

- `calculateTargetPosition(int trackNumber, int endNumber)`: This function calculates the target position based on the track number and the end (head or tail) specified.

- `controlRelays(int trackNumber)`: This function controls the track power relays.

- `moveToTargetPosition(int targetPosition)`: This function moves the turntable to the target position.

### Contributing

Contributions to this project are welcome. If you have a feature request, bug report, or want to contribute code, please open an issue or pull request on the project's GitHub page.

**Author**: Thomas Seitz, TMRCI Layout Systems Engineer

### License

This work is licensed under a
[Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[![CC BY-SA 4.0][cc-by-sa-image]][cc-by-sa]

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-image]: https://licensebuttons.net/l/by-sa/4.0/88x31.png
