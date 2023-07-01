/*
  Aisle-Node: Gilberton Turntable Control (PRODUCTION)
  Project: ESP32-based WiFi/MQTT Turntable Node
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.7
  Date: 2023-07-01
  Description:
  This sketch is designed for an OTA-enabled ESP32 Node controlling the Gilberton Turntable. It utilizes a 3x4 membrane matrix keypad,
  a serial LCD 2004 20x4 display module with I2C interface, (2) 16 Channel I2C Interface Electromagnetic Relay Modules, a STEPPERONLINE CNC
  stepper motor driver, and a STEPPERONLINE stepper motor (Nema 17 Bipolar 40mm 64oz.in(45Ncm) 2A 4 Lead). The ESP32 Node connects to a WiFi
  network, subscribes to MQTT messages published by JMRI, and enables control of the turntable by entering a 2-digit track number on the
  keypad, followed by '*' or '#' to select the head-end or tail-end, respectively. The expected MQTT message format is 'Tracknx', where 'n'
  represents the 2-digit track number (01-24) and 'x' represents 'H' for the head-end or 'T' for the tail-end. The LCD displays the IP
  address, the commanded track number, and the head or tail position. The ESP32 Node is identified by its hostname.
*/

// Include necessary libraries
#include <Wire.h>              // Library for ESP32 I2C connection  https://github.com/esp8266/Arduino/tree/master/libraries/Wire
#include <WiFi.h>              // Library for WiFi connection       https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <Keypad.h>            // Library for 3x3 keypad            https://github.com/Chris--A/Keypad
#include <LiquidCrystal_I2C.h> // Library for Liquid Crystal I2C    https://github.com/johnrickman/LiquidCrystal_I2C
#include <PCF8575.h>           // Library for I2C relay boards      https://github.com/xreef/PCF8575_library/tree/master
#include <AccelStepper.h>      // Library for Accel Stepper         https://github.com/waspinator/AccelStepper
#include <PubSubClient.h>      // Library for MQTT                  https://github.com/knolleary/pubsubclient
#include <EEPROM.h>            // Library for EEPROM read/write     https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM
#include <ArduinoOTA.h>        // Library for OTA updates           https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

// Define constants for steps per revolution and EEPROM addresses for saving positions
#define STEPS_PER_REV 6400
#define EEPROM_POSITION_ADDRESS 0
#define EEPROM_HEADS_ADDRESS 100
#define EEPROM_TAILS_ADDRESS 200

// Define network credentials and MQTT broker address
const char* ssid = "###############";
const char* password = "###############";
const char* mqtt_broker = "###############";

// Create instances for WiFi client and MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Create instances for stepper and LCD screen
AccelStepper stepper(AccelStepper::DRIVER, 33, 32);
LiquidCrystal_I2C lcd(0x3F, 20, 4);

// Create instances for PCF8575
PCF8575 relayBoard1(0x20); // relay board 1 with I2C address 0x20
PCF8575 relayBoard2(0x21); // relay board 2 with I2C address 0x21

// Define keypad layout and create instance
const byte ROW_NUM = 4;
const byte COLUMN_NUM = 3;
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte pin_rows[ROW_NUM] = {19, 23, 13, 14};
byte pin_column[COLUMN_NUM] = {16, 17, 18};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Define variable for current position and track numbers
int currentPosition = 0;
int trackHeads[24];
int trackTails[24];
String trackNumber = "";
unsigned long lastMillis = 0; // variable to keep track of last time

// Forward declare callback and position calculation functions
void callback(char* topic, byte* payload, unsigned int length);
void moveToTargetPosition(int targetPosition);
int calculateTargetPosition(int trackNumber, int endNumber);

// Arduino setup function to initialize the system
void setup() {
  // Start Serial and Wire communications
  Serial.begin(115200);
  Wire.begin();

  // Connect to the WiFi network
  WiFi.begin(ssid, password);

  // Initialize EEPROM and retrieve last known positions
  EEPROM.begin(512);
  // COMMENT OUT DURING CALIBRATION*************************************************************************************
  EEPROM.get(EEPROM_POSITION_ADDRESS, currentPosition);
  EEPROM.get(EEPROM_HEADS_ADDRESS, trackHeads);  // load head positions from EEPROM
  EEPROM.get(EEPROM_TAILS_ADDRESS, trackTails);  // load tail positions from EEPROM

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis(); // get current time
    if (currentMillis - lastMillis >= 500) { // 500ms has passed
      Serial.println("Connecting to WiFi...");
      lastMillis = currentMillis; // save the last time a message was printed
    }
  }
  Serial.println("Connected to WiFi");

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

  // Wait for MQTT connection
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
    }
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Initialize the relay boards
  relayBoard1.begin();
  relayBoard2.begin();

  // Set the pin modes and initial state (HIGH - Track power OFF)
  for(int i = 0; i < 16; i++) {
    relayBoard1.pinMode(i, OUTPUT);
    relayBoard1.digitalWrite(i, HIGH);
    relayBoard2.pinMode(i, OUTPUT);
    relayBoard2.digitalWrite(i, HIGH);
  }

  // Initialize the LCD and print IP
  lcd.begin(20, 4);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  delay(3000); // Keep IP address on screen for 3 seconds before clearing
  lcd.clear();

  // Display calibration mode message
  // COMMENT OUT AFTER CALIBRATION*************************************************************************************
  // lcd.setCursor(0,1); // Move cursor to the second line
  // lcd.print("Calibration Mode");

  // Set the speed of the stepper and subscribe to MQTT topic
  stepper.setSpeed(7); // Set to 7 steps per second (full revolution in ~30 seconds)
  client.subscribe("TMRCI/output/Gilberton/turntable/#");
}

// Arduino main loop function
void loop() {
  ArduinoOTA.handle(); // Handle OTA updates

  // Get pressed key from the keypad
  char key = keypad.getKey();
  if (key) {
    // Build track number from numeric keys
    if (key >= '0' && key <= '9') {
      if (trackNumber.length() < 2) { // Check that the track number doesn't exceed 2 characters
        trackNumber += key; // Append the character to trackNumber
      }
    }
    // Handle '*' and '#' keys
    else if (key == '*' || key == '#') {
      // Ensure trackNumber is within the valid range before using it as an index
      if (trackNumber.toInt() >= 1 && trackNumber.toInt() <= 24) {
        int trackIndex = trackNumber.toInt() - 1;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Save Track " + trackNumber);
        lcd.setCursor(0, 1);
        lcd.print(key == '*' ? "head-end " : "tail-end ");
        lcd.setCursor(0, 2);
        lcd.print("position? '1'=Yes, '3'=No");

        while (true) {
          char confirmKey = keypad.getKey();
          if (confirmKey == '1') {
            if (key == '*') {
              // Save the current position as the head of the track
              // COMMENT OUT AFTER CALIBRATION*************************************************************************************
              // trackHeads[trackIndex] = currentPosition;
              // EEPROM.put(EEPROM_HEADS_ADDRESS + trackIndex * sizeof(int), currentPosition);
            }
            else if (key == '#') {
              // Save the current position as the tail of the track
              // COMMENT OUT AFTER CALIBRATION*************************************************************************************
              // trackTails[trackIndex] = currentPosition;
              // EEPROM.put(EEPROM_TAILS_ADDRESS + trackIndex * sizeof(int), currentPosition);
            }
            trackNumber = "";  // Clear track number
            lcd.clear();
            break;
          }
          else if(confirmKey == '3') {
            // If user decides not to save the position
            trackNumber = "";  // Clear track number
            lcd.clear();
            break;
          }
        }
      } else {
        Serial.println("Invalid track number. Please enter a number from 1 to 24.");
      }
    }
    // Handle '4' and '5' keys for manual adjustment
    else if (key == '4') {
      // Move clockwise by small increments
      // COMMENT OUT AFTER CALIBRATION*****************************************************************************************
      // stepper.move(10);
      // stepper.run();
      // currentPosition = (currentPosition + 10) % STEPS_PER_REV;
    }
    else if (key == '5') {
      // Move counterclockwise by small increments
      // COMMENT OUT AFTER CALIBRATION*****************************************************************************************
      // stepper.move(-10);
      // stepper.run();
      // currentPosition = (currentPosition - 10 + STEPS_PER_REV) % STEPS_PER_REV;
    }
  }

  // Allow MQTT client to process incoming messages
  client.loop();

  // Reconnect to MQTT server if connection was lost
  if (!client.connected()) {
    while (!client.connected()) {
      Serial.println("Reconnecting to MQTT...");
      if (client.connect("ESP32Client")) {
        Serial.println("Connected to MQTT");
        client.subscribe("TMRCI/output/Gilberton/turntable/#");
      }
      else {
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
      }
      delay(10);  // Delay to avoid excessive looping and prevent overwhelming the system
    }
  }
}

// Callback function for handling incoming MQTT messages
void callback(char* topic, byte* message, unsigned int length) {
  // Validate topic format
  String strTopic = String(topic);
  if (strTopic.startsWith("TMRCI/output/Gilberton/turntable/Track") && strTopic.length() == 39) {
    // Extract track number and 'H' or 'T' from the message
    String trackStr = String(trackNumber); // Convert the char array to a String for easier manipulation
    char endChar = strTopic.charAt(36);

    trackStr = String(trackNumber); // Update the trackStr with the new value

    int endNumber = (endChar == 'H') ? 1 : 2;

    // Perform the movement
    int targetPosition = calculateTargetPosition(trackNumber.toInt(), endNumber);

    // Only move if the target position is different from the current position
    if (targetPosition != currentPosition) {
      moveToTargetPosition(targetPosition);

    // Display track and position information on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Track: ");
    lcd.print(trackNumber);
    lcd.setCursor(0, 1);
    lcd.print("Position: ");
    lcd.print((endNumber == 1) ? "Head" : "Tail");
    
    // Display track and position information on Serial Monitor
    Serial.println();
    Serial.print("Track: ");
    Serial.println(trackNumber);
    Serial.print("Position: ");
    Serial.println((endNumber == 1) ? "Head" : "Tail");
    }

    // Reset trackNumber for the next operation
    trackNumber = "";
  }
}

// Function to move stepper to a target position
void moveToTargetPosition(int targetPosition) {
  int stepsToMove = targetPosition - currentPosition;
  if (stepsToMove < 0) {
    stepsToMove += STEPS_PER_REV;
  }
  if (stepsToMove > STEPS_PER_REV / 2) {
    stepper.move(-stepsToMove); // use stepper.move for relative movements
  }
  else if (stepsToMove < STEPS_PER_REV / 2) {
    stepper.move(stepsToMove); // use stepper.move for relative movements
  }
  else {
    // if the shortest path is exactly half the STEPS_PER_REV
    // choose to always move clockwise
    stepper.move(stepsToMove); // use stepper.move for relative movements
  }
  stepper.runToPosition();
  currentPosition = targetPosition;

  // Save current position to EEPROM
  EEPROM.put(EEPROM_POSITION_ADDRESS, currentPosition);
  EEPROM.commit();
}

// Function to calculate the target position based on track number and end number
int calculateTargetPosition(int trackNumber, int endNumber) {
  // Turn off all relays
  for (uint8_t i = 0; i < 16; i++) {
    relayBoard1.digitalWrite(i, HIGH);
    relayBoard2.digitalWrite(i, HIGH);
  }

  // Track numbers start from 1. Subtract 1 to match the 0-indexed array and relay positions
  trackNumber--;

  // Turn on the relay corresponding to the selected track
  if (trackNumber < 16) {
    relayBoard1.digitalWrite(trackNumber, LOW);
  }
  else {
    relayBoard2.digitalWrite(trackNumber - 16, LOW);
  }

  // Return the corresponding track position
  if (endNumber == 1) {
    return trackHeads[trackNumber];
  }
  else { // endNumber == 2
    return trackTails[trackNumber];
  }
}
