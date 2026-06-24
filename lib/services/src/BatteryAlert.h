#pragma once

/**
 * @file BatteryAlert.h
 * @brief Pure low-battery alert policy — no hardware, no globals.
 *
 * Decides, from a corrected battery percentage and a "charging" hint, whether
 * to raise a low-battery warning and how urgent it is. All timing is passed in
 * (millis) so the logic stays deterministic and unit-testable.
 *
 * Two levels:
 *   - LOW      (<= kLowPct):      gentle visual cue, fires once per discharge.
 *   - CRITICAL (<= kCriticalPct): urgent cue, fires immediately then repeats
 *                                  every kCriticalRepeatMs while still critical.
 *
 * Re-arming: a level can fire again only after the battery recovers above its
 * re-arm threshold (i.e. after charging). Charging suppresses all warnings and
 * re-arms both levels.
 *
 * Used by: PeripheryManager (battery read loop).
 * Tests: test/test_native/test_battery_alert/
 */

#include <cstdint>

namespace BatteryAlert
{

// Prefixed to avoid colliding with Arduino's LOW/HIGH GPIO macros.
enum Action : uint8_t
{
    ALERT_NONE = 0,     ///< Nothing to do.
    ALERT_LOW = 1,      ///< Early warning: red battery icon, no sound.
    ALERT_CRITICAL = 2, ///< Critical: red battery icon + beep (every kCriticalRepeatMs).
};

// Thresholds expressed against the corrected battery percentage [0..100].
constexpr uint8_t kLowPct = 20;           ///< Early-warning threshold.
constexpr uint8_t kCriticalPct = 5;       ///< Critical threshold.
constexpr uint8_t kLowRearmPct = 25;      ///< Re-arm LOW once back above this.
constexpr uint8_t kCriticalRearmPct = 10; ///< Re-arm CRITICAL once back above this.
constexpr uint32_t kCriticalRepeatMs = 60000UL; ///< Re-beep interval while critical.

/// Persistent state carried between evaluations (owned by the caller).
struct State
{
    bool lowArmed = true;           ///< LOW may fire.
    bool critArmed = true;          ///< First CRITICAL may fire.
    uint32_t lastCriticalMs = 0;    ///< millis() of the last CRITICAL emitted.
};

/**
 * Evaluate the alert policy for one battery reading.
 *
 * @param percent  Corrected battery percentage [0..100].
 * @param charging True when the battery is charging (warnings suppressed).
 * @param nowMs    Current millis() timestamp (for the critical repeat timer).
 * @param state    Mutable state carried across calls.
 * @return The action to take this tick (NONE / LOW / CRITICAL).
 */
Action evaluate(uint8_t percent, bool charging, uint32_t nowMs, State &state);

} // namespace BatteryAlert
