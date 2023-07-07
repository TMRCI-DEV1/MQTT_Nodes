#ifndef PITTSBURGH_CONFIG_H
#define PITTSBURGH_CONFIG_H

extern
const char * MQTT_TOPIC;
extern
const int NUMBER_OF_TRACKS;
extern int * TRACK_NUMBERS;

#ifdef PITTSBURGH
const char * MQTT_TOPIC = "TMRCI/output/Pittsburgh/turntable/#"; // MQTT topic for Pittsburgh turntable
const int NUMBER_OF_TRACKS = 22; // Number of tracks in Pittsburgh
int pittsburghTrackNumbers[] = {
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
}; // Track numbers in Pittsburgh
int * TRACK_NUMBERS = pittsburghTrackNumbers; // Pointer to the array of track numbers
#endif

#endif
