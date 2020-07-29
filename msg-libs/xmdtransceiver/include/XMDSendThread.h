#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include "Thread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"

class XMDTransceiver;

class XMDSendThread : public XMDThread {
public:
    virtual void* process();
    XMDSendThread(XMDCommonData* commonDa, PacketDispatcher* dispactcher, XMDTransceiver* transceiver);
    XMDSendThread();
    ~XMDSendThread();

    void send(uint32_t ip, int port, char* data , int len);
    void stop() { stopFlag_ = true; }
    void setTestPacketLoss(int value) { testPacketLoss_ = value; }
    void SetListenFd(int fd);
    void flowControl();
private:
    bool stopFlag_;
    uint32_t testPacketLoss_;
    int listenfd_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
    XMDTransceiver* transceiver_;
    bool is_reset_socket_;
    uint32_t send_fail_count_;
    uint64_t last_check_time_ms_;
    int speed_per_ms_;
};




#endif //SENDTHREAD_H