#ifndef _UTIL_SOCKET_HPP_
#define _UTIL_SOCKET_HPP_

#include <optional>
#include <span>
#include <string_view>

#include "util/address_info.hpp"

namespace util {

struct SocketException : public std::runtime_error {
    explicit SocketException(const std::string& what) : std::runtime_error(what) {};
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

    bool bind(const addrinfo& ai) const;
    bool listen(int backlog) const noexcept;
    bool connect(const addrinfo& ai) const noexcept;
    Socket accept(std::optional<std::string_view> expectedHost = std::nullopt) const;

    bool send(std::span<const uint8_t> buffer) const noexcept;
    bool recv(std::span<uint8_t> buffer) const noexcept;
/*     bool sendTo();
    bool recvFrom(); */

private:
    SocketID socketID_;
};

}

#endif
