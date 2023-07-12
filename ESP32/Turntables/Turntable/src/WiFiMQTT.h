#ifndef WIFIMQTT_H
#define WIFIMQTT_H

#include "Turntable.h"

#include <WiFi.h>              // Include the WiFi library to enable WiFi connectivity for the ESP32.                                         
                               // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi

#include <PubSubClient.h>      // Include the PubSubClient library to enable MQTT communication for publishing and subscribing to topics.     
                               // https://github.com/knolleary/pubsubclient

#include <ArduinoOTA.h>        // Include the ArduinoOTA library to enable Over-The-Air updates for the ESP32.                                
                               // https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

/* Constants */
// Network and MQTT Related
extern const char* ssid;           // SSID (network name) of the WiFi network to connect to.
extern const char* password;       // Password for the WiFi network.
extern const char* HOSTNAME;       // Hostname for the ESP32 on the WiFi network. This is used for identifying the ESP32 on the network.
extern const char* mqtt_broker;    // Address (IP or domain name) of the MQTT broker to connect to.
extern const int mqtt_port;        // Port number for the MQTT broker. The standard port for MQTT is 1883.
extern WiFiClient espClient;       // WiFiClient object used as the network client for the MQTT connection.
extern PubSubClient client;        // PubSubClient object used for MQTT communication.
extern const char* MQTT_TOPIC;     // MQTT topic that the ESP32 will subscribe to for receiving commands.

/* Function prototypes */
void connectToWiFi();              // Function to connect the ESP32 to the WiFi network. This function will attempt to connect to the WiFi network using the provided SSID and password.
void connectToMQTT();              // Function to connect the ESP32 to the MQTT broker. This function will attempt to connect to the MQTT broker using the provided address and port.
void callback(char* topic, byte* payload, unsigned int length); // Callback function that is called when an MQTT message is received. This function handles the incoming MQTT messages.
extern void printToLCD(int row, const char* message);  // Helper function to print a message to a specific row on the LCD display. This function clears the specified row before printing the message.
extern void clearLCD();            // Helper function to clear the LCD.

#endif // WIFIMQTT_H