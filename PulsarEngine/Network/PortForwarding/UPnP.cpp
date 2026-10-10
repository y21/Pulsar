#include <core/rvl/SO/SO.hpp>

#include <Network/PortForwarding/UPnP.hpp>

namespace Pulsar {
namespace Network {
namespace PortForwarding {
namespace UPnP {

const SO::SockAddrIn kSsdpAddr = {
    .length = sizeof(SO::SockAddrIn),
    .family = SO::PF_INET,
    .port = 1900,
    .address = (239 << 24) | (255 << 16) | (255 << 8) | 250,
};

const char kSsdpMessage[] =
  "M-SEARCH * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "MAN: \"ssdp:discover\"\r\n"
  "MX: 2\r\n"
  "ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n";

bool ParseSsdpResponse(const char *buf, u32 bufSize, Device &device) {
    const char *kLocationHeader = "Location:";
    const char *locationStart = strstr(buf, kLocationHeader);
    if (!locationStart) {
        return false;
    }

    // Skip any whitespace after "Location:"
    while (*locationStart == ' ') locationStart++;

    const char *locationEnd = strstr(locationStart, "\r\n");
    if (!locationEnd) {
        return false;
    }

    s32 locationLength = locationEnd - locationStart;
    if (locationLength >= sizeof(device.location)) {
        return false;
    }
    memcpy(device.location, locationStart, locationLength);
    device.location[locationLength] = '\0';
    OS::Report("[UPnP] Parsed Location header: %s\n", device.location);

    // Extract ip+port from the Location URL (may be something like 'http://192.168.1.1:1234/path', extract ip=192.168.1.1, port=1234)

    const char *ipCursor = strstr(device.location, "://");
    if (!ipCursor) {
        return false;
    }
    ipCursor += 3;  // Skip "://"

    // Find the end of the IP address:
    // - Look for the next ':' (port separator)
    // - Or look for a '/' (no port at all)
    // - Or if neither is found, the IP address goes until the end of the string
    const char *portSep = strchr(ipCursor, ':');
    const char *pathSep = strchr(ipCursor, '/');
    const char *ipEnd = nullptr;
    if (portSep) {
        ipEnd = portSep;
    } else if (pathSep) {
        ipEnd = pathSep;
    } else {
        ipEnd = ipCursor + strlen(ipCursor);
    }

    s32 octetIndex = 0;
    while (octetIndex < 4) {  // TODO: do we need to check for out of bounds access here?
        char *end = nullptr;

        u8 octet = (u8)strtoul(ipCursor, &end, 10);
        device.ip |= octet << ((3 - octetIndex) * 8);

        OS::Report("[UPnP] Parsed IP octet: %u\n", octet);
        if (*end == '.') {
            ipCursor = end + 1;  // Skip the dot
            octetIndex++;
        } else {
            break;
        }
    }

    if (octetIndex != 3) {
        OS::Report("[UPnP] Failed to parse complete IP address\n");
        return false;
    }
    OS::Report("[UPnP] Parsed complete IP address: %u.%u.%u.%u\n", (device.ip >> 24) & 0xFF, (device.ip >> 16) & 0xFF, (device.ip >> 8) & 0xFF, device.ip & 0xFF);

    if (portSep) {
        device.port = (u16)strtoul(portSep + 1, nullptr, 10);
        OS::Report("[UPnP] Parsed port: %u\n", device.port);
    }

    return true;
}

bool FindDevice(s32 sockfd, Device &device) {
    memset(&device, 0, sizeof(Device));

    s32 written = SO::SOSendTo(sockfd, kSsdpMessage, sizeof(kSsdpMessage), 0, &kSsdpAddr);
    OS::Report("[UDP DEBUG] Sent %d bytes (expect: %d)\n", written, sizeof(kSsdpMessage));
    if (written < 0) {
        OS::Report("[UPnP] Failed to send SSDP message\n");
        return false;
    }

    char buf[1024];
    s32 read = SO::SORecvFrom(sockfd, buf, sizeof(buf), 0, nullptr);
    // TODO: must be polling since this could block indefinitely if no upnp support!!!
    // TODO: think about what to do if we get multiple responses? try parsing each one until a valid device is found?
    OS::Report("[UDP DEBUG] Received %d bytes\n", read);
    if (read < 0) {
        OS::Report("[UPnP] Failed to receive SSDP response\n");
        return false;
    }

    if (read > sizeof(buf)) {
        // TODO: should just never really happen?
        OS::Report("[UPnP] Received more bytes than buffer size (buffer overflow, %d > %d)\n", read, sizeof(buf));
        return false;
    }
    buf[read] = '\0';
    OS::Report("[UDP DEBUG] Received SSDP response:\n%s\n", buf);

    if (!ParseSsdpResponse(buf, read, device)) {
        OS::Report("[UPnP] Failed to parse Location header\n");
        return false;
    }

    OS::Report("[UPnP] Found device at IP: %u.%u.%u.%u, port: %u\n", (device.ip >> 24) & 0xFF, (device.ip >> 16) & 0xFF, (device.ip >> 8) & 0xFF, device.ip & 0xFF, device.port);

    return true;
}

}  // namespace UPnP
}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar
