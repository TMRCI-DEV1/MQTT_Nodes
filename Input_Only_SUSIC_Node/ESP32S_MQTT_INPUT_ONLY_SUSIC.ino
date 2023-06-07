/*
  Project: ESP32S based WiFi CMRI/MQTT enabled SUSIC Input-ONLY Node (72 inputs)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.2
  Date: 2023-06-07
  Description: A sketch for an ESP32S based CMRI SUSIC Input-ONLY Node (72 inputs)
  using MQTT to publish messages subscribed to by JMRI. Message payload is either ACTIVE or INACTIVE.
*/

// Include necessary libraries
#include <Arduino.h>       // Library for Arduino AVR Core    https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Arduino.h
#include <WiFi.h>          // Library for WiFi connection     https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <PubSubClient.h>  // Library for MQTT                https://github.com/knolleary/pubsubclient
#include <SPI.h>           // Library for SPI communication   https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI

// Network configuration
const char ssid[] = "(HO) Touchscreens & MQTT Nodes";     // Name of the WiFi network
const char password[] = "touch.666.pi";                   // Password of the WiFi network
const char* mqtt_server = "129.213.106.87";               // IP address of the MQTT server

// MQTT topic constants
const char* MQTT_TOPIC_PREFIX_SENSOR = "TMRCI/input/";

// Define pins for 74HC165 (input shift register)
const byte LATCH_165 = 9;                                 // UPDATE ACCORDING TO PCB DESIGN***

// Instantiate MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Array to store the last state of the inputs
byte last_input_state[9]; // 72 inputs from 9 74HC165s

// Identifier of the Node
const char* NodeID = "10-A-Node-2"; // ***CHANGE TO APPROPRIATE UNIQUE ID***

// Range of the sensor IDs
const int minSensorId = 1;
const int maxSensorId = 72;

void setup() {
  // Initialize the latch pin for the shift registers as output
  pinMode(LATCH_165, OUTPUT);
  digitalWrite(LATCH_165, HIGH);
  SPI.begin(); // Begin SPI communication

  // Setup serial communication for debugging and initiate WiFi connection
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Check if the WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set MQTT server and the callback function
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // If the client is disconnected, attempt to reconnect
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Read and latch the state of inputs from the shift registers
  digitalWrite(LATCH_165, LOW);
  delayMicroseconds(5);
  digitalWrite(LATCH_165, HIGH);

  byte currentInputState[9]; // Store current state of inputs

  // Iterate over each shift register to read its current state
  for (int i = 0; i < 9; i++) {
    currentInputState[i] = ~SPI.transfer(0); // Invert the bits to reflect active low inputs
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

  // Update the last input state for the next comparison
  memcpy(last_input_state, currentInputState, 9);

  // Delay before next loop iteration
  delay(10);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages if required
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(NodeID)) {
      Serial.println("connected");
      // Subscribe to MQTT topics if required
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
