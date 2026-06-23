#include "MoonPhase.h"
#include <cmath>

namespace {
// Mean synodic month (new moon to new moon), days.
constexpr double kSynodicMonth = 29.530588853;
// Julian Date of a known new moon: 2000-01-06 18:14 UTC.
constexpr double kRefNewMoonJD = 2451550.1;
// Julian Date of the Unix epoch (1970-01-01 00:00 UTC).
constexpr double kUnixEpochJD = 2440587.5;
constexpr double kTwoPi = 6.283185307179586;
} // namespace

MoonData computeMoonPhase(time_t utc)
{
    const double jd = static_cast<double>(utc) / 86400.0 + kUnixEpochJD;
    const double cycles = (jd - kRefNewMoonJD) / kSynodicMonth;

    double frac = cycles - std::floor(cycles); // 0..1
    if (frac < 0.0)
        frac += 1.0;

    MoonData d;
    d.fraction = frac;
    d.ageDays = frac * kSynodicMonth;
    // Illuminated fraction: 0 at new moon, 1 at full moon.
    const double illum = (1.0 - std::cos(kTwoPi * frac)) * 0.5;
    d.illumination = static_cast<uint8_t>(std::lround(illum * 100.0));
    // Discrete phase, with the cardinal phases (new/Q1/full/Q3) centered on
    // their slots: round fraction*8 to the nearest of 8 buckets.
    d.phaseIndex = static_cast<uint8_t>(static_cast<int>(std::floor(frac * 8.0 + 0.5)) % 8);
    d.waxing = frac < 0.5;
    return d;
}

const char *moonPhaseName(uint8_t phaseIndex)
{
    static const char *const names[8] = {
        "Nueva",            // 0 new
        "Creciente",        // 1 waxing crescent
        "Cuarto creciente", // 2 first quarter
        "Gibosa creciente", // 3 waxing gibbous
        "Llena",            // 4 full
        "Gibosa menguante", // 5 waning gibbous
        "Cuarto menguante", // 6 last quarter
        "Menguante"         // 7 waning crescent
    };
    return names[phaseIndex & 7];
}
