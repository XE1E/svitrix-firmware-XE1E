#include "AirQualityLevels.h"

namespace AirQualityLevels
{

// Upper bounds (inclusive) for levels 1..5 in µg/m³; anything above the level-5
// bound is level 6. PM2.5/PM10/O3/NO2/SO2 use the EU EAQI bands; CO has no EAQI
// band, so it uses rough WHO/EPA-derived µg/m³ thresholds.
static const float kBreaks[COUNT][5] = {
    {10.0f, 20.0f, 25.0f, 50.0f, 75.0f},          // PM2.5
    {20.0f, 40.0f, 50.0f, 100.0f, 150.0f},        // PM10
    {50.0f, 100.0f, 130.0f, 240.0f, 380.0f},      // O3
    {40.0f, 90.0f, 120.0f, 230.0f, 340.0f},       // NO2
    {100.0f, 200.0f, 350.0f, 500.0f, 750.0f},     // SO2
    {4400.0f, 9400.0f, 12400.0f, 15400.0f, 30000.0f}, // CO
};

static const char *kLabels[COUNT] = {"PM2.5", "PM10", "O3", "NO2", "SO2", "CO"};

uint8_t level(Pollutant p, float ugm3)
{
    if (p >= COUNT || ugm3 <= 0.0f)
        return 0;
    for (uint8_t i = 0; i < 5; i++)
        if (ugm3 <= kBreaks[p][i])
            return i + 1;
    return 6;
}

const char *label(Pollutant p)
{
    return (p < COUNT) ? kLabels[p] : "";
}

uint32_t levelColor(uint8_t level)
{
    switch (level)
    {
    case 1:
        return 0x00FF00; // green
    case 2:
        return 0xFFFF00; // yellow
    case 3:
        return 0xFFA500; // orange
    case 4:
        return 0xFF0000; // red
    case 5:
        return 0x800080; // purple
    case 6:
        return 0x800000; // maroon
    default:
        return 0x000000;
    }
}

} // namespace AirQualityLevels
