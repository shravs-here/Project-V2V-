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
    // Check Wi-Fi
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
    // Print data
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
    // HTTPS client
    // -------------------------------------------------

    WiFiClientSecure client;

    /*
       Render uses HTTPS.

       setInsecure() skips certificate verification.

       This is acceptable for the prototype/demo.
       For a production system, certificate
       verification should be used.
    */

    client.setInsecure();


    HTTPClient https;


    Serial.println();
    Serial.println("Connecting to emergency server...");


    if (!https.begin(client, SERVER_URL))
    {
        Serial.println("ERROR: HTTPS connection failed.");
        return false;
    }


    // -------------------------------------------------
    // Headers
    // -------------------------------------------------

    https.addHeader(
        "Content-Type",
        "application/json"
    );


    // -------------------------------------------------
    // JSON
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
    // POST
    // -------------------------------------------------

    Serial.println();
    Serial.println("Sending POST request...");

    int httpCode = https.POST(json);


    // -------------------------------------------------
    // Response
    // -------------------------------------------------

    if (httpCode > 0)
    {
        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);


        String response =
            https.getString();

        Serial.println();
        Serial.println("Server Response:");
        Serial.println(response);


        https.end();


        if (httpCode == 200)
        {
            Serial.println();
            Serial.println("================================");
            Serial.println("   EMERGENCY ALERT SENT");
            Serial.println("================================");

            return true;
        }
    }
    else
    {
        Serial.print(
            "HTTP POST failed. Error: "
        );

        Serial.println(
            https.errorToString(httpCode)
        );
    }


    https.end();

    return false;
}