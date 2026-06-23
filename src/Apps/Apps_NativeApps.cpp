/**
 * @file Apps_NativeApps.cpp
 * @brief Built-in native app render functions.
 *
 * Contains all five native display apps (Time, Date, Temperature,
 * Humidity, Battery) and their supporting data (big-digit bitmasks,
 * binary clock colors, time format helper).
 */
#include "Apps_internal.h"
#include "Functions.h"
#include "icons.h"
#include "LayoutEngine.h"
#include "MoonPhase.h"
#include "AlarmManager/AlarmManager.h"
#include <LittleFS.h>
#include <cmath>
#include <ctime>

// ── Big-digit clock data ───────────────────────────────────────────

/// Bitmask glyphs for big-digit clock (mode 5): digits 0–9, colon, blank.
const uint8_t bigdigits_mask[12][7] = {
    {132, 48, 48, 48, 48, 48, 132},      // 0
    {204, 140, 204, 204, 204, 204, 0},   // 1
    {132, 48, 240, 196, 156, 48, 0},     // 2
    {132, 48, 240, 196, 240, 48, 132},   // 3
    {228, 196, 132, 36, 0, 228, 192},    // 4
    {0, 60, 4, 240, 240, 48, 132},       // 5
    {196, 156, 60, 4, 48, 48, 132},      // 6
    {0, 48, 240, 228, 204, 204, 204},    // 7
    {132, 48, 48, 132, 48, 48, 132},     // 8
    {132, 48, 48, 128, 240, 228, 140},   // 9
    {252, 204, 204, 252, 204, 204, 252}, // :
    {252, 252, 252, 252, 252, 252, 252}  // ; (blank)
};

// ── Binary clock colors ────────────────────────────────────────────

uint32_t COLOR_HOUR_ON = 0xFF0000;   ///< Binary clock hour bit color (red)
uint32_t COLOR_MINUTE_ON = 0x00FF00; ///< Binary clock minute bit color (green)
uint32_t COLOR_SECOND_ON = 0x0000FF; ///< Binary clock second bit color (blue)
uint32_t COLOR_OFF = 0xFFFFFF;       ///< Binary clock off bit color (white)

// ── Time format helper ─────────────────────────────────────────────

/// Return the strftime format string for the current time mode.
/// Modes >0 truncate seconds; blinking separator uses space at position 2.
const char *getTimeFormat()
{
    if (timeConfig.timeMode == 0)
    {
        return timeConfig.timeFormat.c_str();
    }
    else
    {
        if (timeConfig.timeFormat.length() > 5)
        {
            return timeConfig.timeFormat[2] == ' ' ? "%H %M" : "%H:%M";
        }
        else
        {
            return timeConfig.timeFormat.c_str();
        }
    }
}

// ── TimeApp ────────────────────────────────────────────────────────

/// Native clock app with 7 display modes:
///   Mode 5: big-digit GIF background clock
///   Mode 6: binary clock (H/M/S as 6-bit rows)
///   Modes 0–4: text clock with optional calendar box and weekday bar
void TimeApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Time"))
        return;

    const char *timeformat = getTimeFormat();
    static File BIGTIME_BG_GIF;
    static bool BIGTIME_BG_ISGIF = false;
    if (BIGTIME_BG_GIF && timeConfig.timeMode != 5)
    {
        BIGTIME_BG_GIF.close();
        BIGTIME_BG_ISGIF = false;
    }

    // Mode 5: big-digit clock with GIF background
    if (timeConfig.timeMode == 5)
    {
        char t[20];
        strftime(t, sizeof(t), timeformat, timer_localtime());

        static bool bigtimeChecked = false;
        if (!BIGTIME_BG_ISGIF && !bigtimeChecked)
        {
            bigtimeChecked = true;
            if (LittleFS.exists("/bigtime.gif"))
            {
                BIGTIME_BG_GIF = LittleFS.open("/bigtime.gif");
                BIGTIME_BG_ISGIF = true;
            }
        }

        if (BIGTIME_BG_ISGIF)
        {
            static uint16_t BIGTIME_BG_CURRENTFRAME = 0;
            gifPlayer->playGif(0 + x, 0 + y, &BIGTIME_BG_GIF, BIGTIME_BG_CURRENTFRAME);
            BIGTIME_BG_CURRENTFRAME = gifPlayer->getFrame();
        }
        else
        {
            DisplayManager.drawFilledRect(0 + x, 0 + y, 32, 8, colorConfig.textColor);
        }

        t[2] = (timeformat[2] == ' ' && timer_time() % 2) ? ';' : ':';
        t[0] = t[0] == ' ' ? ';' : t[0];
        for (int i = 0; i < 5; i++)
        {
            int idx = t[i] - '0';
            if (idx < 0 || idx > 11)
                idx = 11; // blank for any unexpected character
            int xx = i * 7 - (i > 2 ? 2 : 0) - (i == 2);
            matrix->drawBitmap(xx + x, y, bigdigits_mask[idx], 6, 7, 0);
        }

        matrix->drawFastHLine(0 + x, 7 + y, 32, 0);
        matrix->drawFastVLine(6 + x, 0 + y, 7, 0);
        matrix->drawFastVLine(25 + x, 0 + y, 7, 0);
        return;
    }

    // Mode 6: binary clock
    if (timeConfig.timeMode == 6)
    {
        const struct tm *currentTime = timer_localtime();
        int hour = currentTime->tm_hour;
        int minute = currentTime->tm_min;
        int second = currentTime->tm_sec;

        auto drawBit = [&](int bit, int x1, int y1, uint32_t colorOn, uint32_t colorOff)
        {
            uint32_t color = bit ? colorOn : colorOff;
            for (int dx = 0; dx < 2; dx++)
            {
                for (int dy = 0; dy < 2; dy++)
                {
                    matrix->drawPixel(x1 + dx + x, y1 + dy + y, color);
                }
            }
        };

        for (int i = 0; i < 6; i++)
        {
            int bitValue = (hour >> (5 - i)) & 1;
            drawBit(bitValue, 5 + i * 4 + x, 0 + y, COLOR_HOUR_ON, COLOR_OFF);
        }

        for (int i = 0; i < 6; i++)
        {
            int bitValue = (minute >> (5 - i)) & 1;
            drawBit(bitValue, 5 + i * 4 + x, 3 + y, COLOR_MINUTE_ON, COLOR_OFF);
        }

        for (int i = 0; i < 6; i++)
        {
            int bitValue = (second >> (5 - i)) & 1;
            drawBit(bitValue, 5 + i * 4 + x, 6 + y, COLOR_SECOND_ON, COLOR_OFF);
        }

        return;
    }

    // Modes 0–4: text clock with optional calendar and weekday bar
    applyNativeAppColor(colorConfig.timeColor, "Time");

    char t[20];
    size_t fmtLen = strlen(timeformat);
    bool blinkFirst = (fmtLen > 2 && timeformat[2] == ' ');
    bool blinkSecond = (fmtLen > 5 && timeformat[5] == ' ');

    if (blinkFirst || blinkSecond)
    {
        char t2[20];
        strcpy(t2, timeformat);
        bool showColon = (timer_time() % 2) == 0;
        if (blinkFirst)
            t2[2] = showColon ? ':' : ' ';
        if (blinkSecond)
            t2[5] = showColon ? ':' : ' ';
        strftime(t, sizeof(t), t2, timer_localtime());
    }
    else
    {
        strftime(t, sizeof(t), timeformat, timer_localtime());
    }

    int16_t wdPosY;
    int16_t timePosY;
    if (timeConfig.timeMode == 2 || timeConfig.timeMode == 4)
    {
        wdPosY = 0;
        timePosY = 7;
    }
    else
    {
        wdPosY = 7;
        timePosY = 6;
    }

    int16_t baseX = (timeConfig.timeMode == 0) ? 0 : 12;
    DisplayManager.printText(baseX + x, timePosY + y, t, timeConfig.timeMode == 0, 2);

    if (timeConfig.timeMode > 0)
    {
        int offset;
        char day_str[3];
        snprintf(day_str, sizeof(day_str), "%d", timer_localtime()->tm_mday);

        DisplayManager.drawFilledRect(x, y, 9, 8, colorConfig.calendarBodyColor);
        if (timeConfig.timeMode <= 2)
        {
            DisplayManager.drawFilledRect(x, y, 9, 2, colorConfig.calendarHeaderColor);
        }
        else
        {
            DisplayManager.drawLine(1 + x, 0 + y, 2 + x, 0 + y, 0x000000);
            DisplayManager.drawLine(6 + x, 0 + y, 7 + x, 0 + y, 0x000000);
        }

        if (timer_localtime()->tm_mday < 10)
            offset = 3;
        else
            offset = 1;
        DisplayManager.setCursor(offset + x, 7 + y);
        DisplayManager.setTextColor(colorConfig.calendarTextColor);
        DisplayManager.matrixPrint(day_str);
    }

    if (!appConfig.showWeekday)
        return;

    uint8_t LINE_WIDTH = timeConfig.timeMode > 0 ? 2 : 3;
    uint8_t LINE_START = timeConfig.timeMode > 0 ? 10 : 2;
    drawWeekdayBar(x, wdPosY + y, LINE_WIDTH, 1, LINE_START);
}

// ── DateApp ────────────────────────────────────────────────────────

/// Native date app showing formatted date with weekday indicator bar.
void DateApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Date"))
        return;

    applyNativeAppColor(colorConfig.dateColor, "Date");

    char d[20];
    strftime(d, sizeof(d), timeConfig.dateFormat.c_str(), timer_localtime());
    DisplayManager.printText(0 + x, 6 + y, d, true, 2);

    if (!appConfig.showWeekday)
        return;

    drawWeekdayBar(x, y + 7, 3, 1, 2);
}

// ── TempApp ────────────────────────────────────────────────────────

/// Native temperature app showing sensor reading with thermometer icon.
void TempApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Temperature"))
        return;

    applyNativeAppColor(colorConfig.tempColor, "Temperature");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    if (m.hasIcon)
    {
        matrix->drawRGBBitmap(x + m.iconX, y, icon_234, 8, 8);
    }

    String tempStr;
    if (timeConfig.isCelsius)
    {
        tempStr = String(sensorConfig.currentTemp, timeConfig.tempDecimalPlaces) + "°C";
    }
    else
    {
        double tempF = (sensorConfig.currentTemp * 9 / 5) + 32;
        tempStr = String(tempF, timeConfig.tempDecimalPlaces) + "°F";
    }

    uint16_t textWidth = getTextWidth(tempStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(tempStr.c_str());
}

// ── HumApp ─────────────────────────────────────────────────────────

/// Native humidity app showing sensor reading with droplet icon.
void HumApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Humidity"))
        return;

    applyNativeAppColor(colorConfig.humColor, "Humidity");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    if (m.hasIcon)
    {
        matrix->drawRGBBitmap(x + m.iconX, y + 1, icon_2075, 8, 8);
    }

    String humStr = String(static_cast<int>(sensorConfig.currentHum)) + "%";
    uint16_t textWidth = getTextWidth(humStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(humStr.c_str());
}

// ── BatApp ─────────────────────────────────────────────────────────

/// Native battery app showing charge percentage with battery icon (ULANZI only).
void BatApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Battery"))
        return;

    applyNativeAppColor(colorConfig.batColor, "Battery");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    if (m.hasIcon)
    {
        matrix->drawRGBBitmap(x + m.iconX, y, icon_1486, 8, 8);
    }

    String batStr = String(static_cast<int>(batteryConfig.percent)) + "%";
    uint16_t textWidth = getTextWidth(batStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(batStr.c_str());
}

// ── OutdoorTempApp ─────────────────────────────────────────────────

/// Returns weather condition icon based on WeatherAPI condition code.
static const uint16_t *getWeatherConditionIcon(int code)
{
    // Sunny/Clear: 1000
    if (code == 1000)
        return icon_sunny;
    // Rain: 1063, 1150-1201, 1240-1246
    if (code == 1063 || (code >= 1150 && code <= 1201) || (code >= 1240 && code <= 1246))
        return icon_rainy;
    // Cloudy/Overcast/Fog: 1003, 1006, 1009, 1030, 1135, 1147
    return icon_cloudy;
}

/// Weather app showing outdoor temperature from WeatherAPI.
void OutdoorTempApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("OutdoorTemp"))
        return;

    applyNativeAppColor(weatherConfig.outdoorTempColor, "OutdoorTemp");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    if (m.hasIcon)
    {
        const uint16_t *condIcon = getWeatherConditionIcon(weatherData.conditionCode);
        matrix->drawRGBBitmap(x + m.iconX, y, condIcon, 8, 8);
    }

    String tempStr;
    if (weatherData.valid)
    {
        tempStr = String(weatherData.outdoorTemp, timeConfig.tempDecimalPlaces) + "°" + (timeConfig.isCelsius ? "C" : "F");
    }
    else
    {
        tempStr = "--.-°" + String(timeConfig.isCelsius ? "C" : "F");
    }

    uint16_t textWidth = getTextWidth(tempStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(tempStr.c_str());
}

// ── OutdoorHumApp ──────────────────────────────────────────────────

/// Weather app showing outdoor humidity from WeatherAPI.
void OutdoorHumApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("OutdoorHum"))
        return;

    applyNativeAppColor(weatherConfig.outdoorHumColor, "OutdoorHum");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    static File humIconGif;
    static bool humIconChecked = false;
    static bool humIconIsGif = false;
    static uint16_t humIconFrame = 0;

    if (m.hasIcon)
    {
        if (!humIconChecked)
        {
            humIconChecked = true;
            if (LittleFS.exists("/ICONS/61756.gif"))
            {
                humIconGif = LittleFS.open("/ICONS/61756.gif");
                humIconIsGif = true;
            }
        }
        if (humIconIsGif)
        {
            gifPlayer->playGif(x + m.iconX, y, &humIconGif, humIconFrame);
            humIconFrame = gifPlayer->getFrame();
        }
        else
        {
            matrix->drawRGBBitmap(x + m.iconX, y, icon_53628, 8, 8);
        }
    }

    String humStr;
    if (weatherData.valid)
    {
        humStr = String(static_cast<int>(weatherData.outdoorHumidity)) + "%";
    }
    else
    {
        humStr = "--%";
    }

    uint16_t textWidth = getTextWidth(humStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(humStr.c_str());
}

// ── PressureApp ────────────────────────────────────────────────────

/// Weather app showing atmospheric pressure from WeatherAPI.
void PressureApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Pressure"))
        return;

    applyNativeAppColor(weatherConfig.pressureColor, "Pressure");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    static File pressIconGif;
    static bool pressIconChecked = false;
    static bool pressIconIsGif = false;
    static uint16_t pressIconFrame = 0;

    if (m.hasIcon)
    {
        if (!pressIconChecked)
        {
            pressIconChecked = true;
            if (LittleFS.exists("/ICONS/66893.gif"))
            {
                pressIconGif = LittleFS.open("/ICONS/66893.gif");
                pressIconIsGif = true;
            }
        }
        if (pressIconIsGif)
        {
            gifPlayer->playGif(x + m.iconX, y, &pressIconGif, pressIconFrame);
            pressIconFrame = gifPlayer->getFrame();
        }
        else
        {
            matrix->drawRGBBitmap(x + m.iconX, y, icon_66892, 8, 8);
        }
    }

    String pressStr;
    if (weatherData.valid)
    {
        pressStr = String(static_cast<int>(weatherData.pressure));
    }
    else
    {
        pressStr = "----";
    }

    uint16_t textWidth = getTextWidth(pressStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(pressStr.c_str());
}

// ── AirQualityApp ──────────────────────────────────────────────────

/// Weather app showing Air Quality Index from WeatherAPI.
void AirQualityApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("AirQuality"))
        return;

    // Use dynamic color if autoColor enabled, otherwise use config color
    uint32_t aqiColor = weatherConfig.aqiColor;
    bool aqiAuto = weatherConfig.aqiAutoColor && weatherData.valid && weatherData.aqi > 0;
    if (aqiAuto)
    {
        switch (weatherData.aqi)
        {
        case 1:
            aqiColor = 0x00FF00;
            break; // Good - green
        case 2:
            aqiColor = 0xFFFF00;
            break; // Moderate - yellow
        case 3:
            aqiColor = 0xFFA500;
            break; // Unhealthy for sensitive - orange
        case 4:
            aqiColor = 0xFF0000;
            break; // Unhealthy - red
        case 5:
            aqiColor = 0x800080;
            break; // Very unhealthy - purple
        case 6:
            aqiColor = 0x800000;
            break; // Hazardous - maroon
        }
    }
    if (aqiAuto)
        // Auto color wins over any per-item/global color override
        // (still honors display policies such as night mode).
        DisplayManager.setTextColor(DisplayManager.resolveTextColor(aqiColor));
    else
        applyNativeAppColor(aqiColor, "AirQuality");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    static File aqiIconGif;
    static bool aqiIconChecked = false;
    static bool aqiIconIsGif = false;
    static uint16_t aqiIconFrame = 0;

    if (m.hasIcon)
    {
        if (!aqiIconChecked)
        {
            aqiIconChecked = true;
            if (LittleFS.exists("/ICONS/73559.gif"))
            {
                aqiIconGif = LittleFS.open("/ICONS/73559.gif");
                aqiIconIsGif = true;
            }
        }
        if (aqiIconIsGif)
        {
            gifPlayer->playGif(x + m.iconX, y, &aqiIconGif, aqiIconFrame);
            aqiIconFrame = gifPlayer->getFrame();
        }
        else
        {
            matrix->drawRGBBitmap(x + m.iconX, y, icon_6622, 8, 8);
        }
    }

    String aqiStr;
    if (weatherData.valid && weatherData.aqi > 0)
    {
        aqiStr = "ICA:" + String(weatherData.aqi);
    }
    else
    {
        aqiStr = "ICA:--";
    }

    uint16_t textWidth = getTextWidth(aqiStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(aqiStr.c_str());
}

// ── UVApp ──────────────────────────────────────────────────────────

/// Weather app showing UV Index from WeatherAPI.
void UVApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("UV"))
        return;

    // Use dynamic color if autoColor enabled, otherwise use config color
    uint32_t uvColor = weatherConfig.uvColor;
    bool uvAuto = weatherConfig.uvAutoColor && weatherData.valid;
    if (uvAuto)
    {
        float uv = weatherData.uv;
        if (uv < 3)
            uvColor = 0x00FF00; // Low - green
        else if (uv < 6)
            uvColor = 0xFFFF00; // Moderate - yellow
        else if (uv < 8)
            uvColor = 0xFFA500; // High - orange
        else if (uv < 11)
            uvColor = 0xFF0000; // Very high - red
        else
            uvColor = 0x9400D3; // Extreme - violet
    }
    if (uvAuto)
        // Auto color wins over any per-item/global color override
        // (still honors display policies such as night mode).
        DisplayManager.setTextColor(DisplayManager.resolveTextColor(uvColor));
    else
        applyNativeAppColor(uvColor, "UV");

    LayoutMetrics m = LayoutEngine::computeLayout(appConfig.nativeIconLayout, 0);

    static File uvIconGif;
    static bool uvIconChecked = false;
    static bool uvIconIsGif = false;
    static uint16_t uvIconFrame = 0;

    if (m.hasIcon)
    {
        if (!uvIconChecked)
        {
            uvIconChecked = true;
            if (LittleFS.exists("/ICONS/64310.gif"))
            {
                uvIconGif = LittleFS.open("/ICONS/64310.gif");
                uvIconIsGif = true;
            }
        }
        if (uvIconIsGif)
        {
            gifPlayer->playGif(x + m.iconX, y, &uvIconGif, uvIconFrame);
            uvIconFrame = gifPlayer->getFrame();
        }
        else
        {
            matrix->drawRGBBitmap(x + m.iconX, y, icon_59801, 8, 8);
        }
    }

    String uvStr;
    if (weatherData.valid)
    {
        uvStr = "UV:" + String(static_cast<int>(weatherData.uv));
    }
    else
    {
        uvStr = "UV:--";
    }

    uint16_t textWidth = getTextWidth(uvStr.c_str(), 0);
    LayoutMetrics tm = LayoutEngine::computeLayout(appConfig.nativeIconLayout, textWidth);
    int16_t textX = tm.textCenterX;

    DisplayManager.setCursor(textX + x, 6 + y);
    DisplayManager.matrixPrint(uvStr.c_str());
}

// ── MoonApp ────────────────────────────────────────────────────────

namespace
{
// Blue twinkling-star field drawn behind the moon.
constexpr int kMoonStarCount = 12;
struct MoonStar
{
    uint8_t sx, sy; // position
    uint8_t phase;  // twinkle phase offset
    uint8_t speed;  // twinkle speed
    uint8_t peak;   // max blue brightness
};

// Lunar maria (dark patches) on the 8×8 disk, for a touch of realism.
constexpr uint8_t kMariaCount = 4;
constexpr uint8_t kMaria[kMariaCount][2] = {{3, 2}, {2, 4}, {4, 3}, {5, 5}};

// Rotating-text cadence (ms per info item).
constexpr uint32_t kMoonInfoPeriod = 3200;
} // namespace

/// Native moon-phase app: a physically-shaded grayscale moon anchored left,
/// a blue twinkling-star background, and rotating info text (phase name /
/// lunar age / illumination %) on the right. Phase, age and illumination come
/// from the pure MoonPhase service (UTC-based). Hemisphere flips the lit limb.
void MoonApp(FastLED_NeoMatrix *matrix, MatrixDisplayUiState *state, int16_t x, int16_t y, GifPlayer *gifPlayer)
{
    if (nativeAppGuard("Moon"))
        return;

    MoonData moon = computeMoonPhase(time(nullptr));
    const uint32_t now = millis();

    // Detect (re)entry: MoonApp is called every frame while visible, so a gap
    // > 400ms means the rotation just switched to us. On entry we restart the
    // info rotation at slot 0 (phase name) so it always begins the same way.
    static uint32_t lastCallMs = 0;
    static uint32_t appEnterMs = 0;
    if (now - lastCallMs > 400)
        appEnterMs = now;
    lastCallMs = now;

    // Erase whatever the global background effect drew — the Moon app brings
    // its own (blue stars) and should look the same regardless of the
    // configured background effect.
    DisplayManager.drawFilledRect(0 + x, 0 + y, 32, 8, 0x000000);

    // Layout: with info text the moon is anchored left and stars fill the area
    // to its right; with no text ("only moon") the moon is centered and stars
    // fill the whole screen.
    uint8_t info = appConfig.moonInfo & 0x07;
    int items[3];
    int nItems = 0;
    if (info & 0x01)
        items[nItems++] = 0; // phase name
    if (info & 0x02)
        items[nItems++] = 1; // lunar age
    if (info & 0x04)
        items[nItems++] = 2; // illumination %
    const bool onlyMoon = (nItems == 0);
    const uint8_t starXMin = onlyMoon ? 0 : 9;
    const float moonCx = onlyMoon ? 15.5f : 3.5f;

    // ── 1. Blue twinkling-star background (drawn first, behind everything) ──
    static MoonStar stars[kMoonStarCount];
    static bool starsInit = false;
    if (!starsInit)
    {
        for (int i = 0; i < kMoonStarCount; i++)
        {
            stars[i].sx = random8(starXMin, 32);
            stars[i].sy = random8(0, 8);
            stars[i].phase = random8();
            stars[i].speed = random8(2, 6);
            stars[i].peak = random8(70, 170);
        }
        starsInit = true;
    }
    for (int i = 0; i < kMoonStarCount; i++)
    {
        // Each star fades in→out on its own cycle; when it finishes (wraps to
        // dark) it respawns at a new random spot, so the field is genuinely
        // random and constantly shifting (and reaches the moon's near edge).
        uint8_t prev = stars[i].phase;
        stars[i].phase += stars[i].speed;
        if (stars[i].phase < prev) // wrapped → just went dark → relocate
        {
            stars[i].sx = random8(starXMin, 32);
            stars[i].sy = random8(0, 8);
            stars[i].speed = random8(2, 6);
            stars[i].peak = random8(80, 185);
        }
        // Triangle brightness: 0 → peak → 0 across the cycle (dark at the ends).
        uint8_t tri = stars[i].phase < 128 ? static_cast<uint8_t>(stars[i].phase << 1)
                                           : static_cast<uint8_t>((255 - stars[i].phase) << 1);
        uint16_t b = (static_cast<uint16_t>(tri) * stars[i].peak) >> 8; // scale to peak
        uint32_t col = (static_cast<uint32_t>(b >> 2) << 8) | b;        // cool blue
        DisplayManager.drawPixel(stars[i].sx + x, stars[i].sy + y, col);
    }

    // ── 2. Rotating info text (right of the moon; clipped to a 1px gap) ──
    if (!onlyMoon)
    {
        const int16_t areaX0 = 9;          // 1px gap after the 8px moon
        const int16_t areaW = 32 - areaX0; // 23px text area
        uint16_t spd = appConfig.scrollSpeed > 0 ? appConfig.scrollSpeed : 100;

        // Sequencer: show each enabled item in turn, restarting at slot 0
        // (phase name) on entry. A scrolling item stays until it has marqueed
        // once in full, so long names (e.g. "Cuarto creciente") are readable.
        static int curSlot = 0;
        static uint32_t slotStartMs = 0;
        static uint32_t seenEnter = 0;
        if (seenEnter != appEnterMs)
        {
            seenEnter = appEnterMs;
            curSlot = 0;
            slotStartMs = now;
        }
        if (curSlot >= nItems)
            curSlot = 0;

        String s;
        switch (items[curSlot])
        {
        case 0:
            s = moonPhaseName(moon.phaseIndex);
            break;
        case 1:
        {
            int age = static_cast<int>(lround(moon.ageDays));
            s = String(age) + (age == 1 ? " DIA" : " DIAS");
            break;
        }
        default:
            s = String(moon.illumination) + "%";
            break;
        }

        applyNativeAppColor(0, "Moon"); // per-item / global text color (honors night policy)

        uint16_t tw = getTextWidth(s.c_str(), 0);
        bool scrolling = tw > static_cast<uint16_t>(areaW);
        uint32_t elapsed = now - slotStartMs;

        if (!scrolling)
        {
            int16_t tx = areaX0 + (areaW - static_cast<int16_t>(tw)) / 2;
            DisplayManager.setCursor(tx + x, 6 + y);
            DisplayManager.matrixPrint(s.c_str());
        }
        else
        {
            // Marquee: enter from the right and scroll left at the Apps-tab
            // speed (appConfig.scrollSpeed = ms per pixel).
            int off = static_cast<int>(elapsed / spd);
            int16_t drawX = areaX0 + areaW - static_cast<int16_t>(off);
            DisplayManager.setCursor(drawX + x, 6 + y);
            DisplayManager.matrixPrint(s.c_str());
        }

        // Advance: short items after a fixed dwell; scrolling items only after
        // a full pass (+ a short tail) so the whole string has been shown.
        uint32_t slotDur = scrolling ? (static_cast<uint32_t>(tw + areaW) * spd + 700) : kMoonInfoPeriod;
        if (elapsed >= slotDur)
        {
            curSlot = (curSlot + 1) % nItems;
            slotStartMs = now;
        }

        // Clip the text: black out the moon column + 1px gap (cols 0..8) so the
        // scrolling text never merges with the moon or leaves stray LEDs to its left.
        DisplayManager.drawFilledRect(0 + x, 0 + y, 9, 8, 0x000000);
    }

    // ── 3. The moon — physically-shaded grayscale sphere (drawn on top) ──
    // Slightly vertical ellipse (Ry > Rx); centered when there is no info text.
    const float cx = moonCx, cy = 3.5f;
    const float Rx = 3.55f, Ry = 3.8f; // near-round with a faint vertical hint
    const float Rmean = (Rx + Ry) * 0.5f;
    const float theta = static_cast<float>(moon.fraction) * 6.2831853f;
    const float sinT = sinf(theta), cosT = cosf(theta);
    const float hemi = (appConfig.moonHemisphere == 1) ? -1.0f : 1.0f; // S flips lit limb
    const float kEarth = 0.06f;                                        // earthshine floor on the dark side

    // Iterate the columns around the disk center (cols 0..7 when left-anchored,
    // ~12..19 when centered) so the moon draws wherever cx places it.
    const int pxLo = static_cast<int>(cx) - 4;
    const int pxHi = static_cast<int>(cx) + 4;
    const int mariaShift = static_cast<int>(cx - 3.5f); // map abs col → 0..7 local frame for maria
    for (int py = 0; py < 8; py++)
    {
        for (int px = pxLo; px <= pxHi; px++)
        {
            if (px < 0 || px > 31)
                continue;
            float ddx = px - cx;
            float ddy = py - cy;
            // Normalized elliptical coords (1.0 = edge); nx/ny double as the
            // unit-sphere coords so the shading maps onto the ellipse.
            float nx = ddx / Rx;
            float ny = ddy / Ry;
            float e = sqrtf(nx * nx + ny * ny);
            float coverage = (1.0f - e) * Rmean + 0.5f; // ~1px anti-aliased edge
            if (coverage <= 0.0f)
                continue;
            if (coverage > 1.0f)
                coverage = 1.0f;

            float zz = 1.0f - nx * nx - ny * ny;
            float z = (zz > 0.0f) ? sqrtf(zz) : 0.0f;

            // Lit intensity = surface-normal · sun direction (physically-based
            // sphere shading → soft terminator + limb darkening for free).
            float lit = (nx * hemi) * sinT - z * cosT;
            if (lit < 0.0f)
                lit = 0.0f;

            float bright = kEarth + lit * (1.0f - kEarth);

            // Maria: darken patches that are reasonably lit.
            if (lit > 0.12f)
            {
                for (uint8_t mi = 0; mi < kMariaCount; mi++)
                {
                    if (kMaria[mi][0] == px - mariaShift && kMaria[mi][1] == py)
                    {
                        bright *= 0.60f;
                        break;
                    }
                }
            }

            bright *= coverage; // soften the rim
            int g = static_cast<int>(bright * 235.0f);
            if (g < 0)
                g = 0;
            if (g > 255)
                g = 255;
            // Cool grayscale (moonlight): neutral with a faint blue lift.
            int bl = g + (g >> 3);
            if (bl > 255)
                bl = 255;
            uint32_t col = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(bl);
            DisplayManager.drawPixel(px + x, py + y, col);
        }
    }
}
