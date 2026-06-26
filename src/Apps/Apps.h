#pragma once

#include <map>
#include <vector>
#include "MatrixDisplayUi.h"
#include "AppContent.h"

class INotifier;

struct CustomApp : AppContentBase
{
    // CustomApp-only fields
    int bounceDir = 0;
    bool hasCustomColor = false;
    uint8_t currentFrame = 0;
    String iconName;
    bool iconNotFound = false; ///< Negative cache: true if icon was not found on LittleFS
    byte lifetimeMode = 0;
    bool bounce = false;
    long duration = 0;
    int16_t repeat = 0;
    int16_t currentRepeat = 0;
    String name;
    unsigned long lastUpdate = 0;
    uint64_t lifetime = 0;
    bool lifeTimeEnd = false;
};

extern std::vector<std::pair<String, AppCallback>> Apps;
extern String currentCustomApp;
extern std::map<String, CustomApp> customApps;

void setAppsNotifier(INotifier *notifier);

CustomApp *getCustomAppByName(const String& name);

String getAppNameAtIndex(int index);

int findAppIndexByName(const String& name);

const char *getTimeFormat();

void TimeApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void DateApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void TempApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void HumApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void BatApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void OutdoorTempApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void OutdoorHumApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void PressureApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void AirQualityApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

/// Extra display time (ms) the ICA app needs to show its full rotation in one
/// visit: one per-item slice per pollutant currently flagged (level >= 4). 0
/// when only the index shows. The rotation system adds this to the AirQuality
/// item duration.
long airQualityExtraDurationMs();

void UVApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void MoonApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);

void ShowCustomApp(const String& name, FastLED_NeoMatrix *matrix, const MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer);
