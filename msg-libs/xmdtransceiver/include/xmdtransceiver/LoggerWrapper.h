#ifndef LOGGER_WRAPPER_H
#define LOGGER_WRAPPER_H

#include <pthread.h>
#include "ExternalLog.h"


class LoggerWrapper {
private:
    static pthread_mutex_t _mutex;
    static LoggerWrapper* _instance;
    ExternalLog* _externalLog;
    LoggerWrapper();

public:
    static LoggerWrapper* instance();
    void externalLog(ExternalLog* externalLog);

    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warn(const char* format, ...);
    void error(const char* format, ...);
};

#endif

