#include "address_info.hpp"

using namespace util;

AddressInfo::AddressInfo(std::string_view address, std::string_view port, SocketType type) {
    info_ = getAddressInfo(address, port, type);
    logDebug("successfully obtained address info");

    currentInfo_ = info_.get();
}

auto AddressInfo::getAddressInfo(std::string_view address, std::string_view port, SocketType type) -> AddrInfoPtr {
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