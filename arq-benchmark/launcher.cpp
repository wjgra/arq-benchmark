#include <array>
#include <boost/program_options.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <ranges>
#include <string>
#include <thread>
#include <variant>

#include "config.hpp"
#include "util/endpoint.hpp"
#include "util/logging.hpp"

using LogLev_t = std::underlying_type_t<util::LoggingLevel>;

struct ProgramOption {
    std::string name;
    std::variant<std::monostate, // Options with no arg
                 LogLev_t,       // Logging level option
                 std::string>    // String options
    defaultValue;
    std::string helpText;
};

using namespace std::string_literals;

// Possible program options
auto programOptions = std::to_array<ProgramOption>({
    {"help",            std::monostate{},                               "display help message"},
    {"logging",         std::to_underlying(util::LOGGING_LEVEL_INFO),   util::Logger::helpText()},
    {"server-addr",     "127.0.0.1"s,                                   "server IPv4 address"},
    {"server-port",     "65534"s,                                       "server port"},
    {"client-addr",     "127.0.0.1"s,                                   "client IPv4 address"},
    {"client-port",     "65535"s,                                       "client port"},
    {"launch-server",   std::monostate{},                               "start server thread"},
    {"launch-client",   std::monostate{},                               "start client thread"}
});

static auto generateOptionsDescription() {
    namespace po = boost::program_options;
    po::options_description description{"Usage"};

    // Add each possible option to the description object
    for (const auto& option: programOptions) {
        util::logDebug("parsing option: {}", option.name.c_str());
        
        // Add options with no arguments
        if (std::holds_alternative<std::monostate>(option.defaultValue)) {
            description.add_options()(option.name.c_str(), option.helpText.c_str());
        }

        // Add options with LogLev_t type arguments
        else if (std::holds_alternative<LogLev_t>(option.defaultValue)) {
            description.add_options()(
                option.name.c_str(),
                po::value<LogLev_t>()->default_value(std::get<LogLev_t>(option.defaultValue)),
                option.helpText.c_str());
        }

        // Add options with string  arguments
        else if (std::holds_alternative<std::string>(option.defaultValue)) {
            description.add_options()(
                option.name.c_str(),
                po::value<std::string>()->default_value(std::get<std::string>(option.defaultValue)),
                option.helpText.c_str());
        }
        else {
            util::logError("default value incorrectly specified for option {}", option.name);
            throw std::logic_error("default value incorrectly specified in programOptions");
        }
    }

    return description;
}

struct HelpException : public std::invalid_argument {
    explicit HelpException(const std::string& what) : std::invalid_argument(what) {};
};

static auto parseOptions(int argc,
                         char** argv,
                         boost::program_options::options_description description)
{
    arq::config_Launcher config{};
    
    namespace po = boost::program_options;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);

    // Parse each possible option in turn, and verify all have been parsed
    try {
        [[maybe_unused]] size_t idx{0};
        // Help
        assert(programOptions[idx++].name == "help");
        if (vm.contains("help")) {
            throw HelpException("help requested");
        }

        // Logging
        assert(programOptions[idx++].name == "logging");
        if (vm.contains("logging")) {
            const auto newLevel{static_cast<util::LoggingLevel>(vm["logging"].as<LogLev_t>())};
            util::Logger::setLoggingLevel(newLevel);
        }
        util::logInfo("logging level set to {}", util::Logger::loggingLevelStr());

        // Server address
        assert(programOptions[idx++].name == "server-addr");
        if (vm.contains("server-addr")) {
            config.common.serverNames.hostName = vm["server-addr"].as<std::string>();
        }
        else {
            throw HelpException("server-addr not provided");
        }

        // Server port
        assert(programOptions[idx++].name == "server-port");
        if (vm.contains("server-port")) {
            config.common.serverNames.serviceName = vm["server-port"].as<std::string>();
        }
        else {
            throw HelpException("server-port not provided");
        }

        // Client address
        assert(programOptions[idx++].name == "client-addr");
        if (vm.contains("client-addr")) {
            config.common.clientNames.hostName = vm["client-addr"].as<std::string>();
        }
        else {
            throw HelpException("client-addr not provided");
        }

        // Client port
        assert(programOptions[idx++].name == "client-port");
        if (vm.contains("client-port")) {
            config.common.clientNames.serviceName = vm["client-port"].as<std::string>();
        }
        else {
            throw HelpException("client-port not provided");
        }

        // Launch server
        assert(programOptions[idx++].name == "launch-server");
        if (vm.contains("launch-server")) {
            config.server = arq::config_Server{}; // No content currently
        }

        // Launch client
        assert(programOptions[idx++].name == "launch-client");
        if (vm.contains("launch-client")) {
            config.client = arq::config_Client{}; // No content currently
        }

        // Check that all options have been processed
        assert(idx == programOptions.size());
    }
    catch (const HelpException& e) {
        std::println("Displaying usage information ({})", e.what());
        // Output usage information
        std::cout << description << '\n';
        throw std::runtime_error("failed to parse options");
    }

    return config;
}

static auto generateConfiguration(int argc, char** argv) {
    auto description{generateOptionsDescription()};
    return parseOptions(argc, argv, description);
}


// Temp...
/* #include <random>
constexpr size_t sizeof_sendBuffer = 1e6;

auto makeSendBuffer() {
    std::array<uint8_t, sizeof_sendBuffer> buffer;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint8_t> dist(0, UINT8_MAX);
    for (auto& element : buffer) {
        element = dist(mt);
    }
    return buffer;
}

static const auto sendBuffer = makeSendBuffer();


void startServer(arq::config_Launcher& config) {
    using namespace util;
    logDebug("attempting to start server (host: {}, service: {})",
             config.common.serverNames.hostName,
             config.common.serverNames.serviceName);
    
    util::Endpoint endpoint{config.common.serverNames.hostName,
                            config.common.serverNames.serviceName,
                            SocketType::TCP};
    
    logDebug("successfully created server endpoint");

    if (!endpoint.listen(1)) {
        throw std::runtime_error("failed to listen on server endpoint");
    }
    logDebug("listening on server endpoint...");

    if (!endpoint.accept(config.common.clientNames.hostName)) {
        throw std::runtime_error("failed to accept connection at server endpoint");
    }

    logInfo("server connected");

    // std::array<uint8_t, 20> dataToSend{"Hello, client!"};
    auto tick = std::chrono::steady_clock::now();
    if (!endpoint.send(sendBuffer)) {
        throw std::runtime_error("failed to send data to client");
    }
    auto tock = std::chrono::steady_clock::now();
    logInfo("server sent {} bytes in {}", sendBuffer.size(), std::chrono::duration_cast<std::chrono::microseconds>(tock - tick));

    logInfo("server thread shutting down");
}

void startClient(arq::config_Launcher& config) {
    using namespace util;
    logDebug("attempting to start client (host: {}, service: {})",
            config.common.clientNames.hostName,
            config.common.clientNames.serviceName);

    Endpoint endpoint{config.common.clientNames.hostName,
                      config.common.clientNames.serviceName,
                      SocketType::TCP};

    logDebug("successfully created client endpoint");

    endpoint.connectRetry(config.common.serverNames.hostName,
                          config.common.serverNames.serviceName,
                          SocketType::TCP,
                          10,
                          std::chrono::milliseconds(1000));

    std::array<uint8_t, sizeof_sendBuffer> recvBuffer{};

    if (!endpoint.recv(recvBuffer)) {
        throw std::runtime_error("failed to receive data from server");
    }
    logInfo("client received {} bytes from server", sizeof_sendBuffer); // this isn't correct

    logInfo("client thread shutting down");
}
 */

#include "arq/transmitter.hpp"
#include "arq/receiver.hpp"
#include "util/endpoint.hpp"

void shareConversationID(arq::ConversationID id, const arq::config_AddressInfo& myAddress, std::string_view destHost) {
    util::Endpoint controlChannel(myAddress.hostName, myAddress.serviceName, util::SocketType::TCP);

    if (!controlChannel.listen(1)) {
        throw std::runtime_error("failed to listen on control channel");
    }

    if (!controlChannel.accept(destHost)) {
        throw std::runtime_error("failed to accept control channel connection");
    }
    
    if (controlChannel.send({{id}}) != sizeof(id)) {
        throw std::runtime_error("failed to send conversation ID to receiver");
    }

    std::array<uint8_t, sizeof(id)> recvBuffer;
    if (controlChannel.recv(recvBuffer) != sizeof(id)) {
        throw std::runtime_error("failed to receive conversation ID ACK");
    }

    assert(sizeof(id) == 1); // No endianness conversion required
    if (recvBuffer[0] != id) {
        throw std::runtime_error(std::format("conversation ID in ACK is incorrect (received {}, expected {})", recvBuffer[0], id));
    }
    util::logDebug("received correct conversation ID {} as ACK", recvBuffer[0]);
}

arq::ConversationID receiveConversationID(const arq::config_AddressInfo& myAddress, const arq::config_AddressInfo& destAddress) {
    util::Endpoint controlChannel(myAddress.hostName, myAddress.serviceName, util::SocketType::TCP);

    controlChannel.connectRetry(destAddress.hostName, destAddress.serviceName, util::SocketType::TCP, 20, std::chrono::milliseconds(500));

    std::array<uint8_t, sizeof(arq::ConversationID)> recvBuffer;
    if (controlChannel.recv(recvBuffer) != sizeof(arq::ConversationID)) {
        throw std::runtime_error("failed to receive conversation ID");
    }

    assert(sizeof(arq::ConversationID) == 1); // No endianness conversion required
    arq::ConversationID receivedID = recvBuffer[0];
    util::logDebug("received conversation ID {}, sending ACK", receivedID);

        
    if (controlChannel.send({{receivedID}}) != sizeof(receivedID)) {
        throw std::runtime_error("failed to send ACK");
    }

    return receivedID;
}

void startTransmitter(arq::config_Launcher& config) {
    // Generate a new conversation ID and share with receiver
    arq::ConversationIDAllocator allocator{};
    auto convID = allocator.getNewID();

    shareConversationID(convID, config.common.serverNames, config.common.clientNames.hostName);
    util::logInfo("Conversation ID {} shared with receiver", convID);
}

void startReceiver(arq::config_Launcher& config) {
    // Obtain conversation ID from tranmitter
    auto convID = receiveConversationID(config.common.clientNames, config.common.serverNames);
    util::logInfo("Conversation ID {} received from transmitter", convID);

}


int main(int argc, char** argv) {
    arq::config_Launcher cfg;
    try {
        cfg = generateConfiguration(argc, argv);
    }
    catch (const std::exception& e) {
        util::logError("Failed to generate launcher configuration ({})", e.what());
        return EXIT_FAILURE;
    }

    std::thread txThread, rxThread;
    
    if (cfg.server.has_value()) {
        txThread = std::thread(startTransmitter, std::ref(cfg));
    }

    if (cfg.client.has_value()) {
        rxThread = std::thread(startReceiver, std::ref(cfg));
    }

    if (txThread.joinable()) {
        txThread.join();
    }
    
    if (rxThread.joinable()) {
        rxThread.join();
    }

    return EXIT_SUCCESS;
}