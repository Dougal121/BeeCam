#pragma once
#include "esp_camera.h"

typedef struct __attribute__((__packed__)) {     // eeprom stuff
  long frameInterval;
  long postSunrise;
  long numberSamples;
  long currentSample;
  long lastFrameTime;
  bool lapseRunning;
} cam_stuff_t ;          // computer says it's  ??? is my maths crap ????

bool initCamera();
