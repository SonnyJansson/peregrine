#include <chrono>
#include <iostream>

#include "logger.hpp"

using namespace std::chrono;

auto time_start = high_resolution_clock::now();

double time_now() {
    return duration<double>(high_resolution_clock::now() - time_start).count();
}

namespace logging {

string LogLevel_toStr(LogLevel level, bool with_color) {
    string name;
    switch(level) {
        case ANY:      {name = "ANY"; break;}
        case DEBUG:    {name = "DEBUG"; break;}
        case INFO:     {name = "INFO"; break;}
        case WARNING:  {name = "WARNING"; break;}
        case ERROR:    {name = "ERROR"; break;}
        case CRITICAL: {name = "CRITICAL"; break;}
    }

    if(with_color) {
        string color_code;
        switch(level) {
            case ANY:      {color_code = "\033[97m"; break;}
            case DEBUG:    {color_code = "\033[96m"; break;}
            case INFO:     {color_code = "\033[92m"; break;}
            case WARNING:  {color_code = "\033[93m"; break;}
            case ERROR:    {color_code = "\033[91m"; break;}
            case CRITICAL: {color_code = "\033[31m"; break;}
        }

        return color_code + name + "\033[0m";
    }

    return name;
}

Log::Log(string source, double time, LogLevel level, string message) {
    this->source = source;
    this->time = time;
    this->level = level;
    this->message = message;
}

void Sink::subscribe(Logger *logger) {
    logger->attach_sink(weak_from_this());
}

void Sink::subscribe(string logger) {
    subscribe(get(logger));
}

void Sink::unsubscribe(Logger *logger) {
    logger->deattach_sink(weak_from_this());
}

void Sink::unsubscribe(string logger) {
    unsubscribe(get(logger));
}

Logger::Logger(Logger *parent, string name, bool propagate) {
    this->parent = parent; // If parent is a null pointer, it is the root logger
    this->name = name;
    this->propagate = propagate;

    // self.children = {}
    // self.sinks = WeakSet()
}

void Logger::publish_log(Log log) {
    // std::vector<std::weak_ptr<Sink>>::iterator iter;
    // for(iter = sinks.begin(); iter != sinks.end(); ) {
    //     if(auto sink_ptr = (*iter).lock()) {
    //         sink_ptr->handle(log);
    //         ++iter;
    //     } else {
    //         iter = sinks.erase(iter);
    //     }
    // }

    for(std::weak_ptr<Sink> weak_ptr : sinks) {
        if(auto sink_ptr = weak_ptr.lock()) {
            sink_ptr->handle(log);
        } else {
            sinks.erase(weak_ptr);
        }
    }

    if(parent and propagate) {
        parent->publish_log(log);
    }
}

void Logger::attach_sink(std::weak_ptr<Sink> sink) {
    sinks.insert(sink);
}

void Logger::deattach_sink(std::weak_ptr<Sink> sink) {
    sinks.erase(sink);
}

void Logger::ensure_child(string logger_name) {
    if(children.find(logger_name) == children.end()) {
        children.try_emplace(logger_name, this, name + "/" + logger_name);
    }
}

Logger *Logger::get(string logger_name) {
    int delim_index = logger_name.find("/");

    string head = logger_name.substr(0, delim_index);
    ensure_child(head);

    if(delim_index == string::npos) {
        auto iter = children.find(head);
        return iter != children.end() ? &iter->second : nullptr;
    } else {
        string tail = logger_name.erase(0, delim_index + 1);

        auto iter = children.find(head);
        return iter != children.end() ? (&iter->second)->get(tail) : nullptr;
    }
}

void Logger::debug(string message) { publish_log(Log(name, time_now(), LogLevel::DEBUG, message)); }
void Logger::info(string message) { publish_log(Log(name, time_now(), LogLevel::INFO, message)); }
void Logger::warning(string message) { publish_log(Log(name, time_now(), LogLevel::WARNING, message)); }
void Logger::error(string message) { publish_log(Log(name, time_now(), LogLevel::ERROR, message)); }
void Logger::critical(string message) { publish_log(Log(name, time_now(), LogLevel::CRITICAL, message)); }

Logger *get(string logger_name) {return root_logger.get(logger_name);}

namespace sinks {

using std::string;
PrintSink::PrintSink(bool with_color) {
    this->with_color = with_color;
}

void PrintSink::handle(logging::Log log) {
    std::cout << std::setw(9) << std::setprecision(5) << std::fixed << log.time << " ";
    std::cout << "[" << LogLevel_toStr(log.level, with_color = this->with_color) << "] ";
    std::cout << log.message << " ";
    std::cout << "(" << log.source.substr(1) << ")" << std::endl;
}

FileSink::FileSink(string file_path) {
    using ios = std::ios;
    file_stream.open(file_path, ios::out | ios::app);
}

FileSink::~FileSink() {
    file_stream.close();
}

void FileSink::handle(logging::Log log) {
    file_stream << std::setw(9) << std::setprecision(5) << std::fixed << log.time << " ";
    file_stream << "[" << LogLevel_toStr(log.level) << "] ";
    file_stream << log.message << " ";
    file_stream << "(" << log.source.substr(1) << ")" << std::endl;
}

using nlohmann::json;

ZMQPubSink::ZMQPubSink(string host, unsigned int port, string topic): socket{ctx, ZMQ_PUB} {
    std::ostringstream ss;
    ss << "tcp://" << host << ":" << port;
    this->address = ss.str();
    this->topic = topic;
    socket.bind(address);
}

void ZMQPubSink::handle(logging::Log log) {
    json log_json;
    to_json(log_json, log);
    string log_str = log_json.dump();

    std::vector<zmq::message_t> msgs;

    msgs.emplace_back(topic.c_str(),
                      topic.size());

    msgs.emplace_back(log_str.c_str(),
                      log_str.size());

    zmq::send_multipart(socket, msgs);
}
}
}