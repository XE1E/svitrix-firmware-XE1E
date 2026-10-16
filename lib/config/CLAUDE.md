# Config Library — AI Reference

Shared configuration structs consumed by all modules. Single header, no logic, no dependencies beyond Arduino `String`.

## Files

| File | Purpose |
|------|---------|
| `src/ConfigTypes.h` | 20 config structs + 4 enums, all POD-style with public fields |

## Config Structs

### AuthConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `user` | String | `""` | HTTP username |
| `pass` | String | `"svitrix"` | HTTP password |

### WifiNetwork
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `ssid` | String | `""` | Network SSID |
| `password` | String | `""` | Network password |

### WifiConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `networks` | WifiNetwork[3] | — | Up to 3 configured networks, tried in order |

### NetworkConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `isStatic` | bool | `false` | Use static IP |
| `ip` | String | `"192.168.178.10"` | Static IP address |
| `gateway` | String | `"192.168.178.1"` | Gateway |
| `subnet` | String | `"255.255.255.0"` | Subnet mask |
| `primaryDns` | String | `"8.8.8.8"` | Primary DNS |
| `secondaryDns` | String | `"1.1.1.1"` | Secondary DNS |

### BatteryConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `percent` | uint8_t | `0` | Calculated battery % |
| `raw` | uint16_t | `0` | Current ADC reading |
| `minRaw` | uint16_t | `475` | ADC value = 0% |
| `maxRaw` | uint16_t | `665` | ADC value = 100% |

### HaConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `discovery` | bool | `false` | Enable HA discovery |
| `prefix` | String | `"homeassistant"` | HA discovery topic prefix |

### MqttConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `host` | String | `""` | Broker hostname/IP |
| `port` | uint16_t | `1883` | Broker port |
| `user` | String | `""` | MQTT username |
| `pass` | String | `""` | MQTT password |
| `prefix` | String | `""` | Topic prefix (set to deviceId at runtime) |

### SensorConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `currentTemp` | float | `0` | Current temperature reading |
| `currentHum` | float | `0` | Current humidity reading |
| `currentLux` | float | `0` | Current lux reading |
| `ldrRaw` | uint16_t | `0` | Raw LDR ADC value |
| `tempSensorType` | uint8_t | `TEMP_SENSOR_TYPE_NONE` | Detected sensor type |
| `sensorReading` | bool | `true` | Enable sensor polling |
| `sensorsStable` | bool | `false` | Sensors have stabilized |
| `tempOffset` | float | `-9` | Temperature calibration offset |
| `humOffset` | float | `0` | Humidity calibration offset |

### TempSensorType (enum)

| Value | Constant |
|-------|----------|
| 0 | `TEMP_SENSOR_TYPE_NONE` |
| 1 | `TEMP_SENSOR_TYPE_BME280` |
| 2 | `TEMP_SENSOR_TYPE_HTU21DF` |
| 3 | `TEMP_SENSOR_TYPE_BMP280` |
| 4 | `TEMP_SENSOR_TYPE_SHT31` |

### DisplayConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `matrixLayout` | int | `0` | NeoMatrix layout flag |
| `matrixFps` | uint8_t | `42` | Target FPS |
| `matrixOff` | bool | `false` | Display powered off |
| `mirrorDisplay` | bool | `false` | Mirror horizontally |
| `rotateScreen` | bool | `false` | Rotate 180 degrees |
| `uppercaseLetters` | bool | `true` | Force uppercase text |
| `backgroundEffect` | int | `-1` | Active background effect index (-1 = none) |

### BrightnessConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `brightness` | int | `30` | Current brightness (0-255) |
| `brightnessPercent` | int | `0` | Brightness as percentage |
| `autoBrightness` | bool | `true` | Enable LDR auto-brightness |
| `minBrightness` | uint8_t | `2` | Auto-brightness floor |
| `maxBrightness` | uint8_t | `160` | Auto-brightness ceiling |
| `ldrGamma` | float | `3.0` | LDR gamma curve exponent |
| `ldrFactor` | float | `1.0` | LDR scaling factor |
| `ldrOnGround` | bool | `false` | LDR mounted on ground side |

### ColorConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `textColor` | uint32_t | `0xFFFFFF` | Default text color (white) |
| `timeColor` | uint32_t | `0` | Time app color (0 = use textColor) |
| `dateColor` | uint32_t | `0` | Date app color |
| `batColor` | uint32_t | `0` | Battery app color |
| `tempColor` | uint32_t | `0` | Temperature app color |
| `humColor` | uint32_t | `0` | Humidity app color |
| `wdcActive` | uint32_t | `0xFFFFFF` | Weekday indicator active |
| `wdcInactive` | uint32_t | `0x666666` | Weekday indicator inactive |
| `calendarHeaderColor` | uint32_t | `0xFF0000` | Calendar header (red) |
| `calendarTextColor` | uint32_t | `0x000000` | Calendar text (black) |
| `calendarBodyColor` | uint32_t | `0xFFFFFF` | Calendar body (white) |

### TimeConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `timeFormat` | String | `"%H:%M:%S"` | strftime format for time |
| `dateFormat` | String | `"%d.%m.%y"` | strftime format for date |
| `timeMode` | uint8_t | `1` | 0 = 12h, 1 = 24h |
| `startOnMonday` | bool | `false` | Week starts on Monday |
| `ntpServer` | String | `"time.cloudflare.com"` | NTP server |
| `ntpTz` | String | `"CET-1CEST,M3.5.0,M10.5.0/3"` | POSIX timezone |
| `isCelsius` | bool | `true` | Temperature unit |
| `tempDecimalPlaces` | int | `0` | Decimal places for temp display |

### AppConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `showTime` | bool | `true` | Show time app |
| `showDate` | bool | `true` | Show date app |
| `showBat` | bool | `true` | Show battery app (Ulanzi only) |
| `showTemp` | bool | `true` | Show temperature app |
| `showHum` | bool | `true` | Show humidity app |
| `showWeekday` | bool | `true` | Show weekday indicator |
| `autoTransition` | bool | `false` | Auto-cycle through apps |
| `transEffect` | int8_t | `1` | Transition effect index |
| `timePerTransition` | int | `400` | Transition duration (ms) |
| `timePerApp` | long | `7000` | Time per app before auto-transition (ms) |
| `scrollSpeed` | uint8_t | `100` | Text scroll speed (ms per pixel) |
| `nativeIconLayout` | IconLayout | `Left` | Icon position for native apps |
| `blockNavigation` | bool | `false` | Block button navigation |
| `nightMode` | bool | `false` | Enable scheduled night mode |
| `nightStart` | uint16_t | `1260` | Night start (minutes from midnight, 21:00) |
| `nightEnd` | uint16_t | `360` | Night end (minutes from midnight, 06:00) |
| `nightBrightness` | uint8_t | `5` | Brightness during night mode |
| `nightColor` | uint32_t | `0xFF0000` | Text color during night mode (red) |
| `nightBlockTransition` | bool | `true` | Disable auto-transition in night mode |
| `appOrder` | String | `""` | Persisted JSON array of app names (unified app loop order). NVS key `APPORDER` |

### AudioConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `soundActive` | bool | `false` | Enable sound |
| `soundVolume` | uint8_t | `30` | Volume level |
| `bootSound` | String | `""` | RTTTL melody on boot |

### WeatherLocationType (enum)

| Value | Constant |
|-------|----------|
| 0 | `WEATHER_LOC_CITY` |
| 1 | `WEATHER_LOC_COORDS` |
| 2 | `WEATHER_LOC_AUTO_IP` |
| 3 | `WEATHER_LOC_STATION` |

### WeatherConfig
| Field | Type | Notes |
|-------|------|-------|
| `apiKey` | String | WeatherAPI.com API key (ignored when `serverUrl` is set) |
| `locationType` | WeatherLocationType | City / coords / auto-IP / PWS station |
| `city` | String | City name query |
| `latitude` / `longitude` | float | Coordinate query |
| `stationId` | String | PWS station ID (e.g. `"pws:KMAHANOV10"`) |
| `updateInterval` | uint16_t | Minutes between fetches (1, 2, 5, 10, 15, 30, 60 — sub-10min meant for a low-latency own server, not the rate-limited WeatherAPI.com) |
| `serverUrl` | String | Own weather server (XE1E) URL; when non-empty, replaces WeatherAPI.com entirely. Must return the same shape as WeatherAPI's `current.json`, with optional extra fields (`solar_radiation`, `precip_event_mm`, `rain_rate_mm`). HTTP vs HTTPS is chosen from the URL scheme |
| `showOutdoorTemp` / `showOutdoorHumidity` / `showPressure` / `showAirQuality` / `showUV` / `showWind` / `showRadiation` / `showPrecip` | bool | Enable each weather app |
| `showIndoorTemp` / `showIndoorHumidity` | bool | Enable indoor sensor apps |
| `outdoorTempColor` / `outdoorHumColor` / `pressureColor` / `aqiColor` / `uvColor` / `windColor` / `radColor` / `precipColor` | uint32_t | Per-app text color |
| `aqiAutoColor` / `uvAutoColor` / `radAutoColor` | bool | Dynamic color by level instead of fixed color |
| `outdoorTempDuration` / `outdoorHumDuration` / `pressureDuration` / `aqiDuration` / `uvDuration` / `windDuration` / `radDuration` / `precipDuration` | uint8_t | Seconds shown (legacy default; rotation-item overrides win) |
| `aqiShowComponents` | bool | ICA app: rotate pollutants exceeding level 4 after the index |
| `aqiComponentSecs` | uint8_t | Seconds each flagged pollutant is shown (2-10) |
| `windShowGust` | bool | Wind app: rotate a second frame with the gust speed |
| `precipShowRate` | bool | Precip app: append the current rain rate (mm/h) |

### WeatherData
Runtime weather state (not persisted) — written by `DataFetcher::fetchWeather()`, read by the weather apps and `/api/weather/data`.

| Field | Type | Notes |
|-------|------|-------|
| `outdoorTemp` / `outdoorHumidity` / `pressure` (mb) | float | Core readings |
| `aqi` | int | US-EPA index (1-6) |
| `uv` | float | UV index |
| `pm2_5` / `pm10` / `o3` / `no2` / `so2` / `co` | float | Per-pollutant concentrations (µg/m³), from WeatherAPI's `air_quality` (not exposed by `/api/weather/data`, only used by the ICA app) |
| `condition` / `conditionCode` | String / int | WeatherAPI condition text/code |
| `windSpeed` (km/h) / `windDeg` / `windGust` (km/h) / `windDir` | float/int/float/String | Wind, shared by WeatherAPI and the own server |
| `solarRadiation` (W/m²) / `precipEvent` (mm) / `rainRate` (mm/h) | float | Extras only the own server provides; 0 with WeatherAPI |
| `precipToday` | float | mm accumulated today (WeatherAPI `precip_mm`) |
| `lastUpdate` | unsigned long | `millis()` of the last successful fetch |
| `valid` | bool | Data available (at least one successful fetch since boot) |
| `stale` | bool | Last fetch kept the previous value instead of refreshing it — own-server 503 ("no station reading yet") or a response missing `current`. `valid` can stay `true` while `stale` is `true` |
| `isDay` | int | 1 = day, 0 = night; picks sun vs. the real moon phase icon for condition code 1000 ("Sunny"/"Clear", shared by both). Defaults to day (1) if the source omits it |

### PlaylistItemType (enum)

| Value | Constant |
|-------|----------|
| 0 | `PLAYLIST_ITEM_APP` |
| 1 | `PLAYLIST_ITEM_EFFECT` |

### PlaylistItem
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `type` | PlaylistItemType | — | App or standalone effect |
| `name` | String | — | App name or effect name |
| `duration` | uint16_t | `0` | Seconds (0 = use app's default) |

### PlaylistConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `enabled` | bool | `false` | false = use simple appOrder mode |
| `items` | String | `""` | JSON array of PlaylistItem objects |

### RotationItemType (enum)

| Value | Constant |
|-------|----------|
| 0 | `ROTATION_ITEM_APP` |
| 1 | `ROTATION_ITEM_EFFECT` |

### RotationConfig
| Field | Type | Notes |
|-------|------|-------|
| `items` | String | JSON array: `[{id, type, name, enabled, duration, color, icon}, ...]` — unified rotation order, replaces both `appConfig.appOrder` and `PlaylistConfig` |

### SystemConfig
| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `debugMode` | bool | `true` | Enable debug serial output |
| `apTimeout` | uint32_t | `15` | AP mode timeout (seconds) |
| `webPort` | int | `80` | HTTP server port |
| `hostname` | String | `""` | mDNS hostname (set to deviceId) |
| `updateCheck` | bool | `false` | Check for firmware updates |
| `statsInterval` | long | `10000` | MQTT stats publish interval (ms) |
| `newyear` | bool | `false` | New Year countdown mode |
| `swapButtons` | bool | `false` | Swap left/right buttons |
| `buttonCallback` | String | `""` | HTTP URL for button press callback |
| `deviceId` | String | `""` | Unique device ID (from MAC) |
| `updateAvailable` | bool | `false` | Runtime: OTA update available |
| `apMode` | bool | `false` | Runtime: in AP mode |
| `updateVersionUrl` | String | `""` | Custom update version check URL |
| `updateFirmwareUrl` | String | `""` | Custom firmware download URL |

## Persistence

Config values come from **three layers** (later overrides earlier):

1. **Compile-time defaults** — initializer lists in `Globals.cpp`
2. **NVS (Preferences)** — `loadSettings()` / `saveSettings()` (namespace `"svitrix"`)
3. **`/dev.json` (LittleFS)** — `loadDevSettings()` overrides (highest priority)

## Who Uses What

| Config Struct | Consumers |
|---------------|-----------|
| `appConfig` | DisplayManager, MQTTManager, ServerManager, MenuManager, Apps, MatrixDisplayUi |
| `displayConfig` | DisplayManager, MQTTManager, ServerManager, MenuManager, DisplayRenderer |
| `brightnessConfig` | DisplayManager, PeripheryManager, MQTTManager, ServerManager, MenuManager |
| `colorConfig` | DisplayManager, DisplayRenderer, Apps, MQTTManager, ServerManager, MenuManager |
| `timeConfig` | Apps, MQTTManager, ServerManager, MenuManager, Overlays |
| `sensorConfig` | PeripheryManager, Apps, MQTTManager, ServerManager |
| `systemConfig` | Nearly all modules |
| `mqttConfig` | MQTTManager, ServerManager |
| `audioConfig` | ServerManager, MQTTManager, MenuManager, PeripheryManager |
| `batteryConfig` | PeripheryManager, Apps, MQTTManager |
| `haConfig` | MQTTManager |
| `networkConfig` | main.cpp (WiFi setup) |
| `authConfig` | ServerManager |
| `playlistConfig` | DisplayManager, ServerManager |
| `rotationConfig` | DisplayManager, ServerManager |
| `wifiConfig` | main.cpp (WiFi setup), ServerManager |
| `weatherConfig` | DataFetcher, Apps (weather apps), ServerManager |
| `weatherData` | Apps (weather apps), ServerManager, DataFetcher (writer) |

## Design Notes

- Structs are plain C++ aggregates (no constructors, no methods)
- All 13 instances declared `extern` in `Globals.h`, defined in `Globals.cpp`
- Color fields use `uint32_t` (`0xRRGGBB`), not `CRGB` — avoids FastLED dependency
- Some fields serve double duty as runtime state (e.g., `sensorConfig.currentTemp`)
- Only dependency: `Arduino.h` (for `String` type)
- Compiles in both `ulanzi` (ESP32) and `native_test` environments
