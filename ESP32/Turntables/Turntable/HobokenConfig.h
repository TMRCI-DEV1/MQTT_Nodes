#ifndef HOBOKEN_CONFIG_H
#define HOBOKEN_CONFIG_H

extern
const char * MQTT_TOPIC;
extern
const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

#ifdef HOBOKEN
const char * MQTT_TOPIC = "TMRCI/output/Hoboken/turntable/#"; // MQTT topic for Hoboken turntable
const int NUMBER_OF_TRACKS = 22; // Number of tracks in Hoboken
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
}; // Track numbers in Hoboken
int * TRACK_NUMBERS = hobokenTrackNumbers; // Pointer to the array of track numbers
#endif

#endif
