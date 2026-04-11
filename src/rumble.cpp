#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <cstring>
#include <poll.h>
#include <map>
#include <dirent.h>
#include <fstream>
#include <string>

// haptic patterns
const uint8_t HAPTIC_PATTERN_STRONG[5] = {0x93, 0x35, 0x36, 0x1c, 0x0d};
const uint8_t HAPTIC_PATTERN_WEAK[5]   = {0x4b, 0x7d, 0x80, 0x5a, 0x02};
const uint8_t HAPTIC_PATTERN_OFF[5]    = {0x00, 0x00, 0x00, 0x00, 0x00};

// Use hex literals for easier reading
const uint32_t TARGET_VID = 0x057e;
const uint32_t TARGET_PID = 0x2069;

std::string find_nintendo_event() {
    DIR* dir = opendir("/sys/class/input/");
    if (!dir) return "";
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find("event") == 0) { 
            std::string base_path = "/sys/class/input/" + name + "/device/id/";
            std::ifstream vid_file(base_path + "vendor");
            std::ifstream pid_file(base_path + "product");
            
            std::string vid_str, pid_str;
            if (vid_file >> vid_str && pid_file >> pid_str) {
                try {
                    uint32_t vid = std::stoul(vid_str, nullptr, 16);
                    uint32_t pid = std::stoul(pid_str, nullptr, 16);

                    if (vid == TARGET_VID && pid == TARGET_PID) {
                        closedir(dir);
                        return "/dev/input/" + name;
                    }
                } catch (...) { continue; }
            }
        }
    }
    closedir(dir);
    return "";
}

std::string find_nintendo_hidraw() {
    DIR* dir = opendir("/sys/class/hidraw/");
    if (!dir) return "";
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find("hidraw") == 0) {
            std::ifstream file("/sys/class/hidraw/" + name + "/device/uevent");
            std::string line;
            while (std::getline(file, line)) {
                // HID_ID lines look like: HID_ID=0003:0000057E:00002069
                if (line.find("HID_ID=") != std::string::npos) {
                    if ((line.find("057E") != std::string::npos || line.find("057e") != std::string::npos) &&
                        (line.find("2069") != std::string::npos)) {
                        closedir(dir);
                        return "/dev/" + name;
                    }
                }
            }
        }
    }
    closedir(dir);
    return "";
}

int main() {
    std::cout << "Searching for Nintendo Switch 2 Pro Controller..." << std::endl;
    
    std::string event_path = find_nintendo_event();
    std::string hidraw_path = find_nintendo_hidraw();

    if (event_path.empty() || hidraw_path.empty()) {
        std::cerr << "Error: Could not find the controller automatically." << std::endl;
        std::cerr << "Is it plugged in/connected? Are you running as root/su?" << std::endl;
        return 1;
    }

    std::cout << "Found Input Node: " << event_path << std::endl;
    std::cout << "Found Hidraw Node: " << hidraw_path << std::endl;

    int event_fd = open(event_path.c_str(), O_RDONLY | O_NONBLOCK);
    int hid_fd = open(hidraw_path.c_str(), O_WRONLY | O_NONBLOCK);
    int u_fd = open("/dev/uinput", O_RDWR | O_NONBLOCK);

    if (event_fd < 0 || hid_fd < 0 || u_fd < 0) {
        std::cerr << "Failed to open device nodes. Need root." << std::endl;
        return 1;
    }

    if (ioctl(event_fd, EVIOCGRAB, 1) < 0) {
        std::cerr << "Warning: Could not grab exclusive access to original device." << std::endl;
    }

    struct input_id original_id;
    ioctl(event_fd, EVIOCGID, &original_id);
    char dev_name[256] = {0};
    ioctl(event_fd, EVIOCGNAME(sizeof(dev_name)), dev_name);

    ioctl(u_fd, UI_SET_EVBIT, EV_SYN);
    ioctl(u_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(u_fd, UI_SET_EVBIT, EV_ABS);

    // clone all advertised keys/buttons
    uint8_t key_bits[KEY_MAX / 8 + 1] = {0};
    ioctl(event_fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
    for (int i = 0; i < KEY_MAX; i++) {
        if (key_bits[i / 8] & (1 << (i % 8))) {
            ioctl(u_fd, UI_SET_KEYBIT, i);
        }
    }

    // clone all advertised absolute axes
    uint8_t abs_bits[ABS_MAX / 8 + 1] = {0};
    ioctl(event_fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
    for (int i = 0; i < ABS_MAX; i++) {
        if (abs_bits[i / 8] & (1 << (i % 8))) {
            ioctl(u_fd, UI_SET_ABSBIT, i);
            struct input_absinfo absinfo;
            if (ioctl(event_fd, EVIOCGABS(i), &absinfo) >= 0) {
                struct uinput_abs_setup abs_setup = {0};
                abs_setup.code = i;
                abs_setup.absinfo = absinfo;
                ioctl(u_fd, UI_ABS_SETUP, &abs_setup);
            }
        }
    }

    // inject force feedback capability
    ioctl(u_fd, UI_SET_EVBIT, EV_FF);
    ioctl(u_fd, UI_SET_FFBIT, FF_RUMBLE);

    struct uinput_setup usetup = {0};
    usetup.id = original_id; 
    usetup.ff_effects_max = 64;
    snprintf(usetup.name, sizeof(usetup.name), "%s with Rumble", dev_name);

    ioctl(u_fd, UI_DEV_SETUP, &usetup);
    ioctl(u_fd, UI_DEV_CREATE);

    struct pollfd fds[2];
    fds[0].fd = event_fd;
    fds[0].events = POLLIN;
    fds[1].fd = u_fd;
    fds[1].events = POLLIN;

    struct input_event ev;
    std::map<int, struct ff_effect> rumble_effects;
    uint8_t haptic_counter = 0;

    std::cout << "Proxying inputs and listening for rumble..." << std::endl;

    while (poll(fds, 2, -1) > 0) {
        // input forwaring
        if (fds[0].revents & POLLIN) {
            while (read(event_fd, &ev, sizeof(ev)) > 0) {
                write(u_fd, &ev, sizeof(ev)); // dump exactly what was read
            }
        }

        // rumble interception and translation
        if (fds[1].revents & POLLIN) {
            while (read(u_fd, &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_UINPUT) {
                    if (ev.code == UI_FF_UPLOAD) {
                        struct uinput_ff_upload upload;
                        memset(&upload, 0, sizeof(upload));
                        upload.request_id = ev.value;
                        if (ioctl(u_fd, UI_BEGIN_FF_UPLOAD, &upload) == 0) {
                            rumble_effects[upload.effect.id] = upload.effect;
                            upload.retval = 0;
                            ioctl(u_fd, UI_END_FF_UPLOAD, &upload);
                        }
                    } else if (ev.code == UI_FF_ERASE) {
                        struct uinput_ff_erase erase;
                        memset(&erase, 0, sizeof(erase));
                        erase.request_id = ev.value;
                        if (ioctl(u_fd, UI_BEGIN_FF_ERASE, &erase) == 0) {
                            rumble_effects.erase(erase.effect_id);
                            erase.retval = 0;
                            ioctl(u_fd, UI_END_FF_ERASE, &erase);
                        }
                    }
                } 
                else if (ev.type == EV_FF) {
                    int effect_id = ev.code;
                    int play_count = ev.value;
                    
                    unsigned char r[64] = {0}; 
                    r[0] = 0x02; // restored your original command ID
                    r[1] = 0x50 | (haptic_counter & 0x0F);
                    r[17] = r[1]; // restored your original mirror byte

                    const uint8_t* l_pattern = HAPTIC_PATTERN_OFF;
                    const uint8_t* r_pattern = HAPTIC_PATTERN_OFF;

                    if (play_count > 0 && rumble_effects.count(effect_id)) {
                        uint16_t strong = rumble_effects[effect_id].u.rumble.strong_magnitude;
                        uint16_t weak   = rumble_effects[effect_id].u.rumble.weak_magnitude;

                        if (strong > 32768) l_pattern = HAPTIC_PATTERN_STRONG;
                        else if (strong > 0) l_pattern = HAPTIC_PATTERN_WEAK;

                        if (weak > 32768) r_pattern = HAPTIC_PATTERN_STRONG;
                        else if (weak > 0) r_pattern = HAPTIC_PATTERN_WEAK;
                    }

                    for(int i = 0; i < 5; i++) {
                        r[2 + i] = l_pattern[i];   
                        r[18 + i] = r_pattern[i]; // restored your correct memory offset
                    }

                    write(hid_fd, r, 64);
                    haptic_counter = (haptic_counter + 1) & 0x0F;
                }
            }
        }
    }

    ioctl(event_fd, EVIOCGRAB, 0); 
    ioctl(u_fd, UI_DEV_DESTROY);
    close(event_fd);
    close(hid_fd);
    close(u_fd);
    return 0;
}
