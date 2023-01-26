#include <chrono>
#include <iostream>
#include "peregrine/logger.hpp"

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

Log::Log(string source, double time, LogLevel level, string message, string file_name, unsigned int file_no):
    source(source), time(time), level(level), message(message), file_name(file_name), file_no(file_no) {
}

void Filterer::add_filter(std::shared_ptr<Filter> filter) {
    filters.push_back(filter);
}

void Filterer::remove_filter(std::shared_ptr<Filter> filter) {
    filters.erase(std::remove(filters.begin(), filters.end(), filter), filters.end());
}

void Filterer::clear_filters() {
}

bool Filterer::filter(Log log) {
    for(const auto& f : filters) {
        if(!f->filter(log)) {
            return false;
        }
    }
    return true;
}

void Sink::subscribe(Logger *logger) {
    logger->add_sink(weak_from_this());
}

void Sink::subscribe(string logger) {
    subscribe(get(logger));
}

void Sink::unsubscribe(Logger *logger) {
    logger->remove_sink(weak_from_this());
}

void Sink::unsubscribe(string logger) {
    unsubscribe(get(logger));
}

Logger::Logger(Logger *parent, string name, bool propagate) {
    this->parent = parent; // If parent is a null pointer, it is the root logger
    this->name = name;
    this->propagate = propagate;
}

void Logger::create_log(LogLevel level, string message, string file_name, unsigned int file_no) {
    publish_log(Log(name, time_now(), level, message, file_name, file_no));
}

void Logger::publish_log(Log log) {
    auto filter_iter = filters.begin();
    auto sink_iter = sinks.begin();

    while(sink_iter != sinks.end()) {
        while(filter_iter != filters.end() && filter_iter->second <= sink_iter->second) {
            if(filter_iter->second > 0 && !filter_iter->first->filter(log)) {
                return;
            }
            ++filter_iter;
        }
        if(auto sink_ptr = sink_iter->first.lock()) {
            sink_ptr->handle(log);
            ++sink_iter;
        } else {
            sink_iter = sinks.erase(sink_iter);
        }
    }
}

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T> &t, const std::weak_ptr<U> &u) {
    return !(t.owner_before(u) || u.owner_before(t));
}

void Logger::add_sink(std::weak_ptr<Sink> sink) {
    _add_sink(sink, 0);
}

void Logger::_add_sink(std::weak_ptr<Sink> sink, unsigned int depth) {
    auto insert_position = std::find_if(std::begin(sinks), std::end(sinks), [depth](auto p) { return p.second == depth; });
    sinks.insert(insert_position, std::pair(sink, depth));
    for(auto& [child_name, child] : children) {
        child._add_sink(sink, depth + 1);
    }
}

void Logger::remove_sink(std::weak_ptr<Sink> sink) {
    _remove_sink(sink, 0);
}

void Logger::_remove_sink(std::weak_ptr<Sink> sink, unsigned int depth) {
    auto remove_position = std::remove_if(sinks.begin(), sinks.end(),
                                          [&sink, depth](auto p) { return depth == p.second && equals(p.first, sink); });
    sinks.erase(remove_position);

    for(auto& [child_name, child] : children) {
        child._remove_sink(sink, depth + 1);
    }
}

void Logger::ensure_child(string logger_name) {
    if(children.find(logger_name) == children.end()) {
        children.try_emplace(logger_name, this, name + "/" + logger_name);
    }
}

void Logger::add_filter(std::shared_ptr<Filter> filter) {
    _add_filter(filter, 0);
}

void Logger::_add_filter(std::shared_ptr<Filter> filter, unsigned int depth) {
    auto insert_position = std::find_if(std::begin(filters), std::end(filters), [depth](auto p) { return p.second == depth; });
    filters.insert(insert_position, std::pair(filter, depth));

    for(auto& [child_name, child] : children) {
        child._add_filter(filter, depth + 1);
    }
}

void Logger::remove_filter(std::shared_ptr<Filter> filter) {
    _remove_filter(filter, 0);
}

void Logger::_remove_filter(std::shared_ptr<Filter> filter, unsigned int depth) {
    auto remove_position = std::remove_if(filters.begin(), filters.end(),
                                          [&filter, depth](auto p) { return p.second == depth && p.first == filter; });
    filters.erase(remove_position);

    for(auto& [child_name, child] : children) {
        child._remove_filter(filter, depth + 1);
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


Logger *get(string logger_name) {return root_logger.get(logger_name);}

namespace sinks {

using std::string;
PrintSink::PrintSink(bool with_color) {
    this->with_color = with_color;
}

void PrintSink::handle(logging::Log log) {
    if(!filter(log)) {
        return;
    }
    std::cout << std::setw(9) << std::setprecision(5) << std::fixed << log.time << " ";
    std::cout << "[" << LogLevel_toStr(log.level, with_color = this->with_color) << "] ";
    std::cout << log.message << " ";
    std::cout << "(" << log.source.substr(1) << ") ";
    std::cout << "[" << log.file_name << ":" << log.file_no << "]" << std::endl;
}

FileSink::FileSink(string file_path) {
    using ios = std::ios;
    file_stream.open(file_path, ios::out | ios::app);
}

FileSink::~FileSink() {
    file_stream.close();
}

void FileSink::handle(logging::Log log) {
    if(!filter(log)) {
        return;
    }
    file_stream << std::setw(12) << std::setprecision(8) << std::fixed << log.time << " ";
    file_stream << "[" << LogLevel_toStr(log.level) << "] ";
    file_stream << log.message << " ";
    file_stream << "(" << log.source.substr(1) << ")\n"; // << std::endl;
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
