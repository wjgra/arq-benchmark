#ifndef _ARQ_BENCHMARK_CONFIG_HPP_
#define _ARQ_BENCHMARK_CONFIG_HPP_

#include <optional>
#include <stdexcept>
#include <string_view>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace arq {

struct config_AddressInfo {
    std::string hostName;
    std::string serviceName;
};

enum class ArqProtocol { DUMMY_TCP, STOP_AND_WAIT, SLIDING_WINDOW, SELECTIVE_REPEAT };

struct config_common {
    config_AddressInfo serverNames;
    config_AddressInfo clientNames;
    ArqProtocol arqProtocol;
};

struct config_txPkts {
    uint16_t num;
    uint16_t msInterval;
};

struct config_Server {
    bool doNotVerifyClientAddr;
    config_txPkts txPkts;
    uint16_t arqTimeout;
};

struct config_Client {};

struct config_Launcher {
    config_common common;
    std::optional<config_Server> server;
    std::optional<config_Client> client;
};

in_addr stringToSockaddrIn(std::string_view str)
{
    in_addr out;
    if (inet_aton(str.data(), &out) == 0) {
        throw std::invalid_argument("invalid IPv4 address provided");
    }
    return out;
}

} // namespace arq

#endif