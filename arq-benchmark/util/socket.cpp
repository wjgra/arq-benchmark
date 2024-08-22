#include "util/socket.hpp"

#include <arpa/inet.h>
#include <stdexcept>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "util/network_common.hpp"

using namespace util;

constexpr auto SOCKET_ERROR{-1};

// Creates a socket from the given address info
static auto createSocket(addrinfo *addrInfo) noexcept {
    return ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
}

// Iterates over stored address info, creating the first socket possible.
// Returns true if a socket is successfully created.
bool Socket::attemptToCreateSocket() {
    auto info{addressInfo.getCurrentAddrInfo()};
    while ((info = addressInfo.getNextAddrInfo()) != nullptr){
        if ((socketID = createSocket(info)) != SOCKET_ERROR) {
            return true;
        }
    }
    return false;
}

Socket::Socket(std::string_view address, std::string_view port, SocketType type) :
    addressInfo(address, port, type)
{
    if (attemptToCreateSocket() == false) {
        throw std::runtime_error("failed to create socket");
    }
}

// Close current socket
void Socket::closeSocket() {
    if (::close(socketID) == SOCKET_ERROR) {
        throw std::runtime_error("failed to close socket");
    }
}

Socket::~Socket(){
    closeSocket();
}

// Attempts to bind the current socket. If not possible, remaining addrinfo entries are iterated
// over until a socket is successfully bound. Returns false if binding not possible.
bool Socket::bind() {
    do {
        // Allow address reuse
        const int yes{1};
        if (::setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes) == SOCKET_ERROR)) {
            throw std::runtime_error("failed to set socket options");
        }

        // Attempt to bind current socket
        const auto *info{addressInfo.getCurrentAddrInfo()};
        if (::bind(socketID, info->ai_addr, info->ai_addrlen) == SOCKET_ERROR) {
            return true;
        }

        // Clean up socket file descriptor
        closeSocket();
    }
    while (attemptToCreateSocket() == true); // Move on to next socket, if available
    return false;
}

bool Socket::listen(int backlog) {
    return ::listen(socketID, backlog) != SOCKET_ERROR;
}

bool Socket::connect() {
    throw std::logic_error("Socket::connect not implemented");
    return false;
}

bool Socket::accept() {
    throw std::logic_error("Socket::accept not implemented");
    return false;
}

