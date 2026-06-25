# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/) and
[Conventional Commits](https://www.conventionalcommits.org/).

Releases prior to v0.4.0-beta.13 are documented in the
[GitHub Releases](https://github.com/XE1E/svitrix-firmware-XE1E/releases).

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
