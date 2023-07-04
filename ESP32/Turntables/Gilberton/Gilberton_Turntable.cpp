/*
  Aisle-Node: Gilberton Turntable Control
  Project: ESP32-based WiFi/MQTT Turntable Node
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.2.1
  Date: 2023-07-03
  Description:
  This sketch is designed for an OTA-enabled ESP32 Node controlling the Gilberton Turntable. It utilizes a 3x4 membrane matrix keypad,
  a serial LCD 2004 20x4 display module with I2C interface, a 16 Channel I2C Interface Electromagnetic Relay Module, an 8 Channel I2C
  Interface Electromagnetic Relay Module, a STEPPERONLINE CNC stepper motor driver, a photo-interrupter "homing" sensor, a reset button,
  and a STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead). The ESP32 Node connects to a WiFi network, subscribes
  to MQTT messages published by JMRI, and enables control of the turntable by entering a 1 or 2-digit track number on the keypad, followed
  by '*' or '#' to select the head-end or tail-end, respectively. The expected MQTT message format is 'Tracknx', where 'n' represents the
  2-digit track number (01-23) and 'x' represents 'H' for the head-end or 'T' for the tail-end. The LCD displays the IP address, the
  commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname.
  
  The turntable is used to rotate locomotives or cars from one track to another, and the ESP32 provides a convenient way to control it remotely via WiFi and MQTT.
*/

// Include necessary libraries
#include <Wire.h>              // Library for ESP32 I2C connection. Used for communication with the relay boards and LCD screen.
#include <WiFi.h>              // Library for WiFi connection. Used for connecting to the WiFi network.
#include <Keypad.h>            // Library for 3x4 keypad. Used for reading input from the keypad.
#include <LiquidCrystal_I2C.h> // Library for Liquid Crystal I2C. Used for controlling the LCD screen.
#include <PCF8574.h>           // Library for I2C (8) relay board. Used for controlling the 8-channel relay board.
#include <PCF8575.h>           // Library for I2C (16) relay board. Used for controlling the 16-channel relay board.
#include <AccelStepper.h>      // Library for Accel Stepper. Used for controlling the stepper motor.
#include <PubSubClient.h>      // Library for MQTT. Used for subscribing to MQTT messages.
#include <EEPROM.h>            // Library for EEPROM read/write. Used for storing the current position and track head/tail positions.
#include <ArduinoOTA.h>        // Library for OTA updates. Used for updating the sketch over the air.

// Define constants
const int STEPS_PER_REV = 6400; // Number of microsteps per full revolution. Microsteps are used for smoother and more precise control of the stepper motor.
const int EEPROM_POSITION_ADDRESS = 0; // EEPROM address for storing position
const int EEPROM_TRACK_HEADS_ADDRESS = 100; // EEPROM address for storing track head positions
const int EEPROM_TRACK_TAILS_ADDRESS = 200; // EEPROM address for storing track tail positions
const int HOMING_SENSOR_PIN = 25; // Pin for the homing sensor. The homing sensor is used to set a known position for the turntable.
const int RESET_BUTTON_PIN = 19; // Pin for the reset button. The reset button is used to trigger the homing sequence.
const char* MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#"; // MQTT topic for subscribing to turntable commands. The ESP32 will listen for messages on this topic.
const int TRACK_NUMBERS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}; // Array of valid track numbers. These are the track numbers that the turntable can move to.
const int STEPPER_SPEED = 200; // Stepper motor speed (steps per second). This is the speed at which the stepper motor will rotate.
const int EEPROM_SIZE = 512; // EEPROM size in bytes. This is the total amount of storage available in the EEPROM.
const int RELAY_BOARD1_ADDRESS = 0x20; // I2C address of relay board 1. This is the address that the ESP32 will use to communicate with the first relay board.
const int RELAY_BOARD2_ADDRESS = 0x21; // I2C address of relay board 2. This is the address that the ESP32 will use to communicate with the second relay board.
byte KEYPAD_ROW_PINS[] = {13, 12, 14, 27}; // Row pins of the keypad. These are the pins that the keypad rows are connected to.
byte KEYPAD_COLUMN_PINS[] = {16, 17, 18}; // Column pins of the keypad. These are the pins that the keypad columns are connected to.
const bool CALIBRATION_MODE = true; // Set to true during calibration, false otherwise. In calibration mode, the turntable will move to the position specified by the track number directly.
const char CONFIRM_YES = '1'; // Confirmation key for calibration mode. Pressing this key will confirm the current position as the head or tail position for the current track.
const char CONFIRM_NO = '3'; // Cancellation key for calibration mode. Pressing this key will cancel the calibration process.
const int STEP_MOVE_SINGLE_KEYPRESS = 10; // Number of steps to move with a single key press during calibration. This allows for fine control of the turntable position during calibration.
const int STEP_MOVE_HELD_KEYPRESS = 100; // Number of steps to move with a held key press during calibration. This allows for fast movement of the turntable during calibration.
bool emergencyStop = false; // Flag for emergency stop condition. When this flag is set, the turntable will stop moving immediately.
char keypadTrackNumber[3] = ""; // Array to store entered track number on the keypad. This is where the track number entered on the keypad is stored.
char mqttTrackNumber[3] = ""; // Array to store track number from MQTT message. This is where the track number from the MQTT message is stored.

// Define network credentials and MQTT broker address
const char* ssid = "###############"; // WiFi network SSID. This is the name of the WiFi network that the ESP32 will connect to.
const char* password = "###############"; // WiFi network password. This is the password for the WiFi network that the ESP32 will connect to.
const char* mqtt_broker = "###############"; // MQTT broker address. This is the address of the MQTT broker that the ESP32 will connect to.

// Create instances for WiFi client and MQTT client
WiFiClient espClient; // WiFi client instance. This is used for the network connection.
PubSubClient client(espClient); // MQTT client instance. This is used for subscribing to MQTT messages.

// Create instances for stepper and LCD screen
AccelStepper stepper(AccelStepper::DRIVER, 33, 32); // Stepper driver step (pulse) pin, direction pin. This is used for controlling the stepper motor.
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address of LCD, number of rows and columns. This is used for controlling the LCD screen.

// Create instances for PCF8574 & PCF8575
PCF8575 relayBoard1(RELAY_BOARD1_ADDRESS); // Instance for the 16-channel relay board. This is used for controlling the 16-channel relay board.
PCF8574 relayBoard2(RELAY_BOARD2_ADDRESS); // Instance for the 8-channel relay board. This is used for controlling the 8-channel relay board.

// Define keypad layout and create instance
const byte ROW_NUM = 4; // Number of rows on the keypad.
const byte COLUMN_NUM = 3; // Number of columns on the keypad.
const char KEYPAD_LAYOUT[][COLUMN_NUM] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}}; // Layout of the keys on the keypad.
char keys[ROW_NUM][COLUMN_NUM]; // Array to store the current state of the keys on the keypad.
Keypad keypad = Keypad(makeKeymap(keys), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, ROW_NUM, COLUMN_NUM); // Keypad instance. This is used for reading input from the keypad.

// Initialize the keys array
// This function is used to initialize the keys array separately to keep the setup function clean and readable.
// A byte array is used instead of a char array because the keypad library requires a byte array for the row and column pins.
void initializeKeysArray() {
  for (byte row = 0; row < ROW_NUM; ++row) {
    for (byte col = 0; col < COLUMN_NUM; ++col) {
      keys[row][col] = KEYPAD_LAYOUT[row][col]; // Copy the layout from KEYPAD_LAYOUT to keys.
    }
  }
}

// Define variable for current position and track numbers
int currentPosition = 0; // Current position of the turntable. This is updated every time the turntable moves.
int trackHeads[23]; // Array to store the positions of the head ends of the tracks. These positions are stored in EEPROM and loaded at startup.
int trackTails[23]; // Array to store the positions of the tail ends of the tracks. These positions are stored in EEPROM and loaded at startup.

// Function prototypes
void callback(char* topic, byte* payload, unsigned int length); // Callback function for MQTT messages. This function is called whenever an MQTT message is received.
int calculateTargetPosition(int trackNumber, int endNumber); // Function to calculate the target position based on the track number and end number. This function is used to determine where the turntable should move to.
void controlRelays(int trackNumber); // Function to control the track power relays. This function is used to turn on the relay for the selected track and turn off all other relays.
void moveToTargetPosition(int targetPosition); // Function to move the turntable to a target position. This function is used to move the turntable to the desired position.

// Function to write data to EEPROM with error checking
// This function uses a template to allow for writing of different data types to EEPROM.
// memcmp is used for data verification to ensure that the data written to EEPROM is the same as the data intended to be written.
template <typename T>
void writeToEEPROMWithVerification(int address, const T& value) {
  T originalValue;
  EEPROM.get(address, originalValue); // Read original value from EEPROM
  EEPROM.put(address, value); // Write new value to EEPROM
  EEPROM.commit();

  T readValue;
  EEPROM.get(address, readValue); // Read the written value from EEPROM

  if (memcmp(&originalValue, &readValue, sizeof(T)) != 0) {
    // Data verification failed, print an error message
    Serial.println("EEPROM write error!");
  }
}

// Function to read data from EEPROM with error checking
// This function uses a template to allow for reading of different data types from EEPROM.
// memcmp is used for data verification to ensure that the data read from EEPROM is the same as the data stored.
template <typename T>
bool readFromEEPROMWithVerification(int address, T& value) {
  T originalValue;
  EEPROM.get(address, originalValue);
  memcpy(&value, &originalValue, sizeof(T));

  T readValue;
  EEPROM.get(address, readValue);

  if (memcmp(&originalValue, &readValue, sizeof(T)) != 0) {
    // Data verification failed, print an error message
    Serial.println("EEPROM read error!");
    return false;
  }

  return true;
}

// Function to connect to WiFi
// This function uses a while loop to wait for the connection to be established.
// If the connection fails, the function will retry the connection.
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("Failed to connect to WiFi");
    delay(5000); // Wait 5 seconds before retrying
    connectToWiFi(); // Retry connecting to WiFi
  }
}

// Function to connect to MQTT
// This function uses a while loop to wait for the connection to be established.
// If the connection fails, the function will retry the connection.
void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
      client.subscribe(MQTT_TOPIC); // Subscribe to the MQTT topic
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  if (client.connected()) {
    Serial.println("Connected to MQTT");
  } else {
    Serial.println("Failed to connect to MQTT");
    delay(5000); // Wait 5 seconds before retrying
    connectToMQTT(); // Retry connecting to MQTT
  }
}

// ESP32 setup function to initialize the system
// This function uses a separate function to connect to WiFi for better code organization and readability.
// A while loop is used to wait for the homing sensor to ensure that the system is properly initialized before proceeding.
void setup() {
  // Start Serial and Wire communications
  Serial.begin(115200);
  Wire.begin();

  // Initialize the keys array
  initializeKeysArray();

  // Connect to the WiFi network
  connectToWiFi();

  // Initialize EEPROM and retrieve last known positions
  EEPROM.begin(EEPROM_SIZE);
  if (!CALIBRATION_MODE) {
    if (!readFromEEPROMWithVerification(EEPROM_POSITION_ADDRESS, currentPosition)) {
      // Handle EEPROM read error
    }
    if (!readFromEEPROMWithVerification(EEPROM_TRACK_HEADS_ADDRESS, trackHeads)) {
      // Handle EEPROM read error
    }
    if (!readFromEEPROMWithVerification(EEPROM_TRACK_TAILS_ADDRESS, trackTails)) {
      // Handle EEPROM read error
    }
  }

  // Print the IP address to the serial monitor
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set the hostname
  WiFi.setHostname("Gilberton_Turntable_Node");

  // Initialize OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Starting OTA update...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete.");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  // Set password for OTA updates
  ArduinoOTA.setPassword("TMRCI");

  // Start OTA service
  ArduinoOTA.begin();
  Serial.println("OTA Initialized. Waiting for OTA updates...");

  // Set up the MQTT client
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);

  pinMode(HOMING_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // Move to home position at startup
  while (digitalRead(HOMING_SENSOR_PIN) == HIGH) {
    stepper.move(-10); // Change step count for homing functionality if necessary
    stepper.run();
  }
  currentPosition = 0; // Set current position to zero after homing

  // Initialize the relay boards
  relayBoard1.begin();
  relayBoard2.begin();

  // Set the pin modes and initial state (HIGH - Track power OFF)
  for (int i = 0; i < 16; i++) {
    relayBoard1.pinMode(i, OUTPUT);
    relayBoard1.digitalWrite(i, HIGH);
    if (i < 8) { // Only do this for the first 8 pins of the second relay board
      relayBoard2.pinMode(i, OUTPUT);
      relayBoard2.digitalWrite(i, HIGH);
    }
  }

  // Turn on the relay for the turntable bridge (relay 0 on the first board)
  relayBoard1.digitalWrite(0, LOW);

  // Initialize the LCD and print IP
  lcd.begin(20, 4);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  delay(3000); // Keep IP address on screen for 3 seconds before clearing
  lcd.clear();

  // Display calibration mode message
  if (CALIBRATION_MODE) {
    lcd.setCursor(0, 0);
    lcd.print("CALIBRATION MODE");
    lcd.setCursor(0, 1);
    lcd.print("Press 1 to confirm");
    lcd.setCursor(0, 2);
    lcd.print("Press 3 to cancel");
    while (true) {
      char key = keypad.getKey();
      if (key) {
        if (key == CONFIRM_YES) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("CALIBRATION STARTED");
          delay(2000);
          lcd.clear();
          break;
        } else if (key == CONFIRM_NO) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("CALIBRATION CANCELLED");
          delay(2000);
          lcd.clear();
        }
      }
    }
  }

  // Initialize the stepper
  stepper.setMaxSpeed(STEPPER_SPEED);
  stepper.setAcceleration(2000); // Change acceleration/deceleration rate as necessary to provide smooth prototype turntable movements
  stepper.setCurrentPosition(currentPosition);
}

// Add a counter for the '9' key
int emergencyStopCounter = 0;

// ESP32 loop function to handle MQTT messages and keypad inputs
// This function uses a while loop to wait for the stepper to finish moving to ensure that the turntable has reached the target position before proceeding.
// A separate function is used to move to the target position for better code organization and readability.
void loop() {
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
  static bool isKeyHeld = false;  // Track if a key is held down
  static unsigned long keyHoldTime = 0;  // Track the duration of key hold
  const unsigned long keyHoldDelay = 500;  // Delay before continuous movement starts (in milliseconds)

  if (key) {
    if (key == '9') {
      emergencyStopCounter++;
      if (emergencyStopCounter >= 3) {
        // Activate emergency stop if the '9' key is pressed three times consecutively
        emergencyStop = true;
        emergencyStopCounter = 0; // Reset the counter
      }
    } else {
      emergencyStopCounter = 0; // Reset the counter if any other key is pressed
    }

    if (key == '4' || key == '6') {
      int direction = (key == '4') ? -1 : 1;  // Determine direction based on key
      if (!isKeyHeld) {
        // Move the turntable by a fixed number of steps in the direction specified by the key
        stepper.move(direction * STEP_MOVE_SINGLE_KEYPRESS);
        isKeyHeld = true;
        keyHoldTime = millis();  // Start tracking the hold time
      } else if (millis() - keyHoldTime >= keyHoldDelay) {
        // Move the turntable continuously by a fixed number of steps in the direction specified by the key
        stepper.move(direction * STEP_MOVE_HELD_KEYPRESS);
      }
    } else if (key == '*' || key == '#') {
      int trackNumber = atoi(keypadTrackNumber);
      int endNumber = (key == '*') ? 0 : 1;
      if (CALIBRATION_MODE) {
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
      } else {
        if (trackNumber >= 1 && trackNumber <= 23) {
          int targetPosition = calculateTargetPosition(trackNumber, endNumber);
          moveToTargetPosition(targetPosition);
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Invalid track number!");
          delay(2000);
          lcd.clear();
        }
      }
      keypadTrackNumber[0] = '\0'; // Reset keypadTrackNumber after storing position or moving
    } else {
      size_t keypadTrackNumberLength = strlen(keypadTrackNumber);
      if (keypadTrackNumberLength < 2) {
        // Append the pressed key to the keypadTrackNumber array
        keypadTrackNumber[keypadTrackNumberLength] = key;
        keypadTrackNumber[keypadTrackNumberLength + 1] = '\0'; // Null-terminate the char array
      }
    }
  } else {
    isKeyHeld = false;  // Reset isKeyHeld when no key is pressed
    keyHoldTime = 0;    // Reset keyHoldTime when no key is pressed
  }

  // Check for reset button press
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    // Trigger homing sequence instead of restarting ESP32
    while (digitalRead(HOMING_SENSOR_PIN) == HIGH) {
      stepper.move(-10);
      stepper.run();
    }
    currentPosition = 0; // Set current position to zero after homing
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

// MQTT callback function to handle incoming messages
// This function uses a char array to store the MQTT message because the payload is received as a byte array, and converting it to a char array makes it easier to work with.
// strncpy is used to extract the track number from the MQTT message because it allows for copying a specific number of characters from a string.
void callback(char* topic, byte* payload, unsigned int length) {
  char messageTemp[8]; // Char array to store the MQTT message
  for (int i = 0; i < length; i++) {
    messageTemp[i] = (char)payload[i];
  }
  messageTemp[length] = '\0'; // Null-terminate the char array

  char mqttTrackNumber[3];
  strncpy(mqttTrackNumber, messageTemp + 5, 2); // Extract track number from MQTT topic
  mqttTrackNumber[2] = '\0'; // Null-terminate the char array

  int trackNumber = atoi(mqttTrackNumber); // Convert track number to integer
  int endNumber = (messageTemp[7] == 'H') ? 0 : 1; // Determine if it's the head or tail end
  int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate target position
  moveToTargetPosition(targetPosition); // Move to the target position
}

// Function to calculate target position based on track number and end number
// This function is used to calculate the target position separately for better code organization and readability.
// An array is used to store the track heads and tails for efficiency, as it allows for quick access to the head and tail positions of each track.
int calculateTargetPosition(int trackNumber, int endNumber) {
  int targetPosition;
  if (CALIBRATION_MODE) {
    targetPosition = trackNumber; // In calibration mode, the target position is the track number itself
  } else {
    targetPosition = (endNumber == 0) ? trackHeads[trackNumber - 1] : trackTails[trackNumber - 1]; // Retrieve the corresponding head or tail position
  }
  return targetPosition;
}

// Function to control track power relays
// This function is used to control the relays separately for better code organization and readability.
// A for loop is used to turn off all the relays because it allows for iterating through all the relays without having to write separate code for each one.
void controlRelays(int trackNumber) {
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
  } else if (trackNumber >= 16 && trackNumber <= 23) {
    relayBoard2.digitalWrite(trackNumber - 16, LOW);
  }
}

// Function to move the turntable to the target position
// This function is used to move to the target position separately for better code organization and readability.
// A while loop is used to wait for the stepper to finish moving to ensure that the turntable has reached the target position before proceeding.
void moveToTargetPosition(int targetPosition) {
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

  currentPosition = targetPosition; // Update current position after moving

  // Turn on the track power for the target position after the move is complete
  controlRelays(targetPosition);

  // Turn the turntable bridge track power back ON (LOW) once the turntable has completed its move
  relayBoard1.digitalWrite(0, LOW);

  if (!CALIBRATION_MODE) {
    writeToEEPROMWithVerification(EEPROM_POSITION_ADDRESS, currentPosition); // Save current position to EEPROM
  }
}
