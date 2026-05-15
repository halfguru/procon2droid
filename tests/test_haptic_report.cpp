// Tests for the HD Rumble HID report builder.
#include "common/haptic_report.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace procon2droid;

TEST_CASE("Report is exactly 64 bytes", "[haptic_report]")
{
    auto report = build_haptic_report(0, 0, 0, false);
    REQUIRE(report.size() == kHapticReportSize);
}

TEST_CASE("Command ID byte is 0x02", "[haptic_report]")
{
    auto report = build_haptic_report(0, 0, 0, false);
    REQUIRE(report[0] == kHapticCommandId);
}

TEST_CASE("Counter byte is embedded at positions 1 and 17", "[haptic_report]")
{
    auto report = build_haptic_report(0, 0, 5, false);
    uint8_t expected = 0x50 | 0x05;
    REQUIRE(report[1] == expected);
    REQUIRE(report[17] == expected);
}

TEST_CASE("Counter wraps at 4 bits", "[haptic_report]")
{
    auto report15 = build_haptic_report(0, 0, 0x0F, false);
    uint8_t expected15 = 0x50 | 0x0F;
    REQUIRE(report15[1] == expected15);
}

TEST_CASE("OFF state produces zero patterns", "[haptic_report]")
{
    auto report = build_haptic_report(0, 0, 0, false);
    for (size_t i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == 0x00);
        REQUIRE(report[18 + i] == 0x00);
    }
}

TEST_CASE("Active with strong_magnitude 65535 produces strong pattern on left", "[haptic_report]")
{
    auto report = build_haptic_report(65535, 0, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternStrong[i]);
        REQUIRE(report[18 + i] == kHapticPatternOff[i]);
    }
}

TEST_CASE("Active with weak_magnitude 65535 produces strong pattern on right", "[haptic_report]")
{
    auto report = build_haptic_report(0, 65535, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternOff[i]);
        REQUIRE(report[18 + i] == kHapticPatternStrong[i]);
    }
}

TEST_CASE("Active with strong_magnitude 32768 produces weak pattern on left", "[haptic_report]")
{
    auto report = build_haptic_report(32768, 0, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternWeak[i]);
        REQUIRE(report[18 + i] == kHapticPatternOff[i]);
    }
}

TEST_CASE("Active with strong_magnitude 32769 produces strong pattern on left", "[haptic_report]")
{
    auto report = build_haptic_report(32769, 0, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternStrong[i]);
        REQUIRE(report[18 + i] == kHapticPatternOff[i]);
    }
}

TEST_CASE("Both channels active with strong", "[haptic_report]")
{
    auto report = build_haptic_report(65535, 65535, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternStrong[i]);
        REQUIRE(report[18 + i] == kHapticPatternStrong[i]);
    }
}

TEST_CASE("Active with magnitudes but missing effect falls back to OFF", "[haptic_report]")
{
    // active=true but missing effect → looks up effect, finds none → OFF
    auto report = build_haptic_report(65535, 65535, 0, false);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternOff[i]);
        REQUIRE(report[18 + i] == kHapticPatternOff[i]);
    }
}

TEST_CASE("Zero magnitude produces OFF even when active", "[haptic_report]")
{
    auto report = build_haptic_report(0, 0, 0, true);
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(report[2 + i] == kHapticPatternOff[i]);
        REQUIRE(report[18 + i] == kHapticPatternOff[i]);
    }
}
