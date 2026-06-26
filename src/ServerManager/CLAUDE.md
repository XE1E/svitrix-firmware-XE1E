# ServerManager — AI Reference

HTTP REST API server, WiFi connectivity, mDNS discovery, UDP device discovery, and TCP server. Primary external control interface alongside MQTT.

## TL;DR

- **Provides:** `IButtonReporter` → PeripheryManager dispatcher
- **Consumes:** 7 interfaces (IDisplayRenderer, IDisplayControl, IDisplayNavigation, IDisplayNotifier, ISound, IPower, IUpdater)
- **Entry point:** `ServerManager_::setup()`, `::tick()`
- **Config:** loaded from `/DoNotTouch.json` + NVS

> 📌 Auto-loads when reading files in `src/ServerManager/`

## Files

| File | Purpose |
|------|---------|
| `ServerManager.h` | Public API, singleton, IButtonReporter |
| `ServerManager.cpp` | WiFi setup, HTTP endpoints, UDP/TCP, settings loader |

## Interfaces

**Implements:** `IButtonReporter` — forwards button presses to external HTTP callback.

**Consumes (7):**

| Interface | Used for |
|-----------|----------|
| `IDisplayRenderer` | `HSVtext()` during erase/reset |
| `IDisplayControl` | Power, moodlight, settings, stats, LED export |
| `IDisplayNavigation` | App list, switching, reordering, custom apps, effects, transitions |
| `IDisplayNotifier` | Notifications, indicators, dismiss |
| `ISound` | RTTTL playback, sound files, R2D2 |
| `IPower` | Sleep/wake |
| `IUpdater` | OTA firmware update |

## HTTP Endpoints (~50, registered in `addHandler()`)

> Public-facing reference (3 languages): [docs/api.md](../../docs/api.md). The
> `/list` + `/edit` file-manager and WiFi-portal routes come from the webserver
> wrapper — see [lib/webserver/CLAUDE.md](../../lib/webserver/CLAUDE.md).

### Device Control
| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/power` | Set display power state |
| `POST` | `/api/sleep` | Deep sleep (JSON: seconds) |
| `ANY` | `/api/reboot` | Restart ESP32 |
| `ANY` | `/api/erase` | Factory reset: wipe LittleFS + settings (keeps WiFi) |
| `ANY` | `/api/eraseWifi` | Wipe only WiFi credentials + reboot |
| `ANY` | `/api/resetSettings` | Reset settings to defaults |
| `POST` | `/api/doupdate` | Check + apply OTA firmware update |

### Display & Apps
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/loop` | Get current app loop as JSON |
| `GET` | `/api/apps` | Get apps with icon data |
| `POST` | `/api/apps` | Update app vector |
| `POST` | `/api/switch` | Switch to app by name |
| `ANY` | `/api/nextapp` | Navigate to next app |
| `POST` | `/api/previousapp` | Navigate to previous app |
| `POST` | `/api/custom?name=X` | Create/update/delete custom app |
| `POST` | `/api/moodlight` | Set moodlight mode |
| `GET` | `/api/rotation` | Get unified rotation config (apps + effects + per-item overrides) |
| `POST` | `/api/rotation` | Save unified rotation config (replaces deprecated `/api/reorder` + `/api/playlist`) |

### Notifications & Indicators
| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/notify` | Push notification |
| `ANY` | `/api/notify/dismiss` | Dismiss current notification |
| `POST` | `/api/indicator1\|2\|3` | Set indicator 1/2/3 |

### Audio
| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/rtttl` | Play RTTTL melody |
| `POST` | `/api/sound` | Play sound file |
| `POST` | `/api/r2d2` | Play R2D2-style sounds |

### Settings & Stats
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/settings` | Get all settings as JSON |
| `POST` | `/api/settings` | Update settings (partial JSON) |
| `GET` | `/api/settings/export` | Export full settings backup as JSON |
| `POST` | `/api/settings/import` | Restore settings from a backup JSON |
| `GET` | `/api/stats` | Device stats |
| `GET` | `/api/screen` | LED buffer as JSON (256 pixels) |
| `GET` | `/api/effects` | List available visual effects |
| `GET` | `/api/transitions` | List available transition effects |

### Weather
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/weather` | Get weather app config |
| `POST` | `/api/weather` | Update weather config (API key, location, display) |
| `GET` | `/api/weather/data` | Current weather data (temp, humidity, pressure, aqi, uv, condition) |
| `POST` | `/api/weather/fetch` | Force a weather fetch now |

### Alarms & Reminders
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/alarms` | List alarms/reminders + ringing state |
| `POST` | `/api/alarms` | Add an alarm, or `{action:snooze\|dismiss}` |
| `PUT` | `/api/alarms` | Update an alarm by id |
| `DELETE` | `/api/alarms?id=X` | Delete an alarm |

### WiFi
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/wifi` | List configured networks (up to 3) |
| `POST` | `/api/wifi` | Set networks (`{networks:[{ssid,password}]}`) |

### DataFetcher
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/datafetcher` | List configured data sources |
| `POST` | `/api/datafetcher` | Add new data source |
| `DELETE` | `/api/datafetcher?name=X` | Remove data source |
| `POST` | `/api/datafetcher/fetch?name=X` | Force-fetch a data source |

### System & Files
| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/version` | Firmware version string |
| `POST` | `/save` | Apply `/DoNotTouch.json` config to running system |
| `GET` | `/list?dir=/ICONS` | List a LittleFS directory (JSON) — *webserver wrapper* |
| `GET` | `/<path>` | Download a LittleFS file (e.g. `/ICONS/962.jpg`; may be gzip-encoded) |
| `POST` | `/edit?filename=/<path>` | Upload/create a file (multipart `data`) — *webserver wrapper* |
| `PUT` | `/edit` | Create a directory (`path=/<path>`) — *webserver wrapper* |
| `DELETE` | `/edit` | Delete a file (`path=/<path>`) — *webserver wrapper* |

## Config Initialization

- `initConfigDefaults()` — writes default values to `/DoNotTouch.json` on first boot (if file doesn't exist). Keys: `Static IP`, `Local IP`, `Gateway`, `Subnet`, `Primary DNS`, `Secondary DNS`, `Broker`, `Port`, `Username`, `Password`, `Prefix`, `Homeassistant Discovery`, `NTP Server`, `Timezone`, `Auth Username`, `Auth Password`.
- `loadSettings()` — reads `/DoNotTouch.json`, applies values to config structs, then calls `applyAllSettings()`.

## Communication Protocols

- **UDP Discovery (port 4210):** listens for `"FIND_SVITRIX"`, responds on 4211 with hostname.
- **TCP Server (port 8080):** single-client, newline-delimited messages.
- **HTTP Button Callback:** POST to `systemConfig.buttonCallback` URL on button press/release.

## Don't

- Don't reorder routes: `/api/datafetcher/fetch` must be registered before `/api/datafetcher`
- Don't serve HTML from PROGMEM — all SPA pages come from LittleFS root
- Don't call non-thread-safe APIs from TCP/UDP callbacks; queue work to `tick()`

## Important Patterns

- AP mode fallback at `192.168.4.1` — only `/version` endpoint registered
- Auth via `mws.setAuth(user, pass)`
- mDNS registers `http` and `svitrix` TCP services
