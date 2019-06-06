
#include "mqtt.h"
#include "network.h"
#include "zehnder.h"
#include "pressure.h"

/* --------------------------------------------------------------------------
   Definitions
   -------------------------------------------------------------------------- */

#define ARDUINO_BAUDRATE 9600             // debug output port baud rate
#define MEASURE_INTERVAL_PRESSURE 1000    // Measure pressure every 1s

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
        if(millis() > prevMeasureTimePressure + MEASURE_INTERVAL_PRESSURE) {
            pressureMeasure();
            prevMeasureTimePressure = millis();
        }
    }

}
