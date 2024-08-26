#ifndef _UTIL_NETWORK_COMMON_HPP_
#define _UTIL_NETWORK_COMMON_HPP_
namespace util {
    
enum class SocketType {
    UNSPEC,
    TCP,
    UDP
};

static constexpr auto socketType2SockType(const SocketType type) {
    switch (type) {
        case SocketType::TCP: return SOCK_STREAM;
        case SocketType::UDP: return SOCK_DGRAM;
        case SocketType::UNSPEC:
        default:
            throw std::invalid_argument("unknown socket type");
    }
}

static constexpr inline auto socketType2PreferredProtocol(const SocketType type) {
    switch (type) {
        case SocketType::TCP: return IPPROTO_TCP;
        case SocketType::UDP: return IPPROTO_UDP;
        case SocketType::UNSPEC:
        default:
            return IPPROTO_IP; // Default protocol
    }
}

enum class AddressType {
    UNSPEC,
    IPv4,
    IPv6
};

}

#endif