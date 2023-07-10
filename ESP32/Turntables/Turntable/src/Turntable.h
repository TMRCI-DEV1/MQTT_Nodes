#ifndef TURNTABLE_H
#define TURNTABLE_H

/* Libraries */
#include <Wire.h>              // Library for ESP32 I2C connection. Used for communication with the relay boards and LCD screen.        https://github.com/esp8266/Arduino/tree/master/libraries/Wire

#include <Keypad.h>            // Library for 3x4 keypad. Used for reading input from the keypad.                                       https://github.com/Chris--A/Keypad

#include <LiquidCrystal_I2C.h> // Library for Liquid Crystal I2C. Used for controlling the LCD screen.                                  https://github.com/johnrickman/LiquidCrystal_I2C

#include <PCF8574.h>           // Library for I2C (8) relay board. Used for controlling the 8-channel relay board.                      https://github.com/xreef/PCF8574_library

#include <PCF8575.h>           // Library for I2C (16) relay board. Used for controlling the 16-channel relay board.                    https://github.com/xreef/PCF8575_library/tree/master

#include <AccelStepper.h>      // Library for Accel Stepper. Used for controlling the stepper motor.                                    https://github.com/waspinator/AccelStepper

/* Constants */
// Keypad Related
const byte ROW_NUM = 4;                     // Number of rows in the keypad matrix
const byte COLUMN_NUM = 3;                  // Number of columns in the keypad matrix
extern char keys[ROW_NUM][COLUMN_NUM];      // 2D array to store the key characters
extern byte KEYPAD_ROW_PINS[];              // Array of row pins for the keypad
extern byte KEYPAD_COLUMN_PINS[];           // Array of column pins for the keypad
extern Keypad keypad;                       // Keypad object to interface with the keypad
extern char keypadTrackNumber[3];           // Character array to store the entered track number from the keypad

// Stepper Motor Related
extern const int STEPS_PER_REV;             // Number of steps per revolution for the stepper motor
extern const int STEPPER_SPEED;             // Maximum speed for the stepper motor
extern AccelStepper stepper;                // AccelStepper object to control the stepper motor

// Relay Board Related
extern const int RELAY_BOARD1_ADDRESS;      // I2C address of the first relay board
extern const int RELAY_BOARD2_ADDRESS;      // I2C address of the second relay board
extern PCF8575 relayBoard1;                 // PCF8575 object to control the first relay board
extern PCF8574 relayBoard2;                 // PCF8574 object to control the second relay board

// LCD Related
extern const int LCD_ADDRESS;               // I2C address of the LCD display
extern const int LCD_COLUMNS;               // Number of columns in the LCD display
extern const int LCD_ROWS;                  // Number of rows in the LCD display
extern LiquidCrystal_I2C lcd;               // LiquidCrystal_I2C object to control the LCD display

// Miscellaneous
extern const int HOMING_SENSOR_PIN;         // Pin connected to the homing sensor
extern const int RESET_BUTTON_PIN;          // Pin connected to the reset button
extern bool emergencyStop;                  // Flag to indicate emergency stop condition
extern char mqttTrackNumber[3];             // Character array to store the track number received via MQTT
extern bool resetButtonState;               // State of the reset button in the previous iteration

// Calibration Related
extern const bool calibrationMode;               // Flag to indicate calibration mode
extern const char CONFIRM_YES;                   // Character to confirm an action
extern const char CONFIRM_NO;                    // Character to cancel an action
extern const int STEP_MOVE_SINGLE_KEYPRESS;      // Number of steps to move the turntable for a single keypress
extern const int STEP_MOVE_HELD_KEYPRESS;        // Number of steps to move the turntable for a held keypress

// Position and Track Numbers
extern int currentPosition;                      // Current position of the turntable
extern const int NUMBER_OF_TRACKS;               // Number of tracks on the turntable
extern int *TRACK_NUMBERS;                       // Pointer to the array of track numbers
extern int trackHeads[23];                       // Array to store the head positions of each track
extern int trackTails[23];                       // Array to store the tail positions of each track

/* Function prototypes */
int calculateTargetPosition(int trackNumber, int endNumber);      // Calculates the target position based on track and end numbers
void controlRelays(int trackNumber);                              // Controls the relays for the specified track number
void moveToTargetPosition(int targetPosition);                    // Moves the turntable to the target position

#endif
