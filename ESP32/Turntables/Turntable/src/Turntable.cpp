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
const unsigned long KEY_HOLD_DELAY = 5; // Delay before continuous movement starts (in milliseconds).
KeypadState state = WAITING_FOR_INITIAL_KEY; // Initialize the state variable

// Stepper Motor Related
const int STEPS_PER_REV = 6400; // Number of steps per revolution for the stepper motor.
const int STEPPER_SPEED = 200;  // Maximum speed of the stepper motor in steps per second.
AccelStepper stepper(AccelStepper::DRIVER, 33, 32); // AccelStepper object for controlling the stepper motor.

// Relay Board Related
const int RELAY_BOARD1_ADDRESS = 0x20; // I2C address of the first relay board.
const int RELAY_BOARD2_ADDRESS = 0x21; // I2C address of the second relay board.
PCF8575 relayBoard1(RELAY_BOARD1_ADDRESS); // PCF8575 object for controlling the first relay board.
PCF8574 relayBoard2(RELAY_BOARD2_ADDRESS); // PCF8574 object for controlling the second relay board.

// LCD Related
const int LCD_ADDRESS = 0x3F; // I2C address of the LCD display.
const int LCD_COLUMNS = 20; // Number of columns in the LCD display.
const int LCD_ROWS = 4; // Number of rows in the LCD display.
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS); // LiquidCrystal_I2C object for controlling the LCD display.
bool isLCDAvailable = false; // Set this to true if LCD is connected, false otherwise

// Miscellaneous
const int HOMING_SENSOR_PIN = 25;           // Specifies the GPIO pin connected to the homing sensor.
const int RESET_BUTTON_PIN = 19;            // Specifies the GPIO pin connected to the reset button.
int emergencyStopCounter = 0;               // Counter for the '9' key to trigger emergency stop.
bool emergencyStop = false;                 // Flag to indicate if an emergency stop has been triggered.
char mqttTrackNumber[3] = "";               // Array to store the track number received via MQTT.
bool resetButtonState = HIGH;               // Holds the state of the reset button in the previous iteration.
unsigned long lastDebounceTime = 0;         // Records the last time the output pin was toggled for debounce.
bool lastButtonState = LOW;                 // Holds the previous reading from the input pin.
const unsigned long BAUD_RATE = 115200;     // Defines the baud rate for serial communication.
const unsigned long DELAY_TIME = 2000;      // Defines the delay time in milliseconds used for pauses in execution.

// Calibration Related
#ifdef CALIBRATION_MODE
const bool calibrationMode = true; // Flag to indicate whether calibration mode is enabled. This is set to true when CALIBRATION_MODE is defined.
#else
const bool calibrationMode = false; // Flag to indicate that calibration mode is not enabled. This is set to false when CALIBRATION_MODE is not defined.
#endif
const char CONFIRM_YES = '1'; // Character to confirm an action. This is set to '1'.
const char CONFIRM_NO = '3'; // Character to cancel an action. This is set to '3'.
const int STEP_MOVE_SINGLE_KEYPRESS = 1; // Number of steps to move the turntable for a single keypress during calibration. This is set to 1.
const int STEP_MOVE_HELD_KEYPRESS = 1; // Number of steps to move the turntable for a held keypress during calibration. This is set to 100.
bool waitingForConfirmation = false; // Flag to indicate whether the system is waiting for a confirmation input during calibration. This is initially set to false.
int tempTrackNumber = 0; // Temporary storage for the track number during calibration. This is initially set to 0.
char tempEndChar = '\0'; // Temporary storage for the end character ('*' or '#') during calibration. This is initially set to '\0' (null character).

// Position and Track Numbers
int currentPosition = 0; // Current position of the turntable in steps.
extern const int NUMBER_OF_TRACKS; // Total number of tracks on the turntable.
extern int * TRACK_NUMBERS; // Pointer to the array of track numbers.
int trackHeads[23] = {
  0
}; // Array to store the head positions of each track in steps.
int trackTails[23] = {
  0
}; // Array to store the tail positions of each track in steps.

/* Definitions of functions declared in Turntable.h */

/* Function to calculate the target position based on the track number and the end (head or tail) specified.
   This function is used to calculate the target position separately for better code organization and readability.
   An array is used to store the track heads and tails for efficiency, as it allows for quick access to the head and tail positions of each track. */
int calculateTargetPosition(int trackNumber, int endNumber) {
  int targetPosition;
  if (calibrationMode) {
    targetPosition = trackNumber; // In calibration mode, the target position is the track number itself.
  } else {
    targetPosition = (endNumber == 0) ? trackHeads[trackNumber - 1] : trackTails[trackNumber - 1]; // Retrieve the corresponding head or tail position.
  }
  return targetPosition;
}

/* Function to control track power relays.
   This function is used to control the relays separately for better code organization and readability.
   A for loop is used to turn off all the relays because it allows for iterating through all the relays without having to write separate code for each one. */
void controlRelays(int trackNumber) {
  // Check if the relay for the selected track is already on
  if ((trackNumber >= 1 && trackNumber <= 15 && relayBoard1.digitalRead(trackNumber) == LOW) ||
    (trackNumber >= 16 && trackNumber <= NUMBER_OF_TRACKS && relayBoard2.digitalRead(trackNumber - 16) == LOW)) {
    return; // If the relay for the selected track is already on, no need to change the state of any relay.
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

/* Function to move the turntable to the target position.
   This function calculates the shortest path to the target position, either moving forward or backward,
   and moves the turntable to the target position using the stepper motor.
   The function waits for the stepper to finish moving before proceeding to ensure that the turntable has reached the target position.
   It also checks for any discrepancy between the expected and actual position after the move, and prints a warning message if a discrepancy is detected. */
void moveToTargetPosition(int targetPosition) {
  // Store the expected position for comparison after moving
  int expectedPosition = targetPosition;

  // Print the target position and current position
  Serial.print("Moving to target position: ");
  Serial.print(targetPosition);
  Serial.print(", Current position: ");
  Serial.println(currentPosition);

  // Turn off the turntable bridge track power before starting the move
  relayBoard1.digitalWrite(0, HIGH);

  // Adjust the current position if it exceeds STEPS_PER_REV or goes below 0
  // This adjustment is necessary because the turntable's position wraps around at STEPS_PER_REV (equivalent to position 0)
  if (currentPosition >= STEPS_PER_REV) {
    currentPosition -= STEPS_PER_REV;
  } else if (currentPosition < 0) {
    currentPosition += STEPS_PER_REV;
  }

  // Calculate the distances for moving forward and backward
  // The forward distance is the difference between the target and current positions if the target is greater than or equal to the current position
  // Otherwise, it's the distance from the current position to STEPS_PER_REV plus the distance from 0 to the target position
  // The backward distance is the total steps (STEPS_PER_REV) minus the forward distance
  int forwardDistance = (targetPosition >= currentPosition) ? (targetPosition - currentPosition) : (STEPS_PER_REV - currentPosition + targetPosition);
  int backwardDistance = STEPS_PER_REV - forwardDistance;

  // Move in the direction with the shortest distance
  // If the forward distance is less than or equal to the backward distance, move forward to the target position
  // Otherwise, move backward by moving to the target position minus STEPS_PER_REV (since moving backward by a certain distance is equivalent to moving forward by the total steps minus that distance)
  if (forwardDistance <= backwardDistance) {
    stepper.moveTo(targetPosition);
  } else {
    stepper.moveTo(targetPosition - STEPS_PER_REV); // Move backward
  }

  // Wait for the stepper to finish moving
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  // Update current position after moving
  currentPosition = targetPosition;
  
  // Compare the expected position to the actual position
  // If there's a discrepancy, print a warning message to the serial monitor
  if (currentPosition != expectedPosition) {
    Serial.print("Warning: Discrepancy detected between expected position (");
    Serial.print(expectedPosition);
    Serial.print(") and actual position (");
    Serial.print(currentPosition);
    Serial.println("). Check calibration and stepper motor.");
  }

  // Print the updated current position
  Serial.print("Move complete. Current position: ");
  Serial.println(currentPosition);

  // Turn on the track power for the target position after the move is complete
  controlRelays(targetPosition);

  // Turn the turntable bridge track power back ON (LOW) once the turntable has completed its move
  relayBoard1.digitalWrite(0, LOW);

  // Uncomment the following line to save the current position to EEPROM when not in calibration mode
  // if (!calibrationMode) {
  //   writeToEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition);
  // }
}

/* Function to print the current position relative to the "home" position.
   This function is used for debugging purposes to check if the stepper motor is drifting or being manually moved without the sketch being aware of it. */
void printCurrentPositionRelativeToHome() {
  // Calculate the current position relative to the "home" position
  int positionRelativeToHome = currentPosition % STEPS_PER_REV;

  // Print the current position relative to the "home" position
  Serial.print("Current position relative to home: ");
  Serial.println(positionRelativeToHome);
}