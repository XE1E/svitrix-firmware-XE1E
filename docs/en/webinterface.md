# Web Interface

SVITRIX uses a modern Single Page Application (SPA) built with Preact as its web interface. The SPA is served from the device's LittleFS filesystem and communicates with the firmware via REST API.

Once SVITRIX is connected to your Wi-Fi network, access the web interface by entering the device's IP address in your browser (port 80). The IP address is displayed on the matrix at each boot.

## Language

The interface supports **Spanish** (default) and **English**. Use the **ES/EN** toggle in the navigation bar to switch languages. Your preference is saved in the browser.

## Tabs

The interface is organized into focused tabs. Most settings **auto-save** when changed — toggles and selects apply instantly, while sliders and colors wait 500ms after the last change before saving.

**Sections with manual save** (require Save button): WiFi, Network, MQTT, NTP, Authentication, and Weather API — these handle sensitive credentials where accidental saves could cause connection issues.

| Tab | Description |
|-----|-------------|
| **Ajustes / Settings** | WiFi networks, static IP, NTP server, timezone, and Weather API configuration. |
| **MQTT** | MQTT broker connection and HTTP authentication settings. |
| **Pantalla / Display** | Matrix power, brightness, gamma, text color, background effects, night mode, and send notifications. |
| **Apps** | Native apps (time, date, temperature, humidity, battery), weather apps, transitions, and navigation settings. |
| **Fecha/Hora / Time/Date** | Time and date formats, display modes (calendar, big digits, binary), colors. |
| **Sonido / Sound** | Buzzer enable/disable and volume. |
| **Vista / Screen** | Live view of the 32x8 LED matrix with app navigation and PNG download. |
| **Datos / Data** | Configure external HTTP data sources. See [Data Fetcher](./datafetcher). |
| **Alarmas / Alarms** | Configure wake-up alarms with day selection and snooze. |
| **Archivos / Files** | File manager to browse, upload, edit, and delete files on the device. |
| **Iconos / Icons** | Download icons from the LaMetric icon library. |
| **Respaldo / Backup** | Download/restore device backup as JSON. |
| **Sistema / System** | Save all settings, reset to defaults, erase WiFi, or reboot. |
| **Actualizar / Update** | Upload firmware (.bin) for OTA update. |

## First-Time Setup (AP Mode)

When SVITRIX cannot connect to a saved WiFi network, it creates its own access point:

| Parameter | Value |
|-----------|-------|
| Network name | `svitrix_XXXXX` |
| Password | `12345678` |

Connect to this network and open **http://192.168.4.1** in your browser. A minimal WiFi setup page will appear with network scanning and connection. After connecting to your home WiFi, the device reboots and the full SPA becomes available.

## SPA Deployment

The web interface is stored separately from the firmware in the LittleFS filesystem partition. After flashing the firmware, upload the SPA files to the device:

```bash
cd web && npm run upload
```

This builds the SPA and uploads it to the device's LittleFS root directory. The SPA bundle is approximately 30 KB (gzip compressed).

::: tip
Once uploaded, the SPA persists across firmware updates. You only need to re-upload the SPA when the web interface itself is updated.
:::

## Tab Details

### Ajustes / Settings

**Stats Bar** — Read-only bar at the top showing: firmware version, free RAM, WiFi signal, ambient light (lux), uptime, temperature, humidity, and brightness.

**WiFi** — Configure up to 3 WiFi networks. The device tries each in order. Scan for networks or enter manually.

**Network** — Enable **Static IP** to configure fixed IP, gateway, subnet, and DNS.

**NTP & Timezone** — Select NTP server and configure timezone using POSIX format. Find yours at [posix_tz_db](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).

**Weather API** — Configure weatherapi.com API key, location method (city, coordinates, auto-IP, or station ID), and update interval. Click **Fetch Data** to test the configuration.

### MQTT

**MQTT Settings:**
- **Broker** — hostname or IP of your MQTT broker
- **Port** — default 1883
- **Username / Password** — broker credentials
- **Prefix** — MQTT topic prefix (default: device ID)
- **HA Discovery** — enable Home Assistant auto-discovery

**Authentication** — Set web username/password. Leave empty to disable. All pages and API calls require credentials when set.

::: warning
Do not lose your auth credentials — otherwise you will need to factory reset the device.
:::

### Pantalla / Display

**Display Settings:**
- **Matrix Power** — turn the LED matrix on/off
- **Auto Brightness** — adjust brightness based on ambient light. When enabled, two sub-sliders set the range: **Min Brightness** (1–50) and **Max Brightness** (0–255)
- **Brightness** — manual brightness level, shown as a percentage (0–100 %)
- **Background Effect** — a visual effect drawn behind the apps (None by default)
- **Gamma** — gamma correction (0.5–3.0)
- **Text Color** — default text color
- **Background Effect** — visual effect behind apps
- **Color Correction** — a global tint multiplied over the whole screen (applied by FastLED on refresh). **White** is the neutral value (no change); it can only **dim** color channels, never add them. Use it to fix a color cast on the panel. The **Reset** button returns to white.
- **Color Temperature** — the same kind of global tint, meant to warm up or cool down the image. Includes **Warm / Neutral / Cool** shortcuts; **Neutral** (white) effectively disables it. Pick saturated colors to see the effect.

**Night Mode** — Schedule low-brightness mode for nighttime:
- **Enable** — toggle on/off
- **Start / End** — time range (supports crossing midnight)
- **Night Brightness** — brightness during night (1–50)
- **Night Color** — text color (default: red)
- **Block Auto-Transition** — stop app cycling during night

**Send Notification** — Send a one-time message:
- **Text** — message (required)
- **Icon** — icon ID or filename
- **Layout** — icon position (left/right)
- **Hold** — keep until dismissed
- **Duration** — display time (1–60s)
- **Rainbow** — rainbow text effect
- **Color** — text color
- **Sound / RTTTL** — play melody

### Apps

The **Apps** page contains a unified rotation list that controls **all** apps and effects shown in the display cycle:

**Unified Rotation** — A drag-and-drop list including:
- **Native apps** — Time, Date, Temperature, Humidity, Battery
- **Weather apps** — Outdoor Temp, Outdoor Humidity, Pressure, Air Quality, UV
- **Custom apps** — created via MQTT, HTTP or Data Fetcher
- **Effects** — standalone visual effects (no text)

Each item in the rotation has individual settings:
- **Toggle** — enable/disable the item
- **Duration** — per-item display time, set with a **−/+** stepper (tap = ±1 s, press-and-hold = ±10 s). 0 = use the default duration (configurable below)
- **Color** — custom text color (0 = use default color)
- **Icon** — override the item's icon with an icon ID or filename (not available for Time/Date; leave empty for the default icon shown as a placeholder)
- **Celsius** — Temperature and Outdoor Temp only: show °C when on, °F when off
- **Offset** — Temperature only: correction applied to the indoor sensor reading (−15 to +5°)
- **Auto color** — Air Quality (AQI) and UV only: when on, the color is assigned dynamically by level (overrides the fixed/per-item color)

Drag any row to reorder. Changes save instantly and persist across reboots.

**Adding items:**
- **+ Add** button — opens a modal to add native, weather apps or effects
- Custom apps are added automatically when created via MQTT/HTTP/DataFetcher

**Global behavior:**
- **Auto Transition** — cycle through apps automatically
- **Transition Effect** — animation style (slide, dim, zoom, etc.)
- **Transition Speed** — animation speed (100–2000ms)
- **Scroll Speed** — text scroll speed
- **Default app duration** — seconds each app is shown when its per-item duration is 0 (1–30 s; same as the on-device `ROTACION → APP` field)
- **Block Navigation** — disable button navigation

### Fecha/Hora / Time/Date

- **Time Format / Date Format** — strftime format strings
- **Time Mode** — display style, 7 options: 1) Plain text (day below), 2) Calendar (day below), 3) Calendar (day above), 4) Calendar Alt (day below), 5) Calendar Alt (day above), 6) Big digits, 7) Binary. (With time formats that include seconds, only Plain text and Binary are available.)
- **Start on Monday** — week starts Monday
- **Show Weekday** — weekday indicator bar
- **Time / Date Color** — individual colors
- **Weekday Active / Inactive** — weekday dot colors
- **Calendar Header / Text / Body** — calendar box colors

### Sonido / Sound

- **Sound Enabled** — enable/disable buzzer
- **Volume** — buzzer volume (0–30)

### Vista / Screen

Live preview of the 32x8 LED matrix. Use **Prev/Next** buttons to navigate apps. **Download PNG** saves the current frame.

### Datos / Data

Configure external HTTP data sources. See [Data Fetcher](./datafetcher) for details.

### Alarmas / Alarms

Configure wake-up alarms:
- Set time and select days
- Add optional label
- Snooze (5 min) or dismiss when ringing

### Archivos / Files

Built-in file manager:
- Browse directories
- Upload files
- Edit text files inline
- Delete files/folders
- Create new directories

### Iconos / Icons

**Icon Picker** — Download icons from [LaMetric icon library](https://developer.lametric.com/icons):
1. Enter the icon ID number
2. Click **Preview** to see it
3. Click **Download** to save to `/ICONS/`

**Saved Icons Gallery** — Below the picker, a gallery displays all icons stored on the device:
- Grid view with image and ID number
- Delete button (×) on hover
- "Refresh" button to reload the list

### Respaldo / Backup

- **Download Backup** — saves all files and settings as JSON
- **Restore** — upload a backup file to restore (device reboots)

### Sistema / System

- **Save All Settings** — saves display, MQTT, and weather config at once
- **Reset Defaults** — restore factory settings (confirmation required)
- **Erase WiFi** — clear all WiFi credentials (device enters AP mode)
- **Reboot** — restart device (confirmation required)

### Actualizar / Update

Upload firmware (.bin or .bin.gz) for OTA update. The device reboots automatically after successful upload.
