#pragma once

/**
 * @file MoonPhase.h
 * @brief Pure moon-phase calculation — no hardware, no Arduino dependency.
 *
 * Given a UTC epoch timestamp, returns the moon's age, illuminated fraction,
 * a discrete 0..7 phase index, and the waxing/waning flag. Uses the mean
 * synodic month from a reference new moon (good to ~a few hours, plenty for
 * a pixel-clock display).
 *
 * Hemisphere is intentionally NOT handled here: the phase, illumination and
 * names are identical worldwide — only the *drawn* lit limb flips. The render
 * code (MoonApp) mirrors the disk for the southern hemisphere.
 *
 * Used by: MoonApp (src/Apps).
 * Tests:   test/test_native/test_moon_phase/
 */

#include <cstdint>
#include <ctime>

struct MoonData {
    double ageDays;        ///< Days since the last new moon (0 .. ~29.53).
    double fraction;       ///< Cycle position 0..1 (0 = new, 0.5 = full). cos(2π·fraction) drives the terminator.
    uint8_t illumination;  ///< Illuminated fraction as a percentage, 0..100.
    uint8_t phaseIndex;    ///< Discrete phase 0..7 (0 = new, 2 = first quarter, 4 = full, 6 = last quarter).
    bool waxing;           ///< true while the moon is waxing (fraction < 0.5).
};

/**
 * Compute moon data for a UTC epoch timestamp.
 * @param utc seconds since the Unix epoch (UTC).
 */
MoonData computeMoonPhase(time_t utc);

/**
 * Spanish phase name for a 0..7 phase index (northern-hemisphere naming, but
 * the names are hemisphere-independent). Index is masked to 0..7.
 */
const char *moonPhaseName(uint8_t phaseIndex);
