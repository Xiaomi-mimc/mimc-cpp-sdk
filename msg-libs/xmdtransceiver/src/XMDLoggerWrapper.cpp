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
    const int LOG_MAX_LEN = 5120;
    int logLen = 1024;
    va_list args;
    va_start(args, format);
    bool isFinished = false;
    while (!isFinished) {
        char buf[logLen];
        int n = vsnprintf(buf, logLen, format, args);
        if (n < logLen) {
            isFinished = true;
            if (_externalLog != NULL) {
                 _externalLog->debug(buf);
            } else {
                std::cout << buf << std::endl;
            }
        } else {
            if (n < LOG_MAX_LEN) {
                logLen = n+1;
                va_start(args, format);
            } else {
               isFinished = true; 
            }
        }
    }

    va_end(args);
}

void XMDLoggerWrapper::info(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_INFO) {
        return;
    }

    const int LOG_MAX_LEN = 5120;
    int logLen = 1024;
    va_list args;
    va_start(args, format);
    bool isFinished = false;
    while (!isFinished) {
        char buf[logLen];
        int n = vsnprintf(buf, logLen, format, args);
        if (n < logLen) {
            isFinished = true;
            if (_externalLog != NULL) {
                 _externalLog->info(buf);
            } else {
                std::cout << buf << std::endl;
            }
        } else {
            if (n < LOG_MAX_LEN) {
                logLen = n+1;
                va_start(args, format);
            } else {
               isFinished = true; 
            }
        }
    }

    va_end(args);
}

void XMDLoggerWrapper::warn(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_WARN) {
        return;
    }

    const int LOG_MAX_LEN = 5120;
    int logLen = 1024;
    va_list args;
    va_start(args, format);
    bool isFinished = false;
    while (!isFinished) {
        char buf[logLen];
        int n = vsnprintf(buf, logLen, format, args);
        if (n < logLen) {
            isFinished = true;
            if (_externalLog != NULL) {
                 _externalLog->warn(buf);
            } else {
                std::cout << buf << std::endl;
            }
        } else {
            if (n < LOG_MAX_LEN) {
                logLen = n+1;
                va_start(args, format);
            } else {
               isFinished = true; 
            }
        }
    }

    va_end(args);
}

void XMDLoggerWrapper::error(const char* format, ...) {
    if (format == NULL) {
        return;
    }
    if (_logLevel < XMD_ERROR) {
        return;
    }

    const int LOG_MAX_LEN = 5120;
    int logLen = 1024;
    va_list args;
    va_start(args, format);
    bool isFinished = false;
    while (!isFinished) {
        char buf[logLen];
        int n = vsnprintf(buf, logLen, format, args);
        if (n < logLen) {
            isFinished = true;
            if (_externalLog != NULL) {
                 _externalLog->error(buf);
            } else {
                std::cout << buf << std::endl;
            }
        } else {
            if (n < LOG_MAX_LEN) {
                logLen = n+1;
                va_start(args, format);
            } else {
               isFinished = true; 
            }
        }
    }

    va_end(args);
}

