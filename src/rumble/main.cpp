#include "../common/device_finder.hpp"
#include "uinput_proxy.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

int main()
{
    using namespace procon2droid;

    spdlog::info("Searching for Nintendo Switch 2 Pro Controller...");

    const auto event_path = find_input_event();
    const auto hidraw_path = find_hidraw_device();

    if (!event_path || !hidraw_path)
    {
        spdlog::error("Could not find the controller automatically.");
        spdlog::error("Is it plugged in/connected? Are you running as root/su?");
        return EXIT_FAILURE;
    }

    spdlog::info("Found Input Node: {}", *event_path);
    spdlog::info("Found Hidraw Node: {}", *hidraw_path);

    UInputProxy proxy(*event_path, *hidraw_path);

    if (!proxy.init())
    {
        return EXIT_FAILURE;
    }

    proxy.run();
    return EXIT_SUCCESS;
}
