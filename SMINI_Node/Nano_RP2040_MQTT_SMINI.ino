/*
  Project: Arduino-Nano RP2040 based WiFi CMRI/MQTT enabled SMINI Node (48 outputs / 24 inputs)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.2
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

// Define the Arduino ID
const char* arduinoId = "Arduino1"; // ***CHANGE TO APPROPRIATE UNIQUE ID***

// Define the range of output (turnout and light) and sensor IDs
const int minOutputId = 1001;    // Change Node number (1)001 to appropriate Node number
const int maxOutputId = 1048;
const int minSensorId = 1001;
const int maxSensorId = 1024;

// Function declarations for MQTT
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void updateOutputs();

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

  if (client.connect(arduinoId)) {
    Serial.println("connected");

    // Subscribe to the topics for turnouts and lights
    String turnoutTopic = String("TMRCI/cmd/") + String(arduinoId) + "/turnout/T#";
    client.subscribe(turnoutTopic.c_str());

    String lightTopic = String("TMRCI/cmd/") + String(arduinoId) + "/light/L#";
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
  for (int i = 1; i <= 24; i++) {
    int byteIndex = (i - 1) / 8;
    int bitIndex = (i - 1) % 8;
    if (bitRead(currentInputState[byteIndex], bitIndex) != bitRead(last_input_state[byteIndex], bitIndex)) {
      String topic = String("TMRCI/dt/") + String(arduinoId) + "/sensor/S" + String(i);
      String payload = (bitRead(currentInputState[byteIndex], bitIndex) == 1) ? "ACTIVE" : "INACTIVE";
      client.publish(topic.c_str(), payload.c_str());
    }
  }
  // Copy the current input state to the last input state for the next comparison
  memcpy(last_input_state, currentInputState, 3);
  delay(10);
}

// Function to reconnect to MQTT server
void reconnect() {
  // Loop until reconnected to the MQTT server
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect to the MQTT server
    if (client.connect(arduinoId)) {
      Serial.println("connected");

      // Subscribe to the topics for turnouts and lights
      String turnoutTopic = String("TMRCI/cmd/") + String(arduinoId) + "/turnout/T#";
      client.subscribe(turnoutTopic.c_str());

      String lightTopic = String("TMRCI/cmd/") + String(arduinoId) + "/light/L#";
      client.subscribe(lightTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying connection
      delay(5000);
    }
  }
}

// Function to update the outputs (lights and turnouts)
void updateOutputs() {
  // Send the last output state to the 74HC595 shift registers
  digitalWrite(LATCH_595, LOW);
  for (int i = 5; i >= 0; i--) {
    SPI.transfer(last_output_state[i]);
  }
  digitalWrite(LATCH_595, HIGH);
}

// Callback function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  // Construct the received topic and message
  String receivedTopic = String(topic);
  String receivedMessage;
  for (unsigned int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }

  // Construct the topic prefix for turnouts and lights
  String prefixTurnout = String("TMRCI/cmd/") + String(arduinoId) + String("/turnout/T");
  String prefixLight = String("TMRCI/cmd/") + String(arduinoId) + String("/light/L");
  int objectId;
  
  // Handle turnout messages
  if (receivedTopic.startsWith(prefixTurnout)) {
    objectId = receivedTopic.substring(prefixTurnout.length() + 1).toInt();
    bool turnoutState = false; // OFF (NORMAL) by default
    if (receivedMessage == "REVERSE") {
      turnoutState = true; // ON (REVERSE)
    }
    // If the turnout ID is within the defined range, update the corresponding output
    if (objectId >= minOutputId && objectId <= maxOutputId) {
      int byteIndex = (objectId - minOutputId) / 8;
      int bitIndex = (objectId - minOutputId) % 8;
      bitWrite(last_output_state[byteIndex], bitIndex, turnoutState);
      updateOutputs();
    }
  } 
  // Handle light messages
  else if (receivedTopic.startsWith(prefixLight)) {
    objectId = receivedTopic.substring(prefixLight.length() + 1).toInt();
    bool lightState = false; // OFF by default
    if (receivedMessage == "ON") {
      lightState = true; // ON
    }
    // If the light ID is within the defined range, update the corresponding output
    if (objectId >= minOutputId && objectId <= maxOutputId) {
      int byteIndex = (objectId - minOutputId) / 8;
      int bitIndex = (objectId - minOutputId) % 8;
      bitWrite(last_output_state[byteIndex], bitIndex, lightState);
      updateOutputs();
    }
  }
}
