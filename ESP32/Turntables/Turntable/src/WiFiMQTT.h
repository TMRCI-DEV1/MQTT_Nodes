#ifndef WIFIMQTT_H
#define WIFIMQTT_H

#include "Turntable.h"

#include <WiFi.h>                  // Library for WiFi connection. Used for connecting to the WiFi network.
#include <PubSubClient.h>          // Library for MQTT. Used for subscribing to MQTT messages.
#include <ArduinoOTA.h>            // Library for OTA updates. Used for updating the sketch over the air.

// Network and MQTT Related
extern const char* ssid;           // SSID of the WiFi network
extern const char* password;       // Password of the WiFi network
extern const char* mqtt_broker;    // MQTT broker address
extern const int mqtt_port;        // MQTT broker port
extern WiFiClient espClient;       // WiFiClient object for MQTT communication
extern PubSubClient client;        // PubSubClient object for MQTT communication
extern const char* MQTT_TOPIC;     // MQTT topic to subscribe to

void connectToWiFi();              // Function to connect to the WiFi network
void connectToMQTT();              // Function to connect to the MQTT broker
void callback(char* topic, byte* payload, unsigned int length); // Callback function for MQTT messages

#endif // WIFIMQTT_H
