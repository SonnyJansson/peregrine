#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "json.hpp"
#include "zmq_addon.hpp"

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

    Log(string source, double time, LogLevel level, string message);
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Log, source, time, level, message);

class Logger;

class Filter {
public:
    virtual bool filter(Log log) {}
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

    virtual void handle(Log log) {  }
private:
    // std::shared_ptr<Sink> access_ptr{this};
};

class Logger: public Filterer {
public:
    Logger(Logger *parent, string name, bool propagate = true);

    // Get a logger
    Logger *get(string logger_name);

    void attach_sink(std::weak_ptr<Sink> sink);
    void deattach_sink(std::weak_ptr<Sink> sink);

    // User commands for creating logs
    void debug(string message);
    void info(string message);
    void warning(string message);
    void error(string message);
    void critical(string message);

    bool propagate;
private:
    Logger *parent;
    string name;

    std::map<string, Logger> children;

    std::vector<std::weak_ptr<Sink>> sinks;

    void publish_log(Log log);
    void ensure_child(string logger_name);
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

#endif // __LOGGER_H__
