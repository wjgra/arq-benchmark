#ifndef _UTIL_SOCKETS_HPP_
#define _UTIL_SOCKETS_HPP_

#include <string_view>


#include "util/address_info.hpp"

namespace util {

class Socket {
public:
    Socket(std::string_view address, std::string_view port, SocketType type);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(const Socket&&) = default;
    Socket& operator=(const Socket&) = default;
    ~Socket(); // free addrinfo + close
public:
    bool accept();
    bool bind();
    bool connect();
    bool listen();
private:
    bool createSocketWithCurrentInfo();
    AddressInfo addressInfo;
    int socketID;
};

}

#endif