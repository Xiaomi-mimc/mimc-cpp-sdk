#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include "xmd_thread.h"
#include "PacketDispatcher.h"
#include "PacketDecoder.h"
#include "XMDCommonData.h"

class XMDTransceiver;
class XMDRecvThread : public XMDThread {
public:
    virtual void* process();
    XMDRecvThread(int port, XMDCommonData* commonData, PacketDispatcher* dispactcher, XMDTransceiver* transceiver);
    ~XMDRecvThread();

    void stop();
	int listenfd() { return listenfd_; }
    
    void setTestPacketLoss(int value) { testPacketLoss_ = value; }
    int InitSocket();

private:
	int listenfd_; // socket to int?

    bool stopFlag_;
    uint32_t port_;
    uint32_t testPacketLoss_;
    XMDCommonData* commonData_;
    XMDTransceiver* transceiver_;
    PacketDispatcher* dispatcher_;
    bool is_reset_socket_;
    uint32_t recv_fail_count_;

    struct sockaddr_in* getSvrAddr();
    int Bind(int fd);
    void Recvfrom(int fd);
};



#endif //RECVTHREAD_H