#include "util/address_info.hpp"

#include "util/logging.hpp"

using namespace util;

AddressInfo::AddressInfo(std::string_view host, std::string_view service, SocketType type) {
    info_ = getAddressInfo(host, service, type);
    logDebug("successfully obtained address info for host '{}' and service '{}'",
            host,
            service);
}

auto AddressInfo::getAddressInfo(std::string_view host,
                                 std::string_view service,
                                 SocketType type) -> AddrInfoPtr {
    // Obtain address information using hint struct
    addrinfo hints{
        .ai_family{AF_UNSPEC},
        .ai_socktype{socketType2SockType(type)},
        .ai_protocol{socketType2PreferredProtocol(type)}
    };

    addrinfo *out;
    if (auto ret = getaddrinfo(host.data(), service.data(), &hints, &out)) {
        throw AddrInfoException(std::format(
            "failed to get address info for host '{}' and service '{}' ({})",
            gai_strerror(ret),
            host,
            service));
    }

    std::print("!");
    for (int i = 0 ; i < out->ai_addrlen ; ++i) {
        std::print("{}", out->ai_addr->sa_data[i]);
    }
    std::println("");


    return AddrInfoPtr{out};
}
