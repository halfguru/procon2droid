#include "device_finder.hpp"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>

namespace procon2droid
{

namespace
{

bool matches_target_vidpid(std::string_view vid, std::string_view pid)
{
    return vid == kTargetVid && pid == kTargetPid;
}

using DirPtr = std::unique_ptr<DIR, int (*)(DIR*)>;

inline constexpr std::string_view kLogTag = "device_finder";
inline constexpr std::string_view kEventPrefix = "event";
inline constexpr std::string_view kHidrawPrefix = "hidraw";
inline constexpr std::string_view kHidIdKey = "HID_ID=";

} // namespace

std::optional<std::string> find_usb_device()
{
    DIR* dp = opendir("/sys/bus/usb/devices");
    if (!dp)
    {
        spdlog::error("{}: cannot open /sys/bus/usb/devices", kLogTag);
        return std::nullopt;
    }

    DirPtr dp_guard{dp, closedir};
    const struct dirent* entry = nullptr;

    while ((entry = readdir(dp_guard.get())))
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        const std::string dir = std::string("/sys/bus/usb/devices/") + entry->d_name;
        std::ifstream vid_file(dir + "/idVendor");
        std::ifstream pid_file(dir + "/idProduct");
        std::string vid;
        std::string pid;

        if (!(vid_file >> vid && pid_file >> pid))
        {
            continue;
        }
        if (!matches_target_vidpid(vid, pid))
        {
            continue;
        }

        std::ifstream bus_file(dir + "/busnum");
        std::ifstream dev_file(dir + "/devnum");
        int busnum = 0;
        int devnum = 0;

        if (bus_file >> busnum && dev_file >> devnum)
        {
            char path[64];
            snprintf(path, sizeof(path), "/dev/bus/usb/%03d/%03d", busnum, devnum);
            spdlog::info("{}: found USB device at {}", kLogTag, path);
            return std::string(path);
        }
    }

    return std::nullopt;
}

std::optional<std::string> find_input_event()
{
    DIR* dir = opendir("/sys/class/input/");
    if (!dir)
    {
        spdlog::error("{}: cannot open /sys/class/input/", kLogTag);
        return std::nullopt;
    }

    DirPtr dir_guard{dir, closedir};
    const struct dirent* entry = nullptr;

    while ((entry = readdir(dir_guard.get())) != nullptr)
    {
        const std::string name = entry->d_name;
        if (!name.starts_with(kEventPrefix))
        {
            continue;
        }

        const std::string base = "/sys/class/input/" + name + "/device/id/";
        std::ifstream vid_file(base + "vendor");
        std::ifstream pid_file(base + "product");
        std::string vid_str;
        std::string pid_str;

        if (!(vid_file >> vid_str && pid_file >> pid_str))
        {
            continue;
        }

        try
        {
            const uint32_t vid = std::stoul(vid_str, nullptr, 16);
            const uint32_t pid = std::stoul(pid_str, nullptr, 16);

            if (vid == kTargetVidHex && pid == kTargetPidHex)
            {
                const std::string path = "/dev/input/" + name;
                spdlog::info("{}: found input at {}", kLogTag, path);
                return path;
            }
        }
        catch (...)
        {
            continue;
        }
    }

    return std::nullopt;
}

std::optional<std::string> find_hidraw_device()
{
    DIR* dir = opendir("/sys/class/hidraw/");
    if (!dir)
    {
        spdlog::error("{}: cannot open /sys/class/hidraw/", kLogTag);
        return std::nullopt;
    }

    DirPtr dir_guard{dir, closedir};
    const struct dirent* entry = nullptr;

    while ((entry = readdir(dir_guard.get())) != nullptr)
    {
        const std::string name = entry->d_name;
        if (!name.starts_with(kHidrawPrefix))
        {
            continue;
        }

        std::ifstream file("/sys/class/hidraw/" + name + "/device/uevent");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find(kHidIdKey) == std::string::npos)
            {
                continue;
            }

            // HID_ID uevent lines may use uppercase hex; do case-insensitive check.
            std::string upper_line = line;
            std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(), ::toupper);
            const bool vid_match = upper_line.find(kTargetVid) != std::string::npos;
            const bool pid_match = upper_line.find(kTargetPid) != std::string::npos;

            if (vid_match && pid_match)
            {
                const std::string path = "/dev/" + name;
                spdlog::info("device_finder: found hidraw at {}", path);
                return path;
            }
        }
    }

    return std::nullopt;
}

} // namespace procon2droid
