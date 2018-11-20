#include "XMDPacketBuildThread.h"
#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include <unistd.h>

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
            usleep(1000);
            continue;
        }
        PacketBuilder builder(commonData_, dispatcher_);
        builder.build(streamData);
        delete streamData;
    }

    return NULL;
}

void PackketBuildThread::stop() {
    stopFlag_ = true;
}


