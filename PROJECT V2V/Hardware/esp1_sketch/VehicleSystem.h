#ifndef VEHICLE_SYSTEM_H
#define VEHICLE_SYSTEM_H

#include <Arduino.h>

#include "MPUDriver.h"
#include "GPSDriver.h"
#include "InternetDriver.h"
#include "ESPNowDriver.h"


class VehicleSystem
{
public:

    enum SystemState
    {
        MONITORING,
        ACCIDENT_SUSPECTED,
        ACCIDENT_CONFIRMED,
        SYSTEM_LOCKED
    };


    VehicleSystem(
        MPUDriver &mpu,
        GPSDriver &gps,
        InternetDriver &internet,
        ESPNowDriver &espNow
    );


    void begin();

    void update();

    SystemState getState();

    void cancelAccident();

    void confirmAccident();

    void setMPUAvailable(bool available);


private:

    MPUDriver &mpu;

    GPSDriver &gps;

    InternetDriver &internet;

    ESPNowDriver &espNow;


    bool mpuAvailable;

    SystemState state;


    void handleMonitoring();

    void handleAccidentSuspected();

    void handleAccidentConfirmed();


    // PERIODIC LED + BUZZER ALERT
    void updatePhysicalAlert();


    void printAccidentDetails();

    void printGPSData();

    void printState();


    // USB SERIAL COMMANDS
    void checkSerialCommand();


    // ESP-NOW receive callback
    static void onESPNowReceive(
        const uint8_t *mac,
        const uint8_t *data,
        size_t len
    );
};

#endif