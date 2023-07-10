#ifndef GILBERTON_CONFIG_H
#define GILBERTON_CONFIG_H

// External declarations for MQTT topic, number of tracks, and track numbers
extern const char * MQTT_TOPIC;
extern const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

// Define the hostname for the Gilberton turntable. This is used for identifying the turntable on the network.
const char* HOSTNAME = "Gilberton_Turntable_Node";

#ifdef GILBERTON
// Define the MQTT topic for the Gilberton turntable. The '#' wildcard allows the turntable to receive messages for any subtopic under "TMRCI/output/Gilberton/turntable/".
const char * MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#";

// Define the number of tracks on the Gilberton turntable. This is used for validating track numbers received from MQTT messages and keypad inputs.
const int NUMBER_OF_TRACKS = 22;

// Define an array of track numbers for the Gilberton turntable. This is used for mapping track numbers to physical positions on the turntable.
int gilbertonTrackNumbers[] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23
};

// Assign the pointer to the array of track numbers for the Gilberton turntable. This allows the turntable code to access the track numbers array.
int * TRACK_NUMBERS = gilbertonTrackNumbers;
#endif

#endif