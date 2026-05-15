// Persistent input proxy: clones the controller's input device to a virtual
// uinput node, forwarding events and translating EV_FF rumble into HID haptic
// reports sent to the hidraw device.
#pragma once

#include <cstdint>
#include <linux/input.h>
#include <linux/uinput.h>
#include <map>
#include <string>

namespace procon2droid
{

class UInputProxy
{
public:
    UInputProxy(std::string event_path, std::string hidraw_path);
    ~UInputProxy();

    UInputProxy(const UInputProxy&) = delete;
    UInputProxy& operator=(const UInputProxy&) = delete;

    [[nodiscard]] bool init();
    void run();

private:
    void clone_device_capabilities();
    void handle_ff_event(const struct input_event& ev);

    std::string event_path_;
    std::string hidraw_path_;
    int event_fd_ = -1;
    int hid_fd_ = -1;
    int uinput_fd_ = -1;
    std::map<int, struct ff_effect> rumble_effects_;
};

} // namespace procon2droid
