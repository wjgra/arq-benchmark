#include "util/socket.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <utility>

#include "util/logging.hpp"
#include "util/network_common.hpp"

constexpr auto SOCKET_ERROR{-1};

util::Socket::Socket() noexcept : socketID_{SOCKET_ERROR} {};

util::Socket::Socket(SocketType type) :
    socketID_{::socket(AF_UNSPEC, 
                       socketType2SockType(type),
                       socketType2PreferredProtocol(type))}
{
    if (socketID_ == SOCKET_ERROR) {
        throw SocketException("failed to create socket");
    }
}

// Creates a socket from the given address info
static auto createSocket(const addrinfo& info) noexcept {
    return ::socket(info.ai_family, info.ai_socktype, info.ai_protocol);
}

util::Socket::Socket(SocketID id) noexcept : socketID_{id} {};

util::Socket::Socket(const addrinfo& ai) :
    socketID_{createSocket(ai)}
{
    if (socketID_ == SOCKET_ERROR) {
        throw SocketException("failed to create socket");
    }
}

util::Socket::~Socket() noexcept {
    ::close(socketID_);
}

util::Socket::Socket(Socket&& other) noexcept :
    socketID_{std::exchange(other.socketID_, SOCKET_ERROR)}
{}

util::Socket& util::Socket::operator=(Socket&& other) noexcept {
    socketID_ = std::exchange(other.socketID_, SOCKET_ERROR);
    return *this;
}

bool util::Socket::bind(const addrinfo& ai) const {
    // Allow address reuse
    const int yes{1};
    if (::setsockopt(socketID_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == SOCKET_ERROR) {
        throw SocketException("failed to set socket options");
    }
    return ::bind(socketID_, ai.ai_addr, ai.ai_addrlen) != SOCKET_ERROR;
}

bool util::Socket::listen(int backlog) const noexcept {
    return ::listen(socketID_, backlog) != SOCKET_ERROR;
}

bool util::Socket::connect(const addrinfo& ai) const noexcept {
    return ::connect(socketID_, ai.ai_addr, ai.ai_addrlen) != SOCKET_ERROR;
}

static auto getInAddr(sockaddr* addr) noexcept {
    return &reinterpret_cast<sockaddr_in*>(addr)->sin_addr;
}

static auto getIn6Addr(sockaddr* addr) noexcept {
    return &reinterpret_cast<sockaddr_in6*>(addr)->sin6_addr;
}

static auto sockaddr2Str(sockaddr* addr) {
    std::array<char, INET6_ADDRSTRLEN> str;
    if (addr->sa_family == AF_INET) {
        inet_ntop(addr->sa_family,
                  getInAddr(addr),
                  str.data(),
                  str.size());
    }
    else if (addr->sa_family == AF_INET6) {
        inet_ntop(addr->sa_family,
                  getIn6Addr(addr),
                  str.data(),
                  str.size());
    }
    else {
        throw util::SocketException("connecting address has unknown family");
    }
    return std::string{str.data()};
}

util::Socket util::Socket::accept(std::optional<std::string_view> expectedHost) const {
    sockaddr_storage theirAddr;
    socklen_t theirAddrLen = sizeof(theirAddr);
    auto newSocketID = ::accept(socketID_, reinterpret_cast<sockaddr*>(&theirAddr), &theirAddrLen);

    // Check connecting host matches expected
    if (expectedHost.has_value()) {
        auto connectingAddr = sockaddr2Str(reinterpret_cast<sockaddr*>(&theirAddr));

        if (connectingAddr == expectedHost.value()) {
            util::logDebug("validated connection with host {}", connectingAddr);
        }
        else {
            throw util::SocketException(std::format("connecting host ({}) does not match expected ({})",
                                                    connectingAddr,
                                                    expectedHost.value()));
        }
    }
    return Socket{newSocketID};
}

bool util::Socket::send(std::span<const uint8_t> buffer) const noexcept {
    if (::send(socketID_, buffer.data(), buffer.size(), 0) == SOCKET_ERROR) {
        return false;
    }
    return true;
}

bool util::Socket::recv(std::span<uint8_t> buffer) const noexcept {
    if (::recv(socketID_, buffer.data(), buffer.size(), 0) == SOCKET_ERROR) {
        return false;
    }
    return true;
}