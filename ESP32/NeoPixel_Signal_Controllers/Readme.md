# Project Description
These sketches are designed for an ESP32 Node with WiFi and MQTT capabilities. They provide the functionality to control a set of signal masts using an ESP32 board, Neopixel LEDs, and MQTT. It allows for the wireless control of the signal aspects based on messages received from JMRI. The NodeID and IP address are displayed on an OLED display for easy identification. The sketch can be customized and extended to support additional signal mast configurations and features as needed.

The sketch uses MQTT to subscribe to messages published by JMRI (Java Model Railroad Interface) for controlling the signal mast objects. The expected format of the incoming messages is 'Aspect; Lit (or Unlit); Unheld (or Held)'. The sketch listens to MQTT topics specific to each signal mast.

The NodeID and IP address of the ESP32 are displayed on an attached 128×64 OLED display. The NodeID is also used as the ESP32 host name for easy network identification.

## Libraries Used
The sketch utilizes the following libraries:

**Wire**: This library provides support for the I2C connection used by the ESP32. The library is available at [https://github.com/esp8266/Arduino/tree/master/libraries/Wire].

**Adafruit_GFX**: This library is used for working with Adafruit displays and provides a common set of graphics functions. The library can be found at [https://github.com/adafruit/Adafruit-GFX-Library].

**Adafruit_SSD1306**: This library enables the control of monochrome OLED displays, specifically the SSD1306 display used in this sketch. The library is available at [https://github.com/adafruit/Adafruit_SSD1306].

**WiFi**: This library provides the necessary functions to connect to a WiFi network. It is part of the official ESP32 Arduino core and can be found at [https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi].

**PubSubClient**: This library facilitates MQTT communication and allows the ESP32 to subscribe to MQTT topics and publish messages. The library can be found at [https://github.com/knolleary/pubsubclient].

**Adafruit_NeoPixel**: This library is used for controlling Neopixel LEDs. It provides functions to set the color and brightness of individual LEDs. The library can be found at [https://github.com/adafruit/Adafruit_NeoPixel].

**map**: This library provides a key-value mapping data structure called std::map. It is used to store the lookup tables for signal mast aspects. The library documentation can be found at [https://en.cppreference.com/w/cpp/container/map].

**string**: This library provides the std::basic_string class for working with strings. It is used to store and manipulate message payloads and aspect keys. The library documentation can be found at [https://en.cppreference.com/w/cpp/string/basic_string].

**ArduinoOTA**: This library enables Over-The-Air (OTA) updates for the ESP32. It allows you to upload new firmware to the ESP32 wirelessly. The library is available at [https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA].

## Network Configuration
The sketch requires connecting to a WiFi network. The following configuration parameters need to be set:

-**WIFI_SSID**: The SSID (name) of the WiFi network to connect to.
-**WIFI_PASSWORD**: The password for the WiFi network.

## MQTT Configuration
The sketch uses MQTT for communication with JMRI. The MQTT server details need to be configured:

-**MQTT_SERVER**: The IP address or hostname of the MQTT server.
-**MQTT_PORT**: The port number on which the MQTT server is running.

## Hardware Setup
The sketch assumes the following hardware setup:

ESP32 board with WiFi capabilities.
(7) Neopixel LEDs connected to GPIO pins defined in the neoPixelPins array.
128x64 OLED display connected via I2C.

## Functionality
### setup()
The **setup()** function is responsible for initializing the sketch. It performs the following tasks:

-Initializes serial communication for debugging.
-Connects to the WiFi network specified in the configuration.
-Sets the hostname of the ESP32 to the NodeID.
-Initializes the OTA (Over-The-Air) update functionality.
-Connects to the MQTT broker and subscribes to the relevant topics.
-Initializes the Neopixel signal masts to their default state (stop signal).
-Initializes the OLED display and shows the initial NodeID and IP address.

### loop()
The **loop()** function is the main execution loop of the sketch. It performs the following tasks:

-Handles OTA updates.
-Reconnects to WiFi if the connection is lost.
-Reconnects to the MQTT server if the connection is lost.
-Calls the callback() function to handle incoming MQTT messages.
Updates the OLED display if the NodeID or IP address has changed.

### reconnectMQTT()
The **reconnectMQTT()** function is responsible for establishing a connection to the MQTT server. It attempts to reconnect until the connection is successful. Upon successful connection, it subscribes to the MQTT topics for all signal masts.

### reconnectWiFi()
The **reconnectWiFi()** function is responsible for reconnecting to the WiFi network if the connection is lost. It attempts to reconnect until the connection is successful.

### callback(char* topic, byte* payload, unsigned int length)
The **callback()** function is called when a new message is received on an MQTT topic. It handles the parsing and processing of the message payload. The payload is expected to be in the format 'Aspect; Lit (or Unlit); Unheld (or Held)'.

The function extracts the signal mast number from the topic, parses the payload into aspect, lit, and held strings, and then updates the Neopixel colors based on the aspect and lit status. The function also handles special cases such as unlit signals and held signals.

### updateDisplay()
The **updateDisplay()** function is responsible for updating the OLED display with the current NodeID and IP address. It checks if the NodeID or IP address has changed since the last update and updates the display accordingly.

The function uses the Adafruit SSD1306 library to control the display and displays the NodeID and IP address in a clear and readable format.







