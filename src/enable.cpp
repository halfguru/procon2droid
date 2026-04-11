#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>
#include <cstring>
#include <dirent.h>
#include <fstream>

// --- initialization commands as found on https://github.com/HandHeldLegend/handheldlegend.github.io/blob/master/procon2tool/index.html ---
const uint8_t CMD_03[] = {0x03, 0x91, 0x00, 0x0d, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t CMD_07[] = {0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_16[] = {0x16, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_MAC[] = {0x15, 0x91, 0x00, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t CMD_LTK[] = {0x15, 0x91, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t CMD_15_03[] = {0x15, 0x91, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00};
const uint8_t CMD_09[] = {0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_IMU_02[] = {0x0c, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
const uint8_t CMD_11[] = {0x11, 0x91, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_0A[] = {0x0a, 0x91, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_IMU_04[] = {0x0c, 0x91, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
const uint8_t CMD_HAPTICS[] = {0x03, 0x91, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00};
const uint8_t CMD_10[] = {0x10, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_01[] = {0x01, 0x91, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00};
const uint8_t CMD_03_ALT[] = {0x03, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00};
const uint8_t CMD_0A_ALT[] = {0x0a, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00};
const uint8_t CMD_LED[] = {0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct InitPacket { const uint8_t* data; size_t len; };
InitPacket init_sequence[] = {
    {CMD_03, sizeof(CMD_03)}, {CMD_07, sizeof(CMD_07)}, {CMD_16, sizeof(CMD_16)}, 
    {CMD_MAC, sizeof(CMD_MAC)}, {CMD_LTK, sizeof(CMD_LTK)}, {CMD_15_03, sizeof(CMD_15_03)}, 
    {CMD_09, sizeof(CMD_09)}, {CMD_IMU_02, sizeof(CMD_IMU_02)}, {CMD_11, sizeof(CMD_11)}, 
    {CMD_0A, sizeof(CMD_0A)}, {CMD_IMU_04, sizeof(CMD_IMU_04)}, {CMD_HAPTICS, sizeof(CMD_HAPTICS)}, 
    {CMD_10, sizeof(CMD_10)}, {CMD_01, sizeof(CMD_01)}, {CMD_03_ALT, sizeof(CMD_03_ALT)}, 
    {CMD_0A_ALT, sizeof(CMD_0A_ALT)}, {CMD_LED, sizeof(CMD_LED)}
};

std::string find_raw_usb_device() {
    DIR *dp = opendir("/sys/bus/usb/devices");
    if (!dp) return "";
    struct dirent *entry;
    while ((entry = readdir(dp))) {
        if (entry->d_name[0] == '.') continue;
        std::string dir = std::string("/sys/bus/usb/devices/") + entry->d_name;
        std::ifstream vid_file(dir + "/idVendor"), pid_file(dir + "/idProduct");
        std::string vid, pid;
        if (vid_file >> vid && pid_file >> pid && vid == "057e" && pid == "2069") { // Switch 2 Pro Controller
            std::ifstream bus_file(dir + "/busnum"), dev_file(dir + "/devnum");
            int busnum, devnum;
            if (bus_file >> busnum && dev_file >> devnum) {
                char path[64];
                snprintf(path, sizeof(path), "/dev/bus/usb/%03d/%03d", busnum, devnum);
                closedir(dp);
                return std::string(path);
            }
        }
    }
    closedir(dp);
    return "";
}

int main() {
    std::cout << "Locating the Controller" << std::endl;
    
    std::string usbPath = find_raw_usb_device();
    if (usbPath.empty()) {
        std::cerr << "Error: Controller not found. Is it plugged in?" << std::endl;
        return 1;
    }
    
    int fd = open(usbPath.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Error: Could not open USB device. Are you running as root/su?" << std::endl;
        return 1;
    }

    // Detach kernel driver momentarily from Interface 1
    struct usbdevfs_ioctl disconnect_cmd = {1, USBDEVFS_DISCONNECT, NULL};
    ioctl(fd, USBDEVFS_IOCTL, &disconnect_cmd);

    int iface = 1;
    if (ioctl(fd, USBDEVFS_CLAIMINTERFACE, &iface) < 0) {
        std::cerr << "Error: Failed to claim USB Interface 1." << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "Waking up controller..." << std::endl;

    // Discover correct Bulk OUT endpoint silently
    int target_ep = -1;
    for (int test_ep = 1; test_ep <= 5; test_ep++) {
        struct usbdevfs_bulktransfer bulk_test = {(unsigned int)test_ep, sizeof(CMD_03), 1000, (void*)CMD_03};
        if (ioctl(fd, USBDEVFS_BULK, &bulk_test) >= 0) {
            target_ep = test_ep;
            break; 
        }
    }

    if (target_ep == -1) {
        std::cerr << "Error: Could not find valid bulk endpoint." << std::endl;
        close(fd);
        return 1;
    }

    // Send the rest of the command sequence
    for (size_t i = 1; i < sizeof(init_sequence) / sizeof(init_sequence[0]); i++) {
        struct usbdevfs_bulktransfer bulk = {(unsigned int)target_ep, (unsigned int)init_sequence[i].len, 1000, (void*)init_sequence[i].data};
        ioctl(fd, USBDEVFS_BULK, &bulk);
        usleep(10000); // 10ms wait

        // Dummy read to clear potential ACKs
        unsigned char dummy[32];
        struct usbdevfs_bulktransfer bulk_in = {(unsigned int)(target_ep | 0x80), 32, 20, dummy};
        ioctl(fd, USBDEVFS_BULK, &bulk_in);
    }

    // Clean up and release control back to Android OS
    ioctl(fd, USBDEVFS_RELEASEINTERFACE, &iface);
    close(fd);

    std::cout << "Success! Controller is awake. Handing control back to Android." << std::endl;
    return 0;
}
