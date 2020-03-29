/* =============================================================================
   Pressure.cpp
   Written in 2019 by Tim Jacobs
  ============================================================================= */

#include "pressure.h"
#include "mqtt.h"
#include "main.h"

// Measurement store
extern dataManager ourMeasurements;

SDP_Controller SDP810;
double measure_diffpressure = 0;
double measure_temperature = 0; 

bool pressureInit() {
    // Initialize I2C
    DEBUGOUT.println(F("Init I2C communications..."));
    Wire.begin();

    // Initialize pressure sensor + trigger measurement mode on sensor
    DEBUGOUT.println(F("Init SDP810 pressure sensor..."));
    if(!SDP810.begin()) {
        return false;
    }
    SDP810.startContinuousMeasurement(SDP_TEMPCOMP_DIFFERENTIAL_PRESSURE, SDP_AVERAGING_TILL_READ);
    return true;
}

void pressureMeasure() {
    DEBUGOUT.println(F("Measurement (SDP810)..."));
    measure_diffpressure = SDP810.getDiffPressureTrigger(SDP_TEMPCOMP_DIFFERENTIAL_PRESSURE, SDP_CLKST_NONE);
    measure_temperature = SDP810.getTemperature();
    DEBUGOUT.print(F("-> Diff.Pressure : ")); DEBUGOUT.println(measure_diffpressure,6);
    DEBUGOUT.print(F("-> Temperature   : ")); DEBUGOUT.println(measure_temperature,6);

    ourMeasurements.airPressure_sum += measure_diffpressure;
    ourMeasurements.airTemperature_sum += measure_temperature;
    ourMeasurements.air_count += 1;
}