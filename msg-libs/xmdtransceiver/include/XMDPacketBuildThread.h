#ifndef PACKETBUILDTHREAD_H
#define PACKETBUILDTHREAD_H

#include "Thread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"


class PackketBuildThread : public XMDThread {
public:
    virtual void* process();
    PackketBuildThread(int threadId, XMDCommonData* commonData, PacketDispatcher* dispatcher);
    ~PackketBuildThread();

    void stop();

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
};

#endif //PACKETBUILDTHREAD_H


