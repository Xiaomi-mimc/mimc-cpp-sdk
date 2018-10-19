#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <stdint.h>
#include "XMDCommonData.h"


class ConnectionHandler {
public:
    virtual ~ConnectionHandler() { }
    virtual void NewConnection(uint64_t connId, char* data, int len) { }
    virtual void ConnCreateSucc(uint64_t connId, void* ctx) { }
    virtual void ConnCreateFail(uint64_t connId, void* ctx) { }
    virtual void CloseConnection(uint64_t connId, ConnCloseType type) { }
    virtual void ConnIpChange(uint64_t connId, std::string ip, int port) { }
};

#endif //CONNECTIONHANDLER_H
