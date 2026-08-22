#pragma once

#include <Arduino.h>
#include "LayoutEngine.h"

struct AuthConfig {
    String user;
    String pass;
};

struct WifiNetwork {
    String ssid;
    String password;
};

struct WifiConfig {
    WifiNetwork networks[3];
};

struct NetworkConfig {
    bool isStatic;
    String ip;
    String gateway;
    String subnet;
    String primaryDns;
    String secondaryDns;
};

struct BatteryConfig {
    uint8_t percent;
    uint16_t raw;
    uint16_t minRaw;
    uint16_t maxRaw;
};

struct HaConfig {
    bool discovery;
    String prefix;
};

struct MqttConfig {
    String host;
    uint16_t port;
    String user;
    String pass;
    String prefix;
};

enum TempSensorType : uint8_t {
    TEMP_SENSOR_TYPE_NONE    = 0,
    TEMP_SENSOR_TYPE_BME280  = 1,
    TEMP_SENSOR_TYPE_HTU21DF = 2,
    TEMP_SENSOR_TYPE_BMP280  = 3,
    TEMP_SENSOR_TYPE_SHT31   = 4,
};

struct SensorConfig {
    float currentTemp;
    float currentHum;
    float currentLux;
    uint16_t ldrRaw;
    uint8_t tempSensorType;
    bool sensorReading;
    bool sensorsStable;
    float tempOffset;
    float humOffset;
};

struct DisplayConfig {
    int matrixLayout;
    uint8_t matrixFps;
    bool matrixOff;
    bool mirrorDisplay;
    bool rotateScreen;
    bool uppercaseLetters;
    int backgroundEffect;
};

struct BrightnessConfig {
    int brightness;
    int brightnessPercent;
    bool autoBrightness;
    uint8_t minBrightness;
    uint8_t maxBrightness;
    float ldrGamma;
    float ldrFactor;
    bool ldrOnGround;
};

struct ColorConfig {
    uint32_t textColor;
    uint32_t timeColor;
    uint32_t dateColor;
    uint32_t batColor;
    uint32_t tempColor;
    uint32_t humColor;
    uint32_t wdcActive;
    uint32_t wdcInactive;
    uint32_t calendarHeaderColor;
    uint32_t calendarTextColor;
    uint32_t calendarBodyColor;
};

struct TimeConfig {
    String timeFormat;
    String dateFormat;
    uint8_t timeMode;
    bool startOnMonday;
    String ntpServer;
    String ntpTz;
    bool isCelsius;
    int tempDecimalPlaces;
};

struct AppConfig {
    bool showTime;
    bool showDate;
    bool showBat;
    bool showTemp;
    bool showHum;
    bool showWeekday;
    bool showAlarms;
    bool autoTransition;
    int8_t transEffect;
    int timePerTransition;
    long timePerApp;
    uint8_t scrollSpeed;
    uint16_t timeDuration;    // Clock display duration (1-300s)
    uint16_t dateDuration;    // Date display duration (1-60s)
    uint16_t tempDuration;    // Temperature display duration (1-60s)
    uint16_t humDuration;     // Humidity display duration (1-60s)
    uint16_t batDuration;     // Battery display duration (1-60s)
    IconLayout nativeIconLayout;
    bool blockNavigation;
    bool nightMode;
    uint16_t nightStart;      // 21:00 in minutes from midnight
    uint16_t nightEnd;        // 06:00 in minutes from midnight
    uint8_t nightBrightness;
    uint32_t nightColor;      // default: red (0xFF0000)
    bool nightBlockTransition; // disable auto-transition in night mode
    uint8_t moonInfo;          // Moon app: rotating-text bitmask (b0=name, b1=age, b2=illum%); 0 = moon only
    uint8_t moonHemisphere;    // Moon app: 0 = northern, 1 = southern (flips the lit limb)
    String appOrder;          // persisted JSON array of app names (unified app loop order)
};

struct AudioConfig {
    bool soundActive;
    uint8_t soundVolume;
    String bootSound;
};

struct SystemConfig {
    bool debugMode;
    uint32_t apTimeout;
    int webPort;
    String hostname;
    bool updateCheck;
    long statsInterval;
    bool newyear;
    bool swapButtons;
    String buttonCallback;
    String deviceId;
    bool updateAvailable;
    bool apMode;
    String updateVersionUrl;
    String updateFirmwareUrl;
};

enum WeatherLocationType : uint8_t {
    WEATHER_LOC_CITY = 0,
    WEATHER_LOC_COORDS = 1,
    WEATHER_LOC_AUTO_IP = 2,
    WEATHER_LOC_STATION = 3
};

struct WeatherConfig {
    String apiKey;
    WeatherLocationType locationType;
    String city;
    float latitude;
    float longitude;
    String stationId;         // PWS station ID (e.g. "pws:KMAHANOV10")
    uint16_t updateInterval;  // minutes (10, 15, 30, 60)
    bool showOutdoorTemp;
    bool showOutdoorHumidity;
    bool showPressure;
    bool showAirQuality;
    bool showIndoorTemp;
    bool showIndoorHumidity;
    bool showUV;
    // Display settings for weather apps
    uint32_t outdoorTempColor;
    uint32_t outdoorHumColor;
    uint32_t pressureColor;
    uint32_t aqiColor;
    uint32_t uvColor;
    bool aqiAutoColor;            // Use dynamic color based on AQI level
    bool uvAutoColor;             // Use dynamic color based on UV level
    uint8_t outdoorTempDuration;  // seconds
    uint8_t outdoorHumDuration;
    uint8_t pressureDuration;
    uint8_t aqiDuration;
    uint8_t uvDuration;
    bool aqiShowComponents;       // ICA app: rotate pollutants exceeding level 4
    uint8_t aqiComponentSecs;     // ICA app: seconds each flagged pollutant is shown (2-10)
    // Fuente propia (XE1E): si no está vacío, se consulta esta URL en lugar de
    // WeatherAPI.com. Debe devolver la MISMA forma de WeatherAPI current.json
    // (p. ej. https://clima.xe1e.net/api/svitrix), con campos extra opcionales
    // (solar_radiation, rain_rate_mm).
    String serverUrl;
    // App Viento (dirección + velocidad; opción de rotar la ráfaga). Estilo Luna.
    bool showWind;
    uint32_t windColor;
    uint8_t windDuration;
    bool windShowGust;            // rotar un cuadro con la ráfaga
    // App Radiación solar (W/m²) — separada del UV
    bool showRadiation;
    uint32_t radColor;
    bool radAutoColor;            // color dinámico según nivel de radiación (como UV)
    uint8_t radDuration;
    // App Precipitación (lluvia por evento; opción de rotar la tasa mm/h)
    bool showPrecip;
    uint32_t precipColor;
    uint8_t precipDuration;
    bool precipShowRate;
};

struct WeatherData {
    float outdoorTemp;
    float outdoorHumidity;
    float pressure;           // mb/hPa
    int aqi;                  // US EPA index (1-6)
    float uv;                 // UV index (0-11+)
    // Air-quality component concentrations in µg/m³ (from WeatherAPI air_quality)
    float pm2_5;
    float pm10;
    float o3;
    float no2;
    float so2;
    float co;
    String condition;         // "sunny", "cloudy", etc.
    int conditionCode;        // WeatherAPI condition code
    // Viento (WeatherAPI: wind_kph/wind_degree/wind_dir/gust_kph)
    float windSpeed;          // km/h
    int windDeg;              // grados (0-360)
    float windGust;           // km/h
    String windDir;           // rumbo (N, NE, …)
    // Extras del servidor propio (no en WeatherAPI): 0 si la fuente no los da
    float solarRadiation;     // W/m²
    float precipToday;        // mm acumulados hoy (WeatherAPI: precip_mm)
    float precipEvent;        // mm del evento de lluvia actual (extra: precip_event_mm)
    float rainRate;           // mm/h (extra)
    unsigned long lastUpdate; // millis() of last update
    bool valid;               // data available
    // El servidor propio respondió 503 ("sin lectura de la estación todavía")
    // u omitió `current`: se conserva el último valor conocido (valid puede
    // seguir en true) pero NO es un dato fresco. false tras cualquier fetch
    // completo y exitoso.
    bool stale;
    // 1 = día, 0 = noche (WeatherAPI y el servidor propio lo incluyen). Elige
    // sol/luna para el código 1000 ("Sunny"/"Clear"), que WeatherAPI usa para
    // ambos. Va al final del struct a propósito: Globals.cpp inicializa
    // WeatherData por posición con una lista más corta que el número de campos
    // (los que faltan se autoinicializan a cero) — insertarlo en medio
    // desalinea esa lista. NO lleva inicializador de miembro (`= 1`): este
    // header se compila con el framework Arduino-ESP32, que para algunas
    // unidades fuerza `-std=gnu++11` pese al `-std=c++17` de platformio.ini, y
    // antes de C++14 un inicializador de miembro invalida el agregado (mismo
    // error de conversión al compilar). Arranca en 0 hasta el primer fetch
    // exitoso; fetchWeather() ya asume "día" si el campo viene ausente/null.
    int isDay;
};

enum PlaylistItemType : uint8_t {
    PLAYLIST_ITEM_APP = 0,
    PLAYLIST_ITEM_EFFECT = 1,
};

struct PlaylistItem {
    PlaylistItemType type;    // app or effect
    String name;              // app name or effect name
    uint16_t duration;        // seconds (0 = use app's default duration)
};

struct PlaylistConfig {
    bool enabled;             // false = use simple appOrder mode
    String items;             // JSON array of playlist items
};

// Unified rotation config - replaces both appOrder and playlist
// Each item: {id, type, name, enabled, duration, color, icon}
enum RotationItemType : uint8_t {
    ROTATION_ITEM_APP = 0,
    ROTATION_ITEM_EFFECT = 1,
};

struct RotationConfig {
    String items;             // JSON array: [{id, type, name, enabled, duration, color, icon}, ...]
};
