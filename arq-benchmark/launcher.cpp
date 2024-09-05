#include <array>
#include <boost/program_options.hpp>
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

    std::array<uint8_t, 20> dataToSend{"Hello, client!"};

    if (!endpoint.send(dataToSend)) {
        throw std::runtime_error("failed to send data to client");
    }
    logInfo("server sent: {}", std::string(dataToSend.begin(), dataToSend.end()));

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

    std::array<uint8_t, 20> recvBuffer{};

    if (!endpoint.recv(recvBuffer)) {
        throw std::runtime_error("failed to receive data from server");
    }
    logInfo("client received: {}", std::string(recvBuffer.begin(), recvBuffer.end()));

    logInfo("client thread shutting down");
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

    std::thread serverThread, clientThread;
    
    if (cfg.server.has_value()) {
        serverThread = std::thread(startServer, std::ref(cfg));
    }

    if (cfg.client.has_value()) {
        clientThread = std::thread(startClient, std::ref(cfg));
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    if (clientThread.joinable()) {
        clientThread.join();
    }

    return EXIT_SUCCESS;
}