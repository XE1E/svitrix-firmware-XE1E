#pragma once

#include <Arduino.h>
#include <vector>
#include <mutex>
#include "DataFetcherConfig.h"

class IDisplayNavigation;

class DataFetcher_
{
  private:
    DataFetcher_() = default;
    DataFetcher_(const DataFetcher_&) = delete;
    DataFetcher_& operator=(const DataFetcher_&) = delete;
    DataFetcher_(DataFetcher_&&) = delete;
    DataFetcher_& operator=(DataFetcher_&&) = delete;

    IDisplayNavigation *nav_ = nullptr;
    std::vector<DataSourceConfig> sources_;
    std::vector<unsigned long> lastFetch_;
    size_t nextFetchIndex_ = 0;

    // Guards sources_ / lastFetch_ / nextFetchIndex_. Source CRUD runs in the
    // AsyncWebServer task (core 0) while tick() iterates on the main loop
    // (core 1); without this lock a concurrent push_back/erase realloc would
    // corrupt the vector tick() is reading. Critical sections are kept tiny —
    // tick() copies the due source under the lock and fetches outside it.
    std::mutex sourcesMutex_;

    unsigned long lastWeatherFetch_ = 0;
    unsigned long weatherRetryAt_ = 0;       // millis() of a scheduled fast retry (0 = none)
    unsigned long lastWeatherSuccessMs_ = 0; // millis() del último fetch de clima EXITOSO
                                             // (0 = ninguno aún; base para auto-recuperación)
    uint16_t weatherFailStreak_ = 0;         // fallos de RED consecutivos (solo diagnóstico;
                                             // se resetea en cada éxito)

    // Synchronous fetch helpers (run on the main loop in tick()).
    bool fetchAndPush(const DataSourceConfig& src); // custom source -> parseCustomPage (operates on a copy, no shared-state access)
    void fetchWeather();                            // weather API -> weatherData
    // Single HTTP GET with socket cleanup. On HTTP_CODE_OK fills outBody;
    // returns the final HTTP code otherwise.
    int httpGet(const String& url, bool isHttps, String& outBody);

    String extractJsonValue(const String& json, const String& path);
    static String buildCustomAppJson(const DataSourceConfig& src, const String& value);
    static String formatValue(const DataSourceConfig& src, const String& raw);

    String buildWeatherQuery();
    void cleanupOrphanedApps();

  public:
    static DataFetcher_& getInstance();

    void setNavigation(IDisplayNavigation *n);
    bool hasNavigation() const;

    void setup();
    void tick();

    bool addSource(const char *json);
    bool removeSource(const String& name);
    String getSourcesAsJson();
    void forceFetch(const String& name);
    void forceWeatherFetch();
    void loadSources();
    void saveSources();

    // Fallos de red consecutivos del fetch de clima (0 = último intento OK).
    // Expuesto para diagnóstico (/api/stats) y monitoreo de fiabilidad.
    uint16_t weatherFailStreak() const
    {
        return weatherFailStreak_;
    }
};

extern DataFetcher_& DataFetcher;
