#include "XMDPacketDecodeThread.h"
#include "PacketDecoder.h"
#include "XMDLoggerWrapper.h"
#include <thread>
#include <chrono>

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
            //usleep(1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            packetDecoder_->decode(data->ip, data->port, (char*)data->data, data->len);
            delete data;
        }
    }
    while(!commonData_->socketRecvQueueEmpty()) {
        SocketData* data = commonData_->socketRecvQueuePop();
        if (NULL == data) {
            break;
        }
        delete data;
    }

    return NULL;
}

void PackketDecodeThread::stop() {
    stopFlag_ = true;
}



