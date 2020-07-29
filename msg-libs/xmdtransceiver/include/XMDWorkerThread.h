#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include "Thread.h"
#include "XMDCommonData.h"
#include "PacketDispatcher.h"
#include "PacketDecoder.h"
#include "PacketRecover.h"
#include "PacketCallback.h"

class WorkerThread : public XMDThread {
public:
    virtual void* process();
    WorkerThread(int id, XMDCommonData* commonData, PacketDispatcher* dispatcher);
    ~WorkerThread();

    void stop();

    void HandleWorkerThreadData(WorkerThreadData* workerThreadData);
    void CreateConnection(void* data);
    void CreateStream(void* data);
    void CloseConnection(void* data);
    void CloseStream(void* data);
    void SendRTData(void* data);
    void RecvData(void* data);

    void HandleResendTimeout(void* data);
    void HandleDatagramTimeout(void* data);
    void HandleDeleteAckGroupTimeout(void* data);
    void HandleDeleteFECGroupTimeout(void* data);
    void HandleCheckCallbackBuffer(void* data);
    void HandlePingTimeout(void* data);
    void HandlePongTimeout(void* data);
    void HandleCheckConn(void* data);
    void HandleTestRtt(void* data);
    void HandleSendBatchAck(void* data);

private:
    bool stopFlag_;
    int thread_id_;
    XMDCommonData* commonData_;
    PacketDispatcher* dispatcher_;
    PacketDecoder* packetDecoder_;
    PacketRecover* packetRecover_;
    PacketCallback* packetCallback_;
    WorkerCommonData* workerCommonData_;
};

#endif //WORKERTHREAD_H



