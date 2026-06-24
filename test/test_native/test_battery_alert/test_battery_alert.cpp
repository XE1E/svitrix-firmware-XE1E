#include <unity.h>
#include "BatteryAlert.h"

using namespace BatteryAlert;

// --- above thresholds: nothing happens ---

void test_full_battery_no_alert(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(80, false, 1000, s));
}

void test_just_above_low_no_alert(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(21, false, 1000, s));
}

// --- LOW: fires once on crossing, not repeated ---

void test_low_fires_once_at_threshold(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_LOW, evaluate(20, false, 1000, s));
    // Still low, already fired → silent until recovery.
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(18, false, 11000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(15, false, 21000, s));
}

void test_low_rearms_after_recovery(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_LOW, evaluate(19, false, 1000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(16, false, 2000, s));
    // Recover above re-arm threshold (e.g. user charged a bit then unplugged).
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(30, false, 3000, s));
    // Drop again → fires again.
    TEST_ASSERT_EQUAL_UINT8(ALERT_LOW, evaluate(20, false, 4000, s));
}

// --- CRITICAL: fires immediately, repeats every 60s ---

void test_critical_fires_immediately(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(5, false, 1000, s));
}

void test_critical_repeats_every_60s(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, 1000, s));
    // Before 60s elapsed → silent.
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(4, false, 1000 + 30000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(4, false, 1000 + 59000, s));
    // At/after 60s → fires again.
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, 1000 + 60000, s));
    // ...and resets the timer.
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(4, false, 1000 + 70000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, 1000 + 120000, s));
}

void test_critical_supersedes_low(void)
{
    // At critical level the LOW branch is never taken.
    State s;
    s.lowArmed = true;
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(3, false, 1000, s));
}

// --- charging suppresses and re-arms ---

void test_charging_suppresses_alert(void)
{
    State s;
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(5, true, 1000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(15, true, 2000, s));
}

void test_charging_rearms_critical(void)
{
    State s;
    // Critical fires, then user plugs in (charging) → re-arms.
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, 1000, s));
    TEST_ASSERT_EQUAL_UINT8(ALERT_NONE, evaluate(10, true, 2000, s));
    // Unplug again while still low → critical fires immediately (armed).
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, 3000, s));
}

// --- millis() wrap-around safety ---

void test_critical_repeat_across_millis_wrap(void)
{
    State s;
    uint32_t nearMax = 0xFFFFFFFFUL - 10000UL; // 10s before wrap
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, nearMax, s));
    // 70s later the counter has wrapped past 0; unsigned math still measures 70s.
    uint32_t after = nearMax + 70000UL; // wraps around
    TEST_ASSERT_EQUAL_UINT8(ALERT_CRITICAL, evaluate(4, false, after, s));
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(test_full_battery_no_alert);
    RUN_TEST(test_just_above_low_no_alert);

    RUN_TEST(test_low_fires_once_at_threshold);
    RUN_TEST(test_low_rearms_after_recovery);

    RUN_TEST(test_critical_fires_immediately);
    RUN_TEST(test_critical_repeats_every_60s);
    RUN_TEST(test_critical_supersedes_low);

    RUN_TEST(test_charging_suppresses_alert);
    RUN_TEST(test_charging_rearms_critical);

    RUN_TEST(test_critical_repeat_across_millis_wrap);

    return UNITY_END();
}
