/*
  Project: Arduino-Nano RP2040 based WiFi CMRI/MQTT enabled Double Head Searchlight High Absolute 
  Signal Mast SMINI Node (8 signal mast outputs / 24 inputs)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.2
  Date: 2023-05-28
  Description: A sketch for an Arduino-Nano RP2040 based CMRI SMINI Node (8 signal mast outputs / 24 inputs) 
  using MQTT to subscribe to and publish messages published by and subscribed to by JMRI.
  Published Sensor message payload is 'ACTIVE' / 'INACTIVE'. Expected incoming subscribed messages are
  for JMRI Signal Mast objects. Expected message payload is 'Aspect; Lit (or Unlit); Unheld (or Held). 
  Each Signal Mast uses 6 outputs (3 per Signal Haed).
*/

// Include necessary libraries
#include <WiFiNINA.h>     // Library for WiFi connection    https://github.com/arduino-libraries/WiFiNINA
#include <PubSubClient.h> // Library for MQTT               https://github.com/knolleary/pubsubclient
#include <SPI.h>          // Library for SPI communication  https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/SPI
#include <map>            // Library for std::map           https://en.cppreference.com/w/cpp/container/map

// Network configuration
const char ssid[] = "(HO) Touchscreens & MQTT Nodes";     // Name of the WiFi network
const char password[] = "touch.666.pi";    // Password of the WiFi network
const char *mqtt_server = "192.168.50.50"; // IP address of the MQTT server

// MQTT topic constants
const char* MQTT_TOPIC_PREFIX_OUTPUT = "TMRCI/output/"; // Topic prefix for output messages
const char* MQTT_TOPIC_PREFIX_SENSOR = "TMRCI/input/";  // Topic prefix for sensor messages

// Define pins for 74HC165 (input shift register)
const byte LATCH_165 = 9;

// Define pins for 74HC595 (output shift register)
const byte LATCH_595 = 6;
const byte DATA_595 = 7;
const byte CLOCK_595 = 8;

// Map the signal mast pins to the corresponding output pin numbers
std::map<int, std::pair<int, int>> signalMastToOutputPins = {
  {1, {1, 6}},   // Signal Mast 1 uses pins 1-6
  {2, {7, 12}},  // Signal Mast 2 uses pins 7-12
  {3, {13, 18}}, // Signal Mast 3 uses pins 13-18
  {4, {19, 24}}, // Signal Mast 4 uses pins 19-24
  {5, {25, 30}}, // Signal Mast 5 uses pins 25-30
  {6, {31, 36}}, // Signal Mast 6 uses pins 31-36
  {7, {37, 42}}, // Signal Mast 7 uses pins 37-42
  {8, {43, 48}}  // Signal Mast 8 uses pins 43-48
};

// Instantiate MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Arrays to store the last state of the inputs and outputs
byte last_input_state[3];  // Store the last state of the inputs
byte last_output_state[6]; // Store the last state of the outputs (8 pins per shift register, 6 shift registers)

// Identifier of the Node
const char* NodeID = "10-A-Node-2"; // ***CHANGE TO APPROPRIATE UNIQUE ID (Bus, Node #)***

// Define the range of output and sensor IDs
const int minOutputId = 1;
const int maxOutputId = 8;
const int minSensorId = 1;
const int maxSensorId = 24;

// Enum to represent LED colors
enum LEDColor {
  GREEN,
  YELLOW,
  RED,
  DARK
};

// Struct to represent signal mast aspect
struct Aspect {
  LEDColor head1;
  LEDColor head2;
};

// Struct to represent the state of a signal mast
struct SignalMastState {
  Aspect currentAspect;
  bool isLit;
  bool isHeld;
};

// Lookup table for signal mast aspects
std::map<std::string, Aspect> doubleSearchlightHighAbsoluteLookup = {
  {"Clear Alt", {GREEN, GREEN}},
  {"Clear", {GREEN, RED}},
  {"Advance Approach Medium", {GREEN, YELLOW}},
  {"Approach Medium", {YELLOW, GREEN}},
  {"Advance Approach", {YELLOW, YELLOW}},
  {"Medium Clear", {RED, GREEN}},
  {"Approach", {YELLOW, RED}},
  {"Medium Approach", {RED, YELLOW}},
  {"Restricting", {RED, YELLOW}},
  {"Stop", {RED, RED}}
};

// Lookup table for LED colors to output pins
std::map<LEDColor, int> ledColorToOutput = {
  {GREEN, 0b001},    // Only the green LED is on
  {YELLOW, 0b010},   // Only the yellow LED is on
  {RED, 0b100},      // Only the red LED is on
  {DARK, 0b000}      // All LEDs are off
};

std::map<int, SignalMastState> signalMastStates;

// Function declarations for MQTT
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void updateOutputs() {
  // Update the state of the 74HC595 shift registers based on the last_output_state[] array
  digitalWrite(LATCH_595, LOW);
  for (int i = 5; i >= 0; i--) {
    shiftOut(DATA_595, CLOCK_595, MSBFIRST, last_output_state[i]);
  }
  digitalWrite(LATCH_595, HIGH);
}

// Function to reconnect to WiFi
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Trying to reconnect...");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      WiFi.begin(ssid, password);
      delay(5000); // Waiting 5 seconds between each attempt
      attempts++;
      Serial.print("Attempt ");
      Serial.print(attempts);
      Serial.println(" to reconnect WiFi...");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi reconnection failed after 10 attempts. Proceeding without WiFi.");
    }
  }
}

void setup() {
  // Set up input and output shift registers
  pinMode(LATCH_165, OUTPUT);
  pinMode(LATCH_595, OUTPUT);
  pinMode(DATA_595, OUTPUT);
  pinMode(CLOCK_595, OUTPUT);
  digitalWrite(LATCH_165, HIGH);
  digitalWrite(LATCH_595, LOW);
  SPI.begin();

  // Set up WiFi
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect(NodeID)) {
    Serial.println("connected");
    String signalmastsTopic = String(MQTT_TOPIC_PREFIX_OUTPUT) + String(NodeID) + "/signalmast/#";
    client.subscribe(signalmastsTopic.c_str());
  }
  
  for (int i = minOutputId; i <= maxOutputId; i++) {
  signalMastStates[i] = {doubleSearchlightHighAbsoluteLookup["Stop"], true, false};
  }
}

void loop() {
  // Reconnect to WiFi if connection lost
  reconnectWiFi();

  // Reconnect to MQTT server if connection lost
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Read input state from 74HC165 shift registers
  digitalWrite(LATCH_165, LOW);
  delayMicroseconds(5);
  digitalWrite(LATCH_165, HIGH);
  byte currentInputState[3];
  for (int i = 0; i < 3; i++) {
    currentInputState[i] = SPI.transfer(0);
  }

  // Publish input state changes over MQTT
  for (int i = minSensorId; i <= maxSensorId; i++) {
    int arrayIndex = i - minSensorId;
    int byteIndex = arrayIndex / 8;
    int bitIndex = arrayIndex % 8;
    if (bitRead(currentInputState[byteIndex], bitIndex) != bitRead(last_input_state[byteIndex], bitIndex)) {
      String topic = String(MQTT_TOPIC_PREFIX_SENSOR) + String(NodeID) + "/sensor/S" + String(i);
      String payload = (bitRead(currentInputState[byteIndex], bitIndex) == 0) ? "ACTIVE" : "INACTIVE";
      client.publish(topic.c_str(), payload.c_str(), true);
    }
  }

  // Copy the current input state to the last input state for the next comparison
  memcpy(last_input_state, currentInputState, 3);
  
  // Add a delay before next loop
  updateOutputs();
  delay(10);
}

// Function to reconnect to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(NodeID)) {
      Serial.println("connected");
      String outputTopic = String(MQTT_TOPIC_PREFIX_OUTPUT) + String(NodeID) + "/";
      client.subscribe(outputTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Ensure the message is for our topic
  String receivedTopic = String(topic);
  String deviceType = receivedTopic.substring(receivedTopic.lastIndexOf("/") + 1, receivedTopic.lastIndexOf("/") + 2);
  String deviceIdStr = receivedTopic.substring(receivedTopic.lastIndexOf("/") + 2);
  int deviceId = deviceIdStr.toInt();

  // Only process messages within the defined device ID range
  if (deviceType == "SM" && deviceId >= minOutputId && deviceId <= maxOutputId) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    String receivedMessage = String(message);

    // Parse the aspect, lit/unlit state, and held/unheld state from the received message
    String aspectString = receivedMessage.substring(0, receivedMessage.indexOf(";"));
    String litStateString = receivedMessage.substring(receivedMessage.indexOf(";") + 1, receivedMessage.lastIndexOf(";"));
    String heldStateString = receivedMessage.substring(receivedMessage.lastIndexOf(";") + 1);

    bool isLit = (litStateString == "Lit");
    bool isHeld = (heldStateString == "Held");

    // Update the signal mast's lit/unlit and held/unheld states
    signalMastStates[deviceId].isLit = isLit;
    signalMastStates[deviceId].isHeld = isHeld;

    // Update the signal mast's aspect, unless it's held (in which case it should display Stop)
    if (!isHeld) {
      signalMastStates[deviceId].currentAspect = doubleSearchlightHighAbsoluteLookup[std::string(aspectString.c_str())];
    } else {
      signalMastStates[deviceId].currentAspect = doubleSearchlightHighAbsoluteLookup["Stop"];
    }
    
    // Map the aspect LED colors to the output pins
    int outputStartIndex = (deviceId - 1) * 6 + 1;
    int outputPin = outputStartIndex;

    // Set the output pins based on the LED colors in the aspect, if the signal mast is lit
    if (signalMastStates[deviceId].isLit) {
      for (int j = 0; j < 3; j++) {
        int ledState = bitRead(ledColorToOutput[static_cast<LEDColor>(signalMastStates[deviceId].currentAspect.head1)], j);
        bitWrite(last_output_state[(outputPin - 1) / 8], (outputPin - 1) % 8, !ledState);  // Invert the LED state
        outputPin++;
      }
      for (int j = 0; j < 3; j++) {
        int ledState = bitRead(ledColorToOutput[static_cast<LEDColor>(signalMastStates[deviceId].currentAspect.head2)], j);
        bitWrite(last_output_state[(outputPin - 1) / 8], (outputPin - 1) % 8, !ledState);  // Invert the LED state
        outputPin++;
      }
    } else {
      // Turn off the LEDs
      for (int j = 0; j < 6; j++) {
        bitWrite(last_output_state[(outputPin - 1) / 8], (outputPin - 1) % 8, HIGH);  // Invert the LED state
        outputPin++;
      }
    }

    updateOutputs(); // Update the shift register with the new states
  }
}
