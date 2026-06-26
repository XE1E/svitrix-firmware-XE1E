#include <unity.h>
#include "AirQualityLevels.h"

using namespace AirQualityLevels;

void setUp(void) {}
void tearDown(void) {}

void test_invalid_values_return_zero(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, level(PM2_5, 0.0f));
    TEST_ASSERT_EQUAL_UINT8(0, level(PM2_5, -5.0f));
    TEST_ASSERT_EQUAL_UINT8(0, level(COUNT, 10.0f)); // out-of-range pollutant
}

void test_pm25_bands(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, level(PM2_5, 5.0f));   // <= 10
    TEST_ASSERT_EQUAL_UINT8(1, level(PM2_5, 10.0f));  // boundary inclusive
    TEST_ASSERT_EQUAL_UINT8(2, level(PM2_5, 15.0f));  // <= 20
    TEST_ASSERT_EQUAL_UINT8(3, level(PM2_5, 25.0f));  // <= 25
    TEST_ASSERT_EQUAL_UINT8(4, level(PM2_5, 40.0f));  // <= 50
    TEST_ASSERT_EQUAL_UINT8(5, level(PM2_5, 75.0f));  // <= 75
    TEST_ASSERT_EQUAL_UINT8(6, level(PM2_5, 100.0f)); // > 75
}

void test_o3_bands(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, level(O3, 50.0f));
    TEST_ASSERT_EQUAL_UINT8(4, level(O3, 200.0f)); // 130 < x <= 240
    TEST_ASSERT_EQUAL_UINT8(6, level(O3, 500.0f)); // > 380
}

void test_co_high_bands(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, level(CO, 1000.0f));
    TEST_ASSERT_EQUAL_UINT8(4, level(CO, 13000.0f)); // 12400 < x <= 15400
    TEST_ASSERT_EQUAL_UINT8(6, level(CO, 50000.0f)); // > 30000
}

void test_labels(void)
{
    TEST_ASSERT_EQUAL_STRING("PM2.5", label(PM2_5));
    TEST_ASSERT_EQUAL_STRING("PM10", label(PM10));
    TEST_ASSERT_EQUAL_STRING("O3", label(O3));
    TEST_ASSERT_EQUAL_STRING("NO2", label(NO2));
    TEST_ASSERT_EQUAL_STRING("SO2", label(SO2));
    TEST_ASSERT_EQUAL_STRING("CO", label(CO));
    TEST_ASSERT_EQUAL_STRING("", label(COUNT)); // out of range
}

void test_level_colors(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00FF00, levelColor(1));
    TEST_ASSERT_EQUAL_HEX32(0xFFA500, levelColor(3));
    TEST_ASSERT_EQUAL_HEX32(0xFF0000, levelColor(4));
    TEST_ASSERT_EQUAL_HEX32(0x800000, levelColor(6));
    TEST_ASSERT_EQUAL_HEX32(0x000000, levelColor(0)); // invalid
    TEST_ASSERT_EQUAL_HEX32(0x000000, levelColor(7)); // out of range
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_invalid_values_return_zero);
    RUN_TEST(test_pm25_bands);
    RUN_TEST(test_o3_bands);
    RUN_TEST(test_co_high_bands);
    RUN_TEST(test_labels);
    RUN_TEST(test_level_colors);
    return UNITY_END();
}
