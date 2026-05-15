// Scans sysfs for the Nintendo Switch 2 Pro Controller (VID 057e / PID 2069)
// and returns device paths for USB, input event, and hidraw nodes.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace procon2droid
{

inline constexpr std::string_view kTargetVid = "057e";
inline constexpr std::string_view kTargetPid = "2069";
inline constexpr uint32_t kTargetVidHex = 0x057E;
inline constexpr uint32_t kTargetPidHex = 0x2069;

std::optional<std::string> find_usb_device();

std::optional<std::string> find_input_event();

std::optional<std::string> find_hidraw_device();

} // namespace procon2droid
