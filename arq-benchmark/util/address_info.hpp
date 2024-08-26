#ifndef _UTIL_ADDRESS_INFO_HPP_
#define _UTIL_ADDRESS_INFO_HPP_

#include <memory>
#include <netdb.h>
#include <string_view>

#include "util/network_common.hpp"


#include <iterator>

namespace util {

// Owning wrapper for the heap-allocated addrinfo returned by getaddrinfo().
class AddressInfo {
    struct ConstIterator {
        using value_type = const addrinfo;
        using difference_type = std::ptrdiff_t;

        ConstIterator(value_type* ptr) : current_{ptr}{};

        value_type operator*() const {
            return *current_;
        }

        ConstIterator& operator++(){
            current_ = current_->ai_next;
            return *this;
        }

        ConstIterator operator++(int){ // post
            auto temp{*this};
            this->operator++();
            return temp;
        }

        bool operator==(const ConstIterator& other) const {
            return this->current_ == other.current_;
        }

        value_type* current_;
    };
    static_assert(std::input_iterator<ConstIterator>);
public:
    auto begin() const {
        return ConstIterator{info_.get()};
    }

    auto end() const {
        return ConstIterator{nullptr};
    }
public:
    AddressInfo() = default;
    AddressInfo(std::string_view address, std::string_view port, SocketType type);

    auto getFirstAddrInfo() noexcept {
        return currentInfo_ = info_.get();
    }

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
