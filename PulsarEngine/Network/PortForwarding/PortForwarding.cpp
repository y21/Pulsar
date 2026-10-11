#include <kamek.hpp>
#include <core/rvl/SO/SO.hpp>

#include <Network/PortForwarding/PCP.hpp>
#include <Network/PortForwarding/UPnP.hpp>

namespace Pulsar {
namespace Network {
namespace PortForwarding {

extern "C" bool SCGetProductSN(u32 *);

void Init(void *param) {
    u32 sn;
    if (!SCGetProductSN(&sn)) {
        OS::Report("[PortForwarding] Failed to get serial number\n");
        return;
    }
    u16 natnegPort = 22000 + (sn % 1000);

    s32 fd = SO::SOSocket(SO::PF_INET, SO::SOCK_DGRAM, 0);
    if (fd < 0) {
        // TODO: is fd < 0 for errors correct?
        OS::Report("[PortForwarding] Failed to create socket\n");
        return;
    }

    u32 clientIp = SO::SOGetHostID();
    OS::Report("[PortForwarding] Client IP: %d.%d.%d.%d\n", (clientIp >> 24) & 0xFF, (clientIp >> 16) & 0xFF, (clientIp >> 8) & 0xFF, clientIp & 0xFF);

    UPnP::Device device;
    bool upnpAvailable = UPnP::FindDevice(fd, device);

    if (upnpAvailable) {
        if (PCP::SendMapRequest(fd, device.ip, natnegPort, 0, clientIp)) {
            OS::Report("[PortForwarding] PCP request using UPnP IP succeeded\n");
        } else {
            OS::Report("[PortForwarding] PCP request using UPnP IP failed\n");
        }

        // TODO: fallback to normal UPnP
    } else {
        // TODO: try to send a PCP request to known fallback router IPs.
        OS::Report("[PortForwarding] Failed to discover UPnP device\n");
    }
}

}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar
