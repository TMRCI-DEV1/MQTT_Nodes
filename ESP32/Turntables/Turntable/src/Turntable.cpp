#include "Turntable.h"

/* Definitions of variables declared in Turntable.h */

// Keypad Related
char keys[ROW_NUM][COLUMN_NUM] = {
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
}; // 2D array to store the key characters for the keypad.
byte KEYPAD_ROW_PINS[] = {
  13,
  12,
  14,
  27
}; // Array of GPIO pins connected to the keypad rows.
byte KEYPAD_COLUMN_PINS[] = {
  16,
  17,
  18
}; // Array of GPIO pins connected to the keypad columns.
Keypad keypad = Keypad(makeKeymap(keys), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, ROW_NUM, COLUMN_NUM); // Keypad object for interfacing with the keypad.
char keypadTrackNumber[3] = ""; // Character array to store the track number entered by the user via the keypad.

// Stepper Motor Related
const int STEPS_PER_REV = 6400;                     // Number of steps per revolution for the stepper motor.
const int STEPPER_SPEED = 800;                      // Maximum speed of the stepper motor in steps per second.
AccelStepper stepper(AccelStepper::DRIVER, 33, 32); // AccelStepper object for controlling the stepper motor.

// Relay Board Related
const int RELAY_BOARD1_ADDRESS = 0x20;              // I2C address of the first relay board.
const int RELAY_BOARD2_ADDRESS = 0x21;              // I2C address of the second relay board.
PCF8575 relayBoard1(RELAY_BOARD1_ADDRESS);          // PCF8575 object for controlling the first relay board.
PCF8574 relayBoard2(RELAY_BOARD2_ADDRESS);          // PCF8574 object for controlling the second relay board.

// LCD Related
const int LCD_ADDRESS = 0x27;                               // I2C address of the LCD display.
const int LCD_COLUMNS = 20;                                 // Number of columns in the LCD display.
const int LCD_ROWS = 4;                                     // Number of rows in the LCD display.
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);  // LiquidCrystal_I2C object for controlling the LCD display.

// Miscellaneous
const int HOMING_SENSOR_PIN = 25;                           // GPIO pin connected to the homing sensor.
const int RESET_BUTTON_PIN = 19;                            // GPIO pin connected to the reset button.
bool emergencyStop = false;                                 // Flag to indicate whether an emergency stop has been triggered.
char mqttTrackNumber[3] = "";                               // Character array to store the track number received via MQTT.
bool resetButtonState = HIGH;                               // State of the reset button in the previous iteration.

// Calibration Related
#ifdef CALIBRATION_MODE
const bool calibrationMode = true;                    // Flag to indicate whether calibration mode is enabled.
#else
const bool calibrationMode = false;                   // Flag to indicate that calibration mode is not enabled.
#endif
const char CONFIRM_YES = '1';                         // Character to confirm an action.
const char CONFIRM_NO = '3';                          // Character to cancel an action.
const int STEP_MOVE_SINGLE_KEYPRESS = 1;              // Number of steps to move the turntable for a single keypress during calibration.
const int STEP_MOVE_HELD_KEYPRESS = 100;              // Number of steps to move the turntable for a held keypress during calibration.

// Position and Track Numbers
int currentPosition = 0;                              // Current position of the turntable in steps.
extern
const int NUMBER_OF_TRACKS;                           // Total number of tracks on the turntable.
extern int * TRACK_NUMBERS;                           // Pointer to the array of track numbers.
int trackHeads[23] = {
  0
}; // Array to store the head positions of each track in steps.
int trackTails[23] = {
  0
}; // Array to store the tail positions of each track in steps.

/* Definitions of functions declared in Turntable.h */

/* 
  Function to calculate the target position based on the track number and the end (head or tail) specified.
  This function is used to calculate the target position separately for better code organization and readability.
  An array is used to store the track heads and tails for efficiency, as it allows for quick access to the head and tail positions of each track.
*/
int calculateTargetPosition(int trackNumber, int endNumber) {
  int targetPosition;
  if (calibrationMode) {
    targetPosition = trackNumber; // In calibration mode, the target position is the track number itself.
  } else {
    targetPosition = (endNumber == 0) ? trackHeads[trackNumber - 1] : trackTails[trackNumber - 1]; // Retrieve the corresponding head or tail position.
  }
  return targetPosition;
}

/* 
  Function to control track power relays.
  This function is used to control the relays separately for better code organization and readability.
  A for loop is used to turn off all the relays because it allows for iterating through all the relays without having to write separate code for each one. 
*/
void controlRelays(int trackNumber) {
  // Check if the relay for the selected track is already on
  if ((trackNumber >= 1 && trackNumber <= 15 && relayBoard1.digitalRead(trackNumber) == LOW) ||
    (trackNumber >= 16 && trackNumber <= NUMBER_OF_TRACKS && relayBoard2.digitalRead(trackNumber - 16) == LOW)) {
    return; // If the relay for the selected track is already on, no need to change the state of any relay
  }

  // Turn off all relays
  for (uint8_t i = 0; i < 16; i++) {
    relayBoard1.digitalWrite(i, HIGH);
  }

  for (uint8_t i = 0; i < 8; i++) {
    relayBoard2.digitalWrite(i, HIGH);
  }

  // Turn on the relay corresponding to the selected track
  if (trackNumber >= 1 && trackNumber <= 15) {
    relayBoard1.digitalWrite(trackNumber, LOW);
  } else if (trackNumber >= 16 && trackNumber <= NUMBER_OF_TRACKS) {
    relayBoard2.digitalWrite(trackNumber - 16, LOW);
  }
}

/* 
  Function to move the turntable to the target position.
This function is used to move to the target position separately for better code organization and readability.
A while loop is used to wait for the stepper to finish moving to ensure that the turntable has reached the target position before proceeding.
*/

void moveToTargetPosition(int targetPosition) {
  // Print the target position and current position
  Serial.print("Moving to target position: ");
  Serial.print(targetPosition);
  Serial.print(", Current position: ");
  Serial.println(currentPosition);

  // Turn off the turntable bridge track power before starting the move
  relayBoard1.digitalWrite(0, HIGH);

  if (targetPosition < currentPosition) {
    stepper.moveTo(targetPosition);
  } else if (targetPosition > currentPosition) {
    stepper.moveTo(targetPosition);
  }

  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  // Update current position after moving
  currentPosition = targetPosition;

  // Print the updated current position
  Serial.print("Move complete. Current position: ");
  Serial.println(currentPosition);

  // Turn on the track power for the target position after the move is complete
  controlRelays(targetPosition);

  // Turn the turntable bridge track power back ON (LOW) once the turntable has completed its move
  relayBoard1.digitalWrite(0, LOW);

  // if (!calibrationMode) {
  //   writeToEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition); // Save current position to EEPROM
  // }
}