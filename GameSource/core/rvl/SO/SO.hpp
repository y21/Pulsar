#ifndef _RVL_SO_
#define _RVL_SO_

#include <types.hpp>
#include <core/rvl/OS/OS.hpp>

namespace SO {

// TODO: consider making a Socket class with a destructor

// RVL SDK uses an eight-byte IPv4 address, not the BSD sixteen-byte layout.
struct SockAddrIn {
    u8 length;
    u8 family;
    u16 port;
    u32 address;
};

struct PollFD {
    s32 fd;
    s32 events;
    s32 revents;
};

enum {
    PF_INET = 2,
    SOCK_DGRAM = 2,
    F_GETFL = 3,
    F_SETFL = 4,
    O_NONBLOCK = 4,
    POLLIN = 1,
    POLLOUT = 8,
    POLLERR = 32,
    POLLHUP = 64,
    POLLNVAL = 128
};

extern "C" {
s32 SOSocket(s32 family, s32 type, s32 protocol);  // 801ecff4
s32 SOClose(s32 socket);  // 801ed0e4
s32 SORecvFrom(s32 socket, void *buffer, s32 length, s32 flags, SockAddrIn *source);  // 801ed454
s32 SOSendTo(s32 socket, const void *buffer, s32 length, s32 flags, const SockAddrIn *destination);  // 801ed4a0
s32 SOFcntl(s32 socket, s32 command, ...);  // 801ed4ec
s32 SOPoll(PollFD *fds, u32 count, OS::Time timeout);  // 801ed6d0 timeout is ticks
u32 SOGetHostID();  // 801ede88
}

}  // namespace SO

#endif
