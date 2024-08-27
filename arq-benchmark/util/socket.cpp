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

bool util::Socket::bind(const addrinfo& ai) {
    // Allow address reuse
    const int yes{1};
    if (::setsockopt(socketID_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == SOCKET_ERROR) {
        throw SocketException("failed to set socket options");
    }
    return ::bind(socketID_, ai.ai_addr, ai.ai_addrlen) != SOCKET_ERROR;
}

bool util::Socket::listen(int backlog) noexcept {
    return ::listen(socketID_, backlog) != SOCKET_ERROR;
}

bool util::Socket::connect(const addrinfo& ai) {
    return ::connect(socketID_, ai.ai_addr, ai.ai_addrlen) != SOCKET_ERROR;

}

util::Socket util::Socket::accept(std::optional<std::string_view> host, std::optional<std::string_view> service) {
    sockaddr_storage theirAddr;
    socklen_t theirAddrLen = sizeof(theirAddr);
    auto newSocketID = ::accept(socketID_, reinterpret_cast<sockaddr*>(&theirAddr), &theirAddrLen);

    if (host) {
        // validate... or throw socketexception
    }
    if (service) {
        // validate... or throw socketexception
    }

    return Socket{newSocketID};
}
