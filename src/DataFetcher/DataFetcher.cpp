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
static constexpr uint32_t HTTP_CONNECT_TIMEOUT = 10000; // 10s for SSL handshake
static constexpr uint32_t HTTP_READ_TIMEOUT = 15000;    // 15s for slow APIs
static constexpr uint32_t MIN_FREE_HEAP = 60000;        // Increased for SSL overhead

// Background worker task: all blocking HTTP/TLS work happens here on core 0
// so a stalled request can never freeze the render loop (core 1).
static constexpr uint32_t WORKER_STACK = 10240;        // 10 KB — proven 8 KB TLS path + margin
static constexpr UBaseType_t WORKER_PRIORITY = 1;      // same as boot animation task
static constexpr BaseType_t WORKER_CORE = 0;           // Arduino loop runs on core 1
static constexpr UBaseType_t QUEUE_LEN = 6;            // pending fetch requests / results
static constexpr int FETCH_ATTEMPTS = 3;               // GET tries before declaring failure
static constexpr uint32_t FETCH_RETRY_DELAY_MS = 1000; // backoff between attempts
static constexpr uint32_t WEATHER_RETRY_MS = 60000;    // re-attempt 60s after a failed weather fetch (vs full interval)

// Work items exchanged with the worker. Only pointers cross the queues
// (Strings can't be byte-copied through a queue without double-free), so the
// receiver takes ownership and deletes.
struct FetchRequest
{
    enum Type
    {
        CUSTOM,
        WEATHER
    } type;
    DataSourceConfig cfg;  // CUSTOM: full source copy (worker never reads sources_)
    String url;            // WEATHER: pre-built request URL
    bool isCelsius = true; // WEATHER: unit captured on the main thread
};

struct FetchResult
{
    enum Type
    {
        CUSTOM,
        WEATHER
    } type;
    bool success = false;      // value produced and ready to apply
    bool networkError = false; // HTTP/connection failure (feeds the WiFi health LED)
    String name;               // CUSTOM: custom app name
    String appJson;            // CUSTOM: JSON for parseCustomPage()
    WeatherData weather{};     // WEATHER: parsed payload (applied to global on main loop)
};

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

    // Spin up the background fetch worker + handoff queues (once).
    if (!requestQueue_)
        requestQueue_ = xQueueCreate(QUEUE_LEN, sizeof(FetchRequest *));
    if (!resultQueue_)
        resultQueue_ = xQueueCreate(QUEUE_LEN, sizeof(FetchResult *));
    if (requestQueue_ && resultQueue_ && !workerHandle_)
    {
        xTaskCreatePinnedToCore(workerTask, "DataFetch", WORKER_STACK, this,
                                WORKER_PRIORITY, &workerHandle_, WORKER_CORE);
    }

    DEBUG_PRINTF("DataFetcher: loaded %d sources", sources_.size());
}

// ---------- tick: staggered one-source-per-call ----------

void DataFetcher_::tick()
{
    // Apply any fetches the worker completed since last tick. This is the ONLY
    // place display / weatherData are mutated, so it stays on the render core.
    drainResults();

    unsigned long now = millis();

    // Weather API fetch (independent of custom data sources)
    if (!weatherConfig.apiKey.isEmpty())
    {
        unsigned long weatherInterval = weatherConfig.updateInterval * 60000UL;
        // Due on first tick (lastWeatherFetch_==0), after the normal interval, or
        // for a fast retry scheduled after a failed fetch (so a boot/transient
        // failure recovers in ~60s instead of waiting the full interval).
        bool due = (lastWeatherFetch_ == 0) || (now - lastWeatherFetch_ >= weatherInterval);
        bool retryDue = (weatherRetryAt_ != 0) && ((long)(now - weatherRetryAt_) >= 0);
        if (due || retryDue)
        {
            if (ESP.getFreeHeap() > MIN_FREE_HEAP)
            {
                if (enqueueWeather())
                {
                    lastWeatherFetch_ = now;
                    weatherRetryAt_ = 0; // suppress re-enqueue until the result returns
                }
            }
            else
            {
                DEBUG_PRINTLN(F("DataFetcher: low heap, skipping weather fetch"));
            }
        }
    }

    // Custom data sources (round-robin)
    if (!nav_ || sources_.empty())
        return;

    // Round-robin: check one source per tick to avoid bursting the queue
    size_t idx = nextFetchIndex_ % sources_.size();
    nextFetchIndex_ = (idx + 1) % sources_.size();

    // Skip disabled sources
    if (!sources_[idx].enabled)
        return;

    // Fetch immediately if never fetched (lastFetch_==0), or after interval elapsed
    bool shouldFetch = (lastFetch_[idx] == 0) ||
                       (now - lastFetch_[idx] >= sources_[idx].interval * 1000UL);
    if (shouldFetch)
    {
        if (ESP.getFreeHeap() > MIN_FREE_HEAP)
        {
            // Mark as fetched on enqueue so we don't re-queue the same source
            // every tick while the worker is busy. If the queue is full we keep
            // lastFetch_ untouched so it retries next round.
            if (enqueueCustom(sources_[idx]))
                lastFetch_[idx] = now;
        }
        else
        {
            DEBUG_PRINTLN(F("DataFetcher: low heap, skipping fetch"));
        }
    }
}

// ---------- main-loop side: enqueue requests + apply results ----------

bool DataFetcher_::enqueueCustom(const DataSourceConfig& src)
{
    if (!requestQueue_)
        return false;
    FetchRequest *req = new FetchRequest();
    req->type = FetchRequest::CUSTOM;
    req->cfg = src;
    if (xQueueSend(requestQueue_, &req, 0) != pdTRUE)
    {
        DEBUG_PRINTF("DataFetcher: request queue full, dropping %s", src.name.c_str());
        delete req;
        return false;
    }
    return true;
}

bool DataFetcher_::enqueueWeather()
{
    if (!requestQueue_ || weatherConfig.apiKey.isEmpty())
        return false;

    // Build the URL + capture the unit here (main thread) so the worker never
    // touches weatherConfig / timeConfig globals.
    String url = "https://api.weatherapi.com/v1/current.json?key=";
    url += weatherConfig.apiKey;
    url += "&q=";
    url += buildWeatherQuery();
    url += "&aqi=yes";

    FetchRequest *req = new FetchRequest();
    req->type = FetchRequest::WEATHER;
    req->url = url;
    req->isCelsius = timeConfig.isCelsius;
    if (xQueueSend(requestQueue_, &req, 0) != pdTRUE)
    {
        DEBUG_PRINTLN(F("DataFetcher: request queue full, dropping weather"));
        delete req;
        return false;
    }
    return true;
}

void DataFetcher_::drainResults()
{
    if (!resultQueue_)
        return;

    FetchResult *res = nullptr;
    while (xQueueReceive(resultQueue_, &res, 0) == pdTRUE)
    {
        if (!res)
            continue;

        // Track connectivity health for the status LED: a network failure trips
        // it, any successful fetch clears it, a pure data error leaves it as-is.
        if (res->networkError)
            fetchHealthy_ = false;
        else if (res->success)
            fetchHealthy_ = true;

        if (res->type == FetchResult::CUSTOM)
        {
            if (res->success && nav_)
            {
                DEBUG_PRINTF("DataFetcher: applying %s", res->name.c_str());
                nav_->parseCustomPage(res->name, res->appJson.c_str(), false);
            }
        }
        else // WEATHER
        {
            if (res->weather.valid)
            {
                // Keep the last good reading on a failed refresh so the display
                // doesn't blank to "--"; overwrite only on success.
                weatherData = res->weather;
                weatherRetryAt_ = 0; // got data, back to normal interval
            }
            else
            {
                // Failed (incl. the boot fetch before WiFi/DNS settle): retry
                // soon instead of waiting the full 30-min interval.
                weatherRetryAt_ = millis() + WEATHER_RETRY_MS;
            }
        }
        delete res;
    }
}

bool DataFetcher_::fetchHealthy() const
{
    return fetchHealthy_;
}

// ---------- worker task (core 0): blocking HTTP/TLS, no display access ----------

void DataFetcher_::workerTask(void *param)
{
    static_cast<DataFetcher_ *>(param)->workerLoop();
}

void DataFetcher_::workerLoop()
{
    for (;;)
    {
        FetchRequest *req = nullptr;
        if (xQueueReceive(requestQueue_, &req, portMAX_DELAY) != pdTRUE || !req)
            continue;

        FetchResult *res = (req->type == FetchRequest::WEATHER)
                               ? performWeatherFetch(*req)
                               : performCustomFetch(*req);
        delete req;

        // Hand the result back to the main loop. Drop it if the result queue is
        // full (main loop fell behind) rather than blocking the worker.
        if (res)
        {
            if (xQueueSend(resultQueue_, &res, 0) != pdTRUE)
                delete res;
        }
    }
}

// ---------- HTTP GET with retry (worker only) ----------

int DataFetcher_::httpGetWithRetry(const String& url, bool isHttps, String& outBody)
{
    int httpCode = 0;
    for (int attempt = 0; attempt < FETCH_ATTEMPTS; attempt++)
    {
        HTTPClient http;
        WiFiClientSecure secClient;
        WiFiClient plainClient;

        if (isHttps)
        {
            // Don't validate cert — DataFetcher hits arbitrary third-party APIs
            // whose CAs we can't pin in advance. Do NOT call secClient.setTimeout()
            // — it hangs some hosts; HTTPClient's timeouts handle it correctly.
            secClient.setInsecure();
            http.begin(secClient, url);
        }
        else
        {
            http.begin(plainClient, url);
        }

        http.setConnectTimeout(HTTP_CONNECT_TIMEOUT);
        http.setTimeout(HTTP_READ_TIMEOUT);
        http.addHeader("Accept", "application/json");

        httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            outBody = http.getString();
            http.end();
            return httpCode;
        }
        http.end();

        // -1/-11 etc. are transient connection failures — common when this task
        // races the web server / MQTT for the TLS stack. Back off briefly and retry.
        DEBUG_PRINTF("DataFetcher: GET attempt %d/%d failed: %d", attempt + 1, FETCH_ATTEMPTS, httpCode);
        if (attempt + 1 < FETCH_ATTEMPTS)
            vTaskDelay(pdMS_TO_TICKS(FETCH_RETRY_DELAY_MS));
    }
    return httpCode;
}

// ---------- HTTP fetch (custom source) — runs on the worker task ----------

FetchResult *DataFetcher_::performCustomFetch(const FetchRequest& req)
{
    const DataSourceConfig& src = req.cfg;
    FetchResult *res = new FetchResult();
    res->type = FetchResult::CUSTOM;
    res->name = src.name;

    bool isHttps = src.url.startsWith("https");

    // Extra heap check before SSL allocation
    if (isHttps && ESP.getFreeHeap() < MIN_FREE_HEAP + 20000)
    {
        DEBUG_PRINTF("DataFetcher: insufficient heap for HTTPS (%d bytes)", ESP.getFreeHeap());
        return res; // success=false
    }

    DEBUG_PRINTF("DataFetcher: GET %s (heap: %d)...", src.name.c_str(), ESP.getFreeHeap());
    String body;
    int httpCode = httpGetWithRetry(src.url, isHttps, body);
    if (httpCode != HTTP_CODE_OK)
    {
        DEBUG_PRINTF("DataFetcher: GET %s failed: %d", src.name.c_str(), httpCode);
        res->networkError = true; // connectivity problem, not a data problem
        return res;               // success=false
    }

    if (body.length() > MAX_RESPONSE_SIZE)
    {
        DEBUG_PRINTF("DataFetcher: response too large (%d bytes), truncating", body.length());
        body = body.substring(0, MAX_RESPONSE_SIZE);
    }

    DEBUG_PRINTF("DataFetcher: %s body length=%d", src.name.c_str(), body.length());

    String value = extractJsonValue(body, src.jsonPath);
    if (value.isEmpty())
    {
        DEBUG_PRINTF("DataFetcher: path '%s' not found in response", src.jsonPath.c_str());
        DEBUG_PRINTF("DataFetcher: body preview: %.100s", body.c_str());
        return res; // success=false
    }

    // Release body before building the app JSON to limit peak heap.
    body = String();

    String formatted = formatValue(src, value);
    res->appJson = buildCustomAppJson(src, formatted);
    res->success = true;

    DEBUG_PRINTF("DataFetcher: %s = %s (fetched)", src.name.c_str(), formatted.c_str());
    return res; // applied on the main loop in drainResults()
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
    if (enqueueCustom(*it))
        lastFetch_[idx] = millis();
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

FetchResult *DataFetcher_::performWeatherFetch(const FetchRequest& req)
{
    FetchResult *res = new FetchResult();
    res->type = FetchResult::WEATHER;
    res->weather.valid = false;

    DEBUG_PRINTF("DataFetcher: fetching weather from %s", req.url.c_str());

    String body;
    int httpCode = httpGetWithRetry(req.url, true, body);
    if (httpCode != HTTP_CODE_OK)
    {
        DEBUG_PRINTF("DataFetcher: weather fetch failed: %d", httpCode);
        res->networkError = true; // connectivity problem
        return res;               // weather.valid = false
    }

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, body);
    if (err)
    {
        DEBUG_PRINTF("DataFetcher: weather JSON parse error: %s", err.c_str());
        return res;
    }

    JsonObject current = doc["current"];
    if (current.isNull())
    {
        DEBUG_PRINTLN(F("DataFetcher: weather response missing 'current' object"));
        return res;
    }

    WeatherData& w = res->weather;

    // Temperature (unit captured on the main thread when the request was built)
    w.outdoorTemp = req.isCelsius ? current["temp_c"].as<float>()
                                  : current["temp_f"].as<float>();
    w.outdoorHumidity = current["humidity"].as<float>();
    w.pressure = current["pressure_mb"].as<float>();

    // Condition
    JsonObject condition = current["condition"];
    if (!condition.isNull())
    {
        w.condition = condition["text"].as<String>();
        w.conditionCode = condition["code"].as<int>();
    }

    // Air Quality Index (US EPA standard)
    JsonObject airQuality = current["air_quality"];
    w.aqi = airQuality.isNull() ? 0 : airQuality["us-epa-index"].as<int>();

    // UV Index
    w.uv = current["uv"].as<float>();

    w.lastUpdate = millis();
    w.valid = true;
    res->success = true; // clears the connectivity-health flag (drainResults)

    DEBUG_PRINTF("DataFetcher: weather fetched - %.1f%s, %s, AQI=%d, UV=%.1f",
                 w.outdoorTemp, req.isCelsius ? "C" : "F",
                 w.condition.c_str(), w.aqi, w.uv);
    return res; // applied to global weatherData on the main loop
}

void DataFetcher_::forceWeatherFetch()
{
    if (enqueueWeather())
        lastWeatherFetch_ = millis();
}
