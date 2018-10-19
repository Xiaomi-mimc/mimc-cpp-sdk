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
    void setTestPacketLoss(int value) { testPacketLoss_ = value; }

private:
    int listenfd_;
    bool stopFlag_;
    uint32_t port_;
    uint32_t testPacketLoss_;
    XMDCommonData* commonData_;

    struct sockaddr_in* getSvrAddr();
    void Bind(int fd);
    void Recvfrom(int fd);
};



#endif //RECVTHREAD_H