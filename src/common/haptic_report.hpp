// Builds the 64-byte HID haptic report from FF_RUMBLE magnitudes.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace procon2droid
{

inline constexpr size_t kHapticReportSize = 64;
inline constexpr uint8_t kHapticCommandId = 0x02;

inline constexpr std::array<uint8_t, 5> kHapticPatternStrong{0x93, 0x35, 0x36, 0x1c, 0x0d};
inline constexpr std::array<uint8_t, 5> kHapticPatternWeak{0x4b, 0x7d, 0x80, 0x5a, 0x02};
inline constexpr std::array<uint8_t, 5> kHapticPatternOff{0x00, 0x00, 0x00, 0x00, 0x00};

[[nodiscard]] std::array<uint8_t, kHapticReportSize>
build_haptic_report(uint16_t strong_magnitude, uint16_t weak_magnitude, uint8_t counter, bool active);

} // namespace procon2droid
