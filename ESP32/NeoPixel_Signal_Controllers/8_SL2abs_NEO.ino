/*
  Project: ESP32 based WiFi/MQTT enabled Double Searchlight High Absolute Signal Mast Neopixel Node
  (8 signal mast outputs / 16 Neopixel Signal Heads)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.3
  Date: 2023-06-10
  Description: This sketch is designed for an ESP32 Node with 8 signal mast outputs, using MQTT to subscribe to messages published by JMRI.
  The expected incoming subscribed messages are for JMRI Signal Mast objects, and the expected message payload format is 'Aspect; Lit (or Unlit); Unheld (or Held)'.
*/

// Include necessary libraries
#include <WiFi.h>              // Library for WiFi connection     https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <PubSubClient.h>      // Library for MQTT                https://github.com/knolleary/pubsubclient
#include <Adafruit_NeoPixel.h> // Library for Adafruit Neopixels  https://github.com/adafruit/Adafruit_NeoPixel
#include <map>                 // Library for std::map            https://en.cppreference.com/w/cpp/container/map
#include <string>              // Library for std::basic_string   https://en.cppreference.com/w/cpp/string/basic_string

// Network configuration
const char* WIFI_SSID = "(HO) Touchscreens & MQTT Nodes";     // WiFi SSID
const char* WIFI_PASSWORD = "touch.666.pi";                   // WiFi Password

// MQTT configuration
const char* MQTT_SERVER = "129.213.106.87";                   // MQTT server address
const int MQTT_PORT = 1883;                                   // MQTT server port

// Instantiate MQTT client
WiFiClient espClient;                                         // Create a WiFi client
PubSubClient client(espClient);                               // Create a MQTT client using the WiFi client

// Define the GPIO pins for the Neopixels in ascending order
const int neoPixelPins[8] = {16, 17, 18, 19, 21, 22, 23, 32}; // ESP32 Pin numbers for Neopixels which match Signal Masts 1-8 in ascending order

// Define the Neopixel chains and signal masts
Adafruit_NeoPixel signalMasts[8] = {                             // Array of Neopixels, one for each signal mast
    Adafruit_NeoPixel(2, neoPixelPins[0], NEO_GRB + NEO_KHZ800), // SM1
    Adafruit_NeoPixel(2, neoPixelPins[1], NEO_GRB + NEO_KHZ800), // SM2
    Adafruit_NeoPixel(2, neoPixelPins[2], NEO_GRB + NEO_KHZ800), // SM3
    Adafruit_NeoPixel(2, neoPixelPins[3], NEO_GRB + NEO_KHZ800), // SM4
    Adafruit_NeoPixel(2, neoPixelPins[4], NEO_GRB + NEO_KHZ800), // SM5
    Adafruit_NeoPixel(2, neoPixelPins[5], NEO_GRB + NEO_KHZ800), // SM6
    Adafruit_NeoPixel(2, neoPixelPins[6], NEO_GRB + NEO_KHZ800), // SM7
    Adafruit_NeoPixel(2, neoPixelPins[7], NEO_GRB + NEO_KHZ800)  // SM8
};

// Define the NodeID and MQTT topic
String NodeID = "10-A-Node-1";                                // Node identifier
String mqttTopic = "TMRCI/output/" + NodeID + "/signalmast/"; // Base MQTT topic

// Function Prototypes
void callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void reconnectWiFi();

// Define the signal aspects and lookup table
const uint32_t RED = signalMasts[0].Color(255, 0, 0);         // RED color
const uint32_t YELLOW = signalMasts[0].Color(255, 192, 0);    // YELLOW color
const uint32_t GREEN = signalMasts[0].Color(0, 255, 64);      // GREEN color

// Struct to represent signal mast aspect
struct Aspect {
    uint32_t head1;                                           // Color of the first Neopixel
    uint32_t head2;                                           // Color of the second Neopixel
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

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(10);
    Serial.println("Setup started");

    // Initialize WiFi and connect to the network
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Connect to the MQTT broker
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);                             // Set the message received callback function
    reconnectMQTT();
    Serial.println("Connected to MQTT");

    // Initialize each Neopixel signal mast with a stop signal
    for (int i = 0; i < 8; i++) {
        signalMasts[i].begin();
        signalMasts[i].setPixelColor(0, RED);                 // Set first head as RED
        signalMasts[i].setPixelColor(1, RED);                 // Set second head as RED
        signalMasts[i].setBrightness(255);                    // Set brightness
        signalMasts[i].show();                                // Display the set colors
    }
}

void loop() {
    // Reconnect to WiFi if connection lost
    if (WiFi.status() != WL_CONNECTED) {
        reconnectWiFi();
    }

    // Reconnect to MQTT server if connection lost
    if (!client.connected()) {
        reconnectMQTT();
    }

    client.loop();                                            // Run MQTT loop to handle incoming messages
}

void reconnectMQTT() {
    // Attempt to reconnect to MQTT
    while (!client.connected()) {
        Serial.println("Attempting to connect to MQTT...");
        if (client.connect(NodeID.c_str())) {
            client.subscribe((mqttTopic + "#").c_str());      // Subscribe to topics for all signal masts
            Serial.println("Connected to MQTT");
        } else {
            delay(5000);
        }
    }
}

void reconnectWiFi() {
    // Loop until we're reconnected
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(WIFI_SSID);
        // Connect to WPA/WPA2 network:
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait for connection:
        uint8_t i = 0;
        while (WiFi.status() != WL_CONNECTED && i < 20) {
            delay(500);
            Serial.print(".");
            i++;
        }
        Serial.println("");
    }
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    // Create a char array for null-termination of the payload
    char message[length + 1];
    for (unsigned int i = 0; i < length; i++) {
        message[i] = (char)payload[i];
    }
    message[length] = '\0';

    // Convert payload bytes into a string
    String payloadStr(message);
    Serial.print("Received message with payload: ");
    Serial.println(payloadStr);

    // Parse the payload into aspect, lit, and held strings
    int separatorIndex1 = payloadStr.indexOf(';');
    int separatorIndex2 = payloadStr.lastIndexOf(';');

    // Extract and trim the aspect string
    String aspectStr = payloadStr.substring(0, separatorIndex1);
    aspectStr.trim();

    // Extract and trim the lit string
    String litStr = payloadStr.substring(separatorIndex1 + 1, separatorIndex2);
    litStr.trim();

    // Extract and trim the held string
    String heldStr = payloadStr.substring(separatorIndex2 + 1);
    heldStr.trim();

    // Extract the mast number from the topic
    String topicStr = String(topic);
    int lastSlashIndex = topicStr.lastIndexOf('/');
    String mastNumberStr = topicStr.substring(lastSlashIndex + 3); // +3 to account for "SM"
    int mastNumber = mastNumberStr.toInt();

    if (mastNumber < 1 || mastNumber > 8) {
        Serial.println("Error: Invalid mast number.");
        return;
    }
    mastNumber -= 1; // Convert 1-based SM number to 0-based index

    // Check if the signal mast should be unlit
    if (litStr == "Unlit") {
        // Turn off both heads
        signalMasts[mastNumber].setPixelColor(0, 0);
        signalMasts[mastNumber].setPixelColor(1, 0);
        signalMasts[mastNumber].show();
        return;
    }

    // Check if the signal mast should be held
    if (heldStr == "Held") {
        // Set aspect to stop
        aspectStr = "Stop";
    }

    // Convert aspectStr from String to std::string
    std::string aspectKey = aspectStr.c_str();

    // Set the colors based on the aspect if it exists in the lookup table
    if (doubleSearchlightHighAbsoluteLookup.find(aspectKey) != doubleSearchlightHighAbsoluteLookup.end()) {
        Aspect aspect = doubleSearchlightHighAbsoluteLookup[aspectKey];
        signalMasts[mastNumber].setPixelColor(0, aspect.head1);
        signalMasts[mastNumber].setPixelColor(1, aspect.head2);
        signalMasts[mastNumber].show();
    } else {
        Serial.print("Unknown aspect received: ");
        Serial.println(aspectStr);
    }
}
