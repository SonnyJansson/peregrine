#include <memory>
#include "peregrine/logger.hpp"

using namespace logging::sinks;

extern "C" void c_client_log();

int main() {
    auto printer = std::make_shared<PrintSink>();
    printer->subscribe("main_a");

    auto zmq_sink = std::make_shared<ZMQPubSink>("127.0.0.1", 5500, "log");
    zmq_sink->subscribe("main_b");

    c_client_log();
}
