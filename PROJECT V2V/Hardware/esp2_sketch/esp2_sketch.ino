#include "MPUDriver.h"
#include "GPSDriver.h"
#include "InternetDriver.h"
#include "ESPNowDriver.h"
#include "VehicleSystem.h"


// =====================================================
// WIFI SETTINGS
// =====================================================

const char* WIFI_SSID =

    //"LAPTOPLENOVO3I";

    "Realme00";

const char* WIFI_PASSWORD =

    //"4S89:x28";

    "735021ram";


// =====================================================
// DRIVERS
// =====================================================

MPUDriver mpu;


GPSDriver gps(
    16,
    17,
    9600
);


InternetDriver internet(
    WIFI_SSID,
    WIFI_PASSWORD
);


// ESP-NOW channel
ESPNowDriver espNow;


// =====================================================
// VEHICLE SYSTEM
// =====================================================

VehicleSystem vehicleSystem(
    mpu,
    gps,
    internet,
    espNow
);


// =====================================================
// MPU STATUS
// =====================================================

bool mpuAvailable = false;


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);


    Serial.println();
    Serial.println("================================");
    Serial.println("       V2V SYSTEM STARTING");
    Serial.println("================================");


    // =================================================
    // MPU
    // =================================================

    Serial.println();
    Serial.println("Checking MPU6050...");


    if (!mpu.begin())
    {
        Serial.println();
        Serial.println("MPU NOT DETECTED");
        Serial.println(
            "Running in RECEIVER / DEMO mode."
        );

        Serial.println(
            "Local accident detection disabled."
        );

        Serial.println(
            "ESP-NOW communication remains active."
        );

        mpuAvailable = false;
    }
    else
    {
        Serial.println();
        Serial.println("MPU INITIALIZED");

        mpuAvailable = true;
    }


    // =================================================
    // GPS
    // =================================================

    Serial.println();
    Serial.println("Starting GPS...");

    gps.begin();


    // =================================================
    // INTERNET
    // =================================================

    Serial.println();
    Serial.println("Starting Internet Driver...");

    internet.begin();


    // =================================================
    // ESP-NOW
    // =================================================

    Serial.println();
    Serial.println("Starting ESP-NOW...");


    if (!espNow.begin())
    {
        Serial.println();
        Serial.println(
            "ESP-NOW INITIALIZATION FAILED"
        );

        while (true)
        {
            delay(1000);
        }
    }


    // =================================================
    // VEHICLE SYSTEM
    // =================================================

    /*
       Tell VehicleSystem whether this ESP32
       actually has an MPU connected.

       ESP1:
       MPU detected -> local accident detection ON

       ESP2:
       MPU not detected -> receiver/demo mode
    */

    vehicleSystem.setMPUAvailable(
        mpuAvailable
    );


    vehicleSystem.begin();


    // =================================================
    // READY
    // =================================================

    Serial.println();
    Serial.println("================================");
    Serial.println("          SYSTEM READY");
    Serial.println("================================");

    if (mpuAvailable)
    {
        Serial.println(
            "Mode: FULL VEHICLE"
        );

        Serial.println(
            "MPU accident detection: ON"
        );
    }
    else
    {
        Serial.println(
            "Mode: V2V RECEIVER / DEMO"
        );

        Serial.println(
            "MPU accident detection: OFF"
        );
    }


    Serial.println(
        "ESP-NOW: ACTIVE"
    );

    Serial.println(
        "Waiting for vehicles..."
    );

    Serial.println(
        "================================");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    vehicleSystem.update();

    delay(10);
}