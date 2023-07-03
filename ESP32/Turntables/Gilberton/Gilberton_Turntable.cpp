/*
  Aisle-Node: Gilberton Turntable Control
  Project: ESP32-based WiFi/MQTT Turntable Node
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.1.8
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
*/

// Include necessary libraries
#include <Wire.h>              // Library for ESP32 I2C connection  https://github.com/esp8266/Arduino/tree/master/libraries/Wire
#include <WiFi.h>              // Library for WiFi connection       https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <Keypad.h>            // Library for 3x4 keypad            https://github.com/Chris--A/Keypad
#include <LiquidCrystal_I2C.h> // Library for Liquid Crystal I2C    https://github.com/johnrickman/LiquidCrystal_I2C
#include <PCF8574.h>           // Library for I2C (8) relay board   https://github.com/xreef/PCF8574_library
#include <PCF8575.h>           // Library for I2C (16) relay board  https://github.com/xreef/PCF8575_library/tree/master
#include <AccelStepper.h>      // Library for Accel Stepper         https://github.com/waspinator/AccelStepper
#include <PubSubClient.h>      // Library for MQTT                  https://github.com/knolleary/pubsubclient
#include <EEPROM.h>            // Library for EEPROM read/write     https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM
#include <ArduinoOTA.h>        // Library for OTA updates           https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

// Define constants
const int STEPS_PER_REV = 6400; // Number of microsteps per full revolution
const int EEPROM_POSITION_ADDRESS = 0; // EEPROM address for storing position
const int EEPROM_HEADS_ADDRESS = 100; // EEPROM address for storing track head positions
const int EEPROM_TAILS_ADDRESS = 200; // EEPROM address for storing track tail positions
const int HOMING_SENSOR_PIN = 25; // Pin for the homing sensor
const int RESET_BUTTON_PIN = 19; // Pin for the reset button
const char* MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#"; // MQTT topic for subscribing to turntable commands
const int TRACK_NUMBERS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}; // Array of valid track numbers
const int STEPPER_SPEED = 200; // Stepper motor speed (steps per second)
const int EEPROM_SIZE = 512; // EEPROM size in bytes
const int RELAY_BOARD1_I2C_ADDRESS = 0x20; // I2C address of relay board 1
const int RELAY_BOARD2_I2C_ADDRESS = 0x21; // I2C address of relay board 2
byte KEYPAD_ROW_PINS[] = {13, 12, 14, 27}; // Row pins of the keypad
byte KEYPAD_COLUMN_PINS[] = {16, 17, 18}; // Column pins of the keypad
const bool CALIBRATION_MODE = true; // Set to true during calibration, false otherwise
const char CONFIRM_YES = '1'; // Confirmation key for calibration mode
const char CONFIRM_NO = '3'; // Cancellation key for calibration mode
const int STEP_MOVE_SINGLE_KEYPRESS = 10; // Number of steps to move with a single key press during calibration
const int STEP_MOVE_HELD_KEYPRESS = 100; // Number of steps to move with a held key press during calibration
bool emergencyStop = false; // Flag for emergency stop condition
char keypadTrackNumber[3] = ""; // Array to store entered track number on the keypad
char mqttTrackNumber[3] = ""; // Array to store track number from MQTT message

// Define network credentials and MQTT broker address
const char* ssid = "###############"; // WiFi network SSID
const char* password = "###############"; // WiFi network password
const char* mqtt_broker = "###############"; // MQTT broker address

// Create instances for WiFi client and MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Create instances for stepper and LCD screen
AccelStepper stepper(AccelStepper::DRIVER, 33, 32); // Stepper driver step (pulse) pin, direction pin
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address of LCD, number of rows and columns

// Create instances for PCF8574 & PCF8575
PCF8575 relayBoard1(RELAY_BOARD1_I2C_ADDRESS);
PCF8574 relayBoard2(RELAY_BOARD2_I2C_ADDRESS);

// Define keypad layout and create instance
const byte ROW_NUM = 4; 
const byte COLUMN_NUM = 3; 
const char KEYPAD_LAYOUT[][COLUMN_NUM] = {{'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'*','0','#'}};
char keys[ROW_NUM][COLUMN_NUM];
Keypad keypad = Keypad(makeKeymap(keys), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, ROW_NUM, COLUMN_NUM);

// Initialize the keys array
void initializeKeysArray() {
  for (byte row = 0; row < ROW_NUM; ++row) {
    for (byte col = 0; col < COLUMN_NUM; ++col) {
      keys[row][col] = KEYPAD_LAYOUT[row][col];
    }
  }
}

// Define variable for current position and track numbers
int currentPosition = 0;
int trackHeads[23];
int trackTails[23];

// Function prototypes
void callback(char* topic, byte* payload, unsigned int length);
int calculateTargetPosition(int trackNumber, int endNumber);
void controlRelays(int trackNumber);
void moveToTargetPosition(int targetPosition);

// Function to connect to WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("Failed to connect to WiFi");
    delay(5000); // Wait 5 seconds before retrying
    connectToWiFi(); // Retry connecting to WiFi
  }
}

// Function to connect to MQTT
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

// Arduino setup function to initialize the system
void setup() {
  // Start Serial and Wire communications
  Serial.begin(115200);
  Wire.begin();
  
  // Connect to the WiFi network
  connectToWiFi();

  // Initialize EEPROM and retrieve last known positions
  EEPROM.begin(EEPROM_SIZE);
  if (!CALIBRATION_MODE) {
    EEPROM.get(EEPROM_POSITION_ADDRESS, currentPosition);
    EEPROM.get(EEPROM_HEADS_ADDRESS, trackHeads);  // Load head positions from EEPROM
    EEPROM.get(EEPROM_TAILS_ADDRESS, trackTails);  // Load tail positions from EEPROM
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
  for(int i = 0; i < 16; i++) {
    relayBoard1.pinMode(i, OUTPUT);
    relayBoard1.digitalWrite(i, HIGH);
    if(i < 8) { // Only do this for the first 8 pins of the second relay board
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

  // Initialize the keys array
  initializeKeysArray();
}

// Add a counter for the '9' key
int emergencyStopCounter = 0;

// Arduino loop function to handle MQTT messages and keypad inputs
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
          EEPROM.put(EEPROM_HEADS_ADDRESS + (trackNumber - 1) * sizeof(int), currentPosition);
        } else {
          trackTails[trackNumber - 1] = currentPosition;
          EEPROM.put(EEPROM_TAILS_ADDRESS + (trackNumber - 1) * sizeof(int), currentPosition);
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
void controlRelays(int trackNumber) {
  // Turn off all relays
  for (uint8_t i = 0; i < 16; i++) {
    relayBoard1.digitalWrite(i, HIGH);
    if (i < 8) {
      relayBoard2.digitalWrite(i, HIGH);
    }
  }

  // Turn on the relay corresponding to the selected track
  if (trackNumber <= 14) {
    relayBoard1.digitalWrite(trackNumber + 1, LOW); // +1 because relay 0 is for the turntable bridge
  } else if (trackNumber <= 22) {
    relayBoard2.digitalWrite(trackNumber - 15, LOW); // -15 because the first 15 tracks are on the first relay board
  }
}

// Function to move the turntable to the target position
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
    EEPROM.put(EEPROM_POSITION_ADDRESS, currentPosition); // Save current position to EEPROM
    EEPROM.commit();
  }
}