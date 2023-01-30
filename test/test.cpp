#include <memory>
#include <unistd.h>
#include <peregrine/logger.hpp>

using namespace logging::sinks;

class LevelFilter : public logging::Filter {
public:
    LevelFilter(logging::LogLevel min_level): min_level(min_level) {}

    bool filter(logging::Log log) override {
        return log.level >= min_level;
    }
private:
    logging::LogLevel min_level;
};

int main() {
    auto main_logger = logging::get("main");
    auto subA_logger = main_logger->get("subA");
    auto subB_logger = logging::get("main/subB");

    auto printer = std::make_shared<PrintSink>();
    printer->subscribe(main_logger);

    auto printer_filter = std::make_shared<LevelFilter>(logging::LogLevel::WARNING);
    printer->add_filter(printer_filter);

    // auto file_printer = std::make_shared<FileSink>("out.txt");
    // file_printer->subscribe(main_logger);

    auto zmq_sink = std::make_shared<ZMQPubSink>("127.0.0.1", 5500, "log");
    zmq_sink->subscribe(main_logger);

    usleep(200000);

    auto child = logging::get("main/subA/child/child/child/child/");
    (void)child;
    // for(int i = 0; i < 100000; i++) {
    //     child->info("Test: " + std::to_string(i));
    // }

    main_logger->info("Hello, this is info!");
    main_logger->debug("Just some cheeky debug info!");
    usleep(270000);
    subA_logger->warning("I am not doing so well!");
    usleep(130000);
    printer->remove_filter(printer_filter);
    subA_logger->error("Oops! Crashing!");
    subB_logger->info("Uh oh...");
    main_logger->critical("Oops, also dying now! RIP!");

    return 0;
}
