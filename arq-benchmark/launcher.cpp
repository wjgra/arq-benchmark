#include <iostream>
#include <stdexcept>
#include <thread>

#include <string>

#include <boost/program_options.hpp>

#include <netinet/in.h>

#include "util/logging.hpp"

#include "config.hpp"

static void configureServer() {

}

static void configureClient() {

}

static bool parseOptions(arq::config_Launcher& config, int argc, char** argv) {
    // Init options parser
    namespace po = boost::program_options;
    po::options_description description{"Usage"};
    description.add_options()
        ("help", "produce help message")
        ("logging", po::value<int>()->default_value(util::LOGGING_LEVEL_INFO), util::Logger::helpText().c_str())
        ("server-addr", po::value<std::string>()->default_value("127.0.0.1"), "server IPv4 address")
        ("server-port", po::value<in_port_t>()->default_value(0), "server port")
        ("client-addr", po::value<std::string>()->default_value("127.0.0.1"), "client IPv4 address")
        ("client-port", po::value<in_port_t>()->default_value(0), "client port")
        ("launch-server", "start server thread")
        ("launch-client", "start client thread");
    
    bool success = true;
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);    
        
        if (vm.contains("help")) {
            throw std::invalid_argument("help requested"); // is this the right exception type?
        }
        
        // Parse logging level
        if (vm.contains("logging")) {
            const auto newLevel = static_cast<util::LoggingLevel>(vm["logging"].as<int>());
            util::Logger::setLoggingLevel(newLevel);
        }
        util::logInfo("logging level set to {}", util::Logger::loggingLevelStr());

        // Parse other arguments...
        if (vm.contains("launch-server")) {
            config.server = arq::config_Server{};
        }

        if (vm.contains("launch-client")) {
            config.client = arq::config_Client{};
        }

        // Move to functions...
        assert(vm.contains("server-addr")); // change to exception
        config.common.serverAddr.sin_addr = arq::stringToSockaddrIn(vm["server-addr"].as<std::string>());
        config.common.serverAddr.sin_port = vm["server-port"].as<in_port_t>();

        assert(vm.contains("client-addr")); // change to exception
        config.common.clientAddr.sin_addr = arq::stringToSockaddrIn(vm["client-addr"].as<std::string>());
        config.common.clientAddr.sin_port = vm["client-port"].as<in_port_t>();
    }
    catch (const std::exception& e) { // specialise this exception handler
        std::println("Displaying usage information ({})", e.what());
        // Output usage information
        std::cout << description << '\n';
        success = false;
    }
    return success;
}

void startServer(arq::config_Launcher& config) {
    util::logDebug("attempting to start server");

    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    if (bind(sock, reinterpret_cast<sockaddr*>(&config.common.clientAddr), sizeof(config.common.clientAddr)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(sock, 50 /* number of connection requests - listen backlog */) == -1) {
        throw std::runtime_error("Failed to listen");
    }

    socklen_t sizeof_serverAddr = sizeof(config.common.serverAddr);
    auto accepted = accept(sock, reinterpret_cast<sockaddr*>(&config.common.serverAddr), &sizeof_serverAddr);
    if (accepted == -1) {
        throw std::runtime_error("Failed to accept");
    }
    util::logInfo("server connected");

    if (close(sock) == -1) {
        throw std::runtime_error("Failed to close");
    }
}

void startClient(arq::config_Launcher& config) {

}

int main(int argc, char** argv) {
    arq::config_Launcher cfg; // reformulate to use auto X = make() pattern
    if (!parseOptions(cfg, argc, argv)) {
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