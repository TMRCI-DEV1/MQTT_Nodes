/*
  Project: ESP32 based WiFi/MQTT enabled (6) Double Searchlight High Absolute and (3) Single Searchlight High Permissive Signal Mast Neopixel Node
  (9 signal mast outputs / 15 Neopixel Signal Heads)
  Author: Thomas Seitz (thomas.seitz@tmrci.org)
  Version: 1.0.3
  Date: 2023-06-10
  Description: This sketch is designed for an ESP32 Node with 9 signal mast outputs, using MQTT to subscribe to messages published by JMRI.
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
const int neoPixelPins[9] = {16, 17, 18, 19, 21, 22, 23, 32, 33};  // ESP32 Pin numbers for Neopixels which match Signal Masts 1-9 in ascending order

// Define the Neopixel chains and signal masts
Adafruit_NeoPixel signalMasts[9] = {                             // Array of Neopixels, one for each signal mast
    Adafruit_NeoPixel(2, neoPixelPins[0], NEO_GRB + NEO_KHZ800), // SM1
    Adafruit_NeoPixel(2, neoPixelPins[1], NEO_GRB + NEO_KHZ800), // SM2
    Adafruit_NeoPixel(2, neoPixelPins[2], NEO_GRB + NEO_KHZ800), // SM3
    Adafruit_NeoPixel(2, neoPixelPins[3], NEO_GRB + NEO_KHZ800), // SM4
    Adafruit_NeoPixel(2, neoPixelPins[4], NEO_GRB + NEO_KHZ800), // SM5
    Adafruit_NeoPixel(2, neoPixelPins[5], NEO_GRB + NEO_KHZ800), // SM6
    Adafruit_NeoPixel(1, neoPixelPins[6], NEO_GRB + NEO_KHZ800), // SM7
    Adafruit_NeoPixel(1, neoPixelPins[7], NEO_GRB + NEO_KHZ800), // SM8
    Adafruit_NeoPixel(1, neoPixelPins[8], NEO_GRB + NEO_KHZ800)  // SM9
};

// Define the NodeID and MQTT topic
String NodeID = "10-A-Node-1";                                // Node identifier
String mqttTopic = "TMRCI/output/" + NodeID + "/signalmast/"; // Base MQTT topic

// Function Prototypes
void callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void reconnectWiFi();

// Define the signal aspects and lookup tables
const uint32_t RED = signalMasts[0].Color(243, 20, 84);        // RED color
const uint32_t YELLOW = signalMasts[0].Color(255, 246, 134);   // YELLOW color
const uint32_t GREEN = signalMasts[0].Color(25, 220, 211);     // GREEN color

// Struct to represent signal mast aspect
struct Aspect {
    uint32_t head1;                                            // Color of the first Neopixel
    uint32_t head2;                                            // Color of the second Neopixel
};

// Lookup table for double head signal mast aspects
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

// Lookup table for single head signal mast aspects
std::map<std::string, uint32_t> singleSearchlightHighPermissiveLookup = {
    {"Clear", GREEN},
    {"Approach", YELLOW},
    {"Permissive", YELLOW},
    {"Stop and Proceed", RED}
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

    // Initialize each Neopixel signal mast with a stop signal or stop and proceed based on mast number
    for (int i = 0; i < 9; i++) {
        signalMasts[i].begin();
        signalMasts[i].setBrightness(255);                    // Set brightness
    
        if (i < 6) { // For masts 1-6 (double head signal masts)
            signalMasts[i].setPixelColor(0, RED);             // Set first head as RED
            signalMasts[i].setPixelColor(1, RED);             // Set second head as RED
        } else { // For masts 7-9 (single head signal masts)
            signalMasts[i].setPixelColor(0, RED);             // Set head as RED (Stop and Proceed)
        }
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
    // Attempt to reconnect to WiFi
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Attempting to connect to WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(5000);
    }
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

    if (separatorIndex1 == -1 || separatorIndex2 == -1) {
        Serial.println("Error: Invalid payload format.");
        return;
    }

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

    if (mastNumber < 1 || mastNumber > 9) {
        Serial.println("Error: Invalid mast number.");
        return;
    }
    mastNumber -= 1; // Convert 1-based SM number to 0-based index

    // Check if the signal mast should be unlit
    if (litStr == "Unlit") {
        // Turn off both heads
        signalMasts[mastNumber].setPixelColor(0, 0);
        if (mastNumber < 6) {
            signalMasts[mastNumber].setPixelColor(1, 0);  // Turn off the second head if double-head signal mast
        }
        signalMasts[mastNumber].show();
        return;
    }

    // Check if the signal mast should be held
    if (heldStr == "Held") {
        // Set aspect to stop or stop and proceed depending on mast type
        if (mastNumber < 6) {
            aspectStr = "Stop";
        } else {
            aspectStr = "Stop and Proceed";
        }
    }

    // Convert aspectStr from String to std::string
    std::string aspectKey = aspectStr.c_str();

    // Set the colors based on the aspect if it exists in the lookup table
    if (mastNumber < 6 && doubleSearchlightHighAbsoluteLookup.find(aspectKey) != doubleSearchlightHighAbsoluteLookup.end()) {
        // Double-head signal mast
        Aspect aspect = doubleSearchlightHighAbsoluteLookup[aspectKey];
        signalMasts[mastNumber].setPixelColor(0, aspect.head1);
        signalMasts[mastNumber].setPixelColor(1, aspect.head2);
        signalMasts[mastNumber].show();
    } else if (mastNumber >= 6 && mastNumber < 9 && singleSearchlightHighPermissiveLookup.find(aspectKey) != singleSearchlightHighPermissiveLookup.end()) {
        // Single-head signal mast
        uint32_t color = singleSearchlightHighPermissiveLookup[aspectKey];
        signalMasts[mastNumber].setPixelColor(0, color);
        signalMasts[mastNumber].show();
    }
}
