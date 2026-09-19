#include "InternetDriver.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// =====================================================
// RENDER SERVER
// =====================================================

const char* SERVER_URL =
    "https://v2v-emergency-server.onrender.com/accident";


// =====================================================
// CONSTRUCTOR
// =====================================================

InternetDriver::InternetDriver(
    const char* ssid,
    const char* password
)
    : ssid(ssid),
      password(password)
{
}


// =====================================================
// BEGIN
// =====================================================

void InternetDriver::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("      INTERNET DRIVER");
    Serial.println("==============================");

    connectWiFi();
}


// =====================================================
// CONNECT WIFI
// =====================================================

void InternetDriver::connectWiFi()
{
    Serial.println();
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < 20000)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Wi-Fi CONNECTED");

        Serial.print("ESP32 IP: ");
        Serial.println(WiFi.localIP());

        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());

        Serial.print("DNS: ");
        Serial.println(WiFi.dnsIP());

        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");

        Serial.print("Wi-Fi Channel: ");
        Serial.println(WiFi.channel());
    }
    else
    {
        Serial.println("Wi-Fi CONNECTION FAILED");
    }
}


// =====================================================
// CONNECTION STATUS
// =====================================================

bool InternetDriver::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}


// =====================================================
// SEND ACCIDENT ALERT
// =====================================================

bool InternetDriver::sendAccidentAlert(
    const String &vehicleID,
    double latitude,
    double longitude,
    const String &severity
)
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("   SENDING EMERGENCY ALERT");
    Serial.println("==============================");


    // -------------------------------------------------
    // CHECK WIFI
    // -------------------------------------------------

    if (!isConnected())
    {
        Serial.println("Wi-Fi disconnected.");
        Serial.println("Trying to reconnect...");

        connectWiFi();
    }

    if (!isConnected())
    {
        Serial.println("ERROR: No Wi-Fi connection.");
        return false;
    }


    // -------------------------------------------------
    // NETWORK INFORMATION
    // -------------------------------------------------

    Serial.println();
    Serial.println("Network Information:");

    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");


    // -------------------------------------------------
    // PRINT EMERGENCY DATA
    // -------------------------------------------------

    Serial.println();
    Serial.println("Emergency Data:");

    Serial.print("Vehicle ID: ");
    Serial.println(vehicleID);

    Serial.print("Latitude: ");
    Serial.println(latitude, 6);

    Serial.print("Longitude: ");
    Serial.println(longitude, 6);

    Serial.print("Severity: ");
    Serial.println(severity);


    // -------------------------------------------------
    // CREATE JSON
    // -------------------------------------------------

    String json = "{";

    json += "\"vehicle_id\":\"";
    json += vehicleID;
    json += "\",";

    json += "\"latitude\":";
    json += String(latitude, 6);
    json += ",";

    json += "\"longitude\":";
    json += String(longitude, 6);
    json += ",";

    json += "\"severity\":\"";
    json += severity;
    json += "\"";

    json += "}";


    Serial.println();
    Serial.println("JSON:");
    Serial.println(json);


    // -------------------------------------------------
    // HTTPS CLIENT
    // -------------------------------------------------

    WiFiClientSecure client;

    // Prototype/demo:
    // skip certificate verification.
    client.setInsecure();

    HTTPClient https;


    Serial.println();
    Serial.println("Connecting to emergency server...");


    // Give the connection a reasonable timeout
    https.setConnectTimeout(15000);
    https.setTimeout(15000);


    // -------------------------------------------------
    // BEGIN HTTPS
    // -------------------------------------------------

    if (!https.begin(client, SERVER_URL))
    {
        Serial.println("ERROR: HTTPS begin() failed.");
        return false;
    }


    // -------------------------------------------------
    // HEADERS
    // -------------------------------------------------

    https.addHeader(
        "Content-Type",
        "application/json"
    );

    https.addHeader(
        "Accept",
        "application/json"
    );


    // -------------------------------------------------
    // POST
    // -------------------------------------------------

    Serial.println();
    Serial.println("Sending POST request...");

    int httpCode = https.POST(json);


    // -------------------------------------------------
    // RESPONSE
    // -------------------------------------------------

    if (httpCode > 0)
    {
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);

        String response = https.getString();

        Serial.println();
        Serial.println("Server Response:");
        Serial.println(response);


        https.end();


        if (httpCode >= 200 && httpCode < 300)
        {
            Serial.println();
            Serial.println("================================");
            Serial.println("   EMERGENCY ALERT SENT");
            Serial.println("================================");

            return true;
        }

        Serial.println();
        Serial.println("Server returned an error.");

        return false;
    }


    // -------------------------------------------------
    // HTTPS ERROR
    // -------------------------------------------------

    Serial.println();
    Serial.print("HTTP POST failed. Error: ");
    Serial.println(
        https.errorToString(httpCode)
    );

    https.end();

    return false;
}