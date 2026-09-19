#include "MPUDriver.h"
#include "GPSDriver.h"
#include "InternetDriver.h"
#include "ESPNowDriver.h"
#include "VehicleSystem.h"


// =====================================================
// WI-FI SETTINGS
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
    16,     // ESP32 RX2 ← GPS TX
    17,     // ESP32 TX2 → GPS RX
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
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);


    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "       V2V SYSTEM STARTING"
    );

    Serial.println(
        "================================"
    );


    // =================================================
    // MPU
    // =================================================

    if (!mpu.begin())
    {
        Serial.println(
            "MPU INITIALIZATION FAILED"
        );

        while (true)
        {
            delay(1000);
        }
    }


    // =================================================
    // TELL VEHICLE SYSTEM MPU IS AVAILABLE
    // =================================================

    vehicleSystem.setMPUAvailable(true);


    // =================================================
    // GPS
    // =================================================

    gps.begin();


    // =================================================
    // INTERNET
    // =================================================

    internet.begin();


    // =================================================
    // ESP-NOW
    // =================================================

    if (!espNow.begin())
    {
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

    vehicleSystem.begin();


    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "       SYSTEM READY"
    );

    Serial.println(
        "================================"
    );
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    vehicleSystem.update();

    delay(10);
}