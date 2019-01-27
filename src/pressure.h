/* =============================================================================
   Pressure.h
   Written in 2019 by Tim Jacobs
  ============================================================================= */

#ifndef __COMFOAIR_ARDUINO_PRESSURE_H
#define __COMFOAIR_ARDUINO_PRESSURE_H

#include <Arduino.h> 
#include "SDP.h"
#include "Wire.h"

// Function declarations
void pressureInit();
void pressureMeasure();

#endif
