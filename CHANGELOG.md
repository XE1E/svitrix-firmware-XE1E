# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/) and
[Conventional Commits](https://www.conventionalcommits.org/).

Releases prior to v0.4.0-beta.13 are documented in the
[GitHub Releases](https://github.com/XE1E/svitrix-firmware-XE1E/releases).

## [v0.4.0-beta.22] — 2026-08-22

### Bug Fixes

- **weather:** the clock no longer reboots when the own-server has no station
  reading yet. `/api/svitrix` returns **503** in that case (server-side fix),
  but `fetchWeather()` treated any non-200 response as a network failure and
  never refreshed the self-recovery clock — a station outage ended in a
  **reboot every ~15 min**. A 503 (or a 200 response missing `current`) now
  keeps the last known value, doesn't count toward the network fail streak,
  and does refresh the self-recovery clock; the data is flagged
  `weatherData.stale` (exposed in `/api/weather/data`).
- **date:** month names in the Date app are now in **Spanish** ("AGO" instead
  of "AUG"). ESP32's libc has no es_MX locale, so `strftime("%b"/"%B")` always
  resolved in English even though the rest of the clock is already Spanish
  (moon phase names, "DIA"/"DIAS"). New `DateFormat` service substitutes
  `%b`/`%B` with literal Spanish text before calling `strftime()`.

### Other Changes

- **apps:** the weather condition icon now uses `is_day` (already emitted by
  `/api/svitrix`): condition code 1000 ("Sunny"/"Clear" in WeatherAPI) used to
  always draw the sun, even before dawn. At night it now draws the real moon
  phase (same calculation as the Moon app).

## [v0.4.0-beta.21] — 2026-07-25

### Bug Fixes

- **weather:** the weather fetch now **picks HTTP vs HTTPS from the URL scheme**
  instead of always using TLS. `fetchWeather()` hard-coded a TLS client, so even
  an `http://` own-server URL went through `WiFiClientSecure` — whose fd/socket
  leak on arduino-esp32 2.0.9 wedges the fetch after a failed handshake (every
  later fetch fails until a reboot; `failStreak` climbs). Pointing the own-server
  URL at a plain `http://host:8080/...` endpoint now uses the plain client (no
  TLS, no leak), which eliminates the wedging/reboots. WeatherAPI and any
  `https://` URL still use TLS as before.

## [v0.4.0-beta.20] — 2026-07-25

### Bug Fixes

- **weather:** the clock now **syncs immediately when night mode ends** and
  **reboots far less often**. After a whole night with no fetches the WiFi
  association could go stale (`WiFi.status()` still "connected" but the socket
  dead), so the first morning fetch hung until the 15‑min self‑recovery reboot —
  and the same stuck‑fetch reboot also fired a few times during the day. Fixes:
  (1) `WiFi.setSleep(false)` (the clock is USB‑powered) so the link never idles
  into that stale state; (2) on the night→day transition it re‑associates
  (`WiFi.reconnect()`) and schedules the first fetch ~5 s later on a fresh link;
  (3) a **soft `WiFi.reconnect()`** after ~3 consecutive failed fetches, so a
  full reboot is now a last resort instead of the primary recovery.

### Other Changes

- **apps:** the Precipitation app now shows **today's accumulated rain**
  (`precip_mm`, resets at midnight) instead of the current rain event, still
  appending the rate (`<n>/h`) when enabled.

## [v0.4.0-beta.19] — 2026-07-24

### Features

- **weather:** the clock now **pauses all data fetches while night mode is
  active**. During night mode only the clock is shown (weather/custom apps are
  hidden), so polling the server is wasted work. When night mode ends, the
  pending fetch fires immediately, so the weather apps reappear with fresh data.
  The self-recovery timer is frozen during night mode too, so the clock never
  reboots overnight for "no successful fetch."

## [v0.4.0-beta.18] — 2026-07-24

### Bug Fixes

- **weather:** the clock now **self-recovers** if weather updates get stuck, so
  a configured clock keeps working unattended. Two failure modes were observed
  in the field: a leaked TLS socket making every `connect()` fail instantly
  (`reset_reason: wdt`, `UV 0` at midday), and a slow heap decline
  (~120 KB → 65 KB/h) that eventually starves the HTTPS fetch. The DataFetcher
  now tracks time since the last **successful** fetch and, if it exceeds
  `max(15 min, 5× the update interval)` **with WiFi still connected**, performs a
  controlled `ESP.restart()` to reset the network stack and heap. This is
  time-based so it also covers fetches skipped for low heap (not just network
  errors). Consecutive network failures are exposed as `failStreak` in
  `GET /api/weather/data` for monitoring.
  - *Note:* the reboot is a safety net; the underlying HTTPS/heap leak still
    warrants a root-cause fix so reboots stay rare.

## [v0.4.0-beta.17] — 2026-07-24

### Features

- **apps:** the Wind, Solar-radiation and Precipitation apps now use their own
  **animated icons** (`/ICONS/17071`, `56958`, `3527`) and long values scroll
  (Moon-style marquee) instead of being clipped.
  - **Wind** shows direction + speed on a **single line** and, with *Rotate gust*
    on, appends the gust (` R<gust>`) to that same line (e.g. `W 5 R 4`) — it
    scrolls rather than swapping frames.
  - **Precipitation** shows the **current rain event** (mm) on a single line
    (was today's total), appending the rate (`<n>/h`) when enabled
    (e.g. `0.0mm 0.0/h`).
  - Each app's on-screen time is its per-app duration in the Apps tab.
- **apps:** Solar radiation gains an **Auto color** option (like UV) — the value
  is tinted by intensity (blue → amber → orange → red); defaults on.
- **apps:** Wind, Solar-radiation and Precipitation are now first-class apps in
  the **Apps tab** catalog (addable, with their animated default icons), and
  their per-app options are configured inline there just like UV / ICA / Moon:
  *Auto color* (radiation), *Rotate gust* (wind), *Rotate rain rate* (precip).
  The duplicate block under Settings → Weather API was removed (only the own-
  server URL remains there).
- **weather:** faster update intervals (1 / 2 / 5 min) are now selectable —
  useful with an own server (no API rate limit) so the clock tracks
  fast-changing values (e.g. solar radiation at dusk) instead of lagging up to
  10 min behind.

### Bug Fixes

- **weather:** the update-interval validation floor was 10 min, so the new
  1 / 2 / 5-min options were silently reset to 10 on save/reboot. The floor is
  now 1 min (the fetcher keeps its hard 60 s minimum).

### Other Changes

- **server:** the own-server adapter (`/api/svitrix`) now also exposes
  `current.precip_event_mm` (current rain-event accumulation) alongside
  `solar_radiation` and `rain_rate_mm`.
- **web:** `/api/weather/data` now also returns the wind, solar-radiation and
  precipitation fields (they were parsed and displayed but missing from the
  debug endpoint).

## [v0.4.0-beta.16] — 2026-07-23

### Features

- **weather:** the clock can now fetch weather from your **own server** instead
  of WeatherAPI.com. New *Own server (URL)* field in Settings → Weather API; when
  set, the firmware fetches that URL, which must return the WeatherAPI
  `current.json` shape (e.g. `https://clima.xe1e.net/api/svitrix`). It parses the
  same fields plus optional extras (`solar_radiation`, `rain_rate_mm`). Falls back
  to WeatherAPI.com when the field is empty.
- **apps:** three new native weather apps (add them with *Add weather apps*):
  - **Wind** — direction + speed (km/h); optionally rotates the gust.
  - **Solar radiation** — W/m² (from the server's `solar_radiation`).
  - **Precipitation** — today's rain (mm); optionally rotates the rate (mm/h).
- Wind (`wind_kph`/`wind_degree`/`wind_dir`/`gust_kph`) and precipitation
  (`precip_mm`) are now parsed from the standard WeatherAPI shape as well.

## [v0.4.0-beta.15] — 2026-06-26

### Features

- **apps:** the Air Quality (ICA) app now breaks down the pollutants that are
  individually out of range — `PM2.5`, `PM10`, `O3`, `NO2`, `SO2`, `CO`
  (worst-first, max 3) — each shown in the color of its own EAQI level, so a
  pollutant flags even when the overall index is moderate (e.g. `ICA:2` with
  `O3` in red). The per-component display time is web-configurable (2–10 s) and
  the whole sequence plays in a single app appearance; the show-components
  toggle defaults on (#66)
- **apps:** the Moon app now uses designed phase-icon bitmaps (LaMetric set),
  one per lunar phase, instead of the procedural grayscale sphere (#64)
- **mqtt:** Home Assistant per-app duration numbers and visibility switches now
  reflect and control the device's real unified rotation instead of stale
  legacy defaults — what HA shows matches the clock, and edits apply to all
  instances of an app type. Duration numbers render as a value box (#64)

### Documentation

- Documented the previously-undocumented `/version`, `/save` and LittleFS
  file-manager (`/list`, `/edit`) endpoints, and synced the public manuals
  (apps, home-assistant) and config guide in ES/EN/UK with the above features
  (#65, #66)

## [v0.4.0-beta.14] — 2026-06-25

### Bug Fixes

- **web:** the Apps tab no longer overwrites the rotation config with defaults
  when `GET /api/rotation` returns an empty list (which typically happened when
  the SPA polled while the device was still booting). It used to fabricate the
  5 default native apps with `color:0` and auto-save them, silently wiping the
  saved rotation order and per-item colors. The UI now retries on empty, never
  auto-saves defaults, and only falls back to local-only defaults that persist
  on the first explicit edit (#63)

## [v0.4.0-beta.13] — 2026-06-24

### Bug Fixes

- **display:** low-battery alert now renders the red battery glyph and plays a
  double beep-beep-beep pattern. The draw instructions used the array-of-arrays
  format the renderer does not parse, so the screen only went dark (#61)
- **web:** the Moon app now appears in the add-app list (it was defined but
  unreachable from the UI) (#60)
- **web:** hide the no-op uppercase toggle from the Display tab (#59)

### Other Changes

- **mqtt:** drop dead subscription topics `/brightness`, `/timeformat` and
  `/dateformat` (subscribed but never handled) (#61)
- docs: scope the Hardware page and FAQ to the off-the-shelf Ulanzi TC001
  (remove DIY/custom-build content), document the native Moon app, and fill
  documentation gaps — transition effects 11–13, the `GAMMA` setting,
  `/api/wifi` · `/api/eraseWifi` · `/api/settings/export` · `/import`, and the
  physical button combinations (#60, #61)
- docs(xe1e): document color correction, temperature and gamma in the Spanish
  configuration guide (#58)
