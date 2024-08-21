#include "util/sockets.hpp"

#include <arpa/inet.h>
#include <stdexcept>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "util/network_common.hpp"

using namespace util;

constexpr auto SOCKET_FAILURE = -1;

Socket::Socket(std::string_view address, std::string_view port, SocketType type) :
    addressInfo(address, port, type),
    socketID{SOCKET_FAILURE}
{
}

Socket::~Socket(){
    if (close(socketID) == SOCKET_FAILURE) {
        throw std::runtime_error("failed to close socket");
    }
}

bool Socket::accept() {

} // are any of these const?

bool Socket::bind() {

}

bool Socket::connect() {

}

bool Socket::listen() {

}

// Attempt to create a socket with the current address information - return false in the event of failure.
bool Socket::createSocketWithCurrentInfo() {
    auto *info{addressInfo.getCurrentAddrInfo()};
    socketID = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    return socketID != SOCKET_FAILURE;
}