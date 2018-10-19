#include "XMDLoggerWrapper.h"
#include "MutexLock.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>

using namespace std;

XMDLoggerWrapper* XMDLoggerWrapper::_instance = NULL;
pthread_mutex_t XMDLoggerWrapper::_mutex = PTHREAD_MUTEX_INITIALIZER;

XMDLoggerWrapper::XMDLoggerWrapper() {
    _externalLog = NULL;
    _logLevel = XMD_DEBUG;
}

XMDLoggerWrapper* XMDLoggerWrapper::instance() {
    if (_instance == NULL) {
        MutexLock lock(&_mutex);
        if (_instance == NULL) {
            _instance = new XMDLoggerWrapper();
        }
    }

    return _instance;
}

void XMDLoggerWrapper::externalLog(ExternalLog* externalLog) {
    _externalLog = externalLog;
}

void XMDLoggerWrapper::setXMDLogLevel(XMDLogLevel level) {
   _logLevel = level;
}


void XMDLoggerWrapper::debug(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_DEBUG) {
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

void XMDLoggerWrapper::info(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_INFO) {
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

void XMDLoggerWrapper::warn(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_WARN) {
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

void XMDLoggerWrapper::error(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_ERROR) {
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

