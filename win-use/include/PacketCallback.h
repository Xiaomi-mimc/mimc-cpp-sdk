#ifndef PACKETCALLBACK_H
#define PACKETCALLBACK_H

#include "PacketDispatcher.h"
#include "XMDCommonData.h"


class PacketCallback {
public:
    PacketCallback(PacketDispatcher* dispatcher, XMDCommonData* commonData, WorkerCommonData* workerCommonData);
    ~PacketCallback();
    void CallbackFECStreamData(CallbackQueueData* callbackData);
    void CallbackAckStreamData(CallbackQueueData* callbackData);
    void CheckCallbackBuffer(CheckCallbackBufferData* data);

private:
    bool stopFlag_;
    XMDCommonData* commonData_;
    WorkerCommonData* workerCommonData_;
    PacketDispatcher* dispatcher_;
};

#endif //PACKETCALLBACK_H


