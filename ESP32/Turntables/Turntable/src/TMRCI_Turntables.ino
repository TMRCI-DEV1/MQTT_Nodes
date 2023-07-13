// Define the version number.
const char * VERSION_NUMBER = "1.1.44";

/* Aisle-Node: Turntable Control
   Project: ESP32-based WiFi/MQTT Turntable Node
   Author: Thomas Seitz (thomas.seitz@tmrci.org)
   Date: 2023-07-12
   Description:
   This sketch is designed for an OTA-enabled ESP32 Node controlling a Turntable. It utilizes various components, including a DIYables 3x4 membrane matrix keypad,
   a GeeekPi IIC I2C TWI Serial LCD 2004 20x4 Display Module with I2C Interface, KRIDA Electronics Relay Modules, a STEPPERONLINE Stepper Drive, a TT Electronics Photologic 
   Slotted Optical Switch, a reset button, and a STEPPERONLINE stepper motor (Nema 17 Bipolar 0.9deg 46Ncm (65.1oz.in) 2A 42x42x48mm 4 Wires). 
  
   The ESP32 Node connects to a WiFi network, subscribes to MQTT messages published by JMRI, and enables control of the turntable by entering a 1 or 2-digit track 
   number on the keypad, followed by '*' or '#' to select the head-end or tail-end, respectively. The expected MQTT message format is 'Tracknx', where 'n' represents 
   the 2-digit track number (01-23, depending on location) and 'x' represents 'H' for the head-end or 'T' for the tail-end. The LCD displays the IP address, the
   commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname, "LOCATION_Turntable_Node".

   The turntable is used to rotate locomotives or cars from one track to another, and the ESP32 provides a convenient way to control it remotely via WiFi and MQTT. */

// ********************************************************************************************************************************************************
// Uncomment the line below to enable calibration mode. In this mode, you can manually program the turntable track positions without using MQTT commands.
// #define CALIBRATION_MODE
//
// Depending on the location, define the ESP32 hostname, MQTT topics, number of tracks, and track numbers.
// Uncomment one of the lines below to specify the location.
#define GILBERTON
// #define HOBOKEN
// #define PITTSBURGH
// ********************************************************************************************************************************************************

// Include the appropriate configuration file based on the defined location.
#ifdef GILBERTON
#include "GilbertonConfig.h"

#elif defined(HOBOKEN)
#include "HobokenConfig.h"

#elif defined(PITTSBURGH)
#include "PittsburghConfig.h"

#endif

/* This include statement adds the Turntable header file to the sketch.
   The Turntable file contains definitions and declarations related to the operation and control of the turntable.
   This includes functions for calculating target positions, moving the turntable to a target position, controlling the relays for track power,
   and performing the homing sequence for turntable calibration. It also contains declarations for various hardware components such as the stepper motor,
   relay boards, and LCD display, as well as variables for storing the current and target positions of the turntable. */
#include "Turntable.h"

/* This include statement adds the EEPROMConfig header file to the sketch. 
   The EEPROMConfig file contains definitions and declarations related to the EEPROM (Electrically Erasable Programmable Read-Only Memory) 
   configuration, including functions for reading from and writing to the EEPROM. 
   The EEPROM is used in this sketch to store the positions of the turntable tracks. */
#include "EEPROMConfig.h"

/* This include statement adds the WiFiMQTT header file to the sketch. 
   The WiFiMQTT file contains definitions and declarations related to the WiFi and MQTT (Message Queuing Telemetry Transport) configuration, 
   including functions for connecting to the WiFi network and the MQTT broker, and for handling MQTT messages. 
   MQTT is a lightweight messaging protocol that is used in this sketch for remote control of the turntable via WiFi. */
#include "WiFiMQTT.h"

// Helper function to print messages to the LCD display. If the LCD is not available, this function does nothing.
void printToLCD(int row,
  const char * message) {
  // Check if the LCD is available
  if (!isLCDAvailable) {
    return; // If the LCD is not available, exit the function
  }

  // Clear the row before printing a new message
  lcd.setCursor(0, row);
  for (int i = 0; i < LCD_COLUMNS; i++) {
    lcd.print(" ");
  }

  // Check if the message fits on the specified row
  int messageLength = strlen(message);
  if (messageLength > LCD_COLUMNS) {
    // If the message is longer than the number of columns on the LCD, split it across multiple rows
    int start = 0;
    while (start < messageLength) {
      char buffer[LCD_COLUMNS + 1];
      strncpy(buffer, & message[start], LCD_COLUMNS);
      buffer[LCD_COLUMNS] = '\0'; // Null-terminate the buffer

      // Find the last space in the buffer
      for (int i = LCD_COLUMNS - 1; i >= 0; i--) {
        if (buffer[i] == ' ') {
          buffer[i] = '\0'; // Split the string at the last space
          start += i + 1; // Update the start index for the next substring
          break;
        }
      }
    }
  }
}

// Helper function to clear the LCD display. If the LCD is not available, this function does nothing.
void clearLCD() {
  lcd.clear();
}

// Function to initialize the LCD display. If the LCD is not available, this function does nothing.
void initializeLCD() {
  lcd.begin(LCD_COLUMNS, LCD_ROWS);

  // Print the version number on the LCD
  clearLCD();
  printToLCD(0, "Version:");
  printToLCD(1, VERSION_NUMBER);

  #ifdef CALIBRATION_MODE
  clearLCD();
  printToLCD(0, "Calibration Mode");
  #endif
}

// Function to initialize the relay boards. This function initializes two relay boards.
void initializeRelayBoards() {
  relayBoard1.begin(); // Initialize the first relay board.
  relayBoard2.begin(); // Initialize the second relay board.
}

// Function to initialize the stepper motor. This function sets the maximum speed and acceleration for the stepper motor.
void initializeStepper() {
  stepper.setMaxSpeed(STEPPER_SPEED); // Set the maximum speed for the stepper motor.
  stepper.setAcceleration(STEPPER_SPEED); // Set the acceleration for the stepper motor.
}

// Function to read data from EEPROM. This function reads track positions from EEPROM if not in calibration mode.
void readDataFromEEPROM() {
  if (!calibrationMode) {
    // Read track positions from EEPROM
    bool trackHeadsReadSuccess = readFromEEPROMWithVerification(EEPROM_TRACK_HEADS_ADDRESS, trackHeads); // Read the track heads from EEPROM with error checking.
    bool trackTailsReadSuccess = readFromEEPROMWithVerification(getEEPROMTrackTailsAddress(), trackTails); // Read the track tails from EEPROM with error checking.

    // Check if the read operations were successful
    if (!trackHeadsReadSuccess) {
      Serial.println("Error: Failed to read track heads from EEPROM!");
      printToLCD(0, "EEPROM Error!");
      printToLCD(1, "Track heads read fail");
      // Set default values for track heads
      for (int i = 0; i < NUMBER_OF_TRACKS; i++) {
        trackHeads[i] = 0; // Set default value to 0.
      }
    }
    if (!trackTailsReadSuccess) {
      Serial.println("Error: Failed to read track tails from EEPROM!");
      printToLCD(0, "EEPROM Error!");
      printToLCD(1, "Track tails read fail");
      // Set default values for track tails
      for (int i = 0; i < NUMBER_OF_TRACKS; i++) {
        trackTails[i] = 0; // Set default value to 0.
      }
    }
  }
}

// Function to initialize the keypad and LCD. This function adds an event listener to the keypad that handles key presses.
void initializeKeypadAndLCD() {
  keypad.addEventListener([](char key) {
    int trackNumber = atoi(keypadTrackNumber); // Get the track number entered on the keypad.

    if (trackNumber > 0 && trackNumber <= NUMBER_OF_TRACKS) { // Check if the track number is valid.
      char endChar = keypadTrackNumber[2]; // Get the end character entered on the keypad.
      int endNumber = (endChar == 'H') ? 1 : ((endChar == 'T') ? 2 : 0); // Map the end character to a number (1 for head, 2 for tail).

      if (endNumber != 0) { // Check if the end number is valid
        int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate the target position based on the track number and end number.
        moveToTargetPosition(targetPosition); // Move the turntable to the target position.
        controlRelays(trackNumber); // Control the track power relays.

        // Update the LCD display with the selected track information
        clearLCD();
        printToLCD(0, "Track selected:");
        printToLCD(1, String(trackNumber).c_str());
        printToLCD(2, "Position:");
        printToLCD(3, String(targetPosition).c_str());
      }
    }

    keypadTrackNumber[0] = '\0'; // Reset track number input.
  });
}

// Function to enable OTA updates for the ESP32. This function begins the OTA service.
void enableOTAUpdates() {
  ArduinoOTA.begin();
}

// Function to perform the homing sequence. This function moves the turntable to the home position and sets the current position to zero.
void performHomingSequence() {
  while (digitalRead(HOMING_SENSOR_PIN) == HIGH) {
    stepper.move(-10);
    stepper.run();
  }
  currentPosition = 0; // Set current position to zero after homing.
  printToLCD(0, "HOMING SEQUENCE TRIGGERED");
  unsigned long homingSequenceStartTime = millis();
  while (millis() - homingSequenceStartTime < DELAY_TIME) {
    // Do nothing, just wait
  }
  clearLCD();
}

// Function to initialize peripherals. This function initializes serial communication, the I2C bus, and EEPROM.
void initializePeripherals() {
  Serial.begin(BAUD_RATE); // Initialize serial communication.
  Wire.begin(); // Initialize the I2C bus.
  EEPROM.begin(EEPROM_TOTAL_SIZE_BYTES); // Initialize EEPROM.
}

// Function to connect to the WiFi network and MQTT broker. This function connects to the WiFi network and the MQTT broker, and subscribes to the MQTT topic.
void connectToNetwork() {
  connectToWiFi(); // Connect to the WiFi network.
  connectToMQTT(); // Connect to the MQTT broker and subscribe to the topic.
}

// Function to initialize various components. This function initializes the LCD display, relay boards, stepper motor, keypad, and LCD, enables OTA updates for the ESP32, and performs the homing sequence.
void initializeComponents() {
  initializeLCD(); // Initialize the LCD display.
  initializeRelayBoards(); // Initialize the relay boards.
  initializeStepper(); // Initialize the stepper motor.
  initializeKeypadAndLCD(); // Initialize the keypad and LCD.
  enableOTAUpdates(); // Enable OTA updates for the ESP32.
  performHomingSequence(); // Perform the homing sequence to calibrate the turntable.
  
  #ifndef CALIBRATION_MODE
  state = WAITING_FOR_INITIAL_KEY; // Initialize the state machine only in operation mode.
  #endif
  readDataFromEEPROM();
}

// Function to read data from EEPROM
void readEEPROMData() {
  if (!calibrationMode) {
    readDataFromEEPROM(); // Read track positions from EEPROM.
  }
}

// ESP32 setup function to initialize the system. This function initializes peripherals, connects to the network, initializes components, and reads EEPROM data.
void setup() {
  initializePeripherals();
  connectToNetwork();
  initializeComponents();
  readEEPROMData();
}

// Function to handle the emergency stop functionality. This function stops the stepper motor and displays a message on the LCD if the emergency stop flag is set.
void handleEmergencyStop() {
  if (emergencyStop) {
    stepper.stop();
    printToLCD(0, "EMERGENCY STOP");
    unsigned long emergencyStopStartTime = millis();
    while (millis() - emergencyStopStartTime < DELAY_TIME) {
      // Do nothing, just wait
    }
    clearLCD();
    emergencyStop = false;
  }
}

// Function to handle keypad inputs. This function handles key presses and key holds, and performs actions based on the keys pressed.
void handleKeypadInput() {
  char key = keypad.getKey();
  static bool isKeyHeld = false; // Track if a key is held down.
  static unsigned long keyHoldTime = 0; // Track the duration of key hold.

  if (key) {
    if (key == '9') {
      emergencyStopCounter++;
      if (emergencyStopCounter >= 3) {
        // Activate emergency stop if the '9' key is pressed three times consecutively
        emergencyStop = true;
        emergencyStopCounter = 0; // Reset the counter.
      }
    } else {
      emergencyStopCounter = 0; // Reset the counter if any other key is pressed.
    }

    if (calibrationMode) {
      if (key == '4' || key == '6') {
        int direction = (key == '4') ? -1 : 1; // Determine direction based on the key.
        if (!isKeyHeld) {
          // Move the turntable by a fixed number of steps in the direction specified by the key
          stepper.move(direction * STEP_MOVE_SINGLE_KEYPRESS);
          isKeyHeld = true;
          keyHoldTime = millis(); // Start tracking the hold time.
        } else if (millis() - keyHoldTime >= KEY_HOLD_DELAY) {
          // Move the turntable continuously by a fixed number of steps in the direction specified by the key
          stepper.move(direction * STEP_MOVE_HELD_KEYPRESS);
        }
      } else if (key == '*' || key == '#') {
        if (!waitingForConfirmation) {
          tempTrackNumber = atoi(keypadTrackNumber);
          tempEndChar = key;
          printToLCD(0, "Press 1 to confirm, 3 to cancel");
          waitingForConfirmation = true;
        } else if (key == CONFIRM_YES) {
          // Store the current position to the appropriate track head or tail-end position in EEPROM
          if (tempEndChar == '*') {
            trackHeads[tempTrackNumber - 1] = currentPosition;
            EEPROM.put(EEPROM_TRACK_HEADS_ADDRESS + (tempTrackNumber - 1) * sizeof(int), currentPosition);
          } else {
            trackTails[tempTrackNumber - 1] = currentPosition;
            EEPROM.put(getEEPROMTrackTailsAddress() + (tempTrackNumber - 1) * sizeof(int), currentPosition);
          }
          EEPROM.commit();
          printToLCD(0, "Position stored for track ");
          printToLCD(1, String(tempTrackNumber).c_str());
          printToLCD(2, (tempEndChar == '*') ? "Head-end" : "Tail-end");
          unsigned long positionStoredStartTime = millis();
          while (millis() - positionStoredStartTime < DELAY_TIME) {
            // Do nothing, just wait
          }
          clearLCD();
          waitingForConfirmation = false;
          keypadTrackNumber[0] = '\0'; // Reset keypadTrackNumber after storing position or moving.
        } else if (key == CONFIRM_NO) {
          printToLCD(0, "Position storing cancelled");
          unsigned long cancelStartTime = millis();
          while (millis() - cancelStartTime < DELAY_TIME) {
            // Do nothing, just wait
          }
          clearLCD();
          waitingForConfirmation = false;
          keypadTrackNumber[0] = '\0'; // Reset keypadTrackNumber after cancelling.
        }
      }
    } else {
      // Handle the key press based on the current state
      switch (state) {
        case WAITING_FOR_INITIAL_KEY:
          if (key == '*' || key == '#') {
            // Store the initial key press and move to the next state
            keypadTrackNumber[0] = key;
            state = WAITING_FOR_TRACK_NUMBER;
          }
          break;

        case WAITING_FOR_TRACK_NUMBER:
          if (isdigit(key)) {
            // Store the track number and move to the next state
            keypadTrackNumber[1] = key;
            keypadTrackNumber[2] = '\0'; // Null-terminate the char array
            state = WAITING_FOR_CONFIRMATION;
          } else if (key == '*' || key == '#') {
            // If another '*' or '#' key is pressed, reset the state
            keypadTrackNumber[0] = key;
            state = WAITING_FOR_TRACK_NUMBER;
          }
          break;

        case WAITING_FOR_CONFIRMATION:
          if (key == '*' || key == '#') {
            if (key == keypadTrackNumber[0]) {
              // If the confirmation key matches the initial key, handle the track selection
              int trackNumber = keypadTrackNumber[1] - '0'; // Convert the char to an int
              if (trackNumber >= 1 && trackNumber <= NUMBER_OF_TRACKS) {
                int endNumber = (key == '*') ? 0 : 1;
                int targetPosition = calculateTargetPosition(trackNumber, endNumber);
                moveToTargetPosition(targetPosition);
              } else {
                static unsigned long invalidTrackStartTime = 0;
                if (invalidTrackStartTime == 0) { // If the timer has not started yet.
                  printToLCD(0, "Invalid track number!");
                  invalidTrackStartTime = millis(); // Start the timer
                } else if (millis() - invalidTrackStartTime >= DELAY_TIME) { // If 2 seconds have passed.
                  clearLCD();
                  invalidTrackStartTime = 0; // Reset the timer
                }
              }
              // Reset the state
              state = WAITING_FOR_INITIAL_KEY;
            } else {
              // If the confirmation key does not match the initial key, reset the state
              keypadTrackNumber[0] = key;
              state = WAITING_FOR_TRACK_NUMBER;
            }
          }
          break;
      }
    }
  } else {
    isKeyHeld = false; // Reset isKeyHeld when no key is pressed.
    keyHoldTime = 0; // Reset keyHoldTime when no key is pressed.
  }
}

// Function to handle WiFi and MQTT connections and message handling. This function reconnects to the WiFi network and MQTT broker if disconnected, and handles MQTT messages and OTA updates.
void handleWiFiAndMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (!client.connected()) {
    connectToMQTT();
  }

  client.loop();
  ArduinoOTA.handle();
}

// Function to handle the reset button functionality. This function performs the homing sequence if the reset button is pressed.
void handleResetButton() {
  bool currentResetButtonState = digitalRead(RESET_BUTTON_PIN);

  if (currentResetButtonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentResetButtonState != resetButtonState) {
      resetButtonState = currentResetButtonState;

      if (resetButtonState == HIGH) {
        printCurrentPositionRelativeToHome(); // Print the current position relative to home
        performHomingSequence();
      }
    }
  }

  lastButtonState = currentResetButtonState;
}

// Function to handle stepper movement. This function moves the stepper motor if there is a distance to go.
void handleStepperMovement() {
  if (stepper.distanceToGo() != 0) {
    stepper.run();
  }
}

// ESP32 loop function to handle various tasks. This function handles emergency stop, keypad inputs, WiFi and MQTT connections, reset button, and stepper movement.
void loop() {
  handleEmergencyStop();
  handleKeypadInput();
  handleWiFiAndMQTT();
  handleResetButton();
  handleStepperMovement();
}