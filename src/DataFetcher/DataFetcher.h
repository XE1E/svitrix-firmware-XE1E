#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "DataFetcherConfig.h"

class IDisplayNavigation;

// Heap-allocated work items exchanged with the background worker task.
// Defined in DataFetcher.cpp — only pointers cross the FreeRTOS queues, so
// the full definitions stay private to the implementation.
struct FetchRequest;
struct FetchResult;

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

    unsigned long lastWeatherFetch_ = 0;

    // Connectivity health: false after a fetch fails on the network layer,
    // true again after any fetch succeeds. Read by the WiFi status LED.
    // Touched only on the main loop (drainResults / render), so no locking.
    bool fetchHealthy_ = true;

    // Background fetch task (core 0) + lock-free handoff queues.
    // The worker only touches the network and JSON parsing; the main loop
    // owns all display / weatherData mutation via drainResults().
    TaskHandle_t workerHandle_ = nullptr;
    QueueHandle_t requestQueue_ = nullptr; // main -> worker (FetchRequest*)
    QueueHandle_t resultQueue_ = nullptr;  // worker -> main (FetchResult*)

    static void workerTask(void *param);
    void workerLoop();
    FetchResult *performCustomFetch(const FetchRequest& req);  // runs on worker
    FetchResult *performWeatherFetch(const FetchRequest& req); // runs on worker
    void drainResults();                                       // runs on main loop
    bool enqueueCustom(const DataSourceConfig& src);           // runs on main loop
    bool enqueueWeather();                                     // runs on main loop

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

    /// True while the last network-layer fetch succeeded (or none has failed);
    /// false after a connectivity failure. Drives the WiFi status LED.
    bool fetchHealthy() const;

    bool addSource(const char *json);
    bool removeSource(const String& name);
    String getSourcesAsJson();
    void forceFetch(const String& name);
    void forceWeatherFetch();
    void loadSources();
    void saveSources();
};

extern DataFetcher_& DataFetcher;
