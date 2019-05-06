#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include "xmd_thread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"

class XMDSendThread : public XMDThread {
public:
    virtual void* process();
    XMDSendThread(int fd, int port, XMDCommonData* commonDa, PacketDispatcher* dispactcher);
    XMDSendThread();

    void send(uint32_t ip, int port, char* data , int len);
    void stop();
    void setTestPacketLoss(int value) { testPacketLoss_ = value; }
private:
    bool stopFlag_;
    uint32_t testPacketLoss_;
    int listenfd_;
    int port_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
};




#endif //SENDTHREAD_H