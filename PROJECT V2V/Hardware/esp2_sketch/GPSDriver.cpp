
#include "GPSDriver.h"

// =====================================================
// CONSTRUCTOR
// =====================================================

GPSDriver::GPSDriver(
    int rxPin,
    int txPin,
    uint32_t baudRate
)
    : gpsSerial(2),
      rxPin(rxPin),
      txPin(txPin),
      baudRate(baudRate),
      latitude(DEMO_LATITUDE),
      longitude(DEMO_LONGITUDE),
      sensorConnected(false),
      fixAvailable(false),
      lastNMEA(0),
      lastValidFix(0),
      status(GPS_DEMO)
{
}

// =====================================================
// BEGIN
// =====================================================

bool GPSDriver::begin()
{
    gpsSerial.begin(
        baudRate,
        SERIAL_8N1,
        rxPin,
        txPin
    );

    return true;
}

// =====================================================
// UPDATE
// =====================================================

void GPSDriver::update()
{
    while (gpsSerial.available())
    {
        char c = gpsSerial.read();

        if (c == '$')
        {
            nmeaBuffer = "$";
        }
        else if (nmeaBuffer.length() > 0)
        {
            nmeaBuffer += c;

            if (c == '\n')
            {
                processSentence(nmeaBuffer);
                nmeaBuffer = "";
            }
        }
    }

    // -------------------------------------------------
    // GPS SENSOR CONNECTION STATUS
    // -------------------------------------------------

    if (millis() - lastNMEA > SENSOR_TIMEOUT)
    {
        sensorConnected = false;
        fixAvailable = false;

        // Keep fallback coordinates ready.
        latitude = DEMO_LATITUDE;
        longitude = DEMO_LONGITUDE;

        status = GPS_DEMO;

        return;
    }

    sensorConnected = true;

    // -------------------------------------------------
    // VALID GPS FIX
    // -------------------------------------------------

    if (millis() - lastValidFix <= FIX_TIMEOUT)
    {
        fixAvailable = true;
        status = GPS_REAL;
    }

    // -------------------------------------------------
    // GPS CONNECTED BUT NO VALID FIX
    // -------------------------------------------------

    else
    {
        fixAvailable = false;

        // Internally remember that the sensor is connected
        // but currently has no valid position.
        status = GPS_NO_FIX;

        // Use fallback coordinates.
        latitude = DEMO_LATITUDE;
        longitude = DEMO_LONGITUDE;
    }
}

// =====================================================
// PROCESS NMEA SENTENCE
// =====================================================

void GPSDriver::processSentence(String sentence)
{
    sentence.trim();

    if (!isValidNMEA(sentence))
    {
        return;
    }

    lastNMEA = millis();
    sensorConnected = true;

    // -------------------------------------------------
    // RMC
    // -------------------------------------------------

    if (
        sentence.startsWith("$GPRMC") ||
        sentence.startsWith("$GNRMC")
    )
    {
        parseRMC(sentence);
    }

    // -------------------------------------------------
    // GGA
    // -------------------------------------------------

    else if (
        sentence.startsWith("$GPGGA") ||
        sentence.startsWith("$GNGGA")
    )
    {
        parseGGA(sentence);
    }
}

// =====================================================
// PARSE RMC
// =====================================================

bool GPSDriver::parseRMC(String sentence)
{
    String fields[12];

    int field = 0;
    int start = 0;

    for (int i = 0; i <= sentence.length(); i++)
    {
        if (
            sentence[i] == ',' ||
            sentence[i] == '*'
        )
        {
            if (field < 12)
            {
                fields[field] =
                    sentence.substring(
                        start,
                        i
                    );
            }

            field++;
            start = i + 1;
        }
    }

    if (field < 7)
    {
        return false;
    }

    // -------------------------------------------------
    // RMC STATUS
    // A = valid
    // V = invalid
    // -------------------------------------------------

    if (fields[2] != "A")
    {
        return false;
    }

    if (
        fields[3].length() == 0 ||
        fields[5].length() == 0
    )
    {
        return false;
    }

    double newLatitude =
        convertCoordinate(
            fields[3],
            fields[4]
        );

    double newLongitude =
        convertCoordinate(
            fields[5],
            fields[6]
        );

    if (
        newLatitude == 0 &&
        newLongitude == 0
    )
    {
        return false;
    }

    latitude = newLatitude;
    longitude = newLongitude;

    lastValidFix = millis();

    fixAvailable = true;
    sensorConnected = true;

    status = GPS_REAL;

    return true;
}

// =====================================================
// PARSE GGA
// =====================================================

bool GPSDriver::parseGGA(String sentence)
{
    String fields[10];

    int field = 0;
    int start = 0;

    for (int i = 0; i <= sentence.length(); i++)
    {
        if (
            sentence[i] == ',' ||
            sentence[i] == '*'
        )
        {
            if (field < 10)
            {
                fields[field] =
                    sentence.substring(
                        start,
                        i
                    );
            }

            field++;
            start = i + 1;
        }
    }

    if (field < 7)
    {
        return false;
    }

    // -------------------------------------------------
    // GGA FIX QUALITY
    // 1 = GPS fix
    // 2 = DGPS fix
    // -------------------------------------------------

    if (
        fields[6] != "1" &&
        fields[6] != "2"
    )
    {
        return false;
    }

    if (
        fields[2].length() == 0 ||
        fields[4].length() == 0
    )
    {
        return false;
    }

    double newLatitude =
        convertCoordinate(
            fields[2],
            fields[3]
        );

    double newLongitude =
        convertCoordinate(
            fields[4],
            fields[5]
        );

    if (
        newLatitude == 0 &&
        newLongitude == 0
    )
    {
        return false;
    }

    latitude = newLatitude;
    longitude = newLongitude;

    lastValidFix = millis();

    fixAvailable = true;
    sensorConnected = true;

    status = GPS_REAL;

    return true;
}

// =====================================================
// CONVERT NMEA COORDINATE
// =====================================================

double GPSDriver::convertCoordinate(
    String value,
    String direction
)
{
    if (value.length() < 3)
    {
        return 0;
    }

    double raw = value.toDouble();

    int degrees =
        (int)(raw / 100.0);

    double minutes =
        raw - (degrees * 100.0);

    double coordinate =
        degrees +
        (minutes / 60.0);

    if (
        direction == "S" ||
        direction == "W"
    )
    {
        coordinate *= -1;
    }

    return coordinate;
}

// =====================================================
// VALIDATE NMEA TYPE
// =====================================================

bool GPSDriver::isValidNMEA(String sentence)
{
    if (!sentence.startsWith("$"))
    {
        return false;
    }

    if (
        !sentence.startsWith("$GPRMC") &&
        !sentence.startsWith("$GNRMC") &&
        !sentence.startsWith("$GPGGA") &&
        !sentence.startsWith("$GNGGA")
    )
    {
        return false;
    }

    return true;
}

// =====================================================
// GET LATITUDE
// =====================================================

double GPSDriver::getLatitude()
{
    return latitude;
}

// =====================================================
// GET LONGITUDE
// =====================================================

double GPSDriver::getLongitude()
{
    return longitude;
}

// =====================================================
// HAS FIX
// =====================================================

bool GPSDriver::hasFix()
{
    return fixAvailable;
}

// =====================================================
// SENSOR CONNECTED
// =====================================================

bool GPSDriver::isSensorConnected()
{
    return sensorConnected;
}

// =====================================================
// GET STATUS
// =====================================================

GPSDriver::GPSStatus GPSDriver::getStatus()
{
    return status;
}

// =====================================================
// STATUS TEXT
// =====================================================

const char* GPSDriver::getStatusText()
{
    /*
       Internal status only.

       These strings are NOT supposed to be displayed
       by VehicleSystem. They are kept here so the
       driver remains compatible with the existing header.
    */

    switch (status)
    {
        case GPS_REAL:
            return "GPS";

        case GPS_NO_FIX:
            return "GPS";

        case GPS_DEMO:
            return "GPS";

        default:
            return "GPS";
    }
}

