#ifndef _UTIL_ADDRESS_INFO_HPP_
#define _UTIL_ADDRESS_INFO_HPP_

#include <netdb.h>
#include <string_view>

#include "util/network_common.hpp"

namespace util {
    
class AddressInfo {
public:
    AddressInfo(std::string_view address, std::string_view port, SocketType type) {
        // Obtain address information using hint struct
        addrinfo hints = {
            .ai_family = AF_UNSPEC,
            .ai_socktype = type == SocketType::TCP ? SOCK_STREAM : SOCK_DGRAM,
            .ai_protocol = 0
        };
        
        if (getaddrinfo(address.data(), port.data(), &hints, &info) != 0) {
            throw std::runtime_error("failed to get address info");
        }
        util::logDebug("successfully obtained address info");

        currentInfo = info;
    }

    ~AddressInfo() {
        freeaddrinfo(info);
    }

    addrinfo *getCurrentAddrInfo() const {
        return currentInfo;
    }

    addrinfo *getNextAddrInfo() {
        return currentInfo = currentInfo->ai_next;
    }
private:
    addrinfo *info; // Linked list of information structs
    addrinfo *currentInfo; // Current position in the list
};

}

#endif;