#include <memory>
#include <unistd.h>

#include "logger.hpp"

using namespace logging::sinks;

int main() {
    auto main_logger = logging::get("main");
    auto subA_logger = main_logger->get("subA");
    auto subB_logger = logging::get("main/subB");

    auto printer = std::make_shared<PrintSink>();
    printer->subscribe(main_logger);

    // auto file_printer = std::make_shared<FileSink>("out.txt");
    // file_printer->subscribe(main_logger);

    auto zmq_sink = std::make_shared<ZMQPubSink>("127.0.0.1", 5500, "log");
    zmq_sink->subscribe(main_logger);

    usleep(200000);

    auto child = logging::get("main/subA/child/child/child/child/");
    // for(int i = 0; i < 100000; i++) {
    //     child->info("Test: " + std::to_string(i));
    // }

    main_logger->info("Hello, this is info!");
    main_logger->debug("Just some cheeky debug info!");
    usleep(270000);
    subA_logger->warning("I am not doing so well!");
    usleep(130000);
    printer.reset();
    subA_logger->error("Oops! Crashing!");
    subB_logger->info("Uh oh...");
    main_logger->critical("Oops, also dying now! RIP!");

    return 0;
}
