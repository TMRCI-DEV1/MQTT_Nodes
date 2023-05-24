/*
  Project: Arduino-Nano RP2040 based WiFi CMRI/MQTT enabled SMINI Node (48 outputs / 24 inputs)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.6
  Date: 2023-05-24
  Description: A sketch for an Arduino-Nano RP2040 based CMRI SUSIC Input-ONLY Node (48 outputs / 24 inputs) 
  using MQTT to subscribe to and publish messages published by and subscribed to by JMRI.
  Published Sensor message payload is 'ACTIVE' / 'INACTIVE'. Expected incoming subscribed messages are
  for JMRI Turnout and Light objects. Expected message payload is 'NORMAL' / 'REVERSE' for Turnouts and
  'ON' / 'OFF' for Lights.
*/

// Include necessary libraries
#include <WiFiNINA.h>     // Library for WiFi connection
#include <PubSubClient.h> // Library for MQTT
#include <SPI.h>          // Library for SPI communication

// Network configuration
const char ssid[] = "HO Touch Panels";     // Name of the WiFi network
const char password[] = "touch.666.pi";    // Password of the WiFi network
const char *mqtt_server = "192.168.50.50"; // IP address of the MQTT server

// MQTT topic constants
const char* MQTT_TOPIC_PREFIX_TURNOUT = "TMRCI/cmd/turnout/";
const char* MQTT_TOPIC_PREFIX_LIGHT = "TMRCI/cmd/light/";

// Define pins for 74HC165 (input shift register)
const byte LATCH_165 = 9;

// Define pins for 74HC595 (output shift register)
const byte LATCH_595 = 6;
const byte DATA_595 = 7;
const byte CLOCK_595 = 8;

// Instantiate MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Arrays to store the last state of the inputs and outputs
byte last_input_state[3];
byte last_output_state[6];

// Identifier of the Node
const char* NodeID = "10-A-Node-2"; // ***CHANGE TO APPROPRIATE UNIQUE ID (Bus, Node #)***

// Define the range of output (turnout and light) and sensor IDs
const int minOutputId = 1;    // Change to 1
const int maxOutputId = 48;   // Change to 48
const int minSensorId = 1;    // Change to 1
const int maxSensorId = 24;   // Change to 24

// Function declarations for MQTT
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void updateOutputs();
void handleOutputMessage(String receivedTopic, String receivedMessage, const char* topicPrefix);

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

    // Subscribe to the topics for turnouts and lights
    String turnoutTopic = String(MQTT_TOPIC_PREFIX_TURNOUT) + String(NodeID) + "/#";
    client.subscribe(turnoutTopic.c_str());

    String lightTopic = String(MQTT_TOPIC_PREFIX_LIGHT) + String(NodeID) + "/#";
    client.subscribe(lightTopic.c_str());
  }
}

void loop() {
  // Reconnect to WiFi if connection lost
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
    currentInputState[i] = ~(SPI.transfer(0));
  }

  // Publish input state changes over MQTT
  for (int i = minSensorId; i <= maxSensorId; i++) {
    int arrayIndex = i - minSensorId;
    int byteIndex = arrayIndex / 8;
    int bitIndex = arrayIndex % 8;
    if (bitRead(currentInputState[byteIndex], bitIndex) != bitRead(last_input_state[byteIndex], bitIndex)) {
      String topic = String("TMRCI/dt/sensor/") + String(NodeID) + "/S" + String(i);
      String payload = (bitRead(currentInputState[byteIndex], bitIndex) == 1) ? "ACTIVE" : "INACTIVE";
      client.publish(topic.c_str(), payload.c_str());
    }
}

  // Copy the current input state to the last input state for the next comparison
  memcpy(last_input_state, currentInputState, 3);
  
  // Add a delay before next loop
  delay(10);
}

// Function to reconnect to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(NodeID)) {
      Serial.println("connected");

      // Subscribe to each topic for turnouts and lights
      String turnoutTopic = String(MQTT_TOPIC_PREFIX_TURNOUT) + String(NodeID) + "/#";
      client.subscribe(turnoutTopic.c_str());

      String lightTopic = String(MQTT_TOPIC_PREFIX_LIGHT) + String(NodeID) + "/#";
      client.subscribe(lightTopic.c_str());

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String receivedTopic = String(topic);
  String receivedMessage;
  for (unsigned int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }

  if (receivedTopic.startsWith(MQTT_TOPIC_PREFIX_TURNOUT)) {
    handleOutputMessage(receivedTopic, receivedMessage, MQTT_TOPIC_PREFIX_TURNOUT);
  } else if (receivedTopic.startsWith(MQTT_TOPIC_PREFIX_LIGHT)) {
    handleOutputMessage(receivedTopic, receivedMessage, MQTT_TOPIC_PREFIX_LIGHT);
  }
}

void handleOutputMessage(String receivedTopic, String receivedMessage, const char* topicPrefix) {
  String strId = receivedTopic.substring(strlen(topicPrefix) + strlen(NodeID) + 1);
  int id = strId.toInt();

  if (id >= minOutputId && id <= maxOutputId) {
    int arrayIndex = id - minOutputId;
    int byteIndex = arrayIndex / 8;
    int bitIndex = arrayIndex % 8;
    
    if (receivedMessage.equals("REVERSE") || receivedMessage.equals("ON")) {
      bitWrite(last_output_state[byteIndex], bitIndex, 1);
    } else if (receivedMessage.equals("NORMAL") || receivedMessage.equals("OFF")) {
      bitWrite(last_output_state[byteIndex], bitIndex, 0);
    }
    
    // Write new output state to 74HC595 shift registers
    digitalWrite(LATCH_595, LOW);
    for (int i = 5; i >= 0; i--) {
      shiftOut(DATA_595, CLOCK_595, MSBFIRST, last_output_state[i]);
    }
    digitalWrite(LATCH_595, HIGH);
  }
}
