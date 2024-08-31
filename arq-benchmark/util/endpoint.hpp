#ifndef _UTIL_ENDPOINT_HPP_
#define _UTIL_ENDPOINT_HPP_

#include "util/address_info.hpp"
#include "util/socket.hpp"

#include <optional>

namespace util{

struct EndpointException : public std::runtime_error {
    explicit EndpointException(const std::string what) : std::runtime_error(what) {};
};

// A communications endpoint for exchanging data via sockets
class Endpoint {
public:
    // Constructs an endpoint of the given SocketType
    explicit Endpoint(SocketType type);
    // Constructs an endpoint of the given SocketType and binds it to a host/service
    explicit Endpoint(std::string_view host, std::string_view service, SocketType type);

    // Listen for connections at this endpoint
    bool listen(int backlog) noexcept;
    // Connect to a socker with the given host/service
    bool connect(std::string_view host, std::string_view service, SocketType type);
    // Accept a connection at the endpoint. If host/service is provided, the connecting socket must match
    // the provided argument(s).
    bool accept(std::optional<std::string_view> expectedHost = std::nullopt);

    // Not implemented
    void send();
    void recv();
    void send_to();
    void recv_from();
private:
    // The socket used for communication at this endpoint
    Socket socket_;
};

};

#endif
