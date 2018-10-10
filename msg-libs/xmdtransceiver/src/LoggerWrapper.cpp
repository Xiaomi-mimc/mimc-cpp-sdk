#include "LoggerWrapper.h"
#include "MutexLock.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>

using namespace std;

LoggerWrapper* LoggerWrapper::_instance = NULL;
pthread_mutex_t LoggerWrapper::_mutex = PTHREAD_MUTEX_INITIALIZER;

LoggerWrapper::LoggerWrapper() {
    _externalLog = NULL;
}

LoggerWrapper* LoggerWrapper::instance() {
    if (_instance == NULL) {
        MutexLock lock(&_mutex);
        if (_instance == NULL) {
            _instance = new LoggerWrapper();
        }
    }

    return _instance;
}

void LoggerWrapper::externalLog(ExternalLog* externalLog) {
    _externalLog = externalLog;
}

void LoggerWrapper::debug(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    const int LOG_MAX_LEN = 1024;
    char buf[LOG_MAX_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, LOG_MAX_LEN, format, args);
    va_end(args);

    if (_externalLog != NULL) {
        _externalLog->debug(buf);
        return;
    }
    std::cout<<buf<<std::endl;
}

void LoggerWrapper::info(const char* format, ...) {
    if (format == NULL) {
        return;
    }

    const int LOG_MAX_LEN = 1024;
    char buf[LOG_MAX_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, LOG_MAX_LEN, format, args);
    va_end(args);

    if (_externalLog != NULL) {
        _externalLog->info(buf);
        return;
    }
    std::cout<<buf<<std::endl;
}

void LoggerWrapper::warn(const char* format, ...) {
    if (format == NULL) {
        return;
    }

    const int LOG_MAX_LEN = 1024;
    char buf[LOG_MAX_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, LOG_MAX_LEN, format, args);
    va_end(args);

    if (_externalLog != NULL) {
        _externalLog->warn(buf);
        return;
    }
    std::cout<<buf<<std::endl;
}

void LoggerWrapper::error(const char* format, ...) {
    if (format == NULL) {
        return;
    }

    const int LOG_MAX_LEN = 1024;
    char buf[LOG_MAX_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, LOG_MAX_LEN, format, args);
    va_end(args);

    if (_externalLog != NULL) {
        _externalLog->error(buf);
        return;
    }
    std::cout<<buf<<std::endl;
}

