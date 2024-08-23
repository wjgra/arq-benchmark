#ifndef _UTIL_ADDRESS_INFO_HPP_
#define _UTIL_ADDRESS_INFO_HPP_

#include <memory>
#include <netdb.h>
#include <string_view>

#include "util/logging.hpp"
#include "util/network_common.hpp"

namespace util {

// Owning wrapper for the heap-allocated addrinfo returned by getaddrinfo().
class AddressInfo {
    struct AddrInfoDeleter {
        void operator()(addrinfo *p) const { 
            freeaddrinfo(p);
        }
    };

    using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;
public:
    AddressInfo(std::string_view address, std::string_view port, SocketType type) {
        info_ = getAddressInfo(address, port, type);
        util::logDebug("successfully obtained address info");

        currentInfo_ = info_.get();
    }

    auto getCurrentAddrInfo() const noexcept {
        return currentInfo_;
    }

    auto getNextAddrInfo() noexcept {
        return currentInfo_ = currentInfo_->ai_next;
    }
private:
    AddrInfoPtr getAddressInfo(std::string_view address, std::string_view port, SocketType type) {

        // Obtain address information using hint struct
        addrinfo hints{
            .ai_family{AF_UNSPEC},
            .ai_socktype{type == SocketType::TCP ? SOCK_STREAM : SOCK_DGRAM},
            .ai_protocol = 0 // add explicit TCP/UDP value
        };

        addrinfo *out;
        if (auto ret = getaddrinfo(address.data(), port.data(), &hints, &out)) {
            throw std::runtime_error(std::format("failed to get address info ({})", gai_strerror(ret)));
        }
        return AddrInfoPtr{out};
    }

    AddrInfoPtr info_; // Linked list of information structs
    addrinfo *currentInfo_; // Current position in the list
};

}

#endif