#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "ConfigTypes.h"

// Debug serial logging is opt-in via the SVITRIX_DEBUG build flag (platformio.ini).
// Release builds (env `ulanzi`) omit it to save flash; use env `ulanzi_debug`
// for serial diagnostics.
#ifdef SVITRIX_DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#define DEBUG_PRINTLN(x)        \
    {                           \
        Serial.print("[");      \
        Serial.print(millis()); \
        Serial.print("] [");    \
        Serial.print(__func__); \
        Serial.print("]: ");    \
        Serial.println(x);      \
    }
#define DEBUG_PRINTF(format, ...)             \
    {                                         \
        Serial.print("[");                    \
        Serial.print(millis());               \
        Serial.print("] [");                  \
        Serial.print(__func__);               \
        Serial.print("]: ");                  \
        Serial.printf(format, ##__VA_ARGS__); \
        Serial.println();                     \
    }
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(format, ...)
#endif

void formatSettings();

#ifndef VERSION
#define VERSION "dev"
#endif

// Config structs
extern WifiConfig wifiConfig;
extern MqttConfig mqttConfig;
extern NetworkConfig networkConfig;
extern HaConfig haConfig;
extern SensorConfig sensorConfig;
extern BatteryConfig batteryConfig;
extern AuthConfig authConfig;

extern DisplayConfig displayConfig;
extern BrightnessConfig brightnessConfig;
extern ColorConfig colorConfig;
extern TimeConfig timeConfig;
extern AppConfig appConfig;
extern AudioConfig audioConfig;
extern SystemConfig systemConfig;
extern WeatherConfig weatherConfig;
extern WeatherData weatherData;
extern PlaylistConfig playlistConfig;
extern RotationConfig rotationConfig;

constexpr double movementFactor = 0.5;

void loadSettings();
void saveSettings();
void validateSettings();
String exportSettings();
bool importSettings(const char *json);

// Captured once at boot via esp_reset_reason(); mapped via ResetReason service.
// Read-only after setup(). Empty string before setup() runs.
extern String lastResetReason;
