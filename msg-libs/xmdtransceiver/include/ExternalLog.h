#ifndef EXTERNALLOG_H
#define EXTERNALLOG_H


class ExternalLog {
public:
    virtual void info(const char *msg) = 0;

    virtual void debug(const char *msg) = 0;

    virtual void warn(const char *msg) = 0;

    virtual void error(const char *msg) = 0;
};

#endif