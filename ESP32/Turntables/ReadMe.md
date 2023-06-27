TURNTABLE CONTROL:

This sketch is a firmware for an ESP32-based WiFi/MQTT turntable control node. It is designed to control a turntable for HO scale trains using various components such as a 3x4 membrane matrix keypad, a serial LCD 2004 20x4 display module with I2C interface, a STEPPERONLINE CNC stepper motor driver, and a STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead).

The ESP32 Node connects to a WiFi network and subscribes to MQTT messages published by JMRI (Java Model Railroad Interface) software. The turntable can be controlled by entering a 2-digit track number on the keypad, followed by '*' or '#' to select the head-end or tail-end, respectively.

Here's a breakdown of the important parts of the sketch:

Libraries: The sketch includes several libraries required for different functionalities, such as Wire for I2C communication, WiFi for WiFi connectivity, Keypad for keypad input, LiquidCrystal_I2C for controlling the LCD display, AccelStepper for controlling the stepper motor, PubSubClient for MQTT communication, and EEPROM for reading and writing to EEPROM memory.

Constants: The sketch defines constants for the steps per revolution of the stepper motor and EEPROM addresses for saving positions.

Network and MQTT Configuration: The sketch includes network credentials (SSID and password) for connecting to the WiFi network and the MQTT broker's IP address.

Global Variables: Global variables are declared to store the current position, track heads, track tails, and track number.

Setup Function: The setup function initializes the system, including initializing serial and I2C communication, connecting to the WiFi network, setting up the MQTT client, initializing the LCD display, and subscribing to the MQTT topic.

Loop Function: The loop function continuously checks for keypad input, processes the input to build the track number, and handles different key presses. It also allows the MQTT client to process incoming messages and reconnects to the MQTT server if the connection is lost.

Callback Function: The callback function is called when an MQTT message is received. It validates the topic format, extracts the track number and end character (head or tail), calculates the target position based on the track number and end number, and moves the stepper motor to the target position. It also updates the LCD display with track and position information.

Helper Functions: The sketch includes additional helper functions for moving the stepper motor to a target position and calculating the target position based on the track number and end number.

Please note that this sketch contains comments that provide explanations and instructions for calibration purposes. It's important to read and understand the comments before deploying the sketch in a production environment.


CALIBRATION PROCESS:

The calibration process in this sketch is designed to allow you to set and save the head and tail positions for each track on 
the turntable. 

Here's a step-by-step explanation of the calibration process and the relevant functionality in the sketch:

The sketch starts by initializing the system, connecting to the WiFi network, and setting up the MQTT client.

After the WiFi and MQTT connections are established, the sketch enters the calibration mode. The LCD display shows a "Calibration Mode" 
message.

To calibrate a track, you need to enter a 1 or 2-digit track number on the keypad. The track number represents the track on the turntable that 
you want to calibrate.

Once you enter the track number, you can press the '*' key on the keypad to indicate that you want to set the head position of the track 
or the '#' key to set the tail position.

After pressing '*' or '#', the LCD display prompts you to confirm whether you want to save the current position as the head or tail position. 
Pressing '1' will save the position, while pressing '3' will cancel the calibration for that track.

If you press '1' to save the position, the sketch will update the head or tail position for the specified track in the trackHeads or trackTails 
array respectively.

The head and tail positions are also stored in the EEPROM memory using the EEPROM library. The EEPROM address for each track is calculated 
based on the track number.

The track number is cleared, and the LCD display is cleared to prepare for the calibration of the next track.

This process can be repeated for each track on the turntable, allowing you to calibrate the head and tail positions for all tracks.

The saved head and tail positions will be used later when the turntable is controlled via MQTT messages. When a track number and 
end (head or tail) are received in an MQTT message, the sketch will calculate the target position based on the track number and end 
number and move the stepper motor to that position.

The sketch also includes manual adjustment functionality. Pressing '4' on the keypad moves the turntable clockwise by small increments, 
and pressing '5' moves it counterclockwise by small increments. This can be used for fine-tuning the calibration if needed.

The current position of the turntable is continuously stored in the currentPosition variable, and it is also saved to the EEPROM memory 
to retain the last known position even after power cycling the system.
