#include <array>

#include <variant>

#include <iostream>
#include <stdexcept>
#include <thread>

#include <string>

#include <boost/program_options.hpp>

#include <netinet/in.h>

#include "util/logging.hpp"
#include "util/socket.hpp"

#include "config.hpp"

struct ProgramOption {
    std::string name;
    std::variant<std::monostate, uint16_t, std::string> defaultValue; // This matches the type of the (optional) argument
    std::string helpText;
};

// Possible program options
auto programOptions = std::to_array<ProgramOption>({
    {"help",            std::monostate{},           "display help message"},
    {"logging",         util::LOGGING_LEVEL_INFO,   util::Logger::helpText()},
    {"server-addr",     std::string{"127.0.0.1"},   "server IPv4 address"},
    {"server-port",     uint16_t{1000},             "server port"},
    {"client-addr",     std::string{"127.0.0.1"},   "client IPv4 address"},
    {"client-port",     uint16_t{1000},             "client port"},
    {"launch-server",   std::monostate{},           "start server thread"},
    {"launch-client",   std::monostate{},           "start client thread"}
});

static auto generateOptionsDescription() {
    namespace po = boost::program_options;
    po::options_description description{"Usage"};

    // Add each possible option to the description object
    for (const auto& option: programOptions) {
        util::logDebug("parsing option: {}", option.name.c_str());
        // Add options with no arguments (indicated by monostate defaultValue)
        if (std::holds_alternative<std::monostate>(option.defaultValue)) {
            description.add_options()(option.name.c_str(), option.helpText.c_str());
        }
        // Add options with uint16_t type arguments
        else if (std::holds_alternative<uint16_t>(option.defaultValue)) {
            description.add_options()(
                option.name.c_str(),
                po::value<uint16_t>()->default_value(std::get<uint16_t>(option.defaultValue)),
                option.helpText.c_str());
        }
        // Add options with string type arguments
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
        size_t idx{0};
        // Help
        assert(programOptions[idx++].name == "help");
        if (vm.contains("help")) {
            throw std::invalid_argument("help requested");
        }

        // Logging
        assert(programOptions[idx++].name == "logging");
        if (vm.contains("logging")) {
            const auto newLevel{static_cast<util::LoggingLevel>(vm["logging"].as<uint16_t>())};
            util::Logger::setLoggingLevel(newLevel);
        }
        util::logInfo("logging level set to {}", util::Logger::loggingLevelStr());

        // Server address
        assert(programOptions[idx++].name == "server-addr");
        if (vm.contains("server-addr")) {
            config.common.serverAddr.sin_addr = arq::stringToSockaddrIn(vm["server-addr"].as<std::string>());
            config.common.serverAddr.sin_family = AF_INET;
        }
        else {
            throw std::invalid_argument("server-addr not provided");
        }

        // Server port
        assert(programOptions[idx++].name == "server-port");
        if (vm.contains("server-port")) {
            config.common.serverAddr.sin_port = htons(vm["server-port"].as<uint16_t>());
        }
        else {
            throw std::invalid_argument("server-port not provided");
        }

        // Client address
        assert(programOptions[idx++].name == "client-addr");
        if (vm.contains("client-addr")) {
            config.common.clientAddr.sin_addr = arq::stringToSockaddrIn(vm["client-addr"].as<std::string>());
            config.common.clientAddr.sin_family = AF_INET;
        }
        else {
            throw std::invalid_argument("client-addr not provided");
        }

        // Client port
        assert(programOptions[idx++].name == "client-port");
        if (vm.contains("client-port")) {
            config.common.clientAddr.sin_port = htons(vm["client-port"].as<uint16_t>());
        }
        else {
            throw std::invalid_argument("client-port not provided");
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
    catch (const std::invalid_argument& e) {
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
    util::logDebug("attempting to start server (addr: {:08x}, port: {})",
             config.common.serverAddr.sin_addr.s_addr,
             ntohs(config.common.serverAddr.sin_port));

    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("failed to create socket"); // currently not caught - add a wrapper function to catch the exceptions
    }
    util::logDebug("successfully created socket");

    if (bind(sock, reinterpret_cast<sockaddr*>(&config.common.clientAddr), sizeof(config.common.clientAddr)) == -1) {
        util::logError("failed to bind socket ({})", strerror(errno));
        throw std::runtime_error("failed to bind socket");
    }
    util::logDebug("successfully bound socket");

    if (listen(sock, 50 /* number of connection requests - listen backlog */) == -1) {
        throw std::runtime_error("failed to listen");
    }
    util::logDebug("successfully starting listening to socket");

    socklen_t sizeof_serverAddr = sizeof(config.common.serverAddr);
    auto accepted = accept(sock, reinterpret_cast<sockaddr*>(&config.common.serverAddr), &sizeof_serverAddr);
    if (accepted == -1) {
        throw std::runtime_error("failed to accept");
    }

    util::logInfo("server connected");

    if (close(sock) == -1) {
        throw std::runtime_error("failed to close");
    }
    util::logInfo("server thread shutting down");
}

void startClient(arq::config_Launcher& config) {
    util::logDebug("attempting to start client (addr: {:08x}, port: {})",
            config.common.clientAddr.sin_addr.s_addr,
            ntohs(config.common.clientAddr.sin_port));
    
    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("failed to create socket"); // currently not caught - add a wrapper function to catch the exceptions
    }
    util::logDebug("successfully created socket");

    usleep(1000);
    if (connect(sock, reinterpret_cast<sockaddr*>(&config.common.serverAddr), sizeof(config.common.serverAddr)) == -1) {
        throw std::runtime_error("failed to connect to socket");
    }
    util::logDebug("successfully connected to socket");


    if (close(sock) == -1) {
        throw std::runtime_error("failed to close");
    }
    util::logInfo("client thread shutting down");
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

    std::thread serverThread, clientThread; // To do: consider using async
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