
#include "mqtt.h"
#include "network.h"
#include "zehnder.h"
#include "pressure.h"
#include "main.h"
#include <ArduinoJson.h>

/* --------------------------------------------------------------------------
   Definitions
   -------------------------------------------------------------------------- */

#define ARDUINO_BAUDRATE 9600             // debug output port baud rate
#define MEASURE_INTERVAL_PRESSURE 1000    // Measure pressure every 1s
#define SEND_INTERVAL 60000               // Send MQTT payloads every 60s

/* --------------------------------------------------------------------------
   Main variables
   -------------------------------------------------------------------------- */

// Network parameters
byte netMAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress netIP(172, 16, 0, 70);
IPAddress netDNS(171, 16, 0, 1);
IPAddress netGW(172, 16, 0, 1);
IPAddress netSubNet(255, 255, 255, 0);

// The IP address of the server we're connecting to:
IPAddress netMQTTServer_IP(172, 16, 0, 6);
const char* netMQTTServer_DNS = "mqtt.home.local";

// Sensors detected
uint8_t sensorsPresent = 0;
#define SENSOR_RS232 1
#define SENSOR_DP1 2


// Time keeping
uint32_t lastSendTime = 0;

/* --------------------------------------------------------------------------
   Data handling
   -------------------------------------------------------------------------- */

dataManager ourMeasurements;

/* ===========================================================================
   ===========================================================================
   ===========================================================================
   MAIN SETUP
   ===========================================================================
   ===========================================================================
   =========================================================================== */

void setup() {
#if defined(ARDUINO_ARCH_SAMD)
    while(!DEBUGOUT);
#endif
    DEBUGOUT.begin(ARDUINO_BAUDRATE);
    DEBUGOUT.println("");
    DEBUGOUT.println(F("=== SETUP START ==="));

    // Init serial port reader...
    if(zehnderInit()) {
        sensorsPresent |= SENSOR_RS232;
    } else {
        DEBUGOUT.println(F("-> ERROR: Could not initialize Zehnder RS232 communications..."));
    }

    // Initialize pressure sensor
    if(pressureInit()) {
        sensorsPresent |= SENSOR_DP1;
    } else {
        DEBUGOUT.println(F("-> ERROR: Could not initialize Sensirion SDP810..."));
    };

    // Initialize MQTTClient
    mqttInit();

    // Initialize EthernetClient
    networkInit();

    // Connect to MQTT server
    mqttMaintain();

    // Clear measurement data
    resetMeasurements();

    DEBUGOUT.println(F("=== SETUP DONE ==="));
    pinMode(LED_BUILTIN, OUTPUT);
}


/* ===========================================================================
   ===========================================================================
   ===========================================================================
   MAIN LOOP
   ===========================================================================
   ===========================================================================
   =========================================================================== */


uint32_t prevMeasureTimePressure = 0;

void loop() {
    // Get time
    uint32_t timeNow = millis();

    // Maintain Ethernet connection (DHCP lease)
    Ethernet.maintain();
    
    // Maintain MQTT server connection
    mqttMaintain();

    // New data available at the serial port?
    if ((sensorsPresent & SENSOR_RS232) > 0) {
        checkCommand();
    }

    // Perform measurement of differential pressure?
    if ((sensorsPresent & SENSOR_DP1) > 0) {
        // Time for a new measurement?
        if(timeNow - prevMeasureTimePressure > MEASURE_INTERVAL_PRESSURE) {
            pressureMeasure();
            prevMeasureTimePressure = timeNow;
        }
    }

    // Time to publish new data?
    if (timeNow - lastSendTime > SEND_INTERVAL) {
        // Send data
        String dataOutput = "";
        if (generateOutput(dataOutput) ) {
            DEBUGOUT.print("OUTPUT JSON: "); DEBUGOUT.println(dataOutput);
            mqttPublishData( dataOutput );
        }

        // Reset measurement data
        resetMeasurements();

        // Reset timer
        lastSendTime = timeNow;
    }
}

/* ===========================================================================
   ===========================================================================
   ===========================================================================
   DATA HELPER FUNCTIONS
   ===========================================================================
   ===========================================================================
   =========================================================================== */

void resetMeasurements() {
    ourMeasurements.air_count = 0;
    ourMeasurements.airPressure_sum = 0;
    ourMeasurements.airTemperature_sum = 0;
    ourMeasurements.t_comfort_count = 0;
    ourMeasurements.t_comfort_sum = 0;
    ourMeasurements.t1_count = 0;
    ourMeasurements.t1_intake_sum = 0;
    ourMeasurements.t2_count = 0;
    ourMeasurements.t2_tohome_sum = 0;
    ourMeasurements.t3_count = 0;
    ourMeasurements.t3_fromhome_sum = 0;
    ourMeasurements.t4_count = 0;
    ourMeasurements.t4_exhaust_sum = 0;
}

bool generateOutput(String &outputString ) {
    const int jsonCapacity = JSON_OBJECT_SIZE(8);
    StaticJsonDocument<jsonCapacity> measurementsJSON;              // JsonDocuments are not meant to be reused, so reallocate every time! https://arduinojson.org/v6/how-to/reuse-a-json-document/
    outputString = "";

    // Create JsonDocument
    if(ourMeasurements.t_comfort_count > 0) {
        measurementsJSON["t_comfort"] = (ourMeasurements.t_comfort_sum * 1.0) / ourMeasurements.t_comfort_count;
    }
    if(ourMeasurements.t1_count > 0) {
        measurementsJSON["t1_intake"] = (ourMeasurements.t1_intake_sum * 1.0) / ourMeasurements.t1_count;
    }   
    if(ourMeasurements.t2_count > 0) {
        measurementsJSON["t2_tohome"] = (ourMeasurements.t2_tohome_sum * 1.0) / ourMeasurements.t2_count;
    }   
    if(ourMeasurements.t3_count > 0) {
        measurementsJSON["t3_fromhome"] = (ourMeasurements.t3_fromhome_sum * 1.0) / ourMeasurements.t3_count;
    }   
    if(ourMeasurements.t4_count > 0) {
        measurementsJSON["t4_exhaust"] = (ourMeasurements.t4_exhaust_sum * 1.0) / ourMeasurements.t4_count;
    }
    if(ourMeasurements.air_count > 0) {
        measurementsJSON["t_pressure"] = (ourMeasurements.airTemperature_sum * 1.0) / ourMeasurements.air_count;
        measurementsJSON["diffpressure"] = (ourMeasurements.airPressure_sum * 1.0) / ourMeasurements.air_count;
    }
    // Serialize
    if (!measurementsJSON.isNull()) {
        measurementsJSON["msgtype"] = "measurement";
        if( serializeJson(measurementsJSON, outputString) > 0) {
            return true;
        }
    }
    return false;
}