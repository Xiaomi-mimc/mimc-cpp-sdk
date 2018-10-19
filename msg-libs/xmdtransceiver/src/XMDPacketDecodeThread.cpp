#include "XMDPacketDecodeThread.h"
#include "PacketDecoder.h"
#include "XMDLoggerWrapper.h"
#include <unistd.h>

PackketDecodeThread::PackketDecodeThread(int id, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    commonData_ = commonData;
    dispatcher_ = dispatcher;
    packetDecoder_ = new PacketDecoder(dispatcher, commonData);
    stopFlag_ = false;
    thread_id_ = id;
}

PackketDecodeThread::~PackketDecodeThread() {
    delete packetDecoder_;
}

void* PackketDecodeThread::process() {
    while(!stopFlag_) {
        SocketData* data = commonData_->socketRecvQueuePop();
        if (NULL == data) {
            usleep(1000);
        } else {
            packetDecoder_->decode(data->ip, data->port, (char*)data->data, data->len);
            delete data;
        }
    }

    return NULL;
}

void PackketDecodeThread::stop() {
    stopFlag_ = true;
}



