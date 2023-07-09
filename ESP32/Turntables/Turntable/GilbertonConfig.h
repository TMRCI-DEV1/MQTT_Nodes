#ifndef GILBERTON_CONFIG_H
#define GILBERTON_CONFIG_H

// External declarations
extern const char * MQTT_TOPIC;
extern const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

#ifdef GILBERTON
// MQTT topic for Gilberton turntable
const char * MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#";

// Number of tracks in Gilberton
const int NUMBER_OF_TRACKS = 23;

// Track numbers in Gilberton
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

// Pointer to the array of track numbers
int * TRACK_NUMBERS = gilbertonTrackNumbers;
#endif

#endif
