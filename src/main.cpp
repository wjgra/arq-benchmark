#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "util/logging.hpp"

static bool parseOptions(int argc, char** argv) {
    // Init options parser
    namespace po = boost::program_options;
    po::options_description description{"Usage"};
    description.add_options()
        ("help", "produce help message")
        ("logging", po::value<int>()->default_value(util::LOGGING_LEVEL_INFO), util::Logger::helpText().c_str());
    
    bool success = true;
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);    
        
        if (vm.contains("help")) {
            throw std::invalid_argument("help requested");
        }
        
        // Parse logging level
        if (vm.contains("logging")) {
            const auto newLevel = static_cast<util::LoggingLevel>(vm["logging"].as<int>());
            util::Logger::setLoggingLevel(newLevel);
        }
        util::logInfo("logging level set to {}", util::Logger::loggingLevelStr());

        // Parse other arguments...
    }
    catch (const std::exception& e) {
        std::println("Displaying usage information ({})", e.what());
        // Output usage information
        std::cout << description << '\n';
        success = false;
    }
    return success;
}

int main(int argc, char** argv) {
    if (!parseOptions(argc, argv)) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}