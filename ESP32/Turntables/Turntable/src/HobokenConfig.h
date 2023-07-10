#ifndef HOBOKEN_CONFIG_H
#define HOBOKEN_CONFIG_H

// External declarations for MQTT topic, number of tracks, and track numbers
extern const char * MQTT_TOPIC;
extern const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

// Define the hostname for the Hoboken turntable. This is used for identifying the turntable on the network.
const char* HOSTNAME = "Hoboken_Turntable_Node";

#ifdef HOBOKEN
// Define the MQTT topic for the Hoboken turntable. The '#' wildcard allows the turntable to receive messages for any subtopic under "TMRCI/output/Hoboken/turntable/".
const char * MQTT_TOPIC = "TMRCI/output/Hoboken/turntable/#";

// Define the number of tracks on the Hoboken turntable. This is used for validating track numbers received from MQTT messages and keypad inputs.
const int NUMBER_OF_TRACKS = 22;

// Define an array of track numbers for the Hoboken turntable. This is used for mapping track numbers to physical positions on the turntable.
int hobokenTrackNumbers[] = {
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
  22
};

// Assign the pointer to the array of track numbers for the Hoboken turntable. This allows the turntable code to access the track numbers array.
int * TRACK_NUMBERS = hobokenTrackNumbers;
#endif

#endif