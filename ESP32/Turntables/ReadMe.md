# TURNTABLE CONTROL
This sketch is a firmware for an ESP32-based WiFi/MQTT turntable control node. It is designed to control a turntable for HO scale trains using various components such as a 3x4 membrane matrix keypad, a serial LCD 2004 20x4 display module with I2C interface, (2) 16 Channel I2C Interface Electromagnetic Relay Modules, a STEPPERONLINE CNC stepper motor driver, and a STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead).

The ESP32 Node connects to a WiFi network and subscribes to MQTT messages published by JMRI (Java Model Railroad Interface) software. The turntable can be controlled by entering a 2-digit track number on the keypad, followed by '*' or '#' to select the head-end or tail-end, respectively.

## Sketch Overview
Here's a breakdown of the important parts of the sketch:

1. **Libraries**: The sketch includes several libraries required for different functionalities, such as Wire for I2C communication, WiFi for WiFi connectivity, Keypad for keypad input, LiquidCrystal_I2C for controlling the LCD display, PCF8575 for controlling the I2C relay boards, AccelStepper for controlling the stepper motor, PubSubClient for MQTT communication, and EEPROM for reading and writing to EEPROM memory.

2. **Constants**: The sketch defines constants for the steps per revolution of the stepper motor and EEPROM addresses for saving positions.

3. **Network and MQTT Configuration**: The sketch includes network credentials (SSID and password) for connecting to the WiFi network and the MQTT broker's IP address.

4. **Global Variables**: Global variables are declared to store the current position, track heads, track tails, and track number.

5. **Setup Function**: The setup function initializes the system, including initializing serial and I2C communication, connecting to the WiFi network, setting up the MQTT client, initializing the LCD display, and subscribing to the MQTT topic.

6. **Loop Function**: The loop function continuously checks for keypad input, processes the input to build the track number, and handles different key presses. It also allows the MQTT client to process incoming messages and reconnects to the MQTT server if the connection is lost.

7. **Callback Function**: The callback function is called when an MQTT message is received. It validates the topic format, extracts the track number and end character (head or tail), calculates the target position based on the track number and end number, and moves the stepper motor to the target position. It also updates the LCD display with track and position information.

8. **Helper Functions**: The sketch includes additional helper functions for moving the stepper motor to a target position and calculating the target position based on the track number and end number.

## Calibration Process
The calibration process in this sketch is designed to allow you to set and save the head and tail positions for each track on the turntable.

### How-To: Calibrate Turntable Control
The Turntable Control calibration sketch is designed to be used with an ESP32-based WiFi/MQTT Turntable Node. This guide will walk you through the calibration process for setting up the turntable control system. Calibration ensures accurate positioning of the turntable and allows you to save the head and tail positions for each track.

**Prerequisites**
Before starting the calibration process, make sure you have the following:

- An ESP32-based WiFi/MQTT Turntable Node with the Gilberton Turntable Control sketch uploaded.
- The necessary hardware components connected to the ESP32, including the keypad, LCD screen, relay boards, stepper motor driver, and stepper motor.
- Access to a WiFi network for the ESP32 to connect to.
- A computer or device with the Arduino IDE installed for uploading the sketch and monitoring the serial output.

**Step 1**: Set Up the Hardware
Ensure that all the hardware components are properly connected to the ESP32 Node. Double-check the wiring connections to make sure everything is in order. This includes the keypad, LCD screen, relay boards, stepper motor driver, and stepper motor. Refer to the hardware documentation or the sketch code comments for the correct wiring configurations.

**Step 2**: Upload the Calibration Sketch
Using the Arduino IDE, open the Turntable Control Calibration sketch file. Make sure you have selected the correct board and port in the Arduino IDE settings. Then, upload the sketch to the ESP32 Node.

**Step 3**: Connect to WiFi
After uploading the sketch, the ESP32 Node will attempt to connect to the WiFi network specified in the sketch. Check the serial monitor output to verify that the ESP32 successfully connects to the WiFi network. If the connection fails, ensure that you have entered the correct network credentials (SSID and password) in the sketch.

**Step 4**: Enter Calibration Mode
Once the ESP32 Node is connected to the WiFi network, it will enter calibration mode. On the LCD screen, you should see a message indicating that the system is in "Calibration Mode." This mode allows you to set the head and tail positions for each track on the turntable.

**Step 5**: Select a Track and Position
Using the 3x4 membrane matrix keypad, enter the 2-digit track number you want to calibrate. The track number should be within the range of 01 to 24. Press the '*' key to select the head-end position or the '#' key to select the tail-end position.

**Step 6**: Save or Discard the Position
After selecting the track number and position, the LCD screen will prompt you to confirm whether you want to save or discard the position. Press '1' to save the position or '3' to discard it.

If you choose to save the position, the current position of the turntable will be saved as either the head or tail position for the selected track. The saved positions will be used for future track control.
If you choose to discard the position, the current position will not be saved, and the system will return to the calibration mode screen.

**Note**: Enter single-digit track numbers (1-9) as 01-09 with leading zeros.

**Step 7**: Repeat for Other Tracks
Repeat steps 5 and 6 to calibrate the head and tail positions for other tracks on the turntable. Each track can have its own unique head and tail positions.

**Step 8**: Complete Calibration and Exit
Once you have calibrated all the desired tracks, you can exit the calibration mode. Simply stop entering track numbers, and the system will automatically exit calibration mode.

**Step 9**: Test the Calibration
After completing the calibration process, you can test the turntable control by sending MQTT messages to the ESP32 Node. Publish MQTT messages in the format 'Tracknx', where 'n' represents the 2-digit track number (01-24), and 'x' represents 'H' for the head-end or 'T' for the tail-end. The ESP32 Node will receive these messages and move the turntable to the corresponding track and position.

**Step 10**: Upload the Production Sketch
Once the Turntable Control Calibration is complete for all 24 tracks, upload the Production version of the sketch. This will utilize the track positions saved during the calibration process and is suitable for normal operations and use.

**Step 11**: OTA Updates (Optional)
The ESP32 Node supports Over-the-Air (OTA) updates for convenient firmware updates. If you need to update the Turntable Control sketch in the future, you can do so using OTA. To perform OTA updates, follow these steps:

1. Ensure that the ESP32 Node is connected to the same WiFi network as your computer or device.
2. Open the Arduino IDE and go to "Sketch" > "Upload Using Programmer" to compile the sketch.
3. Once the compilation is complete, go to "Sketch" > "Export Compiled Binary" to generate a binary (.bin) file.
4. Use an OTA update tool or platform-specific instructions to upload the binary file to the ESP32 Node over the air.

Please note that OTA updates should only be performed when necessary and with caution to avoid any disruptions or issues with the turntable control system.

**Conclusion**:
By following this comprehensive How-To guide, you should now have successfully calibrated the Turntable Control system. The calibrated positions of the turntable's head and tail for each track will allow precise control and positioning of the turntable.
