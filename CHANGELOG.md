# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/) and
[Conventional Commits](https://www.conventionalcommits.org/).

Releases prior to v0.4.0-beta.13 are documented in the
[GitHub Releases](https://github.com/XE1E/svitrix-firmware-XE1E/releases).

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
