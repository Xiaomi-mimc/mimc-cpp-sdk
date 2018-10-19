#ifndef XMD_LOGGER_WRAPPER_H
#define XMD_LOGGER_WRAPPER_H

#include <pthread.h>
#include "ExternalLog.h"

enum XMDLogLevel {
    XMD_ERROR = 0,
    XMD_WARN,
    XMD_INFO,
    XMD_DEBUG,
};

class XMDLoggerWrapper {
private:
    static pthread_mutex_t _mutex;
    static XMDLoggerWrapper* _instance;
    ExternalLog* _externalLog;
    XMDLoggerWrapper();
    XMDLogLevel _logLevel;

    class DestructLogger {
        public:
            ~DestructLogger()
            {
                if( XMDLoggerWrapper::_instance )
                  delete XMDLoggerWrapper::_instance;
            }
     };

     static DestructLogger destruct;

public:
    static XMDLoggerWrapper* instance();
    void externalLog(ExternalLog* externalLog);
    void setXMDLogLevel(XMDLogLevel level);

    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warn(const char* format, ...);
    void error(const char* format, ...);
};

#endif

