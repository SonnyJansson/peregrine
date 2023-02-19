#include <memory>
#include "peregrine/logger.hpp"

using namespace logging::sinks;

extern "C" void c_client_log();

int main() {
    auto printer = std::make_shared<PrintSink>();
    printer->subscribe("main_a");

    c_client_log();
}
