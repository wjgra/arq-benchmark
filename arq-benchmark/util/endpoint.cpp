#include "util/endpoint.hpp"

#include <ranges>
#include <thread>

#include "util/address_info.hpp"
#include "util/logging.hpp"

util::Endpoint::Endpoint(SocketType type) : socket_{type} {}

util::Endpoint::Endpoint(std::string_view host, std::string_view service, SocketType type)
{
    AddressInfo addressInfo{host, service, type};

    // For each entry in AddressInfo linked list, attempt to create a socket and
    // bind to it. If either operation fails, move onto the next entry in the list.
    for (const auto& ai : addressInfo) {
        try {
            Socket sock{ai};

            if (sock.bind(ai)) {
                socket_ = std::move(sock);
                logDebug("Successfully bound endpoint socket");
                return;
            }
            else {
                logDebug("Failed to bind to socket, trying next addrinfo");
            }
        }
        catch (const SocketException& e) {
            logDebug("SocketException detected ({}), trying next addrinfo", e.what());
        }
    }
    throw EndpointException("failed to create endpoint");
}

bool util::Endpoint::listen(const int backlog) const noexcept
{
    return socket_.listen(backlog);
}

// Connect to the earliest entry in the addrInfo list possible.
static bool attemptConnect(const util::Socket& socket, const util::AddressInfo& addrInfo) noexcept
{
    for (const auto& ai : addrInfo) {
        if (socket.connect(ai)) {
            return true;
        }
    }
    return false;
}

bool util::Endpoint::connect(std::string_view host, std::string_view service, SocketType type) const noexcept
{
    AddressInfo peerInfo{host, service, type};
    return attemptConnect(socket_, peerInfo);
}

bool util::Endpoint::connectRetry(std::string_view host,
                                  std::string_view service,
                                  SocketType type,
                                  const int maxAttempts,
                                  const std::chrono::duration<int, std::milli> cooldown) const noexcept
{
    AddressInfo peerInfo{host, service, type};

    for (const int i : std::views::iota(0, maxAttempts)) {
        logDebug(
            "Attempting to connect to host {} with service {} (attempt {} of {})", host, service, i + 1, maxAttempts);
        if (attemptConnect(socket_, peerInfo)) {
            return true;
        }
        std::this_thread::sleep_for(cooldown);
    }
    return false;
}

bool util::Endpoint::accept(std::optional<std::string_view> expectedHost)
{
    auto acceptedSock = socket_.accept(expectedHost);

    if (acceptedSock.has_value()) {
        // Replace endpoint listening socket with new socket
        socket_ = std::move(acceptedSock.value());
        return true;
    }
    else {
        logWarning("failed to accept connection at endpoint");
        return false;
    }
}

bool util::Endpoint::setRecvTimeout(const uint64_t timeoutSeconds, const uint64_t timeoutMicroseconds) const
{
    return socket_.setRecvTimeout(timeoutSeconds, timeoutMicroseconds);
}

std::optional<size_t> util::Endpoint::send(std::span<const std::byte> buffer) const noexcept
{
    return socket_.send(buffer);
}

std::optional<size_t> util::Endpoint::recv(std::span<std::byte> buffer) const noexcept
{
    return socket_.recv(buffer);
}

std::optional<size_t> util::Endpoint::sendTo(std::span<const std::byte> buffer,
                                             std::string_view destinationHost,
                                             std::string_view destinationService) const noexcept
{
    AddressInfo addr(destinationHost, destinationService, SocketType::UDP);
    for (const auto& ai : addr) {
        auto ret = sendTo(buffer, ai);
        if (ret.has_value()) {
            return ret;
        }
    }
    return std::nullopt;
}

std::optional<size_t> util::Endpoint::sendTo(std::span<const std::byte> buffer, const addrinfo& ai) const noexcept
{
    return socket_.sendTo(buffer, ai);
}

std::optional<size_t> util::Endpoint::recvFrom(std::span<std::byte> buffer) const noexcept
{
    return socket_.recvFrom(buffer);
}