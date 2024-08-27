#include "util/endpoint.hpp"

#include "util/logging.hpp"

util::Endpoint::Endpoint(SocketType type) : socket_{type} {
}

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

bool util::Endpoint::listen(int backlog) noexcept {
    return socket_.listen(backlog);
}

bool util::Endpoint::connect(std::string_view host, std::string_view service, SocketType type) {
    AddressInfo peerInfo{host, service, type};

    // Connect to the earliest entry in the peerInfo list possible.
    for (const auto& ai : peerInfo) {
        if (socket_.connect(ai)) {
            return true;
        }
    }
    return false;
}

bool util::Endpoint::accept(std::optional<std::string_view> host, std::optional<std::string_view> service) {
    try {
        Socket acceptedSock = socket_.accept(host, service);
        
        // Replace endpoint listening socket with new socket
        socket_ = std::move(acceptedSock);
        return true;
    }
    catch (const SocketException& e) {
        logWarning("failed to accept connection at endpoint ({})", e.what());
        return false;
    }
}
