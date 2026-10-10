#ifndef _UPNP_
#define _UPNP_

#include <kamek.hpp>

namespace Pulsar {
namespace Network {
namespace PortForwarding {
namespace UPnP {

struct Device {
    char location[128];
    u32 ip;
    u16 port;
};

/**
 * Sends a SSDP discovery request in the local network to find UPnP-enabled devices (usually the router).
 */
bool FindDevice(s32 sockfd, Device &device);

}  // namespace UPnP
}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar

#endif
