#ifndef INTERNET_DRIVER_H
#define INTERNET_DRIVER_H

#include <Arduino.h>

class InternetDriver
{
public:

    InternetDriver(
        const char* ssid,
        const char* password
    );

    void begin();

    bool isConnected();

    bool sendAccidentAlert(
        const String &vehicleID,
        double latitude,
        double longitude,
        const String &severity
    );

private:

    const char* ssid;
    const char* password;

    void connectWiFi();
};

#endif