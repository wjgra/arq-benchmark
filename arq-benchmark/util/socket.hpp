#ifndef _UTIL_SOCKET_HPP_
#define _UTIL_SOCKET_HPP_

#include <string_view>

#include "util/address_info.hpp"

namespace util {

// Owning wrapper for a valid socket file descriptor.
class Socket {
    using SocketID = int;
public:
    Socket(std::string_view address, std::string_view port, SocketType type);
    Socket(const Socket&) = delete;
    Socket operator=(const Socket&) = delete;
    Socket(const Socket&&) = delete;
    Socket operator=(const Socket&&) = delete;
    ~Socket() noexcept;
public:
    bool bind();
    bool listen(int backlog) noexcept;
    bool connect();
    bool accept();
private:
    bool attemptToCreateSocket();
    void closeSocket() noexcept;
    AddressInfo addressInfo;
    SocketID socketID;
};

}

#endif
