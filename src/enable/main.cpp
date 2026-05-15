// One-shot USB init binary: claims interface 1, sends the 17-packet init
// sequence, then releases the interface so Android HID can take over.
#include "../common/device_finder.hpp"
#include "../common/init_sequence.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace
{

int find_bulk_out_endpoint(int fd, const uint8_t* probe_data, size_t probe_len)
{
    for (int ep = 1; ep <= 5; ++ep)
    {
        struct usbdevfs_bulktransfer bt{};
        bt.ep = static_cast<unsigned int>(ep);
        bt.len = static_cast<unsigned int>(probe_len);
        bt.timeout = 1000;
        bt.data = const_cast<uint8_t*>(probe_data); // NOLINT(cppcoreguidelines-pro-type-const-cast)

        if (ioctl(fd, USBDEVFS_BULK, &bt) >= 0)
        {
            return ep;
        }
    }
    return -1;
}

void send_init_sequence(int fd, int bulk_out_ep)
{
    const unsigned int in_ep = static_cast<unsigned int>(bulk_out_ep) | 0x80;

    for (size_t i = 1; i < procon2droid::kInitSequenceLength; ++i)
    {
        const auto& pkt = procon2droid::kInitSequence[i];

        struct usbdevfs_bulktransfer bulk_out{};
        bulk_out.ep = static_cast<unsigned int>(bulk_out_ep);
        bulk_out.len = static_cast<unsigned int>(pkt.data.size());
        bulk_out.timeout = 1000;
        bulk_out.data = const_cast<uint8_t*>(pkt.data.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)

        ioctl(fd, USBDEVFS_BULK, &bulk_out);
        usleep(10000);

        unsigned char dummy[32]{};
        struct usbdevfs_bulktransfer bulk_in{};
        bulk_in.ep = in_ep;
        bulk_in.len = sizeof(dummy);
        bulk_in.timeout = 20;
        bulk_in.data = dummy;

        ioctl(fd, USBDEVFS_BULK, &bulk_in);
    }
}

} // namespace

int main()
{
    using namespace procon2droid;

    spdlog::info("Locating the Controller");

    auto usb_path = find_usb_device();
    if (!usb_path)
    {
        spdlog::error("Controller not found. Is it plugged in?");
        return EXIT_FAILURE;
    }

    const int fd = open(usb_path->c_str(), O_RDWR);
    if (fd < 0)
    {
        spdlog::error("Could not open USB device. Are you running as root/su?");
        return EXIT_FAILURE;
    }

    struct usbdevfs_ioctl disconnect_cmd{};
    disconnect_cmd.ifno = 1;
    disconnect_cmd.ioctl_code = USBDEVFS_DISCONNECT;
    ioctl(fd, USBDEVFS_IOCTL, &disconnect_cmd);

    int iface = 1;
    if (ioctl(fd, USBDEVFS_CLAIMINTERFACE, &iface) < 0)
    {
        spdlog::error("Failed to claim USB Interface 1");
        close(fd);
        return EXIT_FAILURE;
    }

    spdlog::info("Waking up controller...");

    const int bulk_ep = find_bulk_out_endpoint(fd, kInitSequence[0].data.data(), kInitSequence[0].data.size());
    if (bulk_ep < 0)
    {
        spdlog::error("Could not find valid bulk endpoint");
        ioctl(fd, USBDEVFS_RELEASEINTERFACE, &iface);
        close(fd);
        return EXIT_FAILURE;
    }

    send_init_sequence(fd, bulk_ep);

    ioctl(fd, USBDEVFS_RELEASEINTERFACE, &iface);
    close(fd);

    spdlog::info("Success! Controller is awake. Handing control back to Android.");
    return EXIT_SUCCESS;
}
