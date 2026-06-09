#include <MenuManager.h>
#include <Arduino.h>
#include <Globals.h>
#include "IDisplayRenderer.h"
#include "IDisplayControl.h"
#include "IDisplayNavigation.h"
#include <cassert>
#include "IPeripheryProvider.h"
#include "IUpdater.h"
#include "IAlarmProvider.h"
#include "timer.h"
#include <icons.h>
#include <WiFi.h>

enum MenuState
{
    MainMenu,
    BrightnessMenu,
    ColorMenu,
    RotationMenu, // submenu: ROTAR + DUR-TRA + VEL-DES + DUR-APP (rotationField)
    TimeFormatMenu,
    DateFormatMenu,
    WeekdayMenu,
    TempMenu,
    Appmenu,
    NightMenu,
    InfoMenu,
    SoundMenu,
    VolumeMenu,
    UpdateMenu,
    AlarmsMenu, // alarm list / select (mapped to "ALARMS" menu item)
    MaxMenu,
    AlarmEditMenu // edit a single alarm (sub-state, not a top-level item)
};

const char *menuItems[] PROGMEM = {
    "BRILLO",
    "COLOR",
    "ROTACION",
    "HORA",
    "FECHA",
    "INI-SEM",
    "TEMP",
    "APPS",
    "NOCHE",
    "INFO",
    "SONIDO",
    "VOLUMEN",
    "OTA",
    "ALARMAS"};

int8_t menuIndex = 0;
uint8_t menuItemCount = MaxMenu - 1;

const char *timeFormat[] PROGMEM = {
    "%H %M",    // 24H blink (default)
    "%H:%M",    // 24H colon
    "%H-%M",    // 24H dash
    "%I %M",    // 12H blink
    "%I:%M",    // 12H colon
    "%I-%M",    // 12H dash
    "%H %M %S", // 24H blink + seconds
    "%H:%M:%S", // 24H colon + seconds
    "%H-%M-%S", // 24H dash + seconds
    "%I %M %S", // 12H blink + seconds
    "%I:%M:%S", // 12H colon + seconds
    "%I-%M-%S", // 12H dash + seconds
};
int8_t timeFormatIndex = 0;
uint8_t timeFormatCount = 12;

const char *dateFormat[] PROGMEM = {
    "%d.%m.%y", // DD.MM.YY (EU)
    "%d-%m-%y", // DD-MM-YY
    "%d.%m.",   // DD.MM.
    "%d %b",    // DD Mon
    "%m.%d.%y", // MM.DD.YY (US)
    "%m-%d-%y", // MM-DD-YY
    "%m/%d/%y", // MM/DD/YY
    "%b %d",    // Mon DD
    "%y-%m-%d", // YY-MM-DD (ISO)
};

int8_t dateFormatIndex = 0;
uint8_t dateFormatCount = 9;

/// Find index of format string in array, returns 0 if not found.
static int8_t findFormatIndex(const char *current, const char *formats[], uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        if (strcmp(current, formats[i]) == 0)
            return i;
    }
    return 0;
}

int8_t appsIndex = 0;
static const uint8_t appsCount = 10;

// Native apps shown in the APPS menu (must match the rotation item names used
// by the web). State (ON/OFF) and toggling go through the rotation config.
static const char *kAppMenuNames[appsCount] = {
    "Time", "Date", "Temperature", "Humidity", "Battery",
    "OutdoorTemp", "OutdoorHum", "Pressure", "AirQuality", "UV"};

int8_t infoIndex = 0;
static const uint8_t infoCount = 5; // IP, WIFI, VER, HOST, MEM

// Rotation submenu state — which parameter is being shown/edited
uint8_t rotationField = 0; // 0=ROTAR, 1=DUR-TRA, 2=VEL-DES, 3=DUR-APP
static const uint8_t ROTATION_FIELD_COUNT = 4;

// Alarm menu state
int8_t alarmSel = 0;    // selected alarm index in AlarmsMenu
uint8_t alarmField = 0; // 0=enabled, 1=hour, 2=minute (AlarmEditMenu)
Alarm alarmEdit;        // working copy of the alarm being edited

MenuState currentState = MainMenu;

uint32_t textColors[] = {
    0xFFFFFF, // White
    0xFF0000, // Red
    0x00FF00, // Green
    0x0000FF, // Blue
    0xFFFF00, // Yellow
    0xFF00FF, // Magenta
    0x00FFFF, // Cyan
    0xFFA500, // Orange
    0x800080, // Purple
    0x008080, // Teal
    0x808000, // Olive
    0x800000, // Maroon
    0x008000, // Dark Green
    0x000080, // Navy
    0x808080  // Gray
};

uint8_t currentColor = 0;
static const uint8_t COLOR_COUNT = sizeof(textColors) / sizeof(textColors[0]);
static const uint8_t MAX_VOLUME = 30;
static const int MIN_TRANSITION_SPEED = 200;
static const int MAX_TRANSITION_SPEED = 2000;
static const int TRANSITION_SPEED_STEP = 100;
static const long MIN_APP_TIME = 1000;
static const long MAX_APP_TIME = 30000;
static const long APP_TIME_STEP = 1000;
static const int MIN_SCROLL_SPEED = 10; // matches the web slider (10-100)
static const int MAX_SCROLL_SPEED = 100;
static const int SCROLL_SPEED_STEP = 10;

MenuManager_& MenuManager_::getInstance()
{
    static MenuManager_ instance;
    return instance;
}

// Initialize the global shared instance
MenuManager_& MenuManager = MenuManager.getInstance();

void MenuManager_::setDisplay(IDisplayRenderer *r, IDisplayControl *c, IDisplayNavigation *n)
{
    assert(r && c && n);
    renderer_ = r;
    control_ = c;
    nav_ = n;
}

void MenuManager_::setServices(IPeripheryProvider *pp, IUpdater *u)
{
    assert(pp && u);
    periphery_ = pp;
    updater_ = u;
}

void MenuManager_::setAlarmProvider(IAlarmProvider *a)
{
    assert(a);
    alarm_ = a;
}

int convertBRIPercentTo8Bit(int brightness_percent)
{
    // Linear, rounded mapping — consistent with the web's percent slider so the
    // menu and web agree (no jump/drift). Very low brightness is further damped
    // by DisplayManager's low-brightness handling.
    if (brightness_percent <= 0)
        return 0;
    if (brightness_percent >= 100)
        return 255;
    return (brightness_percent * 255 + 50) / 100;
}

String MenuManager_::menutext()
{
    char buf[24];
    switch (currentState)
    {
    case MainMenu:
        renderer_->drawMenuIndicator(menuIndex, menuItemCount, 0xF80000);
        return menuItems[menuIndex];
    case BrightnessMenu:
        if (brightnessConfig.autoBrightness)
            return "AUTO";
        snprintf(buf, sizeof(buf), "%d%%", brightnessConfig.brightnessPercent);
        return buf;
    case ColorMenu:
        renderer_->drawMenuIndicator(currentColor, COLOR_COUNT, 0xFBC000);
        renderer_->setTextColor(textColors[currentColor]);
        snprintf(buf, sizeof(buf), "0X%X", textColors[currentColor]);
        return buf;
    case SoundMenu:
        return audioConfig.soundActive ? "SI" : "NO";
    case RotationMenu:
        // Submenu with 4 fields; the prefix tells which one (the indicator
        // shows the position). Select cycles fields, Left/Right adjust.
        renderer_->drawMenuIndicator(rotationField, ROTATION_FIELD_COUNT, 0xFBC000);
        // No "s" suffix: the SvitrixFont 's' glyph reads as '5' next to digits
        // on the 8px matrix. Prefix + magnitude already convey seconds.
        if (rotationField == 0)
            snprintf(buf, sizeof(buf), "ROT %s", appConfig.autoTransition ? "SI" : "NO");
        else if (rotationField == 1)
            snprintf(buf, sizeof(buf), "TRA %.1f", appConfig.timePerTransition / 1000.0);
        else if (rotationField == 2)
            snprintf(buf, sizeof(buf), "DES %d", appConfig.scrollSpeed);
        else
            snprintf(buf, sizeof(buf), "APP %.0f", appConfig.timePerApp / 1000.0);
        return buf;
    case TimeFormatMenu:
    {
        renderer_->drawMenuIndicator(timeFormatIndex, timeFormatCount, 0xFBC000);
        char display[9];
        if (timeFormat[timeFormatIndex][2] == ' ')
        {
            snprintf(display, sizeof(display), "%s", timeFormat[timeFormatIndex]);
            display[2] = timer_time() % 2 ? ' ' : ':';
        }
        else
        {
            snprintf(display, sizeof(display), "%s", timeFormat[timeFormatIndex]);
        }
        strftime(buf, sizeof(buf), display, timer_localtime());
        return buf;
    }
    case DateFormatMenu:
        renderer_->drawMenuIndicator(dateFormatIndex, dateFormatCount, 0xFBC000);
        strftime(buf, sizeof(buf), dateFormat[dateFormatIndex], timer_localtime());
        return buf;
    case WeekdayMenu:
        return timeConfig.startOnMonday ? "LUN" : "DOM";
    case TempMenu:
        return timeConfig.isCelsius ? "°C" : "°F";
    case Appmenu:
    {
        // Icons stay the existing per-app bitmaps for now (real-icon unification
        // is a follow-up step). UV (index 9) is the new entry.
        const uint16_t *icons[appsCount] = {
            icon_13, icon_1158, icon_234, icon_2075, icon_1486,
            icon_sunny, icon_weather_hum, icon_weather_pressure,
            icon_weather_aqi, icon_weather_uv};
        if (appsIndex >= 0 && appsIndex < (int)appsCount)
            renderer_->drawBMP(0, 0, icons[appsIndex], 8, 8);
        // ON/OFF reads the rotation config — the same source as the web and the
        // running app loop — so the menu, web and screen always agree.
        const char *appResult =
            (nav_ && nav_->rotationAppState(kAppMenuNames[appsIndex]) == 1) ? "SI" : "NO";
        renderer_->drawMenuIndicator(appsIndex, appsCount, 0xFBC000);
        return appResult;
    }
    case NightMenu:
        return appConfig.nightMode ? "SI" : "NO";
    case InfoMenu:
    {
        renderer_->drawMenuIndicator(infoIndex, infoCount, 0x00FFFF);
        String ip;
        switch (infoIndex)
        {
        case 0: // IP - show last two octets to fit
            ip = WiFi.localIP().toString();
            snprintf(buf, sizeof(buf), "IP %s", ip.substring(ip.lastIndexOf('.', ip.lastIndexOf('.') - 1) + 1).c_str());
            return buf;
        case 1: // WiFi signal
            snprintf(buf, sizeof(buf), "WIFI %d", WiFi.RSSI());
            return buf;
        case 2: // Version
            snprintf(buf, sizeof(buf), "V%s", VERSION);
            return buf;
        case 3: // Hostname - show device ID suffix
            snprintf(buf, sizeof(buf), "ID %s", systemConfig.deviceId.substring(systemConfig.deviceId.length() - 6).c_str());
            return buf;
        case 4: // Free RAM
            snprintf(buf, sizeof(buf), "RAM %dK", ESP.getFreeHeap() / 1024);
            return buf;
        default:
            break;
        }
        break;
    }
    case VolumeMenu:
        snprintf(buf, sizeof(buf), "%d", audioConfig.soundVolume);
        return buf;
    case AlarmsMenu:
    {
        if (!alarm_)
            return "SIN ALARMA";
        auto alarms = alarm_->getAlarms();
        if (alarms.empty())
            return "SIN ALARMAS";
        if (alarmSel >= (int)alarms.size())
            alarmSel = 0;
        renderer_->drawMenuIndicator(alarmSel, alarms.size(), 0xFBC000);
        const Alarm& a = alarms[alarmSel];
        snprintf(buf, sizeof(buf), "%02d:%02d %s", a.hour, a.minute, a.enabled ? "SI" : "NO");
        return buf;
    }
    case AlarmEditMenu:
        renderer_->drawMenuIndicator(alarmField, 3, 0x00FF00);
        if (alarmField == 0)
            snprintf(buf, sizeof(buf), "ACT %s", alarmEdit.enabled ? "SI" : "NO");
        else if (alarmField == 1)
            snprintf(buf, sizeof(buf), "H %02d", alarmEdit.hour);
        else
            snprintf(buf, sizeof(buf), "M %02d", alarmEdit.minute);
        return buf;
    default:
        break;
    }
    return "";
}

void MenuManager_::rightButton()
{
    if (!inMenu)
        return;
    switch (currentState)
    {
    case MainMenu:
        menuIndex = (menuIndex + 1) % menuItemCount;
        break;
    case BrightnessMenu:
        if (!brightnessConfig.autoBrightness)
        {
            brightnessConfig.brightnessPercent = (brightnessConfig.brightnessPercent % 100) + 1;
            brightnessConfig.brightness = convertBRIPercentTo8Bit(brightnessConfig.brightnessPercent);
            control_->setBrightness(brightnessConfig.brightness);
        }
        break;
    case ColorMenu:
        currentColor = (currentColor + 1) % COLOR_COUNT;
        break;
    case RotationMenu:
        if (rotationField == 0)
            appConfig.autoTransition = !appConfig.autoTransition;
        else if (rotationField == 1)
            appConfig.timePerTransition = min(MAX_TRANSITION_SPEED, appConfig.timePerTransition + TRANSITION_SPEED_STEP);
        else if (rotationField == 2)
            appConfig.scrollSpeed = min(MAX_SCROLL_SPEED, appConfig.scrollSpeed + SCROLL_SPEED_STEP);
        else
            appConfig.timePerApp = min(MAX_APP_TIME, appConfig.timePerApp + APP_TIME_STEP);
        break;
    case TimeFormatMenu:
        timeFormatIndex = (timeFormatIndex + 1) % timeFormatCount;
        break;
    case DateFormatMenu:
        dateFormatIndex = (dateFormatIndex + 1) % dateFormatCount;
        break;
    case Appmenu:
        appsIndex = (appsIndex + 1) % appsCount;
        break;
    case InfoMenu:
        infoIndex = (infoIndex + 1) % infoCount;
        break;
    case WeekdayMenu:
        timeConfig.startOnMonday = !timeConfig.startOnMonday;
        break;
    case SoundMenu:
        audioConfig.soundActive = !audioConfig.soundActive;
        break;
    case TempMenu:
        timeConfig.isCelsius = !timeConfig.isCelsius;
        break;
    case VolumeMenu:
        if ((audioConfig.soundVolume + 1) > MAX_VOLUME)
            audioConfig.soundVolume = 0;
        else
            audioConfig.soundVolume++;
        break;
    case AlarmsMenu:
        if (alarm_)
        {
            auto alarms = alarm_->getAlarms();
            if (!alarms.empty())
                alarmSel = (alarmSel + 1) % (int)alarms.size();
        }
        break;
    case AlarmEditMenu:
        if (alarmField == 0)
            alarmEdit.enabled = !alarmEdit.enabled;
        else if (alarmField == 1)
            alarmEdit.hour = (alarmEdit.hour + 1) % 24;
        else
            alarmEdit.minute = (alarmEdit.minute + 1) % 60;
        break;
    default:
        break;
    }
}

void MenuManager_::leftButton()
{
    if (!inMenu)
    {
        return;
    }
    switch (currentState)
    {
    case MainMenu:
        menuIndex = (menuIndex == 0) ? menuItemCount - 1 : menuIndex - 1;
        break;
    case BrightnessMenu:
        if (!brightnessConfig.autoBrightness)
        {
            brightnessConfig.brightnessPercent = (brightnessConfig.brightnessPercent == 1) ? 100 : brightnessConfig.brightnessPercent - 1;
            brightnessConfig.brightness = convertBRIPercentTo8Bit(brightnessConfig.brightnessPercent);
            control_->setBrightness(brightnessConfig.brightness);
        }
        break;
    case ColorMenu:
        currentColor = (currentColor + COLOR_COUNT - 1) % COLOR_COUNT;
        break;
    case RotationMenu:
        if (rotationField == 0)
            appConfig.autoTransition = !appConfig.autoTransition;
        else if (rotationField == 1)
            appConfig.timePerTransition = max(MIN_TRANSITION_SPEED, appConfig.timePerTransition - TRANSITION_SPEED_STEP);
        else if (rotationField == 2)
            appConfig.scrollSpeed = max(MIN_SCROLL_SPEED, appConfig.scrollSpeed - SCROLL_SPEED_STEP);
        else
            appConfig.timePerApp = max(MIN_APP_TIME, appConfig.timePerApp - APP_TIME_STEP);
        break;
    case TimeFormatMenu:
        timeFormatIndex = (timeFormatIndex == 0) ? timeFormatCount - 1 : timeFormatIndex - 1;
        break;
    case DateFormatMenu:
        dateFormatIndex = (dateFormatIndex == 0) ? dateFormatCount - 1 : dateFormatIndex - 1;
        break;
    case Appmenu:
        appsIndex = (appsIndex == 0) ? appsCount - 1 : appsIndex - 1;
        break;
    case InfoMenu:
        infoIndex = (infoIndex == 0) ? infoCount - 1 : infoIndex - 1;
        break;
    case WeekdayMenu:
        timeConfig.startOnMonday = !timeConfig.startOnMonday;
        break;
    case TempMenu:
        timeConfig.isCelsius = !timeConfig.isCelsius;
        break;
    case SoundMenu:
        audioConfig.soundActive = !audioConfig.soundActive;
        break;
    case VolumeMenu:
        if ((audioConfig.soundVolume - 1) < 0)
            audioConfig.soundVolume = MAX_VOLUME;
        else
            audioConfig.soundVolume--;
        break;
    case AlarmsMenu:
        if (alarm_)
        {
            auto alarms = alarm_->getAlarms();
            if (!alarms.empty())
                alarmSel = (alarmSel == 0) ? (int)alarms.size() - 1 : alarmSel - 1;
        }
        break;
    case AlarmEditMenu:
        if (alarmField == 0)
            alarmEdit.enabled = !alarmEdit.enabled;
        else if (alarmField == 1)
            alarmEdit.hour = (alarmEdit.hour == 0) ? 23 : alarmEdit.hour - 1;
        else
            alarmEdit.minute = (alarmEdit.minute == 0) ? 59 : alarmEdit.minute - 1;
        break;
    default:
        break;
    }
}

void MenuManager_::selectButton()
{
    if (!inMenu)
    {
        return;
    }
    switch (currentState)
    {
    case MainMenu:
        currentState = (MenuState)(menuIndex + 1);
        switch (currentState)
        {
        case BrightnessMenu:
            // Inverse of convertBRIPercentTo8Bit (rounded linear) — matches web.
            brightnessConfig.brightnessPercent = (brightnessConfig.brightness * 100 + 127) / 255;
            break;
        case UpdateMenu:
            if (updater_->checkUpdate(true))
            {
                updater_->updateFirmware();
            }
            break;
        case AlarmsMenu:
            alarmSel = 0;
            break;
        case RotationMenu:
            rotationField = 0;
            break;
        case TimeFormatMenu:
            timeFormatIndex = findFormatIndex(timeConfig.timeFormat.c_str(), timeFormat, timeFormatCount);
            break;
        case DateFormatMenu:
            dateFormatIndex = findFormatIndex(timeConfig.dateFormat.c_str(), dateFormat, dateFormatCount);
            break;
        case ColorMenu:
        {
            // Seed the preset index from the current text color so the menu
            // reflects what the web/screen show (nearest of the 15 presets).
            uint32_t cur = colorConfig.textColor;
            uint32_t bestDist = 0xFFFFFFFF;
            for (uint8_t i = 0; i < COLOR_COUNT; i++)
            {
                int dr = (int)((textColors[i] >> 16) & 0xFF) - (int)((cur >> 16) & 0xFF);
                int dg = (int)((textColors[i] >> 8) & 0xFF) - (int)((cur >> 8) & 0xFF);
                int db = (int)(textColors[i] & 0xFF) - (int)(cur & 0xFF);
                uint32_t dist = (uint32_t)(dr * dr + dg * dg + db * db);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    currentColor = i;
                }
            }
            break;
        }
        default:
            break;
        }
        break;
    case BrightnessMenu:
        brightnessConfig.autoBrightness = !brightnessConfig.autoBrightness;
        if (!brightnessConfig.autoBrightness)
        {
            brightnessConfig.brightness = convertBRIPercentTo8Bit(brightnessConfig.brightnessPercent);
            control_->setBrightness(brightnessConfig.brightness);
        }
        break;
    case Appmenu:
        if (nav_ && appsIndex >= 0 && appsIndex < (int)appsCount)
        {
            const char *n = kAppMenuNames[appsIndex];
            // Flip the app's enabled in the rotation config (same source as the
            // web). In-memory; persisted + applied to the live loop on exit
            // (selectButtonLong calls loadNativeApps + saveSettings).
            nav_->setRotationAppEnabled(n, nav_->rotationAppState(n) != 1);
        }
        break;
    case NightMenu:
        appConfig.nightMode = !appConfig.nightMode;
        break;
    case RotationMenu:
        // Short Select cycles the field; Select-long saves + exits.
        rotationField = (rotationField + 1) % ROTATION_FIELD_COUNT;
        break;
    case AlarmsMenu:
        if (alarm_)
        {
            auto alarms = alarm_->getAlarms();
            if (!alarms.empty())
            {
                if (alarmSel >= (int)alarms.size())
                    alarmSel = 0;
                alarmEdit = alarms[alarmSel];
                alarmField = 0;
                currentState = AlarmEditMenu;
            }
        }
        break;
    case AlarmEditMenu:
        alarmField = (alarmField + 1) % 3; // cycle enabled → hour → minute
        break;
    default:
        break;
    }
}

void MenuManager_::selectButtonLong()
{
    // While an alarm is ringing, a long-press turns it off instead of
    // opening the menu (Select = off, Left/Right = snooze).
    if (!inMenu && alarm_ && alarm_->isRinging())
    {
        alarm_->dismiss();
        return;
    }

    if (inMenu)
    {
        switch (currentState)
        {
        case BrightnessMenu:
            // brightnessConfig.brightness = map(brightnessConfig.brightnessPercent, 0, 100, 0, 255);
            saveSettings();
            break;
        case ColorMenu:
            colorConfig.textColor = textColors[currentColor];
            nav_->setCustomAppColors(colorConfig.textColor);
            control_->applyAllSettings(); // apply live, same as the web color path
            saveSettings();
            break;
        case MainMenu:
            inMenu = false;
            break;
        case RotationMenu:
            control_->setAutoTransition(appConfig.autoTransition);
            control_->applyAllSettings(); // applies transition/scroll/app-time live
            saveSettings();
            break;
        case TimeFormatMenu:
            timeConfig.timeFormat = timeFormat[timeFormatIndex];
            saveSettings();
            break;
        case DateFormatMenu:
            timeConfig.dateFormat = dateFormat[dateFormatIndex];
            saveSettings();
            break;
        case WeekdayMenu:
        case SoundMenu:
        case TempMenu:
        case NightMenu:
            saveSettings();
            break;
        case Appmenu:
            nav_->loadNativeApps();
            saveSettings();
            break;
        case VolumeMenu:
            periphery_->setVolume(audioConfig.soundVolume);
            saveSettings();
            break;
        case AlarmEditMenu:
            if (alarm_)
                alarm_->updateAlarm(alarmEdit); // persists
            currentState = AlarmsMenu;          // back to the list, not MainMenu
            return;
        default:
            break;
        }
        currentState = MainMenu;
    }
    else
    {
        inMenu = true;
    }
}
