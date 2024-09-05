#ifndef _UTIL_ENDPOINT_HPP_
#define _UTIL_ENDPOINT_HPP_

#include "util/socket.hpp"

#include <chrono>
#include <optional>

namespace util{

struct EndpointException : public std::runtime_error {
    explicit EndpointException(const std::string& what) : std::runtime_error(what) {};
};

// A communications endpoint for exchanging data via sockets
class Endpoint {
public:
    // Constructs an endpoint of the given SocketType
    explicit Endpoint(SocketType type);
    // Constructs an endpoint of the given SocketType and binds it to a host/service
    explicit Endpoint(std::string_view host, std::string_view service, SocketType type);

    // Listen for connections at this endpoint
    bool listen(const int backlog) const noexcept;
    // Connect to a socket with the given host/service
    bool connect(std::string_view host,
                 std::string_view service,
                 SocketType type) const noexcept;
    // Attempt to connect to a socket maxAttempts times, with a cooldown between attempts
    bool connectRetry(std::string_view host,
                      std::string_view service,
                      SocketType type,
                      const int maxAttempts,
                      const std::chrono::duration<int, std::milli> cooldown) const noexcept;
    // Accept a connection at the endpoint. If host/service is provided, the connecting socket must match
    // the provided argument(s).
    bool accept(std::optional<std::string_view> expectedHost = std::nullopt);

    bool send(std::span<const uint8_t> buffer) const noexcept;
    bool recv(std::span<uint8_t> buffer) const noexcept;
/*     void send_to();
    void recv_from(); */
private:
    // The socket used for communication at this endpoint
    Socket socket_;
};

};

#endif
