#include <netinet/in.h>
#include <signal.h>
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
#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"
#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"
#include "arq/retransmission_buffers/dummy_sctp_rt.hpp"
#include "arq/retransmission_buffers/stop_and_wait_rt.hpp"
#include "arq/transmitter.hpp"
#include "util/endpoint.hpp"
#include "util/logging.hpp"

static_assert(std::is_same_v<std::underlying_type_t<util::LoggingLevel>, uint16_t>);

struct ProgramOption {
    std::string name;
    std::variant<std::monostate, uint16_t, std::string> defaultValue;
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
    {"launch-client",   std::monostate{},                               "start client thread"},
    {"tx-pkt-num",      uint16_t{10},                                   "number of packets to transmit"},
    {"tx-pkt-interval", uint16_t{10},                                   "ms between transmitted packets"},
    {"arq-timeout",     uint16_t{50},                                   "ARQ timeout in ms"},
    {"arq-protocol",    "dummy-sctp"s,                                   "ARQ protocol to use"}
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

        // Add options with uint16_t type arguments
        else if (std::holds_alternative<uint16_t>(option.defaultValue)) {
            description.add_options()(option.name.c_str(),
                                      po::value<uint16_t>()->default_value(std::get<uint16_t>(option.defaultValue)),
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

static arq::ArqProtocol getArqProtocolFromStr(const std::string& input)
{
    if (input == "dummy-sctp") {
        return arq::ArqProtocol::DUMMY_SCTP;
    }
    else if (input == "stop-and-wait") {
        return arq::ArqProtocol::STOP_AND_WAIT;
    }
    else if (input == "sliding-window") {
        return arq::ArqProtocol::SLIDING_WINDOW;
    }
    else if (input == "selective-repeat") {
        return arq::ArqProtocol::SELECTIVE_REPEAT;
    }

    throw HelpException(std::format("invalid ARQ protocol \"{}\" provided", input));
}

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
            const auto newLevel{static_cast<util::LoggingLevel>(vm["logging"].as<uint16_t>())};
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

        // Number of packets to transmit
        assert(programOptions[idx++].name == "tx-pkt-num");
        if (vm.contains("tx-pkt-num") && config.server.has_value()) {
            config.server->txPkts.num = vm["tx-pkt-num"].as<uint16_t>();
        }

        // Interval between packet transmissions in ms
        assert(programOptions[idx++].name == "tx-pkt-interval");
        if (vm.contains("tx-pkt-interval") && config.server.has_value()) {
            config.server->txPkts.msInterval = vm["tx-pkt-interval"].as<uint16_t>();
        }

        // ARQ timeout (may change if adaptive timeout enabled)
        assert(programOptions[idx++].name == "arq-timeout");
        if (vm.contains("arq-timeout") && config.server.has_value()) {
            config.server->arqTimeout = vm["arq-timeout"].as<uint16_t>();
        }

        // ARQ protocol
        assert(programOptions[idx++].name == "arq-protocol");
        if (vm.contains("arq-protocol")) {
            config.common.arqProtocol = getArqProtocolFromStr(vm["arq-protocol"].as<std::string>());
        }

        if (config.server.has_value()) {
            util::logInfo("server configured to transmit {} packets with interval {} ms",
                          config.server->txPkts.num,
                          config.server->txPkts.msInterval);
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
    return; // WJG: control channel needs to use a different socket to data channel
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
    return 1; // WJG: control channel needs to use a different socket to data channel
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

static void transmitPackets(std::function<void(arq::DataPacket&&)> txerSendPacket,
                            const uint16_t numPackets,
                            const uint16_t msPacketInterval)
{
    // Send a few packets with random data
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint8_t> dist(0, UINT8_MAX);

    for (size_t i = 0; i < numPackets; ++i) {
        arq::DataPacket inputPacket{};

        // Populate packet
        inputPacket.updateDataLength(arq::packet_payload_length);
        inputPacket.updateConversationID(1); // WJG temp - should be based on conversation ID
        auto dataSpan = inputPacket.getPayloadSpan();
        for (auto& el : dataSpan) {
            el = std::byte{dist(mt)};
        }

        // Add packet to transmitter's input buffer
        txerSendPacket(std::move(inputPacket));
        usleep(1000 * msPacketInterval);
    }

    // Send end of Tx packet
    txerSendPacket(arq::DataPacket{});
}

static void startTransmitter(arq::config_Launcher& config /* why not const? */)
{
    // Generate a new conversation ID and share with receiver
    arq::ConversationIDAllocator allocator{};
    auto convID = allocator.getNewID();

    const arq::config_AddressInfo& txerAddress = config.common.serverNames;
    const arq::config_AddressInfo& rxerAddress = config.common.clientNames;

    shareConversationID(convID, txerAddress, rxerAddress.hostName);
    util::logInfo("Conversation ID {} shared with receiver", convID);

    util::Endpoint dataChannel(
        txerAddress.hostName,
        txerAddress.serviceName,
        config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP ? util::SocketType::SCTP : util::SocketType::UDP);

    // Use first address info found, if possible
    // auto rxerAddrInfo = [&rxerAddress]() {
    //     util::AddressInfo addrInfo(rxerAddress.hostName, rxerAddress.serviceName, util::SocketType::UDP);
    //     for (auto ai : addrInfo) {
    //         return ai;
    //     }
    //     throw util::AddrInfoException("Failed to find address info"); // This should never be hit
    // }();
    // WJG: For some reason, the address info changes during transmission, leading to failed sendTo calls. As such, we
    // can't use return dataChannel.sendTo(buffer, rxerAddrInfo); in the below lambda.

    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        if (!dataChannel.listen(1)) {
            throw std::runtime_error("failed to listen on data channel");
        }

        if (!dataChannel.accept(rxerAddress.hostName)) {
            throw std::runtime_error("failed to accept data channel connection");
        }
    }

    arq::TransmitFn txToClient;
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        txToClient = [&dataChannel](std::span<const std::byte> buffer) { return dataChannel.send(buffer); };
    }
    else {
        txToClient = [&dataChannel, &rxerAddress](std::span<const std::byte> buffer) {
            return dataChannel.sendTo(buffer, rxerAddress.hostName, rxerAddress.serviceName);
        };
    }

    arq::ReceiveFn rxFromClient;
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        rxFromClient = [&dataChannel](std::span<std::byte> buffer) { return dataChannel.recv(buffer); };
    }
    else {
        rxFromClient = [&dataChannel](std::span<std::byte> buffer) { return dataChannel.recvFrom(buffer); };
    }

    // WJG to clean up branches - possible template function?
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        arq::Transmitter txer(convID, txToClient, rxFromClient, std::make_unique<arq::rt::DummySCTP>());

        auto txerSend = [&txer](arq::DataPacket&& pkt) { txer.sendPacket(std::move(pkt)); };

        transmitPackets(txerSend, config.server->txPkts.num, config.server->txPkts.msInterval);
    }
    else if (config.common.arqProtocol == arq::ArqProtocol::STOP_AND_WAIT) {
        arq::Transmitter txer(
            convID,
            txToClient,
            rxFromClient,
            std::make_unique<arq::rt::StopAndWait>(std::chrono::milliseconds(config.server->arqTimeout), false));

        auto txerSend = [&txer](arq::DataPacket&& pkt) { txer.sendPacket(std::move(pkt)); };

        transmitPackets(txerSend, config.server->txPkts.num, config.server->txPkts.msInterval);
    }
}

static void startReceiver(arq::config_Launcher& config /* why not const? */)
{
    // Obtain conversation ID from tranmitter
    auto convID = receiveConversationID(config.common.clientNames, config.common.serverNames);
    util::logInfo("Conversation ID {} received from transmitter", convID);

    const arq::config_AddressInfo& txerAddress = config.common.serverNames;
    const arq::config_AddressInfo& rxerAddress = config.common.clientNames;

    util::Endpoint dataChannel(
        rxerAddress.hostName,
        rxerAddress.serviceName,
        config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP ? util::SocketType::SCTP : util::SocketType::UDP);

    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        dataChannel.connectRetry(
            txerAddress.hostName, txerAddress.serviceName, util::SocketType::SCTP, 20, std::chrono::milliseconds(500));
    }

    // WJG: same considerations apply here as in Transmitter
    arq::TransmitFn txToServer;
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        txToServer = [&dataChannel](std::span<const std::byte> buffer) { return dataChannel.send(buffer); };
    }
    else {
        txToServer = [&dataChannel, &txerAddress](std::span<const std::byte> buffer) {
            return dataChannel.sendTo(buffer, txerAddress.hostName, txerAddress.serviceName);
        };
    }

    arq::ReceiveFn rxFromServer;
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        rxFromServer = [&dataChannel](std::span<std::byte> buffer) { return dataChannel.recv(buffer); };
    }
    else {
        rxFromServer = [&dataChannel](std::span<std::byte> buffer) { return dataChannel.recvFrom(buffer); };
    }

    // Use rxer.getPacket to get all sent packets...
    if (config.common.arqProtocol == arq::ArqProtocol::DUMMY_SCTP) {
        arq::Receiver rxer(convID, txToServer, rxFromServer, std::make_unique<arq::rs::DummySCTP>());
    }
    else if (config.common.arqProtocol == arq::ArqProtocol::STOP_AND_WAIT) {
        arq::Receiver rxer(convID, txToServer, rxFromServer, std::make_unique<arq::rs::StopAndWait>());
    }
}

int main(int argc, char** argv)
{
    /* Handle SIGPIPE for SCTP connection termination. This is something of a hack, but is fine for the purposes of this
     * project. */
    signal(SIGPIPE, SIG_IGN);

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