#include <netinet/in.h>
#include <array>
#include <boost/program_options.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

#include "config.hpp"

#include "arq/input_buffer.hpp"
#include "arq/receiver.hpp"
#include "arq/transmitter.hpp"
#include "util/endpoint.hpp"
#include "util/logging.hpp"

using LogLevel_t = std::underlying_type_t<util::LoggingLevel>;

struct ProgramOption {
    std::string name;
    std::variant<std::monostate, // Options with no argument
                 LogLevel_t, // Logging level option
                 std::string> // String options
        defaultValue;
    std::string helpText;
};

using namespace std::string_literals;

// Possible program options
// clang-format off
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
// clang-format on

static auto generateOptionsDescription()
{
    namespace po = boost::program_options;
    po::options_description description{"Usage"};

    // Add each possible option to the description object
    for (const auto& option : programOptions) {
        util::logDebug("parsing option: {}", option.name.c_str());

        // Add options with no arguments
        if (std::holds_alternative<std::monostate>(option.defaultValue)) {
            description.add_options()(option.name.c_str(), option.helpText.c_str());
        }

        // Add options with LogLevel_t type arguments
        else if (std::holds_alternative<LogLevel_t>(option.defaultValue)) {
            description.add_options()(option.name.c_str(),
                                      po::value<LogLevel_t>()->default_value(std::get<LogLevel_t>(option.defaultValue)),
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
    explicit HelpException(const std::string& what) : std::invalid_argument(what){};
};

static auto parseOptions(int argc, char** argv, boost::program_options::options_description description)
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
            const auto newLevel{static_cast<util::LoggingLevel>(vm["logging"].as<LogLevel_t>())};
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

static auto generateConfiguration(int argc, char** argv)
{
    auto description{generateOptionsDescription()};
    return parseOptions(argc, argv, description);
}

static void shareConversationID(arq::ConversationID id,
                                const arq::config_AddressInfo& myAddress,
                                std::string_view destHost)
{
    util::Endpoint controlChannel(myAddress.hostName, myAddress.serviceName, util::SocketType::TCP);

    if (!controlChannel.listen(1)) {
        throw std::runtime_error("failed to listen on control channel");
    }

    if (!controlChannel.accept(destHost)) {
        throw std::runtime_error("failed to accept control channel connection");
    }

    if (controlChannel.send({{std::byte(id)}}) != sizeof(id)) {
        throw std::runtime_error("failed to send conversation ID to receiver");
    }

    std::array<std::byte, sizeof(id)> recvBuffer;
    if (controlChannel.recv(recvBuffer) != sizeof(id)) {
        throw std::runtime_error("failed to receive conversation ID ACK");
    }

    assert(sizeof(id) == 1); // No endianness conversion required
    if (std::to_integer<uint8_t>(recvBuffer[0]) != id) {
        throw std::runtime_error(std::format("conversation ID in ACK is incorrect (received {}, expected {})",
                                             std::to_integer<uint8_t>(recvBuffer[0]),
                                             id));
    }
    util::logDebug("received correct conversation ID {} as ACK", std::to_integer<uint8_t>(recvBuffer[0]));
}

static arq::ConversationID receiveConversationID(const arq::config_AddressInfo& myAddress,
                                                 const arq::config_AddressInfo& destAddress)
{
    util::Endpoint controlChannel(myAddress.hostName, myAddress.serviceName, util::SocketType::TCP);

    controlChannel.connectRetry(
        destAddress.hostName, destAddress.serviceName, util::SocketType::TCP, 20, std::chrono::milliseconds(500));

    std::array<std::byte, sizeof(arq::ConversationID)> recvBuffer;
    if (controlChannel.recv(recvBuffer) != sizeof(arq::ConversationID)) {
        throw std::runtime_error("failed to receive conversation ID");
    }

    assert(sizeof(arq::ConversationID) == 1); // No endianness conversion required
    arq::ConversationID receivedID = std::to_integer<uint8_t>(recvBuffer[0]);
    util::logDebug("received conversation ID {}, sending ACK", receivedID);

    if (controlChannel.send({{std::byte(receivedID)}}) != sizeof(receivedID)) {
        throw std::runtime_error("failed to send ACK");
    }

    return receivedID;
}

static void startTransmitter(arq::config_Launcher& config)
{ // why not const?
    // Generate a new conversation ID and share with receiver
    arq::ConversationIDAllocator allocator{};
    auto convID = allocator.getNewID();

    const arq::config_AddressInfo& txerAddress = config.common.serverNames;
    const arq::config_AddressInfo& rxerAddress = config.common.clientNames;

    shareConversationID(convID, txerAddress, rxerAddress.hostName);
    util::logInfo("Conversation ID {} shared with receiver", convID);

    util::Endpoint dataChannel(txerAddress.hostName, txerAddress.serviceName, util::SocketType::UDP);

    // Use first address info found, if possible
    auto rxerAddrInfo = [&rxerAddress]() {
        util::AddressInfo addrInfo(rxerAddress.hostName, rxerAddress.serviceName, util::SocketType::UDP);
        for (auto ai : addrInfo) {
            return ai;
        }
        throw util::AddrInfoException("Failed to find address info"); // This should never be hit
    }();

    arq::TransmitFn txToClient = [&dataChannel, rxerAddrInfo](std::span<const std::byte> buffer) {
        return dataChannel.sendTo(buffer, rxerAddrInfo);
    };

    arq::ReceiveFn rxFromClient = [&dataChannel](std::span<std::byte> buffer) { return dataChannel.recvFrom(buffer); };

    using namespace std::chrono_literals;
    arq::Transmitter txer(convID, txToClient, rxFromClient, std::make_unique<arq::rt::StopAndWait>(100ms, true));
    // Send a few packets with random data
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint8_t> dist(0, UINT8_MAX);

    for (size_t i = 0; i < 10; ++i) {
        arq::DataPacket inputPacket{};

        // populate packet
        inputPacket.updateDataLength(1000);
        auto dataSpan = inputPacket.getPayloadSpan();
        for (auto& el : dataSpan) {
            el = std::byte{dist(mt)};
        }

        // Add packet to transmitter's input buffer
        txer.sendPacket(std::move(inputPacket));
    }

    // Send end of Tx packet
    txer.sendPacket(arq::DataPacket{});
}

static void startReceiver(arq::config_Launcher& config)
{
    // Obtain conversation ID from tranmitter
    auto convID = receiveConversationID(config.common.clientNames, config.common.serverNames);
    util::logInfo("Conversation ID {} received from transmitter", convID);

    arq::Receiver rxer(convID);

    // Temp receiver implementation

    // Receive packet and assert len
    util::Endpoint dataChannel(
        config.common.clientNames.hostName, config.common.clientNames.serviceName, util::SocketType::UDP);

    usleep(1000);

    bool rxEndOfTx = false;
    while (!rxEndOfTx) {
        std::array<std::byte, arq::DATA_PKT_MAX_PAYLOAD_SIZE> recvBuffer;
        auto ret = dataChannel.recvFrom(recvBuffer);
        assert(ret.has_value());

        util::logDebug("Received {} bytes of data", ret.value());

        arq::DataPacket packet(recvBuffer);
        auto pktHdr = packet.getHeader();
        util::logInfo("Received data packet with length {} and SN {}", pktHdr.length_, pktHdr.sequenceNumber_);

        if (packet.isEndOfTx()) {
            rxEndOfTx = true;
        }

        util::logInfo("Sending ACK for packet with SN {}", pktHdr.sequenceNumber_);

        std::array<std::byte, sizeof(arq::SequenceNumber)> ackMsg{};
        arq::serialiseSeqNum(pktHdr.sequenceNumber_, ackMsg);

        dataChannel.sendTo(ackMsg, config.common.serverNames.hostName, config.common.serverNames.serviceName);
    }

    // Send ACKs until EOT received
}

int main(int argc, char** argv)
{
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

    bool txJoined = false;
    bool rxJoined = false;
    while (!(txJoined && rxJoined)) {
        if (txThread.joinable()) {
            txThread.join();
            txJoined = true;
        }
        if (rxThread.joinable()) {
            rxThread.join();
            rxJoined = true;
        }
    }

    return EXIT_SUCCESS;
}