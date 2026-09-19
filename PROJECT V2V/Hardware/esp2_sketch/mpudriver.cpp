#include "MPUDriver.h"
#include <math.h>

MPUDriver::MPUDriver()
{
    monitoring = true;

    currentAcceleration = 0.0f;
    currentTiltAngle = 0.0f;

    previousAcceleration = 9.81f;
    baselineAcceleration = 9.81f;

    impactCandidate = false;
    impactStartTime = 0;

    candidatePeak = 0.0f;
    candidateStartAcceleration = 0.0f;

    eventAcceleration = 0.0f;
    eventAngle = 0.0f;
    eventChange = 0.0f;
    peakAcceleration = 0.0f;

    majorAccelerationEvent = false;
    majorDropEvent = false;
    dangerousAngleEvent = false;

    dangerousAngleStartTime = 0;

    historyIndex = 0;
    historyCount = 0;

    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        accelerationHistory[i] = 9.81f;
    }
}


// ============================================================
// BEGIN
// ============================================================

bool MPUDriver::begin()
{
    Serial.println();
    Serial.println("=== MPU6050 DRIVER ===");

    Wire.begin(21, 22);

    if (!mpu.begin())
    {
        Serial.println("MPU6050 initialization FAILED");
        return false;
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 initialized");
    Serial.println("Accelerometer range: ±8G");
    Serial.println("MPU ready");

    Serial.println();
    Serial.println("Accident detection:");
    Serial.println("  Positive cliff: +80 m/s²");
    Serial.println("  Negative cliff: -50 m/s²");
    Serial.println("  Rollover angle: 70°");
    Serial.println();

    delay(100);

    return true;
}


// ============================================================
// UPDATE
// ============================================================

void MPUDriver::update()
{
    if (!monitoring)
        return;

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;

    mpu.getEvent(
        &accel,
        &gyro,
        &temp
    );

    // --------------------------------------------------------
    // Calculate acceleration magnitude
    // --------------------------------------------------------

    float x = accel.acceleration.x;
    float y = accel.acceleration.y;
    float z = accel.acceleration.z;

    currentAcceleration =
        sqrt(
            x * x +
            y * y +
            z * z
        );

    // --------------------------------------------------------
    // Calculate tilt angle
    // --------------------------------------------------------

    float horizontal =
        sqrt(
            x * x +
            y * y
        );

    currentTiltAngle =
        atan2(
            horizontal,
            fabs(z)
        ) * 180.0f / PI;

    // --------------------------------------------------------
    // Add to history
    // --------------------------------------------------------

    addToHistory(currentAcceleration);

    baselineAcceleration =
        getAccelerationBaseline();

    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    Serial.print("ACC: ");
    Serial.print(currentAcceleration, 2);

    Serial.print(" m/s² | ANGLE: ");
    Serial.print(currentTiltAngle, 2);

    Serial.println("°");

    // ========================================================
    // 1. ROLLOVER / TIP-OVER DETECTION
    // ========================================================

    if (currentTiltAngle >= ANGLE_ACCIDENT_THRESHOLD)
    {
        if (dangerousAngleStartTime == 0)
        {
            dangerousAngleStartTime = millis();
        }

        if (
            millis() - dangerousAngleStartTime
            >= DANGEROUS_ANGLE_TIME
        )
        {
            dangerousAngleEvent = true;

            // For rollover there may be NO acceleration cliff.
            // Therefore change is recorded as 0.
            triggerAccident(
                currentAcceleration,
                currentTiltAngle,
                0.0f
            );

            return;
        }
    }
    else
    {
        dangerousAngleStartTime = 0;
    }

    // ========================================================
    // 2. IMPACT / ACCELERATION CLIFF DETECTION
    // ========================================================

    if (
        detectImpactPattern(
            currentAcceleration,
            baselineAcceleration
        )
    )
    {
        return;
    }

    previousAcceleration = currentAcceleration;
}


// ============================================================
// BASELINE
// ============================================================

float MPUDriver::getAccelerationBaseline()
{
    if (historyCount == 0)
        return 9.81f;

    float total = 0.0f;

    for (int i = 0; i < historyCount; i++)
    {
        total += accelerationHistory[i];
    }

    return total / historyCount;
}


// ============================================================
// HISTORY
// ============================================================

void MPUDriver::addToHistory(float acceleration)
{
    accelerationHistory[historyIndex] = acceleration;

    historyIndex++;

    if (historyIndex >= HISTORY_SIZE)
        historyIndex = 0;

    if (historyCount < HISTORY_SIZE)
        historyCount++;
}


// ============================================================
// IMPACT DETECTION
// ============================================================

bool MPUDriver::detectImpactPattern(
    float acceleration,
    float baseline
)
{
    // --------------------------------------------------------
    // Change from immediately previous sample
    // --------------------------------------------------------

    float cliff =
        acceleration - previousAcceleration;

    // --------------------------------------------------------
    // If there is no active candidate, look for a strong cliff.
    //
    // Positive:
    //     +80 m/s² or more
    //
    // Negative:
    //     -50 m/s² or less
    // --------------------------------------------------------

    if (!impactCandidate)
    {
        bool positiveCliff =
            (
                acceleration >= IMPACT_CANDIDATE &&
                cliff >= POSITIVE_CLIFF
            );

        bool negativeCliff =
            (
                cliff <= NEGATIVE_CLIFF
            );

        if (positiveCliff || negativeCliff)
        {
            impactCandidate = true;

            impactStartTime = millis();

            candidatePeak =
                acceleration;

            candidateStartAcceleration =
                previousAcceleration;

            Serial.println();
            Serial.println(">>> IMPACT CANDIDATE");

            if (positiveCliff)
            {
                Serial.print("Positive cliff: ");
                Serial.print(cliff, 2);
                Serial.println(" m/s²");
            }
            else
            {
                Serial.print("Negative cliff: ");
                Serial.print(cliff, 2);
                Serial.println(" m/s²");
            }

            return false;
        }

        return false;
    }

    // --------------------------------------------------------
    // Candidate active
    // --------------------------------------------------------

    if (acceleration > candidatePeak)
    {
        candidatePeak = acceleration;
    }

    unsigned long elapsed =
        millis() - impactStartTime;

    float totalChange =
        acceleration -
        candidateStartAcceleration;

    float peakChange =
        candidatePeak -
        candidateStartAcceleration;

    // Absolute magnitude of the acceleration event
    float eventMagnitude =
        fabs(peakChange);

    // --------------------------------------------------------
    // Strong impact accepted
    // --------------------------------------------------------

    if (
        elapsed >= MIN_IMPACT_TIME &&
        elapsed <= IMPACT_WINDOW &&
        eventMagnitude >= MODERATE_IMPACT
    )
    {
        if (peakChange >= 0)
        {
            majorAccelerationEvent = true;
        }
        else
        {
            majorDropEvent = true;
        }

        triggerAccident(
            candidatePeak,
            currentTiltAngle,
            peakChange
        );

        return true;
    }

    // --------------------------------------------------------
    // Candidate expired
    // --------------------------------------------------------

    if (elapsed > IMPACT_WINDOW)
    {
        Serial.println(
            ">>> IMPACT CANDIDATE REJECTED"
        );

        resetImpactCandidate();

        return false;
    }

    return false;
}


// ============================================================
// TRIGGER ACCIDENT
// ============================================================

void MPUDriver::triggerAccident(
    float acceleration,
    float angle,
    float change
)
{
    if (
        majorAccelerationEvent == false &&
        majorDropEvent == false &&
        dangerousAngleEvent == false
    )
    {
        return;
    }

    // --------------------------------------------------------
    // Freeze event data NOW.
    // Nothing after this point can overwrite it.
    // --------------------------------------------------------

    eventAcceleration = acceleration;
    eventAngle = angle;
    eventChange = change;

    peakAcceleration = acceleration;

    Serial.println();
    Serial.println("================================");
    Serial.println("      🚨 ACCIDENT DETECTED");
    Serial.println("================================");

    // --------------------------------------------------------
    // Accident type
    // --------------------------------------------------------

    if (dangerousAngleEvent)
    {
        Serial.println(
            "Type: ROLLOVER / TIP-OVER"
        );
    }
    else
    {
        Serial.println(
            "Type: IMPACT / ACCELERATION CLIFF"
        );
    }

    Serial.println();

    Serial.print("Event acceleration: ");
    Serial.print(eventAcceleration, 2);
    Serial.println(" m/s²");

    Serial.print("Peak acceleration: ");
    Serial.print(peakAcceleration, 2);
    Serial.println(" m/s²");

    Serial.print("Event angle: ");
    Serial.print(eventAngle, 2);
    Serial.println("°");

    // --------------------------------------------------------
    // Acceleration change
    // --------------------------------------------------------

    if (dangerousAngleEvent)
    {
        Serial.println(
            "Acceleration change: N/A"
        );
    }
    else
    {
        Serial.print("Acceleration change: ");
        Serial.print(eventChange, 2);
        Serial.println(" m/s²");
    }

    // --------------------------------------------------------
    // Severity
    // --------------------------------------------------------

    int severity =
        calculateSeverity(
            eventAcceleration,
            eventChange,
            eventAngle
        );

    Serial.print("Severity: ");
    Serial.print(severity);
    Serial.print("/100 - ");
    Serial.println(getSeverityLevel());

    Serial.println("================================");

    // --------------------------------------------------------
    // STOP MPU
    // --------------------------------------------------------

    monitoring = false;
}


// ============================================================
// RESET IMPACT CANDIDATE
// ============================================================

void MPUDriver::resetImpactCandidate()
{
    impactCandidate = false;

    impactStartTime = 0;

    candidatePeak = 0.0f;

    candidateStartAcceleration = 0.0f;
}


// ============================================================
// ACCIDENT STATUS
// ============================================================

bool MPUDriver::accidentDetected()
{
    return (
        majorAccelerationEvent ||
        majorDropEvent ||
        dangerousAngleEvent
    );
}


// ============================================================
// GET ACCELERATION
// ============================================================

float MPUDriver::getAcceleration()
{
    return eventAcceleration;
}


// ============================================================
// GET ANGLE
// ============================================================

float MPUDriver::getTiltAngle()
{
    return eventAngle;
}


// ============================================================
// SEVERITY CALCULATION
// ============================================================

int MPUDriver::calculateSeverity(
    float acceleration,
    float change,
    float angle
)
{
    // ========================================================
    // ROLLOVER / TIP-OVER
    // ========================================================

    if (dangerousAngleEvent)
    {
        if (angle >= CRITICAL_ROLLOVER)
        {
            return 100;
        }

        if (angle >= SEVERE_ROLLOVER)
        {
            return 70;
        }

        if (angle >= MODERATE_ROLLOVER)
        {
            return 40;
        }

        return 20;
    }

    // ========================================================
    // IMPACT / ACCELERATION CLIFF
    // ========================================================

    float magnitude =
        fabs(change);

    if (magnitude >= CRITICAL_IMPACT)
    {
        return 100;
    }

    if (magnitude >= SEVERE_IMPACT)
    {
        return 70;
    }

    if (magnitude >= MODERATE_IMPACT)
    {
        return 40;
    }

    return 20;
}


// ============================================================
// SEVERITY INDEX
// ============================================================

int MPUDriver::getSeverityIndex()
{
    return calculateSeverity(
        eventAcceleration,
        eventChange,
        eventAngle
    );
}


// ============================================================
// SEVERITY LEVEL
// ============================================================

const char* MPUDriver::getSeverityLevel()
{
    int severity =
        getSeverityIndex();

    if (severity < 60)
        return "Moderate";

    if (severity < 85)
        return "Severe";

    return "Critical";
}


// ============================================================
// STOP MONITORING
// ============================================================

void MPUDriver::stopMonitoring()
{
    monitoring = false;

    Serial.println(
        "MPU monitoring permanently stopped."
    );
}


// ============================================================
// RESUME MONITORING
// ============================================================

void MPUDriver::resumeMonitoring()
{
    monitoring = true;

    resetEvent();
    resetImpactCandidate();

    previousAcceleration = 9.81f;
    baselineAcceleration = 9.81f;

    historyIndex = 0;
    historyCount = 0;

    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        accelerationHistory[i] = 9.81f;
    }

    Serial.println(
        "MPU monitoring resumed."
    );
}


// ============================================================
// MONITORING STATUS
// ============================================================

bool MPUDriver::isMonitoring()
{
    return monitoring;
}


// ============================================================
// RESET EVENT
// ============================================================

void MPUDriver::resetEvent()
{
    majorAccelerationEvent = false;
    majorDropEvent = false;
    dangerousAngleEvent = false;

    eventAcceleration = 0.0f;
    eventAngle = 0.0f;
    eventChange = 0.0f;
    peakAcceleration = 0.0f;

    dangerousAngleStartTime = 0;
}


// ============================================================
// COMPATIBILITY RESUME
// ============================================================

void MPUDriver::resume()
{
    resumeMonitoring();
}