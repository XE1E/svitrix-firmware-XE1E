#include "BatteryAlert.h"

namespace BatteryAlert
{

Action evaluate(uint8_t percent, bool charging, uint32_t nowMs, State &state)
{
    // Charging: no warnings, and re-arm both levels so the next discharge alerts.
    if (charging)
    {
        state.lowArmed = true;
        state.critArmed = true;
        return ALERT_NONE;
    }

    // Recovery re-arm (covers the case where charging wasn't detected but the
    // level climbed back up anyway).
    if (percent > kLowRearmPct)
        state.lowArmed = true;
    if (percent > kCriticalRearmPct)
        state.critArmed = true;

    if (percent <= kCriticalPct)
    {
        // First entry into critical: fire immediately and start the repeat timer.
        if (state.critArmed)
        {
            state.critArmed = false;
            state.lastCriticalMs = nowMs;
            return ALERT_CRITICAL;
        }
        // Still critical: re-fire every kCriticalRepeatMs. Unsigned subtraction
        // handles millis() wrap correctly.
        if ((uint32_t)(nowMs - state.lastCriticalMs) >= kCriticalRepeatMs)
        {
            state.lastCriticalMs = nowMs;
            return ALERT_CRITICAL;
        }
        return ALERT_NONE;
    }

    if (percent <= kLowPct)
    {
        if (state.lowArmed)
        {
            state.lowArmed = false;
            return ALERT_LOW;
        }
    }

    return ALERT_NONE;
}

} // namespace BatteryAlert
