#include "DataFetcher.h"
#include "Globals.h"
#include "IDisplayNavigation.h"
#include "FormatStringValidator.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <algorithm>

extern const char *rootCACertificate;

static const char *TAG = "DataFetcher";
static const char *SOURCES_PATH = "/DATAFETCHER/sources.json";
static constexpr size_t MAX_RESPONSE_SIZE = 4096;
static constexpr uint32_t MIN_FREE_HEAP = 60000; // headroom for SSL allocation

// Fetches run synchronously on the main loop (no background worker — a parallel
// TLS task triggered a socket-fd leak that froze fetching until reboot). Tight
// timeouts bound how long a single fetch can stall the render loop: a working
// weatherapi fetch is ~2s, a fully failing one is capped at ~connect+handshake+read.
static constexpr uint32_t HTTP_CONNECT_TIMEOUT = 4000; // TCP connect
static constexpr uint32_t HTTP_READ_TIMEOUT = 4000;    // response read
static constexpr uint32_t HANDSHAKE_TIMEOUT_S = 4;     // TLS handshake (default 120s)
static constexpr uint32_t WEATHER_RETRY_MS = 60000;    // re-attempt 60s after a failed weather fetch (vs full interval)

DataFetcher_& DataFetcher_::getInstance()
{
    static DataFetcher_ instance;
    return instance;
}

DataFetcher_& DataFetcher = DataFetcher_::getInstance();

void DataFetcher_::setNavigation(IDisplayNavigation *n)
{
    nav_ = n;
}

bool DataFetcher_::hasNavigation() const
{
    return nav_ != nullptr;
}

void DataFetcher_::setup()
{
    LittleFS.mkdir("/DATAFETCHER");
    loadSources();
    cleanupOrphanedApps();
    DEBUG_PRINTF("DataFetcher: loaded %d sources", sources_.size());
}

// ---------- tick: synchronous, one weather + one round-robin source per call ----------

void DataFetcher_::tick()
{
    unsigned long now = millis();

    // Weather API fetch (synchronous; blocks the loop ~2s on success)
    if (!weatherConfig.apiKey.isEmpty())
    {
        unsigned long weatherInterval = weatherConfig.updateInterval * 60000UL;
        // Due on first tick (lastWeatherFetch_==0 — also how forceWeatherFetch
        // schedules it), after the normal interval, or for a fast retry scheduled
        // after a failed fetch (recovers in ~60s instead of the full interval).
        bool due = (lastWeatherFetch_ == 0) || (now - lastWeatherFetch_ >= weatherInterval);
        bool retryDue = (weatherRetryAt_ != 0) && ((long)(now - weatherRetryAt_) >= 0);
        if (due || retryDue)
        {
            if (ESP.getFreeHeap() > MIN_FREE_HEAP)
            {
                lastWeatherFetch_ = now;
                weatherRetryAt_ = 0; // fetchWeather() re-arms it on failure
                fetchWeather();
            }
            else
            {
                DEBUG_PRINTLN(F("DataFetcher: low heap, skipping weather fetch"));
            }
        }
    }

    // Custom data sources (round-robin, one per tick)
    if (!nav_ || sources_.empty())
        return;

    size_t idx = nextFetchIndex_ % sources_.size();
    nextFetchIndex_ = (idx + 1) % sources_.size();

    if (!sources_[idx].enabled)
        return;

    // Fetch immediately if never fetched (lastFetch_==0 — forceFetch sets this),
    // or after the interval elapsed.
    bool shouldFetch = (lastFetch_[idx] == 0) ||
                       (now - lastFetch_[idx] >= sources_[idx].interval * 1000UL);
    if (shouldFetch)
    {
        if (ESP.getFreeHeap() > MIN_FREE_HEAP)
        {
            lastFetch_[idx] = now;
            fetchAndPush(idx);
        }
        else
        {
            DEBUG_PRINTLN(F("DataFetcher: low heap, skipping fetch"));
        }
    }
}

// ---------- single HTTP GET with socket cleanup ----------

int DataFetcher_::httpGet(const String& url, bool isHttps, String& outBody)
{
    HTTPClient http;
    WiFiClientSecure secClient;
    WiFiClient plainClient;

    if (isHttps)
    {
        // Don't validate cert — DataFetcher hits arbitrary third-party APIs whose
        // CAs we can't pin in advance. Do NOT call secClient.setTimeout() — it
        // hangs some hosts; HTTPClient's timeouts handle it correctly.
        secClient.setInsecure();
        // Bound the TLS handshake (its default is 120s and is NOT covered by
        // setConnectTimeout/setTimeout).
        secClient.setHandshakeTimeout(HANDSHAKE_TIMEOUT_S);
        http.begin(secClient, url);
    }
    else
    {
        http.begin(plainClient, url);
    }

    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT);
    http.setTimeout(HTTP_READ_TIMEOUT);
    http.addHeader("Accept", "application/json");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
        outBody = http.getString();
    http.end();
    // Force-close the socket. http.end() can leave a failed/torn TLS socket fd
    // dangling; unreleased fds accumulate until connect() fails instantly with -1.
    secClient.stop();
    plainClient.stop();
    return httpCode;
}

// ---------- HTTP fetch (custom source) — synchronous ----------

bool DataFetcher_::fetchAndPush(size_t index)
{
    const DataSourceConfig& src = sources_[index];
    bool isHttps = src.url.startsWith("https");

    // Extra heap check before SSL allocation
    if (isHttps && ESP.getFreeHeap() < MIN_FREE_HEAP + 20000)
    {
        DEBUG_PRINTF("DataFetcher: insufficient heap for HTTPS (%d bytes)", ESP.getFreeHeap());
        return false;
    }

    DEBUG_PRINTF("DataFetcher: GET %s (heap: %d)...", src.name.c_str(), ESP.getFreeHeap());
    String body;
    int httpCode = httpGet(src.url, isHttps, body);
    if (httpCode != HTTP_CODE_OK)
    {
        DEBUG_PRINTF("DataFetcher: GET %s failed: %d", src.name.c_str(), httpCode);
        return false;
    }

    if (body.length() > MAX_RESPONSE_SIZE)
    {
        DEBUG_PRINTF("DataFetcher: response too large (%d bytes), truncating", body.length());
        body = body.substring(0, MAX_RESPONSE_SIZE);
    }

    String value = extractJsonValue(body, src.jsonPath);
    if (value.isEmpty())
    {
        DEBUG_PRINTF("DataFetcher: path '%s' not found in response", src.jsonPath.c_str());
        return false; // data error — leaves health flag untouched
    }

    body = String(); // release before building the app JSON

    String formatted = formatValue(src, value);
    String appJson = buildCustomAppJson(src, formatted);
    if (nav_)
        nav_->parseCustomPage(src.name, appJson.c_str(), false);

    DEBUG_PRINTF("DataFetcher: %s = %s (done)", src.name.c_str(), formatted.c_str());
    return true;
}

// ---------- JSON path extraction (dot-notation, supports array indices as numbers) ----------

String DataFetcher_::extractJsonValue(const String& json, const String& path)
{
    // Use larger buffer - some exchange rate APIs return 3KB+ with all currencies
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, json);
    if (err)
        return "";

    // Walk the dot-separated path: "bpi.USD.rate_float" or "data.0.price"
    JsonVariant current = doc.as<JsonVariant>();
    int start = 0;

    while (start < (int)path.length())
    {
        int dot = path.indexOf('.', start);
        if (dot < 0)
            dot = path.length();

        String segment = path.substring(start, dot);

        if (current.is<JsonArray>())
        {
            int idx = segment.toInt();
            current = current[idx];
        }
        else if (current.is<JsonObject>())
        {
            current = current[segment];
        }
        else
        {
            return "";
        }

        if (current.isNull())
            return "";

        start = dot + 1;
    }

    // Return as string regardless of type
    if (current.is<const char *>())
        return current.as<const char *>();
    if (current.is<float>() || current.is<double>())
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", current.as<double>());
        return buf;
    }
    if (current.is<long>())
        return String(current.as<long>());
    if (current.is<bool>())
        return current.as<bool>() ? "true" : "false";

    // Fallback: serialize whatever it is
    String result;
    serializeJson(current, result);
    return result;
}

// ---------- Format value with printf-style pattern ----------

String DataFetcher_::formatValue(const DataSourceConfig& src, const String& raw)
{
    if (src.displayFormat.isEmpty())
        return raw;

    // Defense-in-depth: addSource() and loadSources() also validate, but treat
    // displayFormat as untrusted at every snprintf call site.
    if (!isSafeSingleArgFormat(src.displayFormat.c_str()))
        return raw;

    char buf[64];
    if (src.displayFormat.indexOf('f') >= 0 || src.displayFormat.indexOf('d') >= 0 ||
        src.displayFormat.indexOf('i') >= 0 || src.displayFormat.indexOf('g') >= 0)
    {
        double val = raw.toDouble();
        snprintf(buf, sizeof(buf), src.displayFormat.c_str(), val);
    }
    else
    {
        snprintf(buf, sizeof(buf), src.displayFormat.c_str(), raw.c_str());
    }
    return buf;
}

// ---------- Build custom app JSON ----------

String DataFetcher_::buildCustomAppJson(const DataSourceConfig& src, const String& value)
{
    StaticJsonDocument<512> doc;
    doc["text"] = value;
    doc["lifetime"] = 0; // No expiry — DataFetcher manages updates
    doc["noScroll"] = false;

    if (!src.icon.isEmpty())
        doc["icon"] = src.icon;
    if (!src.textColor.isEmpty())
        doc["color"] = src.textColor;
    if (src.duration > 0)
        doc["duration"] = src.duration;

    String result;
    serializeJson(doc, result);
    return result;
}

// ---------- Source management ----------

bool DataFetcher_::addSource(const char *json)
{
    DEBUG_PRINTF("DataFetcher: addSource received: %s", json);

    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, json))
    {
        DEBUG_PRINTLN(F("DataFetcher: addSource JSON parse error"));
        return false;
    }

    if (!doc.containsKey("name") || !doc.containsKey("url") || !doc.containsKey("jsonPath"))
    {
        DEBUG_PRINTLN(F("DataFetcher: addSource missing required fields"));
        return false;
    }

    DataSourceConfig cfg;
    cfg.name = doc["name"].as<String>();
    cfg.url = doc["url"].as<String>();
    cfg.jsonPath = doc["jsonPath"].as<String>();
    cfg.displayFormat = doc["displayFormat"] | "";
    cfg.icon = doc["icon"] | "";
    cfg.textColor = doc["color"] | "";
    cfg.interval = doc["interval"] | DataSourceConfig::DEFAULT_INTERVAL;
    cfg.duration = doc["duration"] | DataSourceConfig::DEFAULT_DURATION;
    cfg.enabled = doc["enabled"] | true;

    if (cfg.interval < DataSourceConfig::MIN_INTERVAL)
        cfg.interval = DataSourceConfig::MIN_INTERVAL;

    // Reject unsafe printf-style format strings at the API boundary.
    if (!isSafeSingleArgFormat(cfg.displayFormat.c_str()))
    {
        DEBUG_PRINTLN(F("DataFetcher: rejecting source with unsafe displayFormat"));
        return false;
    }

    // Update existing or add new
    auto existing = std::find_if(sources_.begin(), sources_.end(),
                                 [&](const DataSourceConfig& s)
                                 { return s.name == cfg.name; });
    if (existing != sources_.end())
    {
        DEBUG_PRINTF("DataFetcher: updating source '%s' (enabled=%d)", cfg.name.c_str(), cfg.enabled);
        bool wasEnabled = existing->enabled;
        *existing = cfg;
        saveSources();

        // If source was just disabled, remove its custom app from display
        if (wasEnabled && !cfg.enabled && nav_)
        {
            nav_->parseCustomPage(cfg.name, "{}", false);
        }
        return true;
    }

    if (sources_.size() >= DataSourceConfig::MAX_SOURCES)
    {
        DEBUG_PRINTLN(F("DataFetcher: max sources reached"));
        return false;
    }

    sources_.push_back(cfg);
    lastFetch_.push_back(0);
    saveSources();
    return true;
}

bool DataFetcher_::removeSource(const String& name)
{
    auto it = std::find_if(sources_.begin(), sources_.end(),
                           [&](const DataSourceConfig& s)
                           { return s.name == name; });
    if (it == sources_.end())
        return false;

    if (nav_)
        nav_->parseCustomPage(name, "{}", false);

    auto idx = std::distance(sources_.begin(), it);
    sources_.erase(it);
    lastFetch_.erase(lastFetch_.begin() + idx);
    if (nextFetchIndex_ >= sources_.size() && !sources_.empty())
        nextFetchIndex_ = 0;

    saveSources();
    return true;
}

String DataFetcher_::getSourcesAsJson()
{
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& src : sources_)
    {
        JsonObject obj = arr.createNestedObject();
        obj["name"] = src.name;
        obj["url"] = src.url;
        obj["jsonPath"] = src.jsonPath;
        obj["displayFormat"] = src.displayFormat;
        obj["icon"] = src.icon;
        obj["color"] = src.textColor;
        obj["interval"] = src.interval;
        obj["duration"] = src.duration;
        obj["enabled"] = src.enabled;
    }

    String result;
    serializeJson(doc, result);
    return result;
}

void DataFetcher_::forceFetch(const String& name)
{
    auto it = std::find_if(sources_.begin(), sources_.end(),
                           [&](const DataSourceConfig& s)
                           { return s.name == name; });
    if (it == sources_.end())
        return;

    auto idx = std::distance(sources_.begin(), it);
    lastFetch_[idx] = 0; // mark due; the main loop fetches it on the next tick
}

// ---------- Orphaned app cleanup ----------

void DataFetcher_::cleanupOrphanedApps()
{
    // Scan /CUSTOMAPPS/ for .json files that match source names
    // If a file exists but no corresponding enabled source exists, delete it
    // This cleans up custom apps created by DataFetcher when sources are removed

    File root = LittleFS.open("/CUSTOMAPPS");
    if (!root || !root.isDirectory())
    {
        if (root)
            root.close();
        return;
    }

    std::vector<String> toDelete;
    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            String fileName = file.name();
            // Extract app name from filename (remove .json extension)
            if (fileName.endsWith(".json"))
            {
                String appName = fileName.substring(0, fileName.length() - 5);
                // Check if there's an enabled source with this name
                bool hasSource = false;
                for (const auto& src : sources_)
                {
                    if (src.name == appName && src.enabled)
                    {
                        hasSource = true;
                        break;
                    }
                }
                // If no source exists, check if the file looks like a DataFetcher app
                // DataFetcher apps have lifetime:0 and simple structure
                if (!hasSource)
                {
                    String fullPath = String("/CUSTOMAPPS/") + fileName;
                    File appFile = LittleFS.open(fullPath, "r");
                    if (appFile)
                    {
                        StaticJsonDocument<512> doc;
                        if (!deserializeJson(doc, appFile))
                        {
                            // DataFetcher apps have lifetime=0 and no save=true
                            // They shouldn't be persisted, but if they are, clean them up
                            int lifetime = doc["lifetime"] | -1;
                            bool hasSave = doc.containsKey("save");
                            if (lifetime == 0 && !hasSave)
                            {
                                toDelete.push_back(fullPath);
                                DEBUG_PRINTF("DataFetcher: marking orphan for cleanup: %s", appName.c_str());
                            }
                        }
                        appFile.close();
                    }
                }
            }
        }
        file = root.openNextFile();
    }
    root.close();

    // Delete orphaned files and remove from display
    for (const String& path : toDelete)
    {
        String name = path.substring(12, path.length() - 5); // Extract name from /CUSTOMAPPS/X.json
        if (nav_)
            nav_->parseCustomPage(name, "{}", false);
        LittleFS.remove(path);
        DEBUG_PRINTF("DataFetcher: cleaned up orphan: %s", name.c_str());
    }
}

// ---------- LittleFS persistence ----------

void DataFetcher_::loadSources()
{
    File file = LittleFS.open(SOURCES_PATH, "r");
    if (!file)
        return;

    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, file))
    {
        file.close();
        return;
    }
    file.close();

    JsonArray arr = doc.as<JsonArray>();
    sources_.clear();
    lastFetch_.clear();

    for (JsonObject obj : arr)
    {
        DataSourceConfig cfg;
        cfg.name = obj["name"].as<String>();
        cfg.url = obj["url"].as<String>();
        cfg.jsonPath = obj["jsonPath"].as<String>();
        cfg.displayFormat = obj["displayFormat"] | "";
        cfg.icon = obj["icon"] | "";
        cfg.textColor = obj["color"] | "";
        cfg.interval = obj["interval"] | 300;
        cfg.duration = obj["duration"] | 0;
        cfg.enabled = obj["enabled"] | true;

        if (cfg.interval < DataSourceConfig::MIN_INTERVAL)
            cfg.interval = DataSourceConfig::MIN_INTERVAL;

        // Old persisted configs may contain unsafe formats — downgrade to raw
        // rather than dropping the source so users don't lose data silently.
        if (!isSafeSingleArgFormat(cfg.displayFormat.c_str()))
        {
            DEBUG_PRINTF("DataFetcher: source '%s' had unsafe displayFormat, downgrading to raw",
                         cfg.name.c_str());
            cfg.displayFormat = "";
        }

        sources_.push_back(cfg);
        lastFetch_.push_back(0); // Fetch on first tick
    }
}

void DataFetcher_::saveSources()
{
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& src : sources_)
    {
        JsonObject obj = arr.createNestedObject();
        obj["name"] = src.name;
        obj["url"] = src.url;
        obj["jsonPath"] = src.jsonPath;
        obj["displayFormat"] = src.displayFormat;
        obj["icon"] = src.icon;
        obj["color"] = src.textColor;
        obj["interval"] = src.interval;
        obj["duration"] = src.duration;
        obj["enabled"] = src.enabled;
    }

    File file = LittleFS.open(SOURCES_PATH, "w");
    if (!file)
    {
        DEBUG_PRINTLN(F("DataFetcher: failed to open file for writing"));
        return;
    }
    serializeJson(doc, file);
    file.close();
}

// ---------- Weather API ----------

String DataFetcher_::buildWeatherQuery()
{
    String q;
    switch (weatherConfig.locationType)
    {
    case WEATHER_LOC_CITY:
        q = weatherConfig.city;
        break;
    case WEATHER_LOC_COORDS:
        q = String(weatherConfig.latitude, 4) + "," + String(weatherConfig.longitude, 4);
        break;
    case WEATHER_LOC_STATION:
        q = weatherConfig.stationId;
        break;
    case WEATHER_LOC_AUTO_IP:
    default:
        q = "auto:ip";
        break;
    }
    return q;
}

void DataFetcher_::fetchWeather()
{
    String url = "https://api.weatherapi.com/v1/current.json?key=";
    url += weatherConfig.apiKey;
    url += "&q=";
    url += buildWeatherQuery();
    url += "&aqi=yes";

    DEBUG_PRINTF("DataFetcher: fetching weather from %s", url.c_str());

    String body;
    int httpCode = httpGet(url, true, body);
    if (httpCode != HTTP_CODE_OK)
    {
        DEBUG_PRINTF("DataFetcher: weather fetch failed: %d", httpCode);
        weatherRetryAt_ = millis() + WEATHER_RETRY_MS; // retry soon; keep last value
        return;                                        // weatherData left intact
    }

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, body);
    if (err)
    {
        DEBUG_PRINTF("DataFetcher: weather JSON parse error: %s", err.c_str());
        return; // data error — keep last value, leave health/retry untouched
    }

    JsonObject current = doc["current"];
    if (current.isNull())
    {
        DEBUG_PRINTLN(F("DataFetcher: weather response missing 'current' object"));
        return;
    }

    weatherData.outdoorTemp = timeConfig.isCelsius ? current["temp_c"].as<float>()
                                                   : current["temp_f"].as<float>();
    weatherData.outdoorHumidity = current["humidity"].as<float>();
    weatherData.pressure = current["pressure_mb"].as<float>();

    JsonObject condition = current["condition"];
    if (!condition.isNull())
    {
        weatherData.condition = condition["text"].as<String>();
        weatherData.conditionCode = condition["code"].as<int>();
    }

    JsonObject airQuality = current["air_quality"];
    weatherData.aqi = airQuality.isNull() ? 0 : airQuality["us-epa-index"].as<int>();
    weatherData.uv = current["uv"].as<float>();
    weatherData.lastUpdate = millis();
    weatherData.valid = true;

    weatherRetryAt_ = 0;
    DEBUG_PRINTF("DataFetcher: weather updated - %.1f%s, %s, AQI=%d, UV=%.1f",
                 weatherData.outdoorTemp, timeConfig.isCelsius ? "C" : "F",
                 weatherData.condition.c_str(), weatherData.aqi, weatherData.uv);
}

void DataFetcher_::forceWeatherFetch()
{
    // Don't fetch in the web-handler (async_tcp) context — just mark weather due
    // so the main loop does the synchronous fetch on its next tick. Avoids both
    // blocking the web server and the socket contention a parallel fetch caused.
    lastWeatherFetch_ = 0;
    weatherRetryAt_ = 0;
}
