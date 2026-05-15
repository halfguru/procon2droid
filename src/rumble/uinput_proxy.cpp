#include "uinput_proxy.hpp"

#include "../common/haptic_report.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace procon2droid
{

UInputProxy::UInputProxy(std::string event_path, std::string hidraw_path)
    : event_path_(std::move(event_path)), hidraw_path_(std::move(hidraw_path))
{
}

UInputProxy::~UInputProxy()
{
    if (event_fd_ >= 0)
    {
        ioctl(event_fd_, EVIOCGRAB, 0);
        close(event_fd_);
    }
    if (uinput_fd_ >= 0)
    {
        ioctl(uinput_fd_, UI_DEV_DESTROY);
        close(uinput_fd_);
    }
    if (hid_fd_ >= 0)
    {
        close(hid_fd_);
    }
}

bool UInputProxy::init()
{
    event_fd_ = open(event_path_.c_str(), O_RDONLY | O_NONBLOCK);
    hid_fd_ = open(hidraw_path_.c_str(), O_WRONLY | O_NONBLOCK);
    uinput_fd_ = open("/dev/uinput", O_RDWR | O_NONBLOCK);

    if (event_fd_ < 0 || hid_fd_ < 0 || uinput_fd_ < 0)
    {
        spdlog::error("Failed to open device nodes. Need root.");
        return false;
    }

    if (ioctl(event_fd_, EVIOCGRAB, 1) < 0)
    {
        spdlog::warn("Could not grab exclusive access to original device.");
    }

    struct input_id original_id{};
    ioctl(event_fd_, EVIOCGID, &original_id);
    char dev_name[256]{};
    ioctl(event_fd_, EVIOCGNAME(sizeof(dev_name)), dev_name);

    ioctl(uinput_fd_, UI_SET_EVBIT, EV_SYN);
    ioctl(uinput_fd_, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd_, UI_SET_EVBIT, EV_ABS);

    clone_device_capabilities();

    ioctl(uinput_fd_, UI_SET_EVBIT, EV_FF);
    ioctl(uinput_fd_, UI_SET_FFBIT, FF_RUMBLE);

    struct uinput_setup usetup{};
    usetup.id = original_id;
    usetup.ff_effects_max = 64;
    snprintf(usetup.name, sizeof(usetup.name), "%s with Rumble", dev_name);

    ioctl(uinput_fd_, UI_DEV_SETUP, &usetup);
    ioctl(uinput_fd_, UI_DEV_CREATE);

    return true;
}

void UInputProxy::clone_device_capabilities()
{
    uint8_t key_bits[KEY_MAX / 8 + 1]{};
    ioctl(event_fd_, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    for (int i = 0; i < KEY_MAX; ++i)
    {
        if (key_bits[i / 8] & (1 << (i % 8)))
        {
            ioctl(uinput_fd_, UI_SET_KEYBIT, i);
        }
    }

    uint8_t abs_bits[ABS_MAX / 8 + 1]{};
    ioctl(event_fd_, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    for (int i = 0; i < ABS_MAX; ++i)
    {
        if (abs_bits[i / 8] & (1 << (i % 8)))
        {
            ioctl(uinput_fd_, UI_SET_ABSBIT, i);
            struct input_absinfo absinfo{};
            if (ioctl(event_fd_, EVIOCGABS(i), &absinfo) >= 0)
            {
                struct uinput_abs_setup abs_setup{};
                abs_setup.code = i;
                abs_setup.absinfo = absinfo;
                ioctl(uinput_fd_, UI_ABS_SETUP, &abs_setup);
            }
        }
    }
}

void UInputProxy::handle_ff_event(const struct input_event& ev)
{
    if (ev.type == EV_UINPUT)
    {
        if (ev.code == UI_FF_UPLOAD)
        {
            struct uinput_ff_upload upload{};
            upload.request_id = ev.value;
            if (ioctl(uinput_fd_, UI_BEGIN_FF_UPLOAD, &upload) == 0)
            {
                rumble_effects_[upload.effect.id] = upload.effect;
                upload.retval = 0;
                ioctl(uinput_fd_, UI_END_FF_UPLOAD, &upload);
            }
        }
        else if (ev.code == UI_FF_ERASE)
        {
            struct uinput_ff_erase erase{};
            erase.request_id = ev.value;
            if (ioctl(uinput_fd_, UI_BEGIN_FF_ERASE, &erase) == 0)
            {
                rumble_effects_.erase(erase.effect_id);
                erase.retval = 0;
                ioctl(uinput_fd_, UI_END_FF_ERASE, &erase);
            }
        }
    }
    else if (ev.type == EV_FF)
    {
        const int effect_id = ev.code;
        const int play_count = ev.value;

        const bool active = (play_count > 0);
        uint16_t strong_mag = 0;
        uint16_t weak_mag = 0;

        if (active)
        {
            auto it = rumble_effects_.find(effect_id);
            if (it != rumble_effects_.end())
            {
                strong_mag = it->second.u.rumble.strong_magnitude;
                weak_mag = it->second.u.rumble.weak_magnitude;
            }
        }

        static uint8_t haptic_counter = 0;
        const auto report = build_haptic_report(strong_mag, weak_mag, haptic_counter, active);
        haptic_counter = (haptic_counter + 1) & 0x0F;

        if (write(hid_fd_, report.data(), report.size()) < 0)
        {
            spdlog::warn("Failed to write haptic report to hidraw: {}", std::strerror(errno));
        }
    }
}

void UInputProxy::run()
{
    struct pollfd fds[2]{};
    fds[0].fd = event_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = uinput_fd_;
    fds[1].events = POLLIN;

    spdlog::info("Proxying inputs and listening for rumble...");

    struct input_event ev{};

    while (true)
    {
        const int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            spdlog::error("poll() failed: {}", std::strerror(errno));
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            while (read(event_fd_, &ev, sizeof(ev)) > 0)
            {
                if (write(uinput_fd_, &ev, sizeof(ev)) < 0)
                {
                    spdlog::warn("Failed to forward input event: {}", std::strerror(errno));
                }
            }
        }

        if (fds[1].revents & POLLIN)
        {
            while (read(uinput_fd_, &ev, sizeof(ev)) > 0)
            {
                handle_ff_event(ev);
            }
        }
    }
}

} // namespace procon2droid
