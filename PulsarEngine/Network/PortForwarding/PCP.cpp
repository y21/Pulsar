#include <core/rvl/OS/OS.hpp>
#include <core/rvl/SO/SO.hpp>
#include <Network/PortForwarding/UPnP.hpp>

namespace Pulsar {
namespace Network {
namespace PortForwarding {
namespace PCP {

// PCP Map Request Offsets
const s32 kMapRequestVersionOff = 0;
const s32 kMapRBitOpcodeOff = 1;
const s32 kMapRequestLifetimeOff = 4;
const s32 kMapRequestClientIpOff = 8;
const s32 kMapRequestNonceOff = 24;
const s32 kMapRequestProtocolOff = 36;
const s32 kMapRequestInternalPortOff = 40;
const s32 kMapRequestExternalPortOff = 42;
const s32 kMapRequestExternalIpOff = 44;

// PCP Map Response Offsets
const s32 kMapResponseVersionOff = 0;
const s32 kMapResponseResultOff = 3;
const s32 kMapResponseLifetimeOff = 4;
const s32 kMapResponseExternalPortOff = 42;

// PCP MAP Constants
const s32 kMapVersion = 2;
const s32 kMapOpcode = 1;
const s32 kMapProtocolUdp = 17;
const s32 kMapLifetime = 360;  // TODO: change this to something bigger later!!!

const u16 kPcpPort = 5351;
const OS::Time kSocketTimeout = OS::GetTimerClock() * 5;

void WriteU32BE(u8 *buffer, u32 value) {
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
}

u32 ReadU32BE(u8 *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

u16 ReadU16BE(u8 *buffer) {
    return (buffer[0] << 8) | buffer[1];
}

void WriteU16BE(u8 *buffer, u16 value) {
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
}

void WriteIpv4Mapped(u8 *buffer, u32 ipv4) {
    memset(buffer, 0, 16);
    buffer[10] = 0xff;
    buffer[11] = 0xff;
    WriteU32BE(buffer + 12, ipv4);
}

bool SendMapRequest(s32 fd, u32 deviceIp, u16 internalPort, u16 externalPort, u32 clientIp) {
    u8 request[60] = {0};

    // Header (https://datatracker.ietf.org/doc/html/rfc6887#section-7.1)
    request[kMapRequestVersionOff] = kMapVersion;
    request[kMapRBitOpcodeOff] = kMapOpcode;
    WriteU32BE(request + kMapRequestLifetimeOff, kMapLifetime);
    WriteIpv4Mapped(request + kMapRequestClientIpOff, clientIp);

    // MAP specific fields (https://datatracker.ietf.org/doc/html/rfc6887#section-11.1)
    request[kMapRequestNonceOff] = 0;  // TODO: set a random nonce
    request[kMapRequestProtocolOff] = kMapProtocolUdp;
    WriteU16BE(request + kMapRequestInternalPortOff, internalPort);
    WriteU16BE(request + kMapRequestExternalPortOff, externalPort);
    WriteIpv4Mapped(request + kMapRequestExternalIpOff, 0);  // Let the router figure out the external IP

    SO::SockAddrIn pcpAddr = {
        .length = sizeof(SO::SockAddrIn),
        .family = SO::PF_INET,
        .port = kPcpPort,
        .address = deviceIp,
    };

    s32 written = SO::SOSendTo(fd, request, sizeof(request), 0, &pcpAddr);
    OS::Report("[PCP] Sent MAP request, bytes written: %d\n", written);
    if (written != sizeof(request)) {
        OS::Report("[PCP] Failed to send complete MAP request\n");
        return false;
    }

    SO::PollFD pollFd = {.fd = fd, .events = SO::POLLIN, .revents = 0};

    OS::Report("[PCP DEBUG] Polling with timeout %lld\n", (s64)(kSocketTimeout));
    int readyFds = SO::SOPoll(&pollFd, 1, kSocketTimeout);
    OS::Report("[PCP DEBUG] Ready FDs: %d\n", readyFds);

    if (readyFds <= 0) {
        OS::Report("[PCP] MAP request timed out or an error occurred (%d)\n", readyFds);
        return false;
    }

    u8 response[60] = {0};
    u32 read = SO::SORecvFrom(fd, response, sizeof(response), 0, nullptr);
    if (read != sizeof(response)) {
        OS::Report("[PCP] Failed to receive complete MAP response\n");
        return false;
    }

    u8 version = response[kMapResponseVersionOff];
    u8 result = response[kMapResponseResultOff];
    u32 lifetime = ReadU32BE(response + kMapResponseLifetimeOff);
    u16 routerExternalPort = ReadU16BE(response + kMapResponseExternalPortOff);
    OS::Report("[PCP] MAP response version: %d, result: %d, lifetime: %d, external port: %d\n", version, result, lifetime, routerExternalPort);
    if (version != kMapVersion) {
        OS::Report("[PCP] Unexpected MAP response version: %d\n", version);
        return false;
    }
    if (result != 0) {
        OS::Report("[PCP] MAP response failure, result: %d\n", result);
        return false;
    }

    return true;
}

}  // namespace PCP
}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar
