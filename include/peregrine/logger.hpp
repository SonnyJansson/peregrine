#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include "zmq_addon.hpp"

double _time_now();
namespace logging {

using namespace std;
using json = nlohmann::json;

enum LogLevel {
    ANY      = 0,
    DEBUG    = 1,
    INFO     = 2,
    WARNING  = 3,
    ERROR    = 4,
    CRITICAL = 5   
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogLevel, {
        {ANY, "any"},
        {DEBUG, "debug"},
        {INFO, "info"},
        {WARNING, "warning"},
        {ERROR, "error"},
        {CRITICAL, "critical"}
});

string LogLevel_toStr(LogLevel level, bool with_color = false);

class Log {
public:
    string source;
    double time;
    LogLevel level;
    string message; 
    string file_name;
    unsigned int file_no;

    Log(string source, double time, LogLevel level, string message, string file_name, unsigned int file_no);
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Log, source, time, level, message);

class Logger;

class Filter {
public:
    virtual bool filter(Log log) = 0;
};

class Filterer {
public:
    void add_filter(std::shared_ptr<Filter>);
    void remove_filter(std::shared_ptr<Filter>);
    void clear_filters();
protected:
    bool filter(Log log);
    std::vector<std::shared_ptr<Filter>> filters;
};

class Sink : public Filterer, public std::enable_shared_from_this<Sink> {
public:
    Sink() {}

    // Subscribe to logger
    void subscribe(Logger *logger);
    // Handle the case where a string is given instead of Logger object
    void subscribe(string logger);

    // Unsubscribe from logger
    void unsubscribe(Logger *logger);
    // Handle the case where a string is given instead of Logger object
    void unsubscribe(string logger);

    virtual void handle(Log log) = 0;
private:
    // std::shared_ptr<Sink> access_ptr{this};
};

class Logger: public Filterer {
public:
    Logger(Logger *parent, string name, bool propagate = true);

    // Get a logger
    Logger *get(string logger_name);

    void add_sink(std::weak_ptr<Sink> sink);
    void remove_sink(std::weak_ptr<Sink> sink);

    void add_filter(std::shared_ptr<Filter>);
    void remove_filter(std::shared_ptr<Filter>);

    // Use user-interface commands debug, info, warning, error, critical instead
    template<typename S, typename... Args>
    void create_log(LogLevel level, string file_name, unsigned int file_no, const S& format, Args&&... args) {
        publish_log(Log(name, _time_now(), level, fmt::format(format, args...), file_name, file_no));
    }

    // User commands for creating logs
    // void debug(string message);
    // void info(string message);
    // void warning(string message);
    // void error(string message);
    // void critical(string message);

    bool propagate;
private:
    // add_sink is porcelain for _attach_sink
    void _add_sink(std::weak_ptr<Sink> sink, unsigned int depth);
    // remove_sink is porcelain for _remove_sink
    void _remove_sink(std::weak_ptr<Sink> sink, unsigned int depth);

    void _add_filter(std::shared_ptr<Filter>, unsigned int depth);
    void _remove_filter(std::shared_ptr<Filter>, unsigned int depth);


    Logger *parent;
    string name;

    std::map<string, Logger> children;

    // Vector of pairs of sinks together with a number describing which (grand)parent
    // is attached. 0: self, 1: parent, 2: grandparent, etc.
    std::vector<std::pair<std::weak_ptr<Sink>, unsigned int>> sinks;
    std::vector<std::pair<std::shared_ptr<Filter>, unsigned int>> filters;

    void ensure_child(string logger_name);
    void publish_log(Log log);
};

static Logger root_logger = Logger(nullptr, "");

Logger *get(string logger_name);

namespace sinks {

using std::string;

class PrintSink: public Sink {
private:
    bool with_color;
public:
    PrintSink(bool with_color = true);
    void handle(logging::Log log) override;
};

class FileSink: public Sink {
private:
    std::ofstream file_stream;
public:
    FileSink(string file_path);
    ~FileSink();
    void handle(logging::Log log) override;
};

class ZMQPubSink: public Sink {
private:
    string address;
    string topic;

    zmq::context_t ctx;
    zmq::socket_t socket;
public:
    ZMQPubSink(string host, unsigned int port, string topic);
    void handle(logging::Log log) override;
};
}
}

// If using modern C++, use __VAOPT__ to remove comma if __VA_ARGS__ are empty
// Additionally, modern C++ allows us to always compile-time check the format string using FMT_STRING
#if _cplusplus >= 202002L
#define debug(format, ...)    create_log(logging::LogLevel::DEBUG, __FILE__, __LINE__, FMT_STRING(format) __VA_OPT__(,) __VA_ARGS__)
#define info(format, ...)     create_log(logging::LogLevel::INFO, __FILE__, __LINE__, FMT_STRING(format) __VA_OPT__(,) __VA_ARGS__)
#define warning(format, ...)  create_log(logging::LogLevel::WARNING, __FILE__, __LINE__, FMT_STRING(format) __VA_OPT__(,) __VA_ARGS__)
#define error(format, ...)    create_log(logging::LogLevel::ERROR, __FILE__, __LINE__, FMT_STRING(format) __VA_OPT__(,) __VA_ARGS__)
#define critical(format, ...) create_log(logging::LogLevel::CRITICAL, __FILE__, __LINE__, FMT_STRING(format) __VA_OPT__(,) __VA_ARGS__)
#else // Otherwise, use old (not as standardised) ## before __VA_ARGS__ to remove comma
#define debug(format, ...)    create_log(logging::LogLevel::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define info(format, ...)     create_log(logging::LogLevel::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define warning(format, ...)  create_log(logging::LogLevel::WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define error(format, ...)    create_log(logging::LogLevel::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define critical(format, ...) create_log(logging::LogLevel::CRITICAL, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#endif // __LOGGER_H__
