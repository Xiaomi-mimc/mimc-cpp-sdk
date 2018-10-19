#ifndef PACKETBUILDTHREAD_H
#define PACKETBUILDTHREAD_H

#include "Thread.h"
#include "XMDCommonData.h"


class PackketBuildThread : public XMDThread {
public:
    virtual void* process();
    PackketBuildThread(int threadId, XMDCommonData* commonData);
    ~PackketBuildThread();

    void stop();

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
};

#endif //PACKETBUILDTHREAD_H


