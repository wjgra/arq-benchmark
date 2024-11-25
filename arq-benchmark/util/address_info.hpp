#ifndef _UTIL_ADDRESS_INFO_HPP_
#define _UTIL_ADDRESS_INFO_HPP_

#include <netdb.h>
#include <iterator>
#include <memory>
#include <string_view>

#include "util/network_common.hpp"

namespace util {

struct AddrInfoException : public std::runtime_error {
    explicit AddrInfoException(const std::string& what) : std::runtime_error(what){};
};

// Owning wrapper for the heap-allocated addrinfo linked list returned by getaddrinfo().
class AddressInfo {
public:
    explicit AddressInfo(std::string_view host, std::string_view service, SocketType type);
    auto begin() const noexcept { return ConstIterator{info_.get()}; }

    auto end() const noexcept { return ConstIterator{nullptr}; }

private:
    struct AddrInfoDeleter {
        void operator()(addrinfo* p) const noexcept
        {
            if (p != nullptr) {
                freeaddrinfo(p);
            }
        }
    };

    using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;
    AddrInfoPtr getAddressInfo(std::string_view host, std::string_view service, SocketType type);

    AddrInfoPtr info_; // Linked list of information structs

    struct ConstIterator {
        using value_type = const addrinfo;
        using difference_type = std::ptrdiff_t;

        explicit ConstIterator(value_type* ptr) noexcept : current_{ptr} {};

        value_type operator*() const { return *current_; }

        ConstIterator& operator++()
        {
            current_ = current_->ai_next;
            return *this;
        }

        ConstIterator operator++(int)
        { // post
            auto temp{*this};
            this->operator++();
            return temp;
        }

        bool operator==(const ConstIterator& other) const { return this->current_ == other.current_; }

        value_type* current_;
    };
    static_assert(std::input_iterator<ConstIterator>);
};

} // namespace util

#endif
