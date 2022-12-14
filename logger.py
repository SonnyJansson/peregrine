from enum import Enum
import typing

import zmq

class LogLevel(Enum):
    ANY      = 0
    DEBUG    = 1
    INFO     = 2
    WARNING  = 3
    ERROR    = 4
    CRITICAL = 5

    def pretty_name(self, with_color = False):
        name = {
            LogLevel.ANY  : "Not Set",
            LogLevel.DEBUG   : "Debug",
            LogLevel.INFO    : "Info",
            LogLevel.WARNING : "Warning",
            LogLevel.ERROR   : "Error",
            LogLevel.CRITICAL: "CRITICAL",
        }[self]

        if with_color:
            color = {
                LogLevel.ANY  : "\033[97m",
                LogLevel.DEBUG   : "\033[96m",
                LogLevel.INFO    : "\033[92m",
                LogLevel.WARNING : "\033[93m",
                LogLevel.ERROR   : "\033[91m",
                LogLevel.CRITICAL: "\033[31m",
            }[self]
            name = color + name + "\033[0m"

        return ('[' + name + ']')

class Log():
    def __init__(self, source: str, level: LogLevel, message: str):
        self.source = source
        self.level = level
        self.message = message

class LogMedium():
    def __init__(self):
        pass

    # Furnishes source with necessary 
    def equip(self, source: LogSource):
        pass

    def connect(self, source: LogSource, sink: LogSink):
        pass

class LogMediumZMQ(LogMedium):
    def __init__(self, address: str):
        self.ctx = zmq.Context()

    def equip_source(self, source: LogSource):
        pass

    def subscribe(self, source: LogSource, sink: LogSink):
        pass

class LogSource():
    def __init__(self, name: str):
        self.name = name
        self.callbacks = [[] for _ in LogLevel]

    def add_subscriber(self, level: LogLevel, callback):
        self.callbacks[level.value].append(callback)

    def publish_log(self, log):
        for level in LogLevel:
            if level.value > log.level.value: # Could be optimised so it is not looped over to begin with
                continue
            for callback in self.callbacks[level.value]:
                callback(log)

    # User commands for creating logs
    def debug(self, message: str):
        self.publish_log(Log(self.name, LogLevel.DEBUG, message))
    def info(self, message: str):
        self.publish_log(Log(self.name, LogLevel.INFO, message))
    def warning(self, message: str):
        self.publish_log(Log(self.name, LogLevel.WARNING, message))
    def error(self, message: str):
        self.publish_log(Log(self.name, LogLevel.ERROR, message))
    def critical(self, message: str):
        self.publish_log(Log(self.name, LogLevel.CRITICAL, message))
        
class LogSink():
    def __init__(self):
        pass
        
    def subscribe(self, source: LogSource, level: LogLevel, medium = LogMedium):
        # source.add_subscriber(level, self.receive_log)
        medium.subscribe(self, source, level)

    def receive_log(self, log):
        pass

# Different types of sources/sinks/loggers
class Logger(LogSource, LogSink):
    def __init__(self, name: str):
        LogSource.__init__(self, name)
        LogSink.__init__(self)

    # Pass on the logs from below, adding own name to the source
    def receive_log(self, log):
        log.source = "{}.{}".format(self.name, log.source)
        self.publish_log(log)

class LogSinkPrinter(LogSink):
    def __init__(self, with_color = True):
        self.with_color = with_color
        self.max_tag_length = max([
            len(level.pretty_name(with_color = with_color)) for level in LogLevel
        ])

    def receive_log(self, log):
        print("{tag:<{tag_len}} {message} ({source})".format(
            tag = log.level.pretty_name(with_color = self.with_color),
            tag_len = self.max_tag_length,
            message = log.message,
            source = log.source
        ))


if __name__ == "__main__":
    childlogger = Logger("child")

    logger = Logger("main")
    logger.subscribe(childlogger, LogLevel.ANY)

    printer = LogSinkPrinter(with_color = True)
    printer.subscribe(childlogger, LogLevel.ANY)

    logger.info("This is just some info!")
    logger.info("This is just some info!")
    logger.info("This is just some info!")
    logger.debug("This is some good debug info")
    logger.info("This is just some info!")
    logger.info("This is just some info!")
    logger.debug("This is some good debug info")

    childlogger.warning("Oopsies!")
    childlogger.error("It got worse!")

    logger.critical("Shutting down!")


