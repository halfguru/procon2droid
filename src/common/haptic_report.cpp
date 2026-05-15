#include "haptic_report.hpp"

#include <algorithm>

namespace procon2droid
{

[[nodiscard]] std::array<uint8_t, kHapticReportSize>
build_haptic_report(uint16_t strong_magnitude, uint16_t weak_magnitude, uint8_t counter, bool active)
{
    std::array<uint8_t, kHapticReportSize> report{};
    report[0] = kHapticCommandId;

    const uint8_t seq_byte = 0x50 | (counter & 0x0F);
    report[1] = seq_byte;
    report[17] = seq_byte;

    auto l_pattern = kHapticPatternOff;
    auto r_pattern = kHapticPatternOff;

    if (active)
    {
        if (strong_magnitude > 32768)
        {
            l_pattern = kHapticPatternStrong;
        }
        else if (strong_magnitude > 0)
        {
            l_pattern = kHapticPatternWeak;
        }

        if (weak_magnitude > 32768)
        {
            r_pattern = kHapticPatternStrong;
        }
        else if (weak_magnitude > 0)
        {
            r_pattern = kHapticPatternWeak;
        }
    }

    std::copy(l_pattern.begin(), l_pattern.end(), report.begin() + 2);
    std::copy(r_pattern.begin(), r_pattern.end(), report.begin() + 18);

    return report;
}

} // namespace procon2droid
