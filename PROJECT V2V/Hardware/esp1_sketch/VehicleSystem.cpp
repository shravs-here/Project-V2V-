#include "VehicleSystem.h"

// =====================================================
// DEMO SETTINGS
// =====================================================

const char* VEHICLE_ID = "V2V-001";

const double DEMO_LATITUDE = 16.494119;
const double DEMO_LONGITUDE = 80.498230;


// =====================================================
// OUTPUT PINS
// =====================================================

#define LED_PIN     12
#define BUZZER_PIN  27


// =====================================================
// TIMING
// =====================================================

unsigned long lastMPUPrint = 0;

const unsigned long MPU_PRINT_INTERVAL = 500;

unsigned long lastAlertToggle = 0;

const unsigned long ALERT_INTERVAL = 500;

bool alertOutputState = false;


// =====================================================
// CONSTRUCTOR
// =====================================================

VehicleSystem::VehicleSystem(
    MPUDriver &mpu,
    GPSDriver &gps,
    InternetDriver &internet,
    ESPNowDriver &espNow
)
    : mpu(mpu),
      gps(gps),
      internet(internet),
      espNow(espNow),
      mpuAvailable(false),
      state(MONITORING)
{
}


// =====================================================
// MPU STATUS
// =====================================================

void VehicleSystem::setMPUAvailable(bool available)
{
    mpuAvailable = available;

    Serial.print("MPU available: ");
    Serial.println(
        mpuAvailable ? "YES" : "NO"
    );
}


// =====================================================
// BEGIN
// =====================================================

void VehicleSystem::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("       V2V SYSTEM READY");
    Serial.println("==============================");

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    alertOutputState = false;
    lastAlertToggle = millis();

    state = MONITORING;

    lastMPUPrint =
        millis() - MPU_PRINT_INTERVAL;

    espNow.setReceiveCallback(
        VehicleSystem::onESPNowReceive
    );

    printState();
}


// =====================================================
// UPDATE
// =====================================================

void VehicleSystem::update()
{
    // Always listen for PC commands
    checkSerialCommand();

    // Always update LED and buzzer
    updatePhysicalAlert();

    switch (state)
    {
        case MONITORING:
            handleMonitoring();
            break;

        case ACCIDENT_SUSPECTED:
            handleAccidentSuspected();
            break;

        case ACCIDENT_CONFIRMED:
            handleAccidentConfirmed();
            break;

        case SYSTEM_LOCKED:
            // Physical alert continues through
            // updatePhysicalAlert().
            break;
    }
}


// =====================================================
// PERIODIC LED + BUZZER ALERT
// =====================================================

void VehicleSystem::updatePhysicalAlert()
{
    // ---------------------------------------------
    // NORMAL MONITORING
    // ---------------------------------------------

    if (state == MONITORING)
    {
        alertOutputState = false;

        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);

        return;
    }


    // ---------------------------------------------
    // ACCIDENT SUSPECTED
    // LED flashes
    // Buzzer remains OFF
    // ---------------------------------------------

    if (state == ACCIDENT_SUSPECTED)
    {
        digitalWrite(BUZZER_PIN, LOW);

        if (millis() - lastAlertToggle >= ALERT_INTERVAL)
        {
            lastAlertToggle = millis();

            alertOutputState = !alertOutputState;

            digitalWrite(
                LED_PIN,
                alertOutputState ? HIGH : LOW
            );
        }

        return;
    }


    // ---------------------------------------------
    // ACCIDENT CONFIRMED / LOCKED
    // LED + buzzer flash together
    // ---------------------------------------------

    if (state == ACCIDENT_CONFIRMED ||
        state == SYSTEM_LOCKED)
    {
        if (millis() - lastAlertToggle >= ALERT_INTERVAL)
        {
            lastAlertToggle = millis();

            alertOutputState = !alertOutputState;

            digitalWrite(
                LED_PIN,
                alertOutputState ? HIGH : LOW
            );

            digitalWrite(
                BUZZER_PIN,
                alertOutputState ? HIGH : LOW
            );
        }

        return;
    }
}


// =====================================================
// USB SERIAL COMMANDS
// =====================================================

void VehicleSystem::checkSerialCommand()
{
    static String command = "";

    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n')
        {
            command.trim();

            if (command.length() > 0)
            {
                Serial.print("PC COMMAND: ");
                Serial.println(command);

                if (command == "CANCEL")
                {
                    cancelAccident();
                }
                else if (command == "CONFIRM")
                {
                    confirmAccident();
                }
            }

            command = "";
        }
        else if (c != '\r')
        {
            command += c;

            if (command.length() > 32)
            {
                command = "";
            }
        }
    }
}


// =====================================================
// MONITORING
// =====================================================

void VehicleSystem::handleMonitoring()
{
    // =================================================
    // MPU
    // =================================================

    if (mpuAvailable)
    {
        mpu.update();

        if (millis() - lastMPUPrint >= MPU_PRINT_INTERVAL)
        {
            lastMPUPrint = millis();

            Serial.print("Acceleration: ");

            Serial.print(
                mpu.getAcceleration(),
                2
            );

            Serial.print(" m/s²");

            Serial.print(" | Angle: ");

            Serial.print(
                mpu.getTiltAngle(),
                2
            );

            Serial.println("°");
        }

        // ---------------------------------------------
        // ACCIDENT DETECTION
        // ---------------------------------------------

        if (mpu.accidentDetected())
        {
            state = ACCIDENT_SUSPECTED;

            alertOutputState = false;
            lastAlertToggle = millis();

            Serial.println();
            Serial.println("================================");
            Serial.println("      ACCIDENT SUSPECTED");
            Serial.println("================================");

            printAccidentDetails();

            // PC popup notification
            Serial.print("ACCIDENT_SUSPECTED|");
            Serial.println(VEHICLE_ID);

            Serial.println();
            Serial.println(
                "Waiting for PC confirmation..."
            );

            printState();
        }
    }

    // =================================================
    // GPS
    // =================================================

    gps.update();
}


// =====================================================
// ACCIDENT SUSPECTED
// =====================================================

void VehicleSystem::handleAccidentSuspected()
{
    if (mpuAvailable)
    {
        mpu.update();

        if (millis() - lastMPUPrint >= MPU_PRINT_INTERVAL)
        {
            lastMPUPrint = millis();

            Serial.print("Acceleration: ");

            Serial.print(
                mpu.getAcceleration(),
                2
            );

            Serial.print(" m/s²");

            Serial.print(" | Angle: ");

            Serial.print(
                mpu.getTiltAngle(),
                2
            );

            Serial.println("°");
        }
    }

    gps.update();
}


// =====================================================
// CANCEL ACCIDENT
// =====================================================

void VehicleSystem::cancelAccident()
{
    if (state != ACCIDENT_SUSPECTED)
    {
        return;
    }

    Serial.println();
    Serial.println("==============================");
    Serial.println("        FALSE ALARM");
    Serial.println("==============================");

    alertOutputState = false;

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    if (mpuAvailable)
    {
        mpu.resume();
    }

    lastMPUPrint =
        millis() - MPU_PRINT_INTERVAL;

    lastAlertToggle = millis();

    state = MONITORING;

    Serial.println(
        "Resuming MPU monitoring..."
    );

    printState();
}


// =====================================================
// CONFIRM ACCIDENT
// =====================================================

void VehicleSystem::confirmAccident()
{
    if (state != ACCIDENT_SUSPECTED)
    {
        return;
    }

    state = ACCIDENT_CONFIRMED;

    alertOutputState = true;
    lastAlertToggle = millis();

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    Serial.println();
    Serial.println("==============================");
    Serial.println("      ACCIDENT CONFIRMED");
    Serial.println("==============================");

    printAccidentDetails();

    // =================================================
    // GPS
    // =================================================

    Serial.println();
    Serial.println("--------- GPS DATA ---------");

    printGPSData();

    Serial.println("----------------------------");

    Serial.println();
    Serial.println(
        "Starting accident response..."
    );

    printState();

    double latitude =
        gps.getLatitude();

    double longitude =
        gps.getLongitude();

    if (latitude == 0.0 ||
        longitude == 0.0)
    {
        Serial.println();
        Serial.println(
            "No GPS fix available."
        );

        Serial.println(
            "Using DEMO coordinates."
        );

        latitude = DEMO_LATITUDE;
        longitude = DEMO_LONGITUDE;
    }

    // =================================================
    // SEVERITY
    // =================================================

    String severity =
        String(
            mpu.getSeverityLevel()
        );

    // =================================================
    // ESP-NOW ALERT
    // =================================================

    Serial.println();
    Serial.println("================================");
    Serial.println("    ESP-NOW ACCIDENT ALERT");
    Serial.println("================================");

    String espNowMessage = "{";

    espNowMessage +=
        "\"vehicle_id\":\"";

    espNowMessage +=
        VEHICLE_ID;

    espNowMessage +=
        "\",";

    espNowMessage +=
        "\"type\":\"ACCIDENT\",";

    espNowMessage +=
        "\"severity\":\"";

    espNowMessage +=
        severity;

    espNowMessage +=
        "\",";

    espNowMessage +=
        "\"latitude\":";

    espNowMessage +=
        String(
            latitude,
            6
        );

    espNowMessage += ",";

    espNowMessage +=
        "\"longitude\":";

    espNowMessage +=
        String(
            longitude,
            6
        );

    espNowMessage += "}";

    Serial.println();
    Serial.println("ESP-NOW Packet:");
    Serial.println(espNowMessage);

    bool espNowSent =
        espNow.broadcast(
            espNowMessage.c_str()
        );

    if (espNowSent)
    {
        Serial.println();
        Serial.println(
            "ESP-NOW accident alert broadcast."
        );
    }
    else
    {
        Serial.println();
        Serial.println(
            "WARNING: ESP-NOW broadcast failed."
        );
    }

    // =================================================
    // INTERNET ALERT
    // =================================================

    Serial.println();
    Serial.println("================================");
    Serial.println("   INTERNET EMERGENCY ALERT");
    Serial.println("================================");

    bool sent =
        internet.sendAccidentAlert(
            VEHICLE_ID,
            latitude,
            longitude,
            severity
        );

    if (sent)
    {
        Serial.println();
        Serial.println(
            "Emergency server received alert."
        );
    }
    else
    {
        Serial.println();
        Serial.println(
            "WARNING: Emergency server "
            "did not receive alert."
        );
    }

    // =================================================
    // FINISH
    // =================================================

    handleAccidentConfirmed();
}


// =====================================================
// ACCIDENT RESPONSE
// =====================================================

void VehicleSystem::handleAccidentConfirmed()
{
    Serial.println();
    Serial.println(
        ">>> ACCIDENT RESPONSE ACTIVE"
    );

    Serial.println(
        "GPS location processed."
    );

    Serial.println(
        "Emergency alert processed."
    );

    Serial.println(
        "ESP-NOW alert processed."
    );

    state = SYSTEM_LOCKED;

    Serial.println();
    Serial.println("==============================");
    Serial.println("        SYSTEM LOCKED");
    Serial.println("==============================");

    Serial.println(
        "MPU monitoring stopped."
    );

    Serial.println(
        "Physical EN reset required."
    );

    printState();
}


// =====================================================
// ESP-NOW RECEIVE
// =====================================================

void VehicleSystem::onESPNowReceive(
    const uint8_t *mac,
    const uint8_t *data,
    size_t len
)
{
    Serial.println();
    Serial.println("================================");
    Serial.println("     V2V ACCIDENT ALERT");
    Serial.println("================================");

    Serial.print(
        "From vehicle MAC: "
    );

    for (int i = 0; i < 6; i++)
    {
        if (i > 0)
        {
            Serial.print(":");
        }

        Serial.printf(
            "%02X",
            mac[i]
        );
    }

    Serial.println();

    Serial.println();
    Serial.println("Accident Data:");

    Serial.write(
        data,
        len
    );

    Serial.println();

    // Send remote alert to PC popup service
    Serial.print(
        "REMOTE_ACCIDENT|"
    );

    Serial.write(
        data,
        len
    );

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "       END V2V ALERT"
    );

    Serial.println(
        "================================"
    );
}


// =====================================================
// ACCIDENT DETAILS
// =====================================================

void VehicleSystem::printAccidentDetails()
{
    Serial.println();
    Serial.println(
        "--------- ACCIDENT DATA ---------"
    );

    if (!mpuAvailable)
    {
        Serial.println(
            "MPU: NOT AVAILABLE"
        );

        Serial.println(
            "Local accident detection disabled."
        );

        Serial.println(
            "---------------------------------"
        );

        return;
    }

    Serial.print(
        "Severity Index: "
    );

    Serial.println(
        mpu.getSeverityIndex()
    );

    Serial.print(
        "Severity Level: "
    );

    Serial.println(
        mpu.getSeverityLevel()
    );

    Serial.print(
        "Event Acceleration: "
    );

    Serial.print(
        mpu.getAcceleration(),
        2
    );

    Serial.println(
        " m/s²"
    );

    Serial.print(
        "Event Angle: "
    );

    Serial.print(
        mpu.getTiltAngle(),
        2
    );

    Serial.println(
        "°"
    );

    Serial.println(
        "---------------------------------"
    );
}


// =====================================================
// GPS DATA
// =====================================================

void VehicleSystem::printGPSData()
{
    Serial.print(
        "GPS Mode: "
    );

    Serial.println(
        gps.getStatusText()
    );

    Serial.print(
        "Latitude: "
    );

    Serial.println(
        gps.getLatitude(),
        6
    );

    Serial.print(
        "Longitude: "
    );

    Serial.println(
        gps.getLongitude(),
        6
    );

    Serial.println(
        "GPS data checked."
    );
}


// =====================================================
// GET STATE
// =====================================================

VehicleSystem::SystemState
VehicleSystem::getState()
{
    return state;
}


// =====================================================
// PRINT STATE
// =====================================================

void VehicleSystem::printState()
{
    Serial.print(
        "STATE: "
    );

    switch (state)
    {
        case MONITORING:

            Serial.println(
                "MONITORING"
            );

            break;

        case ACCIDENT_SUSPECTED:

            Serial.println(
                "ACCIDENT SUSPECTED"
            );

            break;

        case ACCIDENT_CONFIRMED:

            Serial.println(
                "ACCIDENT CONFIRMED"
            );

            break;

        case SYSTEM_LOCKED:

            Serial.println(
                "SYSTEM LOCKED"
            );

            break;
    }
}