# DataFetcher — AI Reference

Singleton module that periodically fetches values from external HTTP/HTTPS APIs and pushes them as custom apps to the display via `IDisplayNavigation::parseCustomPage()`.

## File Map

| File | LOC | Purpose |
|------|-----|---------|
| `DataFetcher.h` | 67 | Public API, singleton definition |
| `DataFetcher.cpp` | 689 | Synchronous HTTP fetching, JSON extraction, source CRUD, LittleFS persistence |
| `DataFetcherConfig.h` | 18 | `DataSourceConfig` struct — per-source configuration |

## Interfaces

**Implements:** None (no interface of its own).

**Injected Dependencies (1):**
```cpp
void setNavigation(IDisplayNavigation *n);
```

Uses `IDisplayNavigation::parseCustomPage(name, json, show)` to feed fetched data into the custom app pipeline — the same path used by MQTT `/custom/#` and the HTTP API.

## DataSourceConfig

| Field | Type | Description |
|-------|------|-------------|
| `name` | String | Unique ID, becomes the custom app name (e.g., `"btc"`) |
| `url` | String | Full HTTP/HTTPS URL |
| `jsonPath` | String | Dot-notation path to extract (e.g., `"bitcoin.usd"`, `"data.0.price"`) |
| `displayFormat` | String | printf-style format (e.g., `"$%.0f"`) or empty for raw — restricted to single-arg whitelist (see below) |
| `icon` | String | Icon name from LittleFS, or empty |
| `textColor` | String | Hex color `"#RRGGBB"` or empty for default |
| `interval` | uint32_t | Polling interval in seconds |
| `duration` | uint32_t | Display duration in seconds (0 = use global timePerApp) |
| `enabled` | bool | When `false`, source is saved but not fetched/displayed (default: `true`) |

**Constraints:**
- `MIN_INTERVAL` = 60 seconds
- `DEFAULT_INTERVAL` = 900 seconds (15 min)
- `MAX_SOURCES` = 8

## Data Flow

```
External API
    │  HTTP GET (every N seconds)
    ▼
DataFetcher_::fetchAndPush()
    │  1. HTTP GET → response body (max 8 KB)
    │  2. extractJsonValue(body, jsonPath) → raw value string
    │  3. formatValue(src, raw) → printf-formatted string
    │  4. buildCustomAppJson(src, formatted) → {"text","icon","color","lifetime":0}
    ▼
IDisplayNavigation::parseCustomPage(name, json, true)
    │  (same pipeline as MQTT custom apps)
    ▼
Display shows as custom app
```

## Fetch model — synchronous on the main loop

Fetches run **synchronously** inside `tick()` on the Arduino loop (core 1): one
weather fetch (if due) plus one round-robin custom source per call. A working
weatherapi fetch blocks the loop ~2 s; tight timeouts cap a failing one —
`HTTP_CONNECT_TIMEOUT` / `HTTP_READ_TIMEOUT` = 4 s, `HANDSHAKE_TIMEOUT_S` = 4 s
(the handshake's default is 120 s and is NOT covered by the other two).

> **Why not a background task?** An earlier version ran fetches on a dedicated
> core-0 FreeRTOS task to avoid the loop stall. It worked for periodic fetches, but
> running TLS in parallel with the AsyncWebServer/MQTT triggered a socket-fd leak:
> after a manual "fetch now" with the web UI open, every HTTPS `connect()` returned
> `-1` forever until reboot (weather silently froze on its last value, LED stuck).
> The synchronous design — the project's original, proven-stable approach — has no
> such concurrency and is back.

- **`fetchWeather()` / `fetchAndPush(idx)`** do GET + JSON parse + apply
  (`weatherData` / `parseCustomPage()`) inline. `httpGet()` is a single GET that
  always closes its socket (`secClient.stop()` / `plainClient.stop()`).
- **Manual refresh is deferred, never run in the web handler.** `forceWeatherFetch()`
  / `forceFetch()` only mark the item due (`lastWeatherFetch_ = 0` /
  `lastFetch_[idx] = 0`); the main loop does the actual fetch on its next tick.
  This avoids blocking the AsyncWebServer task and the socket contention a
  handler-context fetch caused.

### Resilience

- **Keep last good weather** — `fetchWeather()` overwrites `weatherData` only on a
  *successful* fetch; a failed refresh leaves the last reading on screen instead of
  blanking the weather apps to `--`.
- **Fast retry after failure** — a failed weather fetch schedules a re-attempt in
  `WEATHER_RETRY_MS` (60 s) via `weatherRetryAt_`, instead of waiting the full
  `updateInterval`. Covers the boot fetch failing before WiFi/DNS settle.
- **Heap guard** — `tick()` skips a fetch when free heap < `MIN_FREE_HEAP`.

A failed fetch keeps the last good value on screen and schedules a fast retry; it
does **not** drive any status LED. (The WiFi corner LED is now a pure
WiFi-connectivity indicator — see [Overlays](../Overlays/README.md).)

## Tick Behavior

- **Round-robin**: at most one weather + one custom source fetched per `tick()` call
- **Heap guard**: skips a fetch if free heap < `MIN_FREE_HEAP` (a second check
  guards HTTPS custom sources before SSL allocation)
- **Auto-fetch on boot**: sources with `lastFetch_==0` fetch on first eligible tick
- **Disabled sources skipped**: sources with `enabled=false` are not fetched
- Custom round-robin only runs when `nav_` is set and `sources_` is non-empty

## Key Methods

| Method | Description |
|--------|-------------|
| `setup()` | Creates `/DATAFETCHER/` dir, loads sources, cleans orphaned apps |
| `tick()` | Synchronously fetch one due weather + one round-robin source per call |
| `addSource(json)` | Parse JSON config, upsert by name, save to LittleFS — rejects unsafe `displayFormat` (returns false → HTTP 400) |
| `removeSource(name)` | Remove source + clear its custom app from display |
| `forceFetch(name)` | Mark a source due (`lastFetch_=0`); fetched on the next tick |
| `forceWeatherFetch()` | Mark weather due (`lastWeatherFetch_=0`); fetched on the next tick |
| `getSourcesAsJson()` | Serialize all sources as JSON array |
| `loadSources()` / `saveSources()` | LittleFS persistence to `/DATAFETCHER/sources.json` — `loadSources()` downgrades any persisted unsafe `displayFormat` to empty (raw) and logs the source name |

### Private Methods

| Method | Description |
|--------|-------------|
| `fetchWeather()` | GET weatherapi + parse → write `weatherData` (keep-last-value on failure) |
| `fetchAndPush(idx)` | GET source + extract + format → `parseCustomPage()` |
| `httpGet(url, isHttps, &body)` | Single GET; always `stop()`s the socket; returns the HTTP code |
| `extractJsonValue(json, path)` | Walk dot-notation path through ArduinoJson (supports objects + arrays) |
| `formatValue(src, raw)` | Apply printf-style `displayFormat` to raw value (validates with `isSafeSingleArgFormat`; returns raw on unsafe format) |
| `buildCustomAppJson(src, value)` | Build JSON for `parseCustomPage()` with text, icon, color, lifetime=0 |

## HTTP API Endpoints (registered in ServerManager)

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/datafetcher` | List all sources as JSON array |
| `POST` | `/api/datafetcher` | Add/update source (body: DataSourceConfig JSON) |
| `DELETE` | `/api/datafetcher?name=X` | Remove source by name |
| `POST` | `/api/datafetcher/fetch?name=X` | Force immediate fetch for a source |
| `GET` | `/datafetcher` | Web UI page (`datafetcher_html`) |

### Add/Update Source JSON

```json
{
  "name": "btc",
  "url": "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd",
  "jsonPath": "bitcoin.usd",
  "displayFormat": "$%.0f",
  "icon": "btc",
  "color": "#F7931A",
  "interval": 300,
  "duration": 1,
  "enabled": true
}
```

Required fields: `name`, `url`, `jsonPath`. All others optional (defaults: `enabled=true`, `interval=900`, `duration=0`).

### `displayFormat` whitelist

Validated by `isSafeSingleArgFormat()` from
`lib/services/FormatStringValidator.h` to defeat CWE-134 format-string
injection. Allowed:

- One conversion specifier from `d i u o x X f g e E s` (or none)
- Optional flags `+ - # space 0`, width `[0-9]{0,2}`, precision `.[0-9]{0,2}`
- `%%` literal anywhere
- Surrounding plain text

Rejected: `%n %p %c %a %A`, length modifiers (`l h ll …`), variable
width (`%*d`, `%.*d`), positional args (`%1$d`), 3+ digit width or
precision, more than one specifier.

Validation runs in three places: `addSource()` (HTTP 400),
`loadSources()` (silent downgrade with debug log), and `formatValue()`
(defense-in-depth at the snprintf call site).

## HTTPS Handling

- HTTPS requests use `WiFiClientSecure` with `setInsecure()` (no cert validation)
- Rationale: DataFetcher hits arbitrary third-party APIs whose CAs cannot be pinned in advance
- HTTP requests use plain `WiFiClient`
- **Important**: Do NOT call `secClient.setTimeout()` — it causes hangs on some hosts; HTTPClient's `setConnectTimeout()`/`setTimeout()` handle timeouts correctly
- Response body is released before `parseCustomPage()` to prevent heap fragmentation on large responses

## Persistence

- Sources stored in LittleFS at `/DATAFETCHER/sources.json`
- Directory created in `setup()` if missing
- `saveSources()` called on every add/remove operation
- Sources loaded automatically on `setup()`

## Wiring in main.cpp

```cpp
DataFetcher.setNavigation(&DisplayManager);     // IDisplayNavigation
assert(DataFetcher.hasNavigation());

// In setup(), after WiFi:
DataFetcher.setup();

// In loop(), only when connected:
if (ServerManager.isConnected) {
    DataFetcher.tick();
}
```

## Important Constraints

- Max response body: 8 KB (`MAX_RESPONSE_SIZE`) — larger responses truncated
- JSON parsing buffer: 8 KB `DynamicJsonDocument` (`JSON_DOC_SIZE`) for response extraction (handles large exchange-rate / finance APIs); weather parse buffer is 4 KB
- Connect/read timeouts 4 s, TLS handshake 4 s — they bound how long the
  **synchronous** fetch can stall the render loop (there is no worker task)
- One HTTP request in flight per tick (round-robin), run synchronously on the loop
- **Thread-safety:** `sources_` / `lastFetch_` / `nextFetchIndex_` are guarded by
  `sourcesMutex_` (`std::mutex`). Source CRUD (`addSource`/`removeSource`/
  `getSourcesAsJson`/`forceFetch`) runs in the AsyncWebServer task; `tick()` runs
  on the loop. `tick()` copies the due source under the lock and fetches OUTSIDE
  it, so a blocking GET never makes a web handler wait on the mutex.
- **Fast retry on failure:** a failed custom fetch reschedules a retry in
  `CUSTOM_RETRY_MS` (60 s) instead of waiting the full interval (mirrors weather)
- Custom apps created with `lifetime: 0` — DataFetcher manages their lifecycle, they never auto-expire
- On `removeSource()`, the custom app is cleared from display via `parseCustomPage(name, "{}", false)`
- On disabling a source (`enabled=false`), its custom app is removed from display
- No authentication support — only public APIs (no API key headers)
