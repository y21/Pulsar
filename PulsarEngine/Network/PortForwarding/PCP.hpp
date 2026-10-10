#ifndef _PCP_
#define _PCP_

namespace Pulsar {
namespace Network {
namespace PortForwarding {
namespace PCP {

/**
 * Sends a port mapping request to a potentially PCP-enabled device with a given internal- and external port.
 * Internal port must be specified, external port can be left as zero to let the router decide.
 */
bool SendMapRequest(s32 fd, u32 deviceIp, u16 internalPort, u16 externalPort, u32 clientIp);

}  // namespace PCP
}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar

#endif
