#ifndef _UTIL_ADDRESS_INFO_HPP_
#define _UTIL_ADDRESS_INFO_HPP_

#include <memory>
#include <netdb.h>
#include <string_view>

#include "util/network_common.hpp"

namespace util {

// Owning wrapper for the heap-allocated addrinfo returned by getaddrinfo().
class AddressInfo {
public:
    AddressInfo() = default;
    AddressInfo(std::string_view address, std::string_view port, SocketType type);

    auto getCurrentAddrInfo() const noexcept {
        return currentInfo_;
    }

    auto getNextAddrInfo() noexcept {
        return currentInfo_ = currentInfo_->ai_next;
    }
private:
    struct AddrInfoDeleter {
        void operator()(addrinfo *p) const { 
            if (p != nullptr) freeaddrinfo(p);
        }
    };

    using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;
    AddrInfoPtr getAddressInfo(std::string_view address, std::string_view port, SocketType type);

    AddrInfoPtr info_; // Linked list of information structs
    addrinfo *currentInfo_; // Current position in the list
};

}

#endif