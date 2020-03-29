/* =============================================================================
   Main.h
   Written in 2020 by Tim Jacobs
  ============================================================================= */

#ifndef __COMFOAIR_ARDUINO_MAIN_H
#define __COMFOAIR_ARDUINO_MAIN_H

#include <Arduino.h>

/* --------------------------------------------------------------------------
   Data handling
   -------------------------------------------------------------------------- */

typedef struct _dataManager {
    uint16_t t_comfort_count;           // Total number of datapoints collected for RS232
    float t_comfort_sum;                // Sum thusfar
    uint16_t t1_count;
    float t1_intake_sum;
    uint16_t t2_count;
    float t2_tohome_sum;
    uint16_t t3_count;
    float t3_fromhome_sum;
    uint16_t t4_count;
    float t4_exhaust_sum;
    uint16_t air_count;                 // Total number of datapoints collected for DP1
    double airPressure_sum;
    double airTemperature_sum; 
} dataManager;

void resetMeasurements();
bool generateOutput(String &outputString );

#endif
