#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <Arduino.h>

class GPSDriver
{
public:

    enum GPSStatus
    {
        GPS_DEMO,
        GPS_REAL,
        GPS_NO_FIX
    };

    GPSDriver(
        int rxPin = 16,
        int txPin = 17,
        uint32_t baudRate = 9600
    );

    bool begin();
    void update();

    double getLatitude();
    double getLongitude();

    bool hasFix();
    bool isSensorConnected();

    GPSStatus getStatus();

    const char* getStatusText();

private:

    HardwareSerial gpsSerial;

    int rxPin;
    int txPin;
    uint32_t baudRate;

    double latitude;
    double longitude;

    bool sensorConnected;
    bool fixAvailable;

    unsigned long lastNMEA;
    unsigned long lastValidFix;

    static constexpr unsigned long
        SENSOR_TIMEOUT = 3000;

    static constexpr unsigned long
        FIX_TIMEOUT = 5000;

    static constexpr double
        DEMO_LATITUDE = 16.494367;

    static constexpr double
        DEMO_LONGITUDE = 80.498942;

    GPSStatus status;

    String nmeaBuffer;

    void processSentence(String sentence);

    bool parseRMC(String sentence);

    bool parseGGA(String sentence);

    double convertCoordinate(
        String value,
        String direction
    );

    bool isValidNMEA(String sentence);
};

#endif