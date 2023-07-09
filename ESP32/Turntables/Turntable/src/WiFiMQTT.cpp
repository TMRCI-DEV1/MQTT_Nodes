#include "WiFiMQTT.h"

// Network and MQTT Related
const char *ssid = "Your_WiFi_SSID";           // SSID of the WiFi network
const char *password = "Your_WiFi_Password";   // Password of the WiFi network
const char *mqtt_broker = "Your_MQTT_Broker";  // MQTT broker address
const int mqtt_port = 1883;                    // MQTT broker port
WiFiClient espClient;                          // WiFiClient object for MQTT communication
PubSubClient client(espClient);                // PubSubClient object for MQTT communication

/*
  Function to connect to WiFi. This function uses a while loop to wait for the connection to be established.
  If the connection fails, the function will retry the connection.
*/
void connectToWiFi() {
  // Begin WiFi connection with the given ssid and password
  WiFi.begin(ssid, password);

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
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(ipAddressString);
  } else {
    // If WiFi connection failed, print an error message and wait 5 seconds before retrying
    Serial.println("Failed to connect to WiFi");
    delay(5000);
  }
}

/*
  Function to connect to MQTT. This function uses a while loop to wait for the connection to be established.
  If the connection fails, the function will retry the connection.
*/
void connectToMQTT() {
  client.setServer(mqtt_broker, mqtt_port);  // Connect to MQTT broker

  // Loop until MQTT connection is established
  while (!client.connected()) {
    // Check if WiFi connection is lost and reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }

    // Attempt to connect to the MQTT broker with address and port
    if (client.connect("ESP32Client")) {    // ESP32Client is the Client ID
      Serial.println("Connected to MQTT");
      client.setCallback(callback);         // Set the callback function
      client.subscribe(MQTT_TOPIC);         // Subscribe to the MQTT topic
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

/* 
  MQTT callback function to handle incoming messages
  This function uses a char array to store the MQTT message because the payload is received as a byte array, and converting it to a char array makes it easier to work with.
  strncpy is used to extract the track number from the MQTT message because it allows for copying a specific number of characters from a string.
*/
void callback(char *topic, byte *payload, unsigned int length) {
  // Print the received MQTT topic
  Serial.print("Received MQTT topic: ");
  Serial.println(topic);

  // Find the position of "Track" in the topic string
  char *trackPosition = strstr(topic, "Track");
  if (trackPosition == NULL) {
    Serial.println("Invalid MQTT topic: 'Track' not found");
    return;
  }

  // Extract the track number and end (head or tail) from the MQTT topic
  char mqttTrackNumber[3];
  strncpy(mqttTrackNumber, trackPosition + 5, 2);  // Extract track number from MQTT topic.
  mqttTrackNumber[2] = '\0';                       // Null-terminate the char array.

  int trackNumber = atoi(mqttTrackNumber);         // Convert track number to integer.

  if (trackNumber > NUMBER_OF_TRACKS || trackNumber < 1) {
    Serial.println("Invalid track number received in MQTT topic");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid track number received in MQTT topic");
    return;
  }

  int endNumber = (trackPosition[7] == 'H') ? 0 : 1;  // Determine if it's the head or tail end.
  int targetPosition = calculateTargetPosition(trackNumber, endNumber);  // Calculate target position.
  moveToTargetPosition(targetPosition);               // Move to the target position.

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Track selected:");
  lcd.setCursor(0, 1);
  lcd.print(trackNumber);
  lcd.setCursor(0, 2);
  lcd.print("Position:");
  lcd.setCursor(0, 3);
  lcd.print(targetPosition);
}