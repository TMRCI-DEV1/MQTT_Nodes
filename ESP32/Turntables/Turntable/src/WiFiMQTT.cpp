#include "WiFiMQTT.h"

/* Definitions of variables declared in WiFiMQTT.h */
// Network and MQTT Related
const char * ssid = "MyAltice 976DFF"; // SSID (network name) of the WiFi network to which the ESP32 will connect.
const char * password = "lemon.463.loud"; // Password for the WiFi network.
const char * mqtt_broker = "129.213.106.87"; // Address (IP or domain name) of the MQTT broker to which the ESP32 will connect.
const int mqtt_port = 1883; // Port number for the MQTT broker. The standard port for MQTT is 1883.
WiFiClient espClient; // WiFiClient object used as the network client for the MQTT connection.
PubSubClient client(espClient); // PubSubClient object used for MQTT communication.

/* Function to connect to WiFi. This function uses a while loop to wait for the connection to be established.
   If the connection fails, the function will retry the connection. This is important to ensure that the ESP32 can always connect to the WiFi network,
   even if the first attempt fails (for example, due to network issues or the router being temporarily unavailable). */
void connectToWiFi() {
  // Begin WiFi connection with the given ssid and password
  WiFi.begin(ssid, password);

  // Set the hostname based on the location-specific file
  WiFi.setHostname(HOSTNAME);

  // Wait until WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  // If WiFi connection is successful, print the IP address to the serial monitor and the LCD display
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");

    // Get the IP address and convert it to a string
    IPAddress ipAddress = WiFi.localIP();
    String ipAddressString = ipAddress.toString();

    // Print the IP address to the serial monitor
    Serial.print("IP Address: ");
    Serial.println(ipAddressString);

    // Print the IP address to the LCD display
    printToLCD(0, "IP Address:");
    printToLCD(1, ipAddressString.c_str());
  } else {
    // If WiFi connection failed, print an error message and wait 5 seconds before retrying
    Serial.println("Failed to connect to WiFi");
    delay(5000);
  }
}

/* Function to connect to MQTT. This function uses a while loop to wait for the connection to be established.
   If the connection fails, the function will retry the connection. This is important to ensure that the ESP32 can always connect to the MQTT broker,
   even if the first attempt fails (for example, due to network issues or the broker being temporarily unavailable). */
void connectToMQTT() {
  client.setServer(mqtt_broker, mqtt_port); // Connect to MQTT broker.

  // Loop until MQTT connection is established
  while (!client.connected()) {
    // Check if WiFi connection is lost and reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }

    // Attempt to connect to the MQTT broker with address and port
    if (client.connect("ESP32Client")) { // ESP32Client is the Client ID.
      Serial.println("Connected to MQTT");
      client.setCallback(callback); // Set the callback function.
      client.subscribe(MQTT_TOPIC); // Subscribe to the MQTT topic.
    } else {
      // If MQTT connection failed, print an error message and retry after a delay
      Serial.print("Failed to connect to MQTT. Retrying in 2 seconds... ");
      delay(2000);
    }
  }

  // Check MQTT connection status and print success/failure message
  if (client.connected()) {
    Serial.println("Connected to MQTT");
  } else {
    Serial.println("Failed to connect to MQTT");
    delay(5000);
  }
}

/* MQTT callback function to handle incoming messages.
   This function uses a char array to store the MQTT message because the payload is received as a byte array, and converting it to a char array makes it easier to work with.
   strncpy is used to extract the track number from the MQTT message because it allows for copying a specific number of characters from a string.
   This function is called whenever an MQTT message is received on the subscribed topic. It parses the message to extract the track number and end (head or tail),
   calculates the target position based on this information, and moves the turntable to the target position. */
void callback(char * topic, byte * payload, unsigned int length) {
  // Print the received MQTT topic
  Serial.print("Received MQTT topic: ");
  Serial.println(topic);

  // Find the position of "Track" in the topic string
  char * trackPosition = strstr(topic, "Track");
  if (trackPosition == NULL) {
    Serial.println("Invalid MQTT topic: 'Track' not found");
    return;
  }

  // Extract the track number and end (head or tail) from the MQTT topic
  char mqttTrackNumber[3];
  strncpy(mqttTrackNumber, trackPosition + 5, 2); // Extract track number from MQTT topic.
  mqttTrackNumber[2] = '\0'; // Null-terminate the char array.

  int trackNumber = atoi(mqttTrackNumber); // Convert track number to integer.

  if (trackNumber > NUMBER_OF_TRACKS || trackNumber < 1) {
    Serial.println("Invalid track number received in MQTT topic");
    printToLCD(0, "Invalid track number received in MQTT topic");
    return;
  }

  int endNumber = (trackPosition[7] == 'H') ? 0 : 1; // Determine if it's the head or tail end.
  int targetPosition = calculateTargetPosition(trackNumber, endNumber); // Calculate target position.
  moveToTargetPosition(targetPosition); // Move to the target position.

  // Update the LCD display with the selected track information
  clearLCD(); // Clear LCD.
  printToLCD(0, "Track selected:"); // Print the static message "Track selected:" to the first row of the LCD.
  printToLCD(1, String(trackNumber).c_str()); // Convert the track number to a string and print it to the second row of the LCD.
  printToLCD(2, "Position:"); // Print the static message "Position:" to the third row of the LCD.
  printToLCD(3, String(targetPosition).c_str()); // Convert the target position to a string and print it to the fourth row of the LCD.
}