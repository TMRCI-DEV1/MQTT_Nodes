#ifndef EEPROMCONFIG_H
#define EEPROMCONFIG_H

#include "Turntable.h"

#include <EEPROM.h>            // Include the EEPROM library to enable read/write operations on the ESP32's EEPROM. This is used for storing the current position and track head/tail positions.
                               // https://github.com/espressif/arduino-esp32/tree/master/libraries/EEPROM
// EEPROM Related
const int EEPROM_TRACK_HEADS_ADDRESS = 100; // Starting address in EEPROM to store track heads.
const int EEPROM_TOTAL_SIZE_BYTES = 4096; // Total size of the EEPROM in bytes.

extern void printToLCD(int row,
  const char * message); // Declare the printToLCD function

/* This function calculates and returns the EEPROM address for storing track tail positions.
   It does this by adding the size (in bytes) of the total number of track heads to the 
   starting address of the track heads in the EEPROM. The result is the starting address 
   for the track tails in the EEPROM. */
int getEEPROMTrackTailsAddress() {
  return EEPROM_TRACK_HEADS_ADDRESS + NUMBER_OF_TRACKS * sizeof(int);
}

/* Function to write data to EEPROM with error checking. This function uses a template to allow for writing of different data types to EEPROM.
   memcmp is used for data verification to ensure that the data written to EEPROM is the same as the data intended to be written. */
template < typename T >
  void writeToEEPROMWithVerification(int address,
    const T & value) {
    // Define maximum number of write retries
    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool writeSuccess = false;

    // Retry writing to EEPROM until successful or maximum retries reached
    while (retryCount < MAX_RETRIES && !writeSuccess) {
      T originalValue;
      EEPROM.get(address, originalValue); // Read original value from EEPROM.
      EEPROM.put(address, value); // Write new value to EEPROM.
      EEPROM.commit();
      delay(10);

      T readValue;
      EEPROM.get(address, readValue); // Read the written value from EEPROM.

      // If the new value and read values are not the same, increment retry count and delay before retrying
      if (memcmp( & value, & readValue, sizeof(T)) != 0) {
        retryCount++;
        delay(500);
      } else {
        writeSuccess = true;
      }
    }

    // If writing to EEPROM failed, print an error message
    if (!writeSuccess) {
      Serial.println("EEPROM write error!");
      printToLCD(0, "EEPROM write error!"); // Print error to LCD
    }
  }

/* Function to read data from EEPROM with error checking. This function uses a template to allow for reading of different data types from EEPROM.
   memcmp is used for data verification to ensure that the data read from EEPROM is the same as the data stored. */
template < typename T >
  bool readFromEEPROMWithVerification(int address, T & value) {
    const int MAX_RETRIES = 3;
    int retryCount = 0;
    bool readSuccess = false;

    while (retryCount < MAX_RETRIES && !readSuccess) {
      T readValue;
      EEPROM.get(address, readValue);

      if (memcmp( & value, & readValue, sizeof(T)) != 0) {
        retryCount++;
        delay(500);
      } else {
        for (int i = 0; i < sizeof(T) / sizeof(T[0]); i++) {
          value[i] = readValue[i];
        }
        readSuccess = true;
      }
    }

    if (!readSuccess) {
      Serial.println("EEPROM read error!");
    }

    return readSuccess;
  }

#endif // EEPROMCONFIG_H