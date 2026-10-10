#ifndef _PORT_FORWARDING_
#define _PORT_FORWARDING_

namespace Pulsar {
namespace Network {
namespace PortForwarding {

/**
* Sets up automatic port forwarding for the 22xxx NATNEG port
*/
void Init(void *);

}  // namespace PortForwarding
}  // namespace Network
}  // namespace Pulsar

#endif
