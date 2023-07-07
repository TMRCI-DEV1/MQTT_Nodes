#ifndef TURNTABLE_H
#define TURNTABLE_H

/* Libraries */
#include <Wire.h>              // Library for ESP32 I2C connection. Used for communication with the relay boards and LCD screen.        https://github.com/esp8266/Arduino/tree/master/libraries/Wire

#include <WiFi.h>              // Library for WiFi connection. Used for connecting to the WiFi network.                                 https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi

#include <Keypad.h>            // Library for 3x4 keypad. Used for reading input from the keypad.                                       https://github.com/Chris--A/Keypad

#include <LiquidCrystal_I2C.h> // Library for Liquid Crystal I2C. Used for controlling the LCD screen.                                  https://github.com/johnrickman/LiquidCrystal_I2C

#include <PCF8574.h>           // Library for I2C (8) relay board. Used for controlling the 8-channel relay board.                      https://github.com/xreef/PCF8574_library

#include <PCF8575.h>           // Library for I2C (16) relay board. Used for controlling the 16-channel relay board.                    https://github.com/xreef/PCF8575_library/tree/master

#include <AccelStepper.h>      // Library for Accel Stepper. Used for controlling the stepper motor.                                    https://github.com/waspinator/AccelStepper

#include <PubSubClient.h>      // Library for MQTT. Used for subscribing to MQTT messages.                                              https://github.com/knolleary/pubsubclient

#include <EEPROM.h>            // Library for EEPROM read/write. Used for storing the current position and track head/tail positions.   https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM

#include <ArduinoOTA.h>        // Library for OTA updates. Used for updating the sketch over the air.                                   https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

/* Constants */
// Keypad Related
const byte ROW_NUM = 4; // Number of rows on the keypad.
const byte COLUMN_NUM = 3; // Number of columns on the keypad.
char keys[ROW_NUM][COLUMN_NUM] = { // Layout of the keys on the keypad.
  {
    '1',
    '2',
    '3'
  },
  {
    '4',
    '5',
    '6'
  },
  {
    '7',
    '8',
    '9'
  },
  {
    '*',
    '0',
    '#'
  }
};
byte KEYPAD_ROW_PINS[] = {
  13,
  12,
  14,
  27
}; // Row pins of the keypad. These are the pins that the keypad rows are connected to.
byte KEYPAD_COLUMN_PINS[] = {
  16,
  17,
  18
}; // Column pins of the keypad. These are the pins that the keypad columns are connected to.
Keypad keypad = Keypad(makeKeymap(keys), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, ROW_NUM, COLUMN_NUM); // Keypad instance. This is used for reading input from the keypad.
char keypadTrackNumber[3] = ""; // Array to store entered track number on the keypad. This is where the track number entered on the keypad is stored.

// Stepper Motor Related
// Stepper Motor Related
const int STEPS_PER_REV = 6400; // Number of microsteps per full revolution. Microsteps are used for smoother and more precise control of the stepper motor.
const int STEPPER_SPEED = 200; // Stepper motor speed (steps per second). This is the speed at which the stepper motor will rotate.
AccelStepper stepper(AccelStepper::DRIVER, 33, 32); // Stepper driver step (pulse) pin, direction pin. This is used for controlling the stepper motor.

// Relay Board Related
const int RELAY_BOARD1_ADDRESS = 0x20; // I2C address of relay board 1. This is the address that the ESP32 will use to communicate with the first relay board.
const int RELAY_BOARD2_ADDRESS = 0x21; // I2C address of relay board 2. This is the address that the ESP32 will use to communicate with the second relay board.
PCF8575 relayBoard1(RELAY_BOARD1_ADDRESS); // Instance for the 16-channel relay board. This is used for controlling the 16-channel relay board (turntable bridge and tracks 1-15.
PCF8574 relayBoard2(RELAY_BOARD2_ADDRESS); // Instance for the 8-channel relay board. This is used for controlling the 8-channel relay board (tracks 16-23).

// LCD Related
const int LCD_ADDRESS = 0x27; // I2C address of the LCD.
const int LCD_COLUMNS = 20; // Number of columns in the LCD.
const int LCD_ROWS = 4; // Number of rows in the LCD.
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS); // LCD instance.

// Miscellaneous
const int HOMING_SENSOR_PIN = 25; // Pin for the homing sensor. The homing sensor is used to set a known position for the turntable.
const int RESET_BUTTON_PIN = 19; // Pin for the reset button. The reset button is used to trigger the homing sequence.
bool emergencyStop = false; // Flag for emergency stop condition. When this flag is set, the turntable will stop moving immediately.
char mqttTrackNumber[3] = ""; // Array to store track number from MQTT message. This is where the track number from the MQTT message is stored.

// Network and MQTT Related
const char * ssid = "Your_WiFi_SSID"; // WiFi network SSID. This is the name of the WiFi network that the ESP32 will connect to.
const char * password = "Your_WiFi_Password"; // WiFi network password. This is the password for the WiFi network that the ESP32 will connect to.
const char * mqtt_broker = "Your_MQTT_Broker"; // MQTT broker address. This is the address of the MQTT broker that the ESP32 will connect to.
const int mqtt_port = 1883; // MQTT port number.
WiFiClient espClient; // WiFi client instance. This is used for the network connection.
PubSubClient client(espClient); // MQTT client instance. This is used for subscribing to MQTT messages.

// EEPROM Related
const int CURRENT_POSITION_EEPROM_ADDRESS = 0; // EEPROM address for storing the current position of the turntable
const int EEPROM_TRACK_HEADS_ADDRESS = 100; // EEPROM address for storing track head positions
const int EEPROM_TRACK_TAILS_ADDRESS = 200; // EEPROM address for storing track tail positions
const int EEPROM_TOTAL_SIZE_BYTES = 512; // EEPROM size in bytes. This is the total amount of storage available in the EEPROM.

// Calibration Related
#ifdef CALIBRATION_MODE // Set to true during calibration, false otherwise. In calibration mode, the turntable will move to the position specified by the track number directly.
const bool calibrationMode = true;
#else
const bool calibrationMode = false;
#endif

const char CONFIRM_YES = '1'; // Confirmation key for calibration mode. Pressing this key will confirm the current position as the head or tail position for the current track.
const char CONFIRM_NO = '3'; // Cancellation key for calibration mode. Pressing this key will cancel the calibration process.
const int STEP_MOVE_SINGLE_KEYPRESS = 10; // Number of steps to move with a single key press during calibration. This allows for fine control of the turntable position during calibration.
const int STEP_MOVE_HELD_KEYPRESS = 100; // Number of steps to move with a held key press during calibration. This allows for fast movement of the turntable during calibration.

// Position and Track Numbers
int currentPosition = 0; // Current position of the turntable. This is updated every time the turntable moves.
extern
const int NUMBER_OF_TRACKS; // External constant variable representing the number of tracks available on the turntable. The value of this variable is defined in the main sketch file.
extern int * TRACK_NUMBERS; // External pointer to an integer array representing the track numbers on the turntable. The array contains the track numbers in the order they are arranged on the turntable. The value of this pointer is defined in the main sketch file.

/* Function prototypes */
void callback(char * topic, byte * payload, unsigned int length); // Callback function for MQTT messages. This function is called whenever an MQTT message is received.
int calculateTargetPosition(int trackNumber, int endNumber); // Function to calculate the target position based on the track number and end number. This function is used to determine where the turntable should move to.
void controlRelays(int trackNumber); // Function to control the track power relays. This function is used to turn on the relay for the selected track and turn off all other relays.
void moveToTargetPosition(int targetPosition); // Function to move the turntable to a target position. This function is used to move the turntable to the desired position.

#endif
