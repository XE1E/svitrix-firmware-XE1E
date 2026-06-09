# MenuManager — AI Reference

On-device settings menu rendered on the LED matrix. Adjusts display, time, audio, and app settings via physical buttons.

## TL;DR

- **Provides:** `IButtonHandler` → PeripheryManager
- **Consumes:** `IDisplayRenderer`, `IDisplayControl`, `IDisplayNavigation`, `IPeripheryProvider`, `IUpdater`
- **Entry point:** `MenuManager_::setup()`, `::tick()`, `::show(bool)`
- **UI:** 14 top-level items on 32×8 matrix (Spanish labels)
- **Indicator limit:** `drawMenuIndicator` draws 1px per item on row 7 → max ~16 items fit (16 = 31px). Group related settings into submenus rather than adding top-level items.

> 📌 Auto-loads when reading files in `src/MenuManager/`

## Files

| File | Purpose |
|------|---------|
| `MenuManager.h` | Singleton, `IButtonHandler` implementation |
| `MenuManager.cpp` | Menu state machine, menu items, button handlers |

## Menu Structure (14 top-level items, Spanish)

| Menu | Controls |
|------|----------|
| BRILLO | `brightnessPercent` (1-100%) or AUTO |
| COLOR | `textColor` (15 preset colors) |
| ROTACION | **Submenu** (`rotationField` 0-3): Select cycles fields, Left/Right adjust, Select-long applies + saves + exits. Fields: `ROT` `autoTransition` (SI/NO) · `TRA` `timePerTransition` (0.2-2.0s) · `DES` `scrollSpeed` (10-100) · `APP` `timePerApp` (1-30s) |
| HORA | `timeFormat` (12 strftime patterns) |
| FECHA | `dateFormat` (9 strftime patterns) |
| INI-SEM | `startOnMonday` (LUN/DOM) |
| TEMP | `isCelsius` (°C/°F) |
| APPS | Toggle native apps (drives rotation config) |
| NOCHE | `nightMode` (SI/NO) |
| INFO | IP / WIFI / VER / ID / RAM (read-only) |
| SONIDO | `soundActive` (SI/NO) |
| VOLUMEN | `soundVolume` (0-30) |
| OTA | Triggers OTA update |
| ALARMAS | List alarms → Select edits one (Select cycles enabled/hour/minute, Left/Right adjust, Select-long saves). Days/label/melody via web/MQTT. Needs `IAlarmProvider` injection. |

`SWITCH`/`T-SPEED`/`APPTIME` were merged into the **ROTACION** submenu (frees indicator space + groups the app-rotation knobs). Same field-cycling pattern as `ALARMAS`.

When an alarm is **ringing** (handled in `DisplayManager`, not the menu): Left/Right = snooze (per-alarm `snoozeMinutes`), Select / Select-long = dismiss. A long-press while ringing dismisses instead of opening the menu.

## Navigation

| Button | MainMenu | Submenu |
|--------|----------|---------|
| Right | Next item | Increase value |
| Left | Previous item | Decrease value |
| Select (short) | Enter submenu | Toggle |
| Select (long) | Exit menu | Save + back |

## Interfaces

**Provides:** `IButtonHandler` → PeripheryManager

**Consumes:** `IDisplayRenderer`, `IDisplayControl`, `IDisplayNavigation`, `IPeripheryProvider`, `IUpdater`

## Don't

- Don't render menu when notifications are active — defer to NotificationManager
- Don't persist every value change — only on submenu exit (`Select long`)
- Don't call `IUpdater::updateFirmware()` without user confirmation in UPDATE menu
