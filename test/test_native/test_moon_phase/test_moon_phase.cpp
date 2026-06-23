#include <unity.h>
#include "MoonPhase.h"
#include <cstring>

// Days from 1970-01-01 to a civil date (Howard Hinnant's algorithm), so tests
// don't depend on timegm() being available on the host toolchain.
static long daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= (m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097L + static_cast<long>(doe) - 719468L;
}

// Build a UTC epoch timestamp from a calendar UTC date/time.
static time_t utc(int y, unsigned mon, unsigned day, int h, int mi)
{
    return static_cast<time_t>(daysFromCivil(y, mon, day) * 86400L + h * 3600L + mi * 60L);
}

void setUp(void) {}
void tearDown(void) {}

// --- Known lunar events (UTC), 2024 ---

void test_new_moon_dark(void)
{
    // New moon: 2024-12-01 06:21 UTC
    MoonData d = computeMoonPhase(utc(2024, 12, 1, 6, 21));
    TEST_ASSERT_UINT_WITHIN(3, 0, d.illumination);
    TEST_ASSERT_EQUAL_UINT8(0, d.phaseIndex);
}

void test_full_moon_bright(void)
{
    // Full moon: 2024-12-15 09:02 UTC
    MoonData d = computeMoonPhase(utc(2024, 12, 15, 9, 2));
    TEST_ASSERT_UINT_WITHIN(3, 100, d.illumination);
    TEST_ASSERT_EQUAL_UINT8(4, d.phaseIndex);
}

void test_first_quarter(void)
{
    // First quarter: 2024-12-08 15:27 UTC
    MoonData d = computeMoonPhase(utc(2024, 12, 8, 15, 27));
    TEST_ASSERT_UINT_WITHIN(6, 50, d.illumination);
    TEST_ASSERT_EQUAL_UINT8(2, d.phaseIndex);
    TEST_ASSERT_TRUE(d.waxing);
}

void test_last_quarter(void)
{
    // Last quarter: 2024-12-22 22:18 UTC
    MoonData d = computeMoonPhase(utc(2024, 12, 22, 22, 18));
    TEST_ASSERT_UINT_WITHIN(6, 50, d.illumination);
    TEST_ASSERT_EQUAL_UINT8(6, d.phaseIndex);
    TEST_ASSERT_FALSE(d.waxing);
}

// --- Invariants over a long sweep ---

void test_ranges_over_sweep(void)
{
    // Step daily across ~5 years; every result must stay in range.
    time_t base = utc(2023, 1, 1, 0, 0);
    for (int i = 0; i < 365 * 5; i++)
    {
        MoonData d = computeMoonPhase(base + static_cast<time_t>(i) * 86400L);
        TEST_ASSERT_TRUE(d.illumination <= 100);
        TEST_ASSERT_TRUE(d.phaseIndex <= 7);
        TEST_ASSERT_TRUE(d.fraction >= 0.0 && d.fraction < 1.0);
        TEST_ASSERT_TRUE(d.ageDays >= 0.0 && d.ageDays < 29.6);
        TEST_ASSERT_EQUAL(d.fraction < 0.5, d.waxing);
    }
}

void test_waxing_then_waning(void)
{
    // Day after the new moon → waxing; day before next new moon → waning.
    MoonData a = computeMoonPhase(utc(2024, 12, 3, 0, 0));
    TEST_ASSERT_TRUE(a.waxing);
    MoonData b = computeMoonPhase(utc(2024, 12, 28, 0, 0));
    TEST_ASSERT_FALSE(b.waxing);
}

// --- Names ---

void test_names_cardinal(void)
{
    TEST_ASSERT_EQUAL_STRING("Nueva", moonPhaseName(0));
    TEST_ASSERT_EQUAL_STRING("Cuarto creciente", moonPhaseName(2));
    TEST_ASSERT_EQUAL_STRING("Llena", moonPhaseName(4));
    TEST_ASSERT_EQUAL_STRING("Cuarto menguante", moonPhaseName(6));
}

void test_name_index_wraps(void)
{
    // Index is masked to 0..7 — out-of-range must not read past the array.
    TEST_ASSERT_EQUAL_STRING("Nueva", moonPhaseName(8));
    TEST_ASSERT_EQUAL_STRING("Llena", moonPhaseName(12));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_new_moon_dark);
    RUN_TEST(test_full_moon_bright);
    RUN_TEST(test_first_quarter);
    RUN_TEST(test_last_quarter);
    RUN_TEST(test_ranges_over_sweep);
    RUN_TEST(test_waxing_then_waning);
    RUN_TEST(test_names_cardinal);
    RUN_TEST(test_name_index_wraps);
    return UNITY_END();
}
