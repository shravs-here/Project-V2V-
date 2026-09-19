#ifndef MPUDRIVER_H
#define MPUDRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class MPUDriver
{
public:

    MPUDriver();

    bool begin();
    void update();

    bool accidentDetected();

    float getAcceleration();
    float getTiltAngle();

    int getSeverityIndex();
    const char* getSeverityLevel();

    void stopMonitoring();
    void resumeMonitoring();
    void resume();

    bool isMonitoring();

private:

    Adafruit_MPU6050 mpu;

    bool monitoring;

    // Current readings
    float currentAcceleration;
    float currentTiltAngle;

    // Previous reading
    float previousAcceleration;

    // Baseline acceleration
    float baselineAcceleration;

    // Impact tracking
    bool impactCandidate;
    unsigned long impactStartTime;

    float candidatePeak;
    float candidateStartAcceleration;

    // Event data - frozen when accident occurs
    float eventAcceleration;
    float eventAngle;
    float eventChange;
    float peakAcceleration;

    // Accident type
    bool majorAccelerationEvent;
    bool majorDropEvent;
    bool dangerousAngleEvent;

    unsigned long dangerousAngleStartTime;

    // Recent acceleration history
    static const int HISTORY_SIZE = 8;

    float accelerationHistory[HISTORY_SIZE];
    int historyIndex;
    int historyCount;

    // ========================================================
    // DETECTION PARAMETERS
    // ========================================================

    // Acceleration cliff thresholds
    static constexpr float POSITIVE_CLIFF = 80.0f;
    static constexpr float NEGATIVE_CLIFF = -50.0f;

    // Minimum acceleration needed before considering a cliff
    static constexpr float IMPACT_CANDIDATE = 18.0f;

    // Impact candidate timing
    static constexpr unsigned long IMPACT_WINDOW = 300;
    static constexpr unsigned long MIN_IMPACT_TIME = 30;

    // Dangerous angle
    static constexpr unsigned long DANGEROUS_ANGLE_TIME = 1000;

    static constexpr float ANGLE_WARNING_THRESHOLD = 15.0f;
    static constexpr float ANGLE_ACCIDENT_THRESHOLD = 70.0f;

    // ========================================================
    // SEVERITY THRESHOLDS
    // ========================================================

    // Impact severity
    static constexpr float MODERATE_IMPACT = 80.0f;
    static constexpr float SEVERE_IMPACT = 120.0f;
    static constexpr float CRITICAL_IMPACT = 160.0f;

    // Rollover severity
    static constexpr float MODERATE_ROLLOVER = 70.0f;
    static constexpr float SEVERE_ROLLOVER = 80.0f;
    static constexpr float CRITICAL_ROLLOVER = 85.0f;

    // ========================================================
    // INTERNAL FUNCTIONS
    // ========================================================

    float getAccelerationBaseline();

    void addToHistory(float acceleration);

    bool detectImpactPattern(
        float acceleration,
        float baseline
    );

    void triggerAccident(
        float acceleration,
        float angle,
        float change
    );

    void resetImpactCandidate();

    void resetEvent();

    int calculateSeverity(
        float acceleration,
        float change,
        float angle
    );
};

#endif