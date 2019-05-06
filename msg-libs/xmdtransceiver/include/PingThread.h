#ifndef PINGTHREAD_H
#define PINGTHREAD_H

#include "xmd_thread.h"
#include "PacketDispatcher.h"
#include "XMDCommonData.h"

#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

class PingThread : public XMDThread {
public:
    PingThread(PacketDispatcher* dispatcher, XMDCommonData* commonData);
    ~PingThread();
    virtual void* process();
    void stop();
    int sendPing(uint64_t connId, ConnInfo connInfo);
    int checkTimeout(uint64_t conn_id, ConnInfo connInfo);

private:
    bool stopFlag_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
};

#endif //PINGTHREAD_H


