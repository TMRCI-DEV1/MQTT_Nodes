#ifndef GILBERTON_CONFIG_H
#define GILBERTON_CONFIG_H

extern
const char * MQTT_TOPIC;
extern
const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

#ifdef GILBERTON
const char * MQTT_TOPIC = "TMRCI/output/Gilberton/turntable/#"; // MQTT topic for Gilberton turntable
const int NUMBER_OF_TRACKS = 23; // Number of tracks in Gilberton
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
}; // Track numbers in Gilberton
int * TRACK_NUMBERS = gilbertonTrackNumbers; // Pointer to the array of track numbers
#endif

#endif
