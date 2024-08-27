#ifndef _UTIL_SOCKET_HPP_
#define _UTIL_SOCKET_HPP_

#include <optional>
#include <string_view>

#include "util/address_info.hpp"

namespace util {

struct SocketException : public std::runtime_error {
    explicit SocketException(const std::string what) : std::runtime_error(what) {};
};

// Owning wrapper for a socket file descriptor.
class Socket {
public:
    using SocketID = int;
    explicit Socket() noexcept;
    explicit Socket(SocketType type);
    explicit Socket(SocketID id) noexcept;
    // Constructs a socket for the given address info and attempts to bind it at the
    // corresponding address/port.
    explicit Socket(const addrinfo& ai);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;
    ~Socket() noexcept;

    bool bind(const addrinfo& ai);
    bool listen(int backlog) noexcept;
    bool connect(const addrinfo& ai);
    Socket accept(std::optional<std::string_view> host = std::nullopt, 
                  std::optional<std::string_view> service = std::nullopt);

    // Not implemented
    bool send();
    bool recv();
    bool sendTo();
    bool recvFrom();

private:
    SocketID socketID_;
};

}

#endif
