#ifndef TURNTABLE_H
#define TURNTABLE_H

/* Libraries */
#include <Wire.h>              // Include the Wire library to enable I2C communication for the ESP32. This is used for communication with the relay boards and LCD screen.                
                               // https://github.com/esp8266/Arduino/tree/master/libraries/Wire

#include <Keypad.h>            // Include the Keypad library to enable interfacing with the 3x4 keypad. This is used for reading user input from the keypad.                              
                               // https://github.com/Chris--A/Keypad

#include <LiquidCrystal_I2C.h> // Include the LiquidCrystal_I2C library to enable control of the LCD screen over I2C. This is used for displaying information to the user.                
                               // https://github.com/johnrickman/LiquidCrystal_I2C

#include <PCF8574.h>           // Include the PCF8574 library to enable interfacing with the PCF8574 8-channel I2C relay board. This is used for controlling the 8-channel relay board.   
                               // https://github.com/xreef/PCF8574_library

#include <PCF8575.h>           // Include the PCF8575 library to enable interfacing with the PCF8575 16-channel I2C relay board. This is used for controlling the 16-channel relay board. 
                               // https://github.com/xreef/PCF8575_library/tree/master

#include <AccelStepper.h>      // Include the AccelStepper library to enable control of stepper motors. This is used for controlling the turntable's stepper motor.                       
                               // https://github.com/waspinator/AccelStepper

/* Constants */
// Keypad Related
const byte ROW_NUM = 4;                     // Four rows in the keypad matrix.
const byte COLUMN_NUM = 3;                  // Three columns in the keypad matrix.
extern char keys[ROW_NUM][COLUMN_NUM];      // 2D array to store the key characters.
extern byte KEYPAD_ROW_PINS[];              // Array of GPIO pins connected to the keypad rows.
extern byte KEYPAD_COLUMN_PINS[];           // Array of GPIO pins connected to the keypad columns.
extern Keypad keypad;                       // Keypad object for interfacing with the keypad.
extern char keypadTrackNumber[3];           // Character array to store the track number entered by the user via the keypad.
extern const unsigned long KEY_HOLD_DELAY;  // Delay before continuous movement starts (in milliseconds).

// Define the states
enum KeypadState {
  WAITING_FOR_INITIAL_KEY,
  WAITING_FOR_TRACK_NUMBER,
  WAITING_FOR_CONFIRMATION
};

extern KeypadState state;                   // Declare the state variable as extern

// Stepper Motor Related
extern const int STEPS_PER_REV;             // Number of steps per revolution for the stepper motor.
extern const int STEPPER_SPEED;             // Maximum speed of the stepper motor in steps per second.
extern AccelStepper stepper;                // AccelStepper object for controlling the stepper motor.

// Relay Board Related
extern const int RELAY_BOARD1_ADDRESS;      // I2C address of the first relay board.
extern const int RELAY_BOARD2_ADDRESS;      // I2C address of the second relay board.
extern PCF8575 relayBoard1;                 // PCF8575 object for controlling the first relay board.
extern PCF8574 relayBoard2;                 // PCF8574 object for controlling the second relay board.

// LCD Related
extern const int LCD_ADDRESS;               // I2C address of the LCD display.
extern const int LCD_COLUMNS;               // Number of columns in the LCD display.
extern const int LCD_ROWS;                  // Number of rows in the LCD display.
extern LiquidCrystal_I2C lcd;               // LiquidCrystal_I2C object for controlling the LCD display.
extern bool isLCDAvailable;                 // Set this to true if LCD is connected, false otherwise

// Miscellaneous
#define DEBOUNCE_DELAY 50                   // Defines the debounce delay in milliseconds for button inputs.
extern const int HOMING_SENSOR_PIN;         // Specifies the GPIO pin connected to the homing sensor.
extern const int RESET_BUTTON_PIN;          // Specifies the GPIO pin connected to the reset button.
extern int emergencyStopCounter;            // Counter for the '9' key to trigger emergency stop.
extern bool emergencyStop;                  // Flag to indicate if an emergency stop has been triggered.
extern char mqttTrackNumber[3];             // Array to store the track number received via MQTT.
extern bool resetButtonState;               // Holds the state of the reset button in the previous iteration.
extern unsigned long lastDebounceTime;      // Records the last time the output pin was toggled for debounce.
extern bool lastButtonState;                // Holds the previous reading from the input pin.
extern const unsigned long BAUD_RATE;       // Defines the baud rate for serial communication.
extern const unsigned long DELAY_TIME;      // Defines the delay time in milliseconds used for pauses in execution.

// Calibration Related
extern const bool calibrationMode;          // Flag to indicate whether calibration mode is enabled.
extern const char CONFIRM_YES;              // Character to confirm an action. Typically set to '1'.
extern const char CONFIRM_NO;               // Character to cancel an action. Typically set to '3'.
extern const int STEP_MOVE_SINGLE_KEYPRESS; // Number of steps to move the turntable for a single keypress during calibration.
extern const int STEP_MOVE_HELD_KEYPRESS;   // Number of steps to move the turntable for a held keypress during calibration.
extern bool waitingForConfirmation;         // Flag to indicate whether the system is waiting for a confirmation input during calibration.
extern int tempTrackNumber;                 // Temporary storage for the track number during calibration.
extern char tempEndChar;                    // Temporary storage for the end character ('*' or '#') during calibration.

// Position and Track Numbers
extern int currentPosition;                 // Current position of the turntable in steps.
extern const int NUMBER_OF_TRACKS;          // Total number of tracks on the turntable.
extern int *TRACK_NUMBERS;                  // Pointer to the array of track numbers.
extern int trackHeads[23];                  // Array to store the head positions of each track in steps.
extern int trackTails[23];                  // Array to store the tail positions of each track in steps.

/* Function prototypes */
int calculateTargetPosition(int trackNumber, int endNumber);   // Calculates the target position based on the track number and end number.
void controlRelays(int trackNumber);                           // Controls the relays to switch the track power to the specified track number.
void moveToTargetPosition(int targetPosition);                 // Moves the turntable to the target position using the stepper motor.
void printCurrentPositionRelativeToHome();                     // Prints the current position of the turntable relative to the "home" position. 
                                                               // This function is used for debugging purposes to check if the stepper motor is drifting 
                                                               // or being manually moved without the sketch being aware of it.

#endif