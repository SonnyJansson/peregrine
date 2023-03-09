#include <string>

#include "peregrine/logger.hpp"
#include "peregrine/logger_c.h"

extern "C" {

Logger *root_get(char *name) {
    logging::Logger *cpp_logger = logging::get(std::string(name));
    return reinterpret_cast<struct logger *>(cpp_logger);
}

Logger *logger_get(Logger *parent, char *name) {
    logging::Logger *cpp_parent = reinterpret_cast<logging::Logger *>(parent);
    logging::Logger *cpp_logger = cpp_parent->get(std::string(name));
    return reinterpret_cast<struct logger *>(cpp_logger);
}

void send_log(Logger *logger, LogLevel level, char *file_name, unsigned int line_no, char *message) {
    logging::Logger *cpp_logger = reinterpret_cast<logging::Logger *>(logger);
    cpp_logger->create_log(logging::LogLevel(level), file_name, line_no, message);
}

}
