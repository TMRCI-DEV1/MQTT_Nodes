const char* VERSION_NUMBER = "1.1.33"; // Define the version number

/*
  Aisle-Node: Turntable Control
  Project: ESP32-based WiFi/MQTT Turntable Node
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Date: 2023-07-10
  Description:
  This sketch is designed for an OTA-enabled ESP32 Node controlling a Turntable. It utilizes various components, including a DIYables 3x4 membrane matrix keypad,
  a GeeekPi IIC I2C TWI Serial LCD 2004 20x4 Display Module with I2C Interface, KRIDA Electronics Relay Modules, a STEPPERONLINE Stepper Drive, a TT Electronics Photologic 
  Slotted Optical Switch, a reset button, and a STEPPERONLINE stepper motor (Nema 17 Bipolar 0.9deg 46Ncm (65.1oz.in) 2A 42x42x48mm 4 Wires). 
  
  The ESP32 Node connects to a WiFi network, subscribes to MQTT messages published by JMRI, and enables control of the turntable by entering a 1 or 2-digit track 
  number on the keypad, followed by '*' or '#' to select the head-end or tail-end, respectively. The expected MQTT message format is 'Tracknx', where 'n' represents 
  the 2-digit track number (01-23, depending on location) and 'x' represents 'H' for the head-end or 'T' for the tail-end. The LCD displays the IP address, the
  commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname, "LOCATION_Turntable_Node".

  The turntable is used to rotate locomotives or cars from one track to another, and the ESP32 provides a convenient way to control it remotely via WiFi and MQTT.
*/

// Uncomment this line to enable calibration mode. Calibration mode allows manual positioning of the turntable without using MQTT commands.
// #define CALIBRATION_MODE

/* Depending on the location, define the MQTT topics, number of tracks, and track numbers
   Uncomment one of these lines to indicate the location */

#define GILBERTON
// #define HOBOKEN
// #define PITTSBURGH

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

// Function to initialize LCD
void initializeLCD() {
  lcd.begin(LCD_COLUMNS, LCD_ROWS);       // Initialize the LCD display

  // Print the version number on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Version:");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_NUMBER);

#ifdef CALIBRATION_MODE
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration Mode");
#endif
}

// Function to initialize Relay Boards
void initializeRelayBoards() {
  relayBoard1.begin();                    // Initialize the first relay board
  relayBoard2.begin();                    // Initialize the second relay board
}

// Function to initialize Stepper
void initializeStepper() {
  stepper.setMaxSpeed(STEPPER_SPEED);     // Set the maximum speed for the stepper motor
  stepper.setAcceleration(STEPPER_SPEED); // Set the acceleration for the stepper motor
}

// Function to read data from EEPROM
void readDataFromEEPROM() {
  if (!calibrationMode) {
    // Read data from EEPROM
    bool trackHeadsReadSuccess = readFromEEPROMWithVerification(EEPROM_TRACK_HEADS_ADDRESS, trackHeads);    // Read the track heads from EEPROM with error checking
    bool trackTailsReadSuccess = readFromEEPROMWithVerification(getEEPROMTrackTailsAddress(), trackTails);  // Read the track tails from EEPROM with error checking

    // Check if the read operations were successful
    if (!trackHeadsReadSuccess) {
      Serial.println("Error: Failed to read track heads from EEPROM!");
      lcd.setCursor(0, 0);
      lcd.print("EEPROM Error!");
      lcd.setCursor(0, 1);
      lcd.print("Track heads read fail");
      // Set default values for track heads
      for (int i = 0; i < NUMBER_OF_TRACKS; i++) {
        trackHeads[i] = 0; // Set default value to 0
      }
    }
    if (!trackTailsReadSuccess) {
      Serial.println("Error: Failed to read track tails from EEPROM!");
      lcd.setCursor(0, 2);
      lcd.print("EEPROM Error!");
      lcd.setCursor(0, 3);
      lcd.print("Track tails read fail");
      // Set default values for track tails
      for (int i = 0; i < NUMBER_OF_TRACKS; i++) {
        trackTails[i] = 0; // Set default value to 0
      }
    }
  }
}

// Function to initialize keypad and LCD
void initializeKeypadAndLCD() {
  keypad.addEventListener([](char key) {
    int trackNumber = atoi(keypadTrackNumber); // Get the track number entered on the keypad

    if (trackNumber > 0 && trackNumber <= NUMBER_OF_TRACKS) // Check if the track number is valid
    {
      char endChar = keypadTrackNumber[2]; // Get the end character entered on the keypad
      int endNumber = (endChar == 'H') ? 1 : ((endChar == 'T') ? 2 : 0); // Map the end character to a number (1 for head, 2 for tail)

      if (endNumber != 0) // Check if the end number is valid
      {
        int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate the target position based on the track number and end number
        moveToTargetPosition(targetPosition); // Move the turntable to the target position
        controlRelays(trackNumber); // Control the track power relays

        // writeToEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition);

        // Update the LCD display with the selected track information
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Track selected:");
        lcd.setCursor(0, 1);
        lcd.print(trackNumber);
        lcd.setCursor(0, 2);
        lcd.print("Position:");
        lcd.setCursor(0, 3);
        lcd.print(targetPosition);
      }
    }

    keypadTrackNumber[0] = '\0'; // Reset track number input
  });
}

// Function to enable OTA updates
void enableOTAUpdates() {
  ArduinoOTA.begin();
}

// Function to perform homing sequence
void performHomingSequence() {
  while (digitalRead(HOMING_SENSOR_PIN) == HIGH) {
    stepper.move(-10);
    stepper.run();
  }
  currentPosition = 0; // Set current position to zero after homing.
  lcd.setCursor(0, 0);
  lcd.print("HOMING SEQUENCE TRIGGERED");
  delay(2000);
  lcd.clear();
}

/*
  ESP32 setup function to initialize the system
  This function uses a separate function to connect to WiFi for better code organization and readability.
  A while loop is used to wait for the homing sensor to ensure that the system is properly initialized before proceeding.
*/
void setup() {
  // Initialize libraries and peripherals
  Serial.begin(115200); // Initialize serial communication
  Wire.begin(); // Initialize the I2C bus
  EEPROM.begin(EEPROM_TOTAL_SIZE_BYTES); // Initialize EEPROM
  connectToWiFi(); // Connect to the WiFi network

  // Connect to MQTT broker
  connectToMQTT(); // Connect to the MQTT broker and subscribe to the topic

  // Initialize various components and settings
  // This includes the LCD display, relay boards, stepper motor, hostname, and track head and tail arrays
  initializeLCD();          // Initialize the LCD display
  initializeRelayBoards();  // Initialize the relay boards
  initializeStepper();      // Initialize the stepper motor

  // Only read from EEPROM if not in calibration mode
  #ifndef CALIBRATION_MODE
    readDataFromEEPROM();   // Read track positions from EEPROM
  #endif

  initializeKeypadAndLCD(); // Initialize the keypad and LCD
  enableOTAUpdates(); // Enable OTA updates for the ESP32
  performHomingSequence();  // Perform the homing sequence to calibrate the turntable
}

/* 
  ESP32 loop function to handle MQTT messages and keypad inputs
  This function uses a while loop to wait for the stepper to finish moving to ensure that the turntable has reached the target position before proceeding.
  A separate function is used to move to the target position for better code organization and readability.
*/
void loop() {
  // Add a counter for the '9' key
  int emergencyStopCounter = 0;

  // Check and reconnect to WiFi if the connection is lost
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  // Check and reconnect to MQTT if the connection is lost
  if (!client.connected()) {
    connectToMQTT();
  }

  client.loop();
  ArduinoOTA.handle();

  // Check for emergency stop
  if (emergencyStop) {
    // Stop the stepper and display an emergency stop message
    stepper.stop();
    lcd.setCursor(0, 0);
    lcd.print("EMERGENCY STOP");
    delay(2000);
    lcd.clear();
    emergencyStop = false; // Reset the emergency stop flag
  }

  // Check for keypad input
  char key = keypad.getKey();
  static bool isKeyHeld = false;          // Track if a key is held down.
  static unsigned long keyHoldTime = 0;   // Track the duration of key hold.
  const unsigned long keyHoldDelay = 500; // Delay before continuous movement starts (in milliseconds).

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
        int direction = (key == '4') ? -1 : 1; // Determine direction based on key.
        if (!isKeyHeld) {
          // Move the turntable by a fixed number of steps in the direction specified by the key
          stepper.move(direction * STEP_MOVE_SINGLE_KEYPRESS);
          isKeyHeld = true;
          keyHoldTime = millis(); // Start tracking the hold time
        } else if (millis() - keyHoldTime >= keyHoldDelay) {
          // Move the turntable continuously by a fixed number of steps in the direction specified by the key
          stepper.move(direction * STEP_MOVE_HELD_KEYPRESS);
        }
      } else if (key == '*' || key == '#') {
        int trackNumber = atoi(keypadTrackNumber);
        int endNumber = (key == '*') ? 0 : 1;
        // Store the current position to the appropriate track head or tail-end position in EEPROM
        if (endNumber == 0) {
          trackHeads[trackNumber - 1] = currentPosition;
          EEPROM.put(EEPROM_TRACK_HEADS_ADDRESS + (trackNumber - 1) * sizeof(int), currentPosition);
        } else {
          trackTails[trackNumber - 1] = currentPosition;
          EEPROM.put(getEEPROMTrackTailsAddress() + (trackNumber - 1) * sizeof(int), currentPosition);
        }
        EEPROM.commit();
        lcd.setCursor(0, 0);
        lcd.print("Position stored for track ");
        lcd.print(trackNumber);
        lcd.setCursor(0, 1);
        lcd.print((endNumber == 0) ? "Head-end" : "Tail-end");
        delay(2000);
        lcd.clear();
        keypadTrackNumber[0] = '\0'; // Reset keypadTrackNumber after storing position or moving.
      }
    } else {
      if (key == '*' || key == '#') {
        int trackNumber = atoi(keypadTrackNumber);
        if (trackNumber >= 1 && trackNumber <= NUMBER_OF_TRACKS) {
          int endNumber = (key == '*') ? 0 : 1;
          int targetPosition = calculateTargetPosition(trackNumber, endNumber);
          moveToTargetPosition(targetPosition);
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Invalid track number!");
          delay(2000);
          lcd.clear();
        }
        keypadTrackNumber[0] = '\0'; // Reset keypadTrackNumber after storing position or moving.
      } else {
        size_t keypadTrackNumberLength = strlen(keypadTrackNumber);
        if (keypadTrackNumberLength < 2) {
          // Append the pressed key to the keypadTrackNumber array
          keypadTrackNumber[keypadTrackNumberLength] = key;
          keypadTrackNumber[keypadTrackNumberLength + 1] = '\0'; // Null-terminate the char array.
        }
      }
    }
  } else {
    isKeyHeld = false;  // Reset isKeyHeld when no key is pressed.
    keyHoldTime = 0;    // Reset keyHoldTime when no key is pressed.
  }

  // Check for reset button press
  bool currentResetButtonState = digitalRead(RESET_BUTTON_PIN);
  if (resetButtonState == LOW && currentResetButtonState == HIGH) {
    // Trigger the homing sequence instead of restarting the ESP32
    while (digitalRead(HOMING_SENSOR_PIN) == HIGH) {
      stepper.move(-10);
      stepper.run();
    }
    currentPosition = 0; // Set current position to zero after homing.
    lcd.setCursor(0, 0);
    lcd.print("HOMING SEQUENCE TRIGGERED");
    delay(2000);
    lcd.clear();
  }
  resetButtonState = currentResetButtonState;

  // Run the stepper if there are remaining steps to move
  if (stepper.distanceToGo() != 0) {
    stepper.run();
  }
}