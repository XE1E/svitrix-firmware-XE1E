#pragma once

/**
 * @file AirQualityLevels.h
 * @brief Pure air-quality classification — concentration (µg/m³) → 1..6 level.
 *
 * WeatherAPI returns each pollutant concentration in µg/m³ (with `aqi=yes`).
 * These functions map a concentration to a discrete 1..6 severity level using
 * the European Air Quality Index (EAQI) µg/m³ bands (CO uses rough WHO bands),
 * and expose the matching display color + short label. The 6 levels share the
 * same palette as the app's US-EPA index colors so a single color scale reads
 * consistently across the overall index and the per-pollutant breakdown.
 *
 * Used by: AirQualityApp (src/Apps) to flag pollutants at level >= 4.
 * Tests:   test/test_native/test_air_quality/
 */

#include <cstdint>

namespace AirQualityLevels
{

/// Pollutants exposed by WeatherAPI's air_quality object.
enum Pollutant : uint8_t
{
    PM2_5 = 0,
    PM10,
    O3,
    NO2,
    SO2,
    CO,
    COUNT
};

/// Discrete severity level for a pollutant concentration in µg/m³.
/// @return 1..6 (1 = best, 6 = worst); 0 when the value is invalid (<= 0).
uint8_t level(Pollutant p, float ugm3);

/// Short uppercase label for a pollutant (e.g. "PM2.5", "O3"). Masked to range.
const char *label(Pollutant p);

/// Packed 0xRRGGBB color for a 1..6 level (shared with the EPA-index palette).
/// Level 0 or out-of-range returns 0 (black).
uint32_t levelColor(uint8_t level);

} // namespace AirQualityLevels
