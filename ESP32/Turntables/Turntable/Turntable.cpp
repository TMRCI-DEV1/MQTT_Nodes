#define VERSION_NUMBER "1.0.9" // Define the version number

/*
  Aisle-Node: Turntable Control
  Project: ESP32-based WiFi/MQTT Turntable Node
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Date: 2023-07-06
  Description:
  This sketch is designed for an OTA-enabled ESP32 Node controlling a Turntable. It utilizes a 3x4 membrane matrix keypad,
  a serial LCD 2004A 20x4 display module with I2C interface, a 16 Channel I2C Interface Electromagnetic Relay Module, an 8 Channel I2C
  Interface Electromagnetic Relay Module, a STEPPERONLINE CNC stepper motor driver, a photo-interrupter "homing" sensor, a reset button,
  and a STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead). The ESP32 Node connects to a WiFi network, subscribes
  to MQTT messages published by JMRI, and enables control of the turntable by entering a 1 or 2-digit track number on the keypad, followed
  by '*' or '#' to select the head-end or tail-end, respectively. The expected MQTT message format is 'Tracknx', where 'n' represents the
  2-digit track number (01-24, depending on location) and 'x' represents 'H' for the head-end or 'T' for the tail-end. The LCD displays the IP address, the
  commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname,("LOCATION_Turntable_Node").

  The turntable is used to rotate locomotives or cars from one track to another, and the ESP32 provides a convenient way to control it remotely via WiFi and MQTT.
*/

// Include the Turntable header file which contains definitions and declarations related to the turntable control.
#include "Turntable.h"

// Uncomment this line to enable calibration mode. Calibration mode allows manual positioning of the turntable without using MQTT commands.
// #define calibrationMode  

// Define the GILBERTON constant to indicate that the sketch is configured for controlling the turntable in the Gilberton location.
#define GILBERTON

// Uncomment the following line to indicate that the sketch is configured for controlling the turntable in the Pittsburgh location.
// #define PITTSBURGH  

// Depending on the location, define the MQTT topics, number of tracks, and track numbers
#ifdef GILBERTON
const char * MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#"; // MQTT topic for Gilberton turntable
const int NUMBER_OF_TRACKS = 23; // Number of tracks in Gilberton
int gilbertonTrackNumbers[] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23
}; // Track numbers in Gilberton
int * TRACK_NUMBERS = gilbertonTrackNumbers; // Pointer to the array of track numbers
#endif

#ifdef PITTSBURGH
const char * MQTT_TOPIC = "TMRCI/output/Pittsburgh/turntable/#"; // MQTT topic for Pittsburgh turntable
const int NUMBER_OF_TRACKS = 22; // Number of tracks in Pittsburgh
int pittsburghTrackNumbers[] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22
}; // Track numbers in Pittsburgh
int * TRACK_NUMBERS = pittsburghTrackNumbers; // Pointer to the array of track numbers
#endif

/*
  Function to write data to EEPROM with error checking. This function uses a template to allow for writing of different data types to EEPROM.
  memcmp is used for data verification to ensure that the data written to EEPROM is the same as the data intended to be written.
*/
template < typename T >
  void writeToEEPROMWithVerification(int address, const T & value) {
    // Define maximum number of write retries
    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool writeSuccess = false;

    // Retry writing to EEPROM until successful or maximum retries reached
    while (retryCount < MAX_RETRIES && !writeSuccess) {
      T originalValue;
      EEPROM.get(address, originalValue); // Read original value from EEPROM.
      EEPROM.put(address, value); // Write new value to EEPROM.
      EEPROM.commit();
      delay(10);

      T readValue;
      EEPROM.get(address, readValue); // Read the written value from EEPROM.

      // If the original and read values are not the same, increment retry count and delay before retrying
      if (memcmp( & originalValue, & readValue, sizeof(T)) != 0) {
        retryCount++;
        delay(500);
      } else {
        writeSuccess = true;
      }
    }

    // If writing to EEPROM failed, print an error message
    if (!writeSuccess) {
      Serial.println("EEPROM write error!");
    }
  }

/*
  Function to read data from EEPROM with error checking. This function uses a template to allow for reading of different data types from EEPROM.
  memcmp is used for data verification to ensure that the data read from EEPROM is the same as the data stored.
*/
template < typename T >
  bool readFromEEPROMWithVerification(int address, T & value) {
    // Define maximum number of read retries
    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool readSuccess = false;

    // Retry reading from EEPROM until successful or maximum retries reached
    while (retryCount < MAX_RETRIES && !readSuccess) {
      T readValue;
      EEPROM.get(address, readValue); // Read the value from EEPROM.

      T writtenValue = value; // Store the read value for verification.
      EEPROM.put(address, writtenValue); // Write the value back to EEPROM.
      EEPROM.commit();
      delay(10);
      EEPROM.get(address, writtenValue); // Read the written value from EEPROM.

      // If the read and written values are not the same, increment retry count and delay before retrying
      if (memcmp( & readValue, & writtenValue, sizeof(T)) != 0) {
        retryCount++;
        delay(500);
      } else {
        value = readValue;
        readSuccess = true;
      }
    }

    // If reading from EEPROM failed, print an error message
    if (!readSuccess) {
      Serial.println("EEPROM read error!");
    }

    return readSuccess;
  }

/*
  Function to connect to WiFi. This function uses a while loop to wait for the connection to be established.
  If the connection fails, the function will retry the connection.
*/
void connectToWiFi() {
  // Begin WiFi connection with the given ssid and password
  WiFi.begin(ssid, password);
  // Wait until WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  // If WiFi connection is successful, print the IP address to the LCD display
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");

    // Get the IP address and convert it to a string
    IPAddress ipAddress = WiFi.localIP();
    String ipAddressString = ipAddress.toString();

    // Print the IP address to the LCD display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(ipAddressString);
  } else {
    // If WiFi connection failed, print an error message and wait 5 seconds before retrying
    Serial.println("Failed to connect to WiFi");
    delay(5000);
  }
}

/*
  Function to connect to MQTT. This function uses a while loop to wait for the connection to be established.
  If the connection fails, the function will retry the connection.
*/
void connectToMQTT() {
  // Wait until MQTT connection is established
  while (!client.connected()) {
    // If WiFi connection is lost, reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }

    // Try to connect to MQTT
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
      client.subscribe(MQTT_TOPIC); // Subscribe to the MQTT topic.
    } else {
      // If MQTT connection failed, print an error message and wait 2 seconds before retrying
      Serial.print("Failed to connect to MQTT. Retrying in 2 seconds... ");
      delay(2000);
    }
  }
  // If MQTT connection is successful, print a success message
  if (client.connected()) {
    Serial.println("Connected to MQTT");
  } else {
    // If MQTT connection failed, print an error message and wait 5 seconds before retrying
    Serial.println("Failed to connect to MQTT");
    delay(5000);
  }
}

// Array to store the head positions of each track. The head position is the starting point of a track.
int * trackHeads;

// Array to store the tail positions of each track. The tail position is the ending point of a track.
int * trackTails;

/*
  ESP32 setup function to initialize the system
  This function uses a separate function to connect to WiFi for better code organization and readability.
  A while loop is used to wait for the homing sensor to ensure that the system is properly initialized before proceeding.
*/
void setup() {
  // Initialize libraries and peripherals
  Wire.begin(); // Initialize the I2C bus
  connectToWiFi(); // Connect to the WiFi network
  lcd.begin(LCD_COLUMNS, LCD_ROWS); // Initialize the LCD display

  // Print the version number on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Version:");
  lcd.setCursor(0, 1);
  lcd.print(VERSION_NUMBER);

  #ifdef calibrationMode
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration Mode");
  #endif

  relayBoard1.begin(); // Initialize the first relay board
  relayBoard2.begin(); // Initialize the second relay board
  stepper.setMaxSpeed(STEPPER_SPEED); // Set the maximum speed for the stepper motor
  stepper.setAcceleration(STEPPER_SPEED); // Set the acceleration for the stepper motor

  // Set hostname based on location
  #ifdef GILBERTON
  WiFi.setHostname("Gilberton_Turntable_Node"); // Set the hostname for the Gilberton turntable node
  #endif

  #ifdef PITTSBURGH
  WiFi.setHostname("Pittsburgh_Turntable_Node"); // Set the hostname for the Pittsburgh turntable node
  #endif

  // Connect to MQTT broker
  connectToMQTT(); // Connect to the MQTT broker and subscribe to the topic

  #ifdef GILBERTON
  trackHeads = new int[23]; // Create an array to store the head positions of each track
  trackTails = new int[23]; // Create an array to store the tail positions of each track
  #endif

  #ifdef PITTSBURGH
  trackHeads = new int[22]; // Create an array to store the head positions of each track
  trackTails = new int[22]; // Create an array to store the tail positions of each track
  #endif

  if (!calibrationMode) {
    // Read data from EEPROM
    readFromEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition); // Read the current position from EEPROM with error checking
    readFromEEPROMWithVerification(EEPROM_TRACK_HEADS_ADDRESS, trackHeads); // Read the track heads from EEPROM with error checking
    readFromEEPROMWithVerification(EEPROM_TRACK_TAILS_ADDRESS, trackTails); // Read the track tails from EEPROM with error checking
  }

  // Initialize keypad and LCD
  keypad.addEventListener([](char key) {
    int trackNumber = atoi(keypadTrackNumber); // Get the track number entered on the keypad

    if (trackNumber > 0 && trackNumber <= NUMBER_OF_TRACKS) { // Check if the track number is valid
      char endChar = keypadTrackNumber[2]; // Get the end character entered on the keypad
      int endNumber = (endChar == 'H') ? 1 : ((endChar == 'T') ? 2 : 0); // Map the end character to a number (1 for head, 2 for tail)

      if (endNumber != 0) { // Check if the end number is valid
        int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate the target position based on the track number and end number
        moveToTargetPosition(targetPosition); // Move the turntable to the target position
        controlRelays(trackNumber); // Control the track power relays

        writeToEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition);

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

  // Enable OTA updates
  ArduinoOTA.begin();
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
  static bool isKeyHeld = false; // Track if a key is held down.
  static unsigned long keyHoldTime = 0; // Track the duration of key hold.
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
          EEPROM.put(EEPROM_TRACK_TAILS_ADDRESS + (trackNumber - 1) * sizeof(int), currentPosition);
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
    isKeyHeld = false; // Reset isKeyHeld when no key is pressed.
    keyHoldTime = 0; // Reset keyHoldTime when no key is pressed.
  }

  // Check for reset button press
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    // Trigger homing sequence instead of restarting ESP32
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

  // Run the stepper if there are remaining steps to move
  if (stepper.distanceToGo() != 0) {
    stepper.run();
  }
}

/* 
  MQTT callback function to handle incoming messages
  This function uses a char array to store the MQTT message because the payload is received as a byte array, and converting it to a char array makes it easier to work with.
  strncpy is used to extract the track number from the MQTT message because it allows for copying a specific number of characters from a string.
*/
void callback(char * topic, byte * payload, unsigned int length) {
  // Print the received MQTT topic
  Serial.print("Received MQTT topic: ");
  Serial.println(topic);

  char mqttTrackNumber[3];
  strncpy(mqttTrackNumber, topic + 5, 2); // Extract track number from MQTT topic.
  mqttTrackNumber[2] = '\0'; // Null-terminate the char array.

  int trackNumber = atoi(mqttTrackNumber); // Convert track number to integer.

  if (trackNumber > NUMBER_OF_TRACKS || trackNumber < 1) {
    Serial.println("Invalid track number received in MQTT topic");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid track number received in MQTT topic");
    return;
  }

  int endNumber = (topic[7] == 'H') ? 0 : 1; // Determine if it's the head or tail end.
  int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate target position.
  moveToTargetPosition(targetPosition); // Move to the target position.

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

/* 
  Function to calculate the target position based on the track number and the end (head or tail) specified
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

/* Function to control track power relays
   This function is used to control the relays separately for better code organization and readability.
   A for loop is used to turn off all the relays because it allows for iterating through all the relays without having to write separate code for each one. */
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
  Function to move the turntable to the target position
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

  if (!calibrationMode) {
    writeToEEPROMWithVerification(CURRENT_POSITION_EEPROM_ADDRESS, currentPosition); // Save current position to EEPROM
  }
}
