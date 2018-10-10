#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include "Thread.h"
#include "PacketDispatcher.h"
#include "PacketDecoder.h"

class XMDRecvThread : public XMDThread {
public:
    virtual void* process();
    XMDRecvThread(int port, XMDCommonData* commonData);
    ~XMDRecvThread();

    void stop();
    int listenfd() { return listenfd_; }
    void setTestFlat(bool flag) { testNetFlag_ = flag; }

private:
    int listenfd_;
    bool stopFlag_;
    uint32_t port_;
    bool testNetFlag_;
    XMDCommonData* commonData_;

    struct sockaddr_in* getSvrAddr();
    void Bind(int fd);
    void Recvfrom(int fd);
};



#endif //RECVTHREAD_H