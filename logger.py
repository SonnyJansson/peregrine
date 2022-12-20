from enum import IntEnum
import json
import threading
import typing

import time
import zmq

ZMQ_PORT = 5556
ZMQ_TIMEOUT_MS = 100

log_manager = None

start_time = time.time()

# Would be replaced by PLib_GetTime call
def time_now():
    return (time.time() - start_time)

# LogLevel has two (related, but different) applications:
#  1. Allow loggers to specify the severity of messages
#  2. Sinks use them to filter messages, based on how the subscription was configured
# Perhaps the latter use case could be extracted into a different "Filter" class, allowing
# sinks to more accurately filter what logs they are interested in and removing the need for "ANY"
class LogLevel(IntEnum):
    ANY      = 0
    DEBUG    = 1
    INFO     = 2
    WARNING  = 3
    ERROR    = 4
    CRITICAL = 5

    def pretty_name(self, with_color = False):
        name = {
            LogLevel.ANY     : "Not Set",
            LogLevel.DEBUG   : "Debug",
            LogLevel.INFO    : "Info",
            LogLevel.WARNING : "Warning",
            LogLevel.ERROR   : "Error",
            LogLevel.CRITICAL: "CRITICAL",
        }[self]

        if with_color:
            color = {
                LogLevel.ANY     : "\033[97m",
                LogLevel.DEBUG   : "\033[96m",
                LogLevel.INFO    : "\033[92m",
                LogLevel.WARNING : "\033[93m",
                LogLevel.ERROR   : "\033[91m",
                LogLevel.CRITICAL: "\033[31m",
            }[self]
            name = color + name + "\033[0m"

        return ('[' + name + ']')

class Log():
    def __init__(self, source: [str], time: float, level: LogLevel, message: str):
        self.source = source
        self.time = time
        self.level = level
        self.message = message

    def from_json(json_str: str):
        d = json.loads(json_str)
        return Log(d["source"], d["time"], LogLevel(d["level"]), d["message"])

    def to_json(self):
        return json.dumps(self.__dict__)

class LogSink():
    def __init__(self):
        pass
        
    # def subscribe(self, logger_name: str, level: LogLevel, medium = LogMedium):
    #     # source.add_subscriber(level, self.receive_log)
    #     medium.subscribe(self, logger, level)

    def receive_log(self, log):
        pass

    def subscribe(self, logger_name: str, level: LogLevel, medium = "default"):
        log_manager.mediums[medium].subscribe(self, logger_name, level)

    def unsubscribe(self, logger_name: str, medium = "default"):
        log_manager.mediums[medium].unsubscribe(self, logger_name)

class Logger(LogSink):
    def __init__(self, manager, name: str):
        LogSink.__init__(self)
        self.manager = manager
        self.name = name
        # self.callbacks = [[] for _ in LogLevel]
        self.callbacks = []

    def add_subscriber(self, level: LogLevel, callback):
        self.callbacks.append(callback)

    # Pass on the logs from below, adding own name to the source
    def receive_log(self, log):
        log.source.insert(0, self.name)
        self.publish_log(log)

    def publish_log(self, log):
        for callback in self.callbacks:
            callback(log)

    def equip(self, medium: str):
        log_manager.mediums[medium].equip(self)

    # User commands for creating logs
    def debug(self, message: str):
        self.publish_log(Log([self.name], time_now(), LogLevel.DEBUG, message))
    def info(self, message: str):
        self.publish_log(Log([self.name], time_now(), LogLevel.INFO, message))
    def warning(self, message: str):
        self.publish_log(Log([self.name], time_now(), LogLevel.WARNING, message))
    def error(self, message: str):
        self.publish_log(Log([self.name], time_now(), LogLevel.ERROR, message))
    def critical(self, message: str):
        self.publish_log(Log([self.name], time_now(), LogLevel.CRITICAL, message))

class LogMedium():
    def __init__(self):
        self.loggers = []

    # Register a new logger in the medium
    def add_logger(self, logger: Logger):
        self.loggers += logger
        pass

    # Publish a log from a given logger 
    def publish_log(self, logger, log):
        pass

    # Connect a logger to a sink
    def connect(self, source: Logger, sink: LogSink):
        pass

class LogMediumZMQ(LogMedium):
    def __init__(self, port: int):
        super().__init__()
        self.ctx = zmq.Context()

        self.pub_socket = self.ctx.socket(zmq.PUB)
        self.pub_socket.bind("tcp://*:{}".format(port))

        self.sub_socket = self.ctx.socket(zmq.SUB)
        self.sub_socket.connect("tcp://localhost:{}".format(port))

        # Create a poller to poll messages
        self.poller = zmq.Poller()
        self.poller.register(self.sub_socket, zmq.POLLIN)

        self.zmq_lock = threading.Lock()

        self.subscription_thread = None
        self.exist_subscriptions = False # Do there exist any subscriptions?

        # Dictionary of logger name to dict sink to level
        self.subscriptions = {}

    def stop(self):
        self.exist_subscriptions = False
        self.subscription_thread.join()

    # Function that updates all sinks once new 
    def subscription_handler(self):
        while self.exist_subscriptions:
            self.propagate(timeout = ZMQ_TIMEOUT_MS)

    def propagate(self, timeout = ZMQ_TIMEOUT_MS):
        while self.sub_socket in dict(self.poller.poll(timeout)):
            log_topic = None
            log_json = None

            # We need to check that there is still stuff to read after acquiring the lock
            # since someone could have stolen our stuff in between! (It happens a lot otherwise)
            with self.zmq_lock:
                if self.sub_socket in dict(self.poller.poll(timeout)):
                    log_topic = bytes.decode(self.sub_socket.recv())
                    log_json = bytes.decode(self.sub_socket.recv())

            if not (log_topic and log_json):
                continue

            # [log_topic, log_json] = list(map(bytes.decode, multipart_resp))
            log = Log.from_json(log_json)

            for (sink, level) in self.subscriptions[log_topic].items():
                # Give log to sink if it is of a high enough level
                if log.level >= level:
                    sink.receive_log(log)

    def publish_log(self, log):
        # Propagate messages if any exist, but do not wait if there are none
        self.propagate(timeout = 0)

        with self.zmq_lock:
            self.pub_socket.send_multipart([
                log.source[0].encode('ascii'),
                log.to_json().encode('ascii')
            ])

    def equip(self, logger: Logger):
        logger.callbacks.append(self.publish_log)

    def subscribe(self, sink: LogSink, logger_name: str, level: LogLevel):
        if logger_name not in self.subscriptions:
            self.subscriptions[logger_name] = {}
            self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, logger_name)
        self.subscriptions[logger_name][sink] = level

        if not self.subscription_thread:
            self.exist_subscriptions = True
            self.subscription_thread = threading.Thread(target = self.subscription_handler)
            self.subscription_thread.start()

    def unsubscribe(self, sink: LogSink, logger_name: str):
        if logger_name not in self.subscriptions:
            return

        self.subscriptions[logger_name].pop(sink, None)

        # If the topic has no more subscribers, stop listening for it on the socket
        if not self.subscriptions[logger_name]:
            self.sub_socket.setsockopt_string(zmq.UNSUBSCRIBE, logger_name)
            self.subscriptions.pop(logger_name)

        # Check if there are any subscriptions whatsoever
        if not self.subscriptions:
            self.exist_subscriptions = False

class LogManager():
    def __init__(self, config = None):
        self.loggers = {}
        if config:
            with open(config) as json_conf:
                self.conf = json.load(json_conf)
        else:
            self.conf = {}

        self.mediums = {
            "default": LogMedium(),
            "zmq": LogMediumZMQ(ZMQ_PORT),
        }

    def getLogger(self, name: str):
        # Return if it has been initialised, otherwise initialise it first
        if name in self.loggers:
            return self.loggers[name]
        else:
            logger = Logger(self, name)
            self.loggers[name] = logger
            
            # if name in self.conf:
            #     for medium_name, medium_conf in self.conf[name]:
            #         pass

            return logger

# Different types of sinks/loggers 
class LogSinkPrinter(LogSink):
    def __init__(self, with_color = True):
        self.with_color = with_color
        self.max_tag_length = max([
            len(level.pretty_name(with_color = with_color)) for level in LogLevel
        ])

    def receive_log(self, log):
        print("{time:9.5f} {tag:<{tag_len}} {message} ({source})".format(
            time = log.time,
            tag = log.level.pretty_name(with_color = self.with_color),
            tag_len = self.max_tag_length,
            message = log.message,
            source = '.'.join(log.source)
        ))

log_manager = LogManager()

def getLogger(name: str):
    return log_manager.getLogger(name)

if __name__ == "__main__":
    child_logger = getLogger("child")
    child_logger.equip("zmq")
    
    logger = getLogger("main")
    logger.equip("zmq")
    logger.subscribe("child", LogLevel.WARNING, medium = "zmq")

    printer = LogSinkPrinter(with_color = True)
    printer.subscribe("main", LogLevel.ANY, medium = "zmq")

    # child_printer = LogSinkPrinter()
    # child_printer.subscribe("child", LogLevel.ANY, medium = "zmq")

    logger.info("This is just some info! 1")
    logger.info("This is just some info! 2")
    logger.info("This is just some info! 3")
    logger.debug("This is some good debug info 1")
    logger.info("This is just some info! 4")
    logger.info("This is just some info! 5")
    logger.debug("This is some good debug info 2")

    child_logger.info("Everything is good for now! :)")
    child_logger.warning("Oopsies!")
    # time.sleep(0.001)
    child_logger.error("It got worse!")

    # time.sleep(0.001)

    logger.critical("Shutting down!")

    time.sleep(1)

    # This causes issues with the logger currently if unsubscription happens
    # while logs are being sent out
    logger.unsubscribe("child", medium = "zmq")
    printer.unsubscribe("main", medium = "zmq")
    # child_printer.unsubscribe("child", medium = "zmq")

    # !!! There is also the issue that sometimes logs coming from children are published after logs coming from parent, even if the child log came in before

    # childlogger = Logger("child")

    # logger = Logger("main")
    # logger.subscribe(childlogger, LogLevel.ANY)

    # printer = LogSinkPrinter(with_color = True)
    # printer.subscribe(childlogger, LogLevel.ANY)

    # logger.info("This is just some info!")
    # logger.info("This is just some info!")
    # logger.info("This is just some info!")
    # logger.debug("This is some good debug info")
    # logger.info("This is just some info!")
    # logger.info("This is just some info!")
    # logger.debug("This is some good debug info")

    # childlogger.warning("Oopsies!")
    # childlogger.error("It got worse!")

    # logger.critical("Shutting down!")


