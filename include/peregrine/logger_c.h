#ifndef __LOGGER_C__H__
#define __LOGGER_C__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct logger Logger;

Logger *root_get(char *name);

Logger *logger_get(Logger *parent, char *name);
void logger_publish(Logger *logger);

typedef struct log Log;

typedef enum log_level {
    ANY      = 0,
    DEBUG    = 1,
    INFO     = 2,
    WARNING  = 3,
    ERROR    = 4,
    CRITICAL = 5   
} LogLevel;

void send_log(Logger *logger, LogLevel level, char *message, char *file_name, unsigned int line_no);

#define log_debug(logger, message)    send_log(logger, DEBUG, message, __FILE__, __LINE__);
#define log_info(logger, message)     send_log(logger, INFO, message, __FILE__, __LINE__);
#define log_warning(logger, message)  send_log(logger, WARNING, message, __FILE__, __LINE__);
#define log_error(logger, message)    send_log(logger, ERROR, message, __FILE__, __LINE__);
#define log_critical(logger, message) send_log(logger, CRITICAL, message, __FILE__, __LINE__);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_C__H__
