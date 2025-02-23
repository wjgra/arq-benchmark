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

#include "arq/common/input_buffer.hpp"
#include "arq/receiver.hpp"
#include "arq/resequencing_buffers/dummy_sctp_rs.hpp"
#include "arq/resequencing_buffers/go_back_n_rs.hpp"
#include "arq/resequencing_buffers/selective_repeat_rs.hpp"
#include "arq/resequencing_buffers/stop_and_wait_rs.hpp"
#include "arq/retransmission_buffers/dummy_sctp_rt.hpp"
#include "arq/retransmission_buffers/go_back_n_rt.hpp"
#include "arq/retransmission_buffers/selective_repeat_rt.hpp"
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

// Program option strings
#define PROG_OPTION_HELP "help"
#define PROG_OPTION_LOGGING "logging"
#define PROG_OPTION_SERVER_ADDR "server-addr"
#define PROG_OPTION_SERVER_PORT "server-port"
#define PROG_OPTION_CLIENT_ADDR "client-addr"
#define PROG_OPTION_CLIENT_PORT "client-port"
#define PROG_OPTION_LAUNCH_SERVER "launch-server"
#define PROG_OPTION_LAUNCH_CLIENT "launch-client"
#define PROG_OPTION_TX_PKT_NUM "tx-pkt-num"
#define PROG_OPTION_TX_PKT_INTERVAL "tx-pkt-interval"
#define PROG_OPTION_ARQ_TIMEOUT "arq-timeout"
#define PROG_OPTION_ARQ_PROTOCOL "arq-protocol"
#define PROG_OPTION_ARQ_WINDOW_SZ "window-size"

using namespace std::string_literals;
// clang-format off
auto programOptionData = std::to_array<ProgramOption>({
    {PROG_OPTION_HELP,            std::monostate{},                                  "display help message"},
    {PROG_OPTION_LOGGING,         std::to_underlying(util::LOGGING_LEVEL_INFO),      util::Logger::helpText()},
    {PROG_OPTION_SERVER_ADDR,     "127.0.0.1"s,                                      "server IPv4 address"},
    {PROG_OPTION_SERVER_PORT,     "65534"s,                                          "server port"},
    {PROG_OPTION_CLIENT_ADDR,     "127.0.0.1"s,                                      "client IPv4 address"},
    {PROG_OPTION_CLIENT_PORT,     "65535"s,                                          "client port"},
    {PROG_OPTION_LAUNCH_SERVER,   std::monostate{},                                  "start server thread"},
    {PROG_OPTION_LAUNCH_CLIENT,   std::monostate{},                                  "start client thread"},
    {PROG_OPTION_TX_PKT_NUM,      uint16_t{10},                                      "number of packets to transmit"},
    {PROG_OPTION_TX_PKT_INTERVAL, uint16_t{10},                                      "ms between transmitted packets"},
    {PROG_OPTION_ARQ_TIMEOUT,     uint16_t{50},                                      "ARQ timeout in ms"},
    {PROG_OPTION_ARQ_PROTOCOL,    arqProtocolToString(arq::ArqProtocol::DUMMY_SCTP), "ARQ protocol to use"},
    {PROG_OPTION_ARQ_WINDOW_SZ,   uint16_t{100},                                     "window size for GBN and SR ARQ"}
});
// clang-format on

const uint32_t socket_rx_timeout_seconds = 10;

static auto generateOptionsDescription()
{
    namespace po = boost::program_options;
    po::options_description description{"Usage"};

    // Add each possible option to the description object
    for (const auto& option : programOptionData) {
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
            throw std::logic_error("default value incorrectly specified in programOptionData");
        }
    }

    return description;
}

struct HelpException : public std::invalid_argument {
    explicit HelpException(const std::string& what) : std::invalid_argument(what){};
};

static arq::ArqProtocol getArqProtocolFromStr(const std::string& input)
{
    if (input == arqProtocolToString(arq::ArqProtocol::DUMMY_SCTP)) {
        return arq::ArqProtocol::DUMMY_SCTP;
    }
    else if (input == arqProtocolToString(arq::ArqProtocol::STOP_AND_WAIT)) {
        return arq::ArqProtocol::STOP_AND_WAIT;
    }
    else if (input == arqProtocolToString(arq::ArqProtocol::GO_BACK_N)) {
        return arq::ArqProtocol::GO_BACK_N;
    }
    else if (input == arqProtocolToString(arq::ArqProtocol::SELECTIVE_REPEAT)) {
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

    // Parse each possible option in turn
    try {
        if (vm.contains(PROG_OPTION_HELP)) {
            throw HelpException("help requested");
        }

        if (vm.contains(PROG_OPTION_LOGGING)) {
            const auto newLevel{static_cast<util::LoggingLevel>(vm[PROG_OPTION_LOGGING].as<uint16_t>())};
            util::Logger::setLoggingLevel(newLevel);
            util::logInfo("logging level set to {}", util::Logger::loggingLevelStr());
        }

        if (vm.contains(PROG_OPTION_SERVER_ADDR)) {
            config.common.serverNames.hostName = vm[PROG_OPTION_SERVER_ADDR].as<std::string>();
            util::logInfo("server address set to {}", config.common.serverNames.hostName);
        }
        else {
            throw HelpException("server-addr not provided");
        }

        if (vm.contains(PROG_OPTION_SERVER_PORT)) {
            config.common.serverNames.serviceName = vm[PROG_OPTION_SERVER_PORT].as<std::string>();
            util::logInfo("server port set to {}", config.common.serverNames.serviceName);
        }
        else {
            throw HelpException("server-port not provided");
        }

        if (vm.contains(PROG_OPTION_CLIENT_ADDR)) {
            config.common.clientNames.hostName = vm[PROG_OPTION_CLIENT_ADDR].as<std::string>();
            util::logInfo("client address set to {}", config.common.clientNames.hostName);
        }
        else {
            throw HelpException("client-addr not provided");
        }

        if (vm.contains(PROG_OPTION_CLIENT_PORT)) {
            config.common.clientNames.serviceName = vm[PROG_OPTION_CLIENT_PORT].as<std::string>();
            util::logInfo("client port set to {}", config.common.clientNames.serviceName);
        }
        else {
            throw HelpException("client-addr not provided");
        }

        if (vm.contains(PROG_OPTION_LAUNCH_SERVER)) {
            config.server = arq::config_Server{}; // No content currently
        }

        if (vm.contains(PROG_OPTION_LAUNCH_CLIENT)) {
            config.client = arq::config_Client{}; // No content currently
        }

        if (vm.contains(PROG_OPTION_TX_PKT_NUM) && config.server.has_value()) {
            config.server->txPkts.num = vm[PROG_OPTION_TX_PKT_NUM].as<uint16_t>();
        }

        if (vm.contains(PROG_OPTION_TX_PKT_INTERVAL) && config.server.has_value()) {
            config.server->txPkts.msInterval = vm[PROG_OPTION_TX_PKT_INTERVAL].as<uint16_t>();
        }

        if (vm.contains(PROG_OPTION_ARQ_TIMEOUT) && config.server.has_value()) {
            config.server->arqTimeout = vm[PROG_OPTION_ARQ_TIMEOUT].as<uint16_t>();
        }

        if (vm.contains(PROG_OPTION_ARQ_PROTOCOL)) {
            config.common.arqProtocol = getArqProtocolFromStr(vm[PROG_OPTION_ARQ_PROTOCOL].as<std::string>());
        }

        if (vm.contains(PROG_OPTION_ARQ_WINDOW_SZ)) {
            config.common.windowSize = vm[PROG_OPTION_ARQ_WINDOW_SZ].as<uint16_t>();
        }

        if (config.server.has_value()) {
            util::logInfo(
                "server configured to transmit {} packets with interval {} ms using ARQ protocol {} with initial timeout {} ms",
                config.server->txPkts.num,
                config.server->txPkts.msInterval,
                arqProtocolToString(config.common.arqProtocol),
                config.server->arqTimeout);
        }
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

    // Send end of Tx packet (WJG: should have function to make one of these)
    arq::DataPacket endOfTxPacket{};
    endOfTxPacket.updateConversationID(1);
    endOfTxPacket.updateDataLength(0);
    assert(endOfTxPacket.isEndOfTx());
    txerSendPacket(std::move(endOfTxPacket));
}

static void startTransmitter(const arq::config_Launcher& config)
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

    // Add Rx timeout in case last ACK is lost
    if (config.common.arqProtocol != arq::ArqProtocol::DUMMY_SCTP &&
        !dataChannel.setRecvTimeout(socket_rx_timeout_seconds, 0)) {
        throw std::runtime_error("failed to set data channel Rx timeout");
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
            std::make_unique<arq::rt::StopAndWait>(std::chrono::milliseconds(config.server->arqTimeout)));

        auto txerSend = [&txer](arq::DataPacket&& pkt) { txer.sendPacket(std::move(pkt)); };

        transmitPackets(txerSend, config.server->txPkts.num, config.server->txPkts.msInterval);
    }
    else if (config.common.arqProtocol == arq::ArqProtocol::GO_BACK_N) {
        auto windowSize = config.common.windowSize;
        if (!config.common.windowSize.has_value()) {
            windowSize = 100;
            util::logWarning("Unspecified window size - using default of {}", windowSize.value());
        }

        arq::Transmitter txer(convID,
                              txToClient,
                              rxFromClient,
                              std::make_unique<arq::rt::GoBackN>(windowSize.value(),
                                                                 std::chrono::milliseconds(config.server->arqTimeout)));

        auto txerSend = [&txer](arq::DataPacket&& pkt) { txer.sendPacket(std::move(pkt)); };

        transmitPackets(txerSend, config.server->txPkts.num, config.server->txPkts.msInterval);
    }
    else if (config.common.arqProtocol == arq::ArqProtocol::SELECTIVE_REPEAT) {
        auto windowSize = config.common.windowSize;
        if (!config.common.windowSize.has_value()) {
            windowSize = 100;
            util::logWarning("Unspecified window size - using default of {}", windowSize.value());
        }

        arq::Transmitter txer(convID,
                              txToClient,
                              rxFromClient,
                              std::make_unique<arq::rt::SelectiveRepeat>(
                                  windowSize.value(), std::chrono::milliseconds(config.server->arqTimeout)));

        auto txerSend = [&txer](arq::DataPacket&& pkt) { txer.sendPacket(std::move(pkt)); };

        transmitPackets(txerSend, config.server->txPkts.num, config.server->txPkts.msInterval);
    }
    else {
        util::logError("Unsupported ARQ protocol: {}", arqProtocolToString(config.common.arqProtocol));
    }
}

static void startReceiver(const arq::config_Launcher& config)
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

    // Add Rx timeout in case last packets are lost
    if (config.common.arqProtocol != arq::ArqProtocol::DUMMY_SCTP &&
        !dataChannel.setRecvTimeout(socket_rx_timeout_seconds, 0)) {
        throw std::runtime_error("failed to set data channel Rx timeout");
    }

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
    else if (config.common.arqProtocol == arq::ArqProtocol::GO_BACK_N) {
        arq::Receiver rxer(convID, txToServer, rxFromServer, std::make_unique<arq::rs::GoBackN>());
    }
    else if (config.common.arqProtocol == arq::ArqProtocol::SELECTIVE_REPEAT) {
        // temp add window config
        arq::Receiver rxer(convID, txToServer, rxFromServer, std::make_unique<arq::rs::SelectiveRepeat>(100));
    }
    else {
        util::logError("Unsupported ARQ protocol: {}", arqProtocolToString(config.common.arqProtocol));
    };
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

    util::Logger::enableTimestamps();

    std::thread txThread, rxThread;

    if (cfg.server.has_value()) {
        txThread = std::thread(startTransmitter, std::ref(cfg));
    }

    if (cfg.client.has_value()) {
        rxThread = std::thread(startReceiver, std::ref(cfg));
    }

    bool txJoined = !cfg.server.has_value();
    bool rxJoined = !cfg.client.has_value();
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