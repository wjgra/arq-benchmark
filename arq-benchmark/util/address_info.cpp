#include "util/address_info.hpp"

#include "util/logging.hpp"

using namespace util;

struct AddrInfoException : public std::runtime_error {
    AddrInfoException(const std::string what) : std::runtime_error(what) {};
};

AddressInfo::AddressInfo(std::string_view address, std::string_view port, SocketType type) {
    try {
        info_ = getAddressInfo(address, port, type);
        logDebug("successfully obtained address info");
        currentInfo_ = info_.get();
    }
    catch (const AddrInfoException& e) {
        logWarning("{}", e.what());
    }
}

auto AddressInfo::getAddressInfo(std::string_view address, std::string_view port, SocketType type) -> AddrInfoPtr {
    // Obtain address information using hint struct
    addrinfo hints{
        .ai_family{AF_UNSPEC},
        .ai_socktype{socketType2SockType(type)},
        .ai_protocol{socketType2PreferredProtocol(type)}
    };

    addrinfo *out;
    if (auto ret = getaddrinfo(address.data(), port.data(), &hints, &out)) {
        throw AddrInfoException(std::format("failed to get address info ({})", gai_strerror(ret)));
    }
    return AddrInfoPtr{out};
}
