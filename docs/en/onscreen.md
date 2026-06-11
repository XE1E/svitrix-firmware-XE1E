# Onscreen menu

SVITRIX provides a **onscreen menu** directly on your clock.
Press and hold the middle button for 2 seconds to access the menu.
Navigate through the items with the left and right buttons and choose the submenu with a push on the middle button.
Hold down the middle button for 2s to exit the current menu and to save your setting.

::: warning
You can easily turn your SVITRIX matrix on or off by simply double-pressing the middle button if youre not in Menu.
:::

The on-device menu labels are in **Spanish** (the project's default language)
and the menu has 14 items.

| Menu Item | Description |
| --- | --- |
| `BRILLO` | Brightness of the display (0–100 %) or `AUTO`. Toggle auto/manual with the middle button. |
| `COLOR` | Select one of 15 text colors (hex value displayed). |
| `ROTACION` | **Submenu** with the app-rotation settings (see below). |
| `HORA` | Time format. |
| `FECHA` | Date format. |
| `INI-SEM` | Start of week (`LUN` = Monday / `DOM` = Sunday). |
| `TEMP` | Temperature unit (°C or °F). |
| `APPS` | Enable or disable internal apps. |
| `NOCHE` | Toggle night mode. |
| `INFO` | Shows IP, WiFi signal, version, ID and free RAM (read-only). |
| `SONIDO` | Enable or disable sound output. |
| `VOLUMEN` | Buzzer volume (0–30). |
| `OTA` | Check and download new firmware if available. |
| `ALARMAS` | Lists alarms; the middle button edits one (enabled / hour / minute). Days, label and melody are set from the web UI or MQTT. |

## `ROTACION` submenu

Groups the four app-rotation settings. Inside the submenu:

- **Middle button (short press):** move to the next field.
- **Left / Right:** adjust the current field's value.
- **Middle button (hold):** apply, save and exit.

| Field | Setting | Range |
| --- | --- | --- |
| `ROT` | Auto app rotation (`SI`/`NO` = on/off) | — |
| `TRA` | Transition duration (seconds) | 0.2 – 2.0 |
| `DES` | Text scroll speed | 10 – 100 |
| `APP` | Time per app before switching (seconds) | 1 – 30 |

::: tip
Time values (`TRA`, `APP`) are shown without an "s" suffix; the prefix and
magnitude already imply seconds.
:::

## Status indicators (corner LEDs)

The clock shows two status pixels in the left corners of the matrix for an
at-a-glance connectivity check. When everything is fine, **both are off**.

**Top-left — WiFi (red):**

| Pattern | Meaning |
| --- | --- |
| Off | Connected, internet reachable |
| Slow blink | WiFi connected but no internet access (downloads failing, e.g. weather) |
| Fast blink | WiFi down, reconnecting |
| Solid on | AP mode — waiting for WiFi configuration |

**Bottom-left — Home Assistant / MQTT (yellow):**

| Pattern | Meaning |
| --- | --- |
| Off | Connected, or MQTT not configured |
| Slow blink | MQTT configured but not connected to the broker |

::: tip
The WiFi indicator clears as soon as the connection and downloads recover, so a
brief blink only signals a transient hiccup.
:::
