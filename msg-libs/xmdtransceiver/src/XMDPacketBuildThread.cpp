#include "XMDPacketBuildThread.h"
#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include <thread>
#include <chrono>

PackketBuildThread::PackketBuildThread(int threadId, XMDCommonData* commonData, PacketDispatcher* dispatcher) {
    commonData_ = commonData;
    stopFlag_ = false;
    thread_id_ = threadId;
    dispatcher_ = dispatcher;
}

PackketBuildThread::~PackketBuildThread() {}

void* PackketBuildThread::process() {
    while(!stopFlag_) {
        StreamQueueData* streamData = commonData_->streamQueuePop();
        if (NULL == streamData) {
            //usleep(1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        PacketBuilder builder(commonData_, dispatcher_);
        builder.build(streamData);
        delete streamData;
    }

    while(!commonData_->streamQueueEmpty()) {
        StreamQueueData* streamData = commonData_->streamQueuePop();
        if (NULL == streamData) {
            break;
        }
        delete streamData;
    }
    return NULL;
}

void PackketBuildThread::stop() {
    stopFlag_ = true;
}


