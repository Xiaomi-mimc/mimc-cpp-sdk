#include "PacketCallback.h"
#include <sstream>


PacketCallback::PacketCallback(PacketDispatcher* dispatcher, XMDCommonData* commonData, WorkerCommonData* workerCommonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    workerCommonData_ = workerCommonData;
}

PacketCallback::~PacketCallback() {
}

void PacketCallback::CallbackFECStreamData(CallbackQueueData* callbackData) {
    dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, callbackData->groupId, (char*)callbackData->data, callbackData->len);
    delete callbackData;
}

void PacketCallback::CallbackAckStreamData(CallbackQueueData* callbackData) {
    std::stringstream ss;
    ss << callbackData->connId << callbackData->streamId;
    std::string tmpKey = ss.str();
    uint32_t lastCallbackGroupId = -1;
    workerCommonData_->getLastCallbackGroupId(tmpKey, lastCallbackGroupId);
    if (lastCallbackGroupId + 1 != callbackData->groupId) {
        if(callbackData->groupId <= lastCallbackGroupId && (int32_t)lastCallbackGroupId != -1) {
            XMDLoggerWrapper::instance()->debug("data group id less than last callback group id.conn(%ld),stream(%d),group(%d)",
                                                 callbackData->connId, callbackData->streamId, callbackData->groupId);
            delete callbackData;
        } else {
            StreamInfo sInfo;
            if(workerCommonData_->getStreamInfo(callbackData->connId, callbackData->streamId, sInfo)) {
                int ret = workerCommonData_->insertCallbackDataMap(tmpKey, callbackData->groupId, sInfo.callbackWaitTimeout, callbackData);
                if (ret == 0) {
                    CheckCallbackBufferData* checkCallbackBuffer = new CheckCallbackBufferData();
                    checkCallbackBuffer->key = tmpKey;
                    checkCallbackBuffer->connId = callbackData->connId;
                    workerCommonData_->startTimer(callbackData->connId, WORKER_CHECK_CALLBACK_BUFFER, CHECK_CALLBACK_BUFFER_INTERVAL, (void*)checkCallbackBuffer, sizeof(CheckCallbackBufferData));
                }
            } else {
                XMDLoggerWrapper::instance()->warn("callback thread get stream info failed.conn(%ld),stream(%d)",
                                                    callbackData->connId, callbackData->streamId);
                delete callbackData;
            }
        }
    }else {
        dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, callbackData->groupId, (char*)callbackData->data, callbackData->len);
        workerCommonData_->updateLastCallbackGroupId(tmpKey, callbackData->groupId);
        delete callbackData;
        
    }
}

void PacketCallback::CheckCallbackBuffer(CheckCallbackBufferData* data) {
    CallbackQueueData* callbackData = NULL;
    uint32_t lastCallbackGroupId = -1;
    workerCommonData_->getLastCallbackGroupId(data->key, lastCallbackGroupId);
    while (workerCommonData_->getCallbackData(data->key, lastCallbackGroupId, callbackData) >= 0) {
        if (callbackData == NULL) {
            break;
        }

        if (callbackData->groupId <= lastCallbackGroupId && (int32_t)lastCallbackGroupId != -1) {
            XMDLoggerWrapper::instance()->debug("callback thread drop repeated data.connid(%ld),streamid(%d),groupid(%d)",
                                                 callbackData->connId, callbackData->streamId, callbackData->groupId);
            delete callbackData;
            callbackData = NULL;
            continue;
        }

        dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, 
                                          callbackData->groupId, (char*)callbackData->data, callbackData->len);
        workerCommonData_->updateLastCallbackGroupId(data->key, callbackData->groupId);
        lastCallbackGroupId = callbackData->groupId;
        
        
        delete callbackData;
        callbackData = NULL;
    }

    if (!workerCommonData_->isCallbackBufferMapEmpty(data->key)) {
        CheckCallbackBufferData* checkCallbackBuffer = new CheckCallbackBufferData();
        checkCallbackBuffer->key = data->key;
        checkCallbackBuffer->connId = data->connId;
        workerCommonData_->startTimer(data->connId, WORKER_CHECK_CALLBACK_BUFFER, CHECK_CALLBACK_BUFFER_INTERVAL, (void*)checkCallbackBuffer, sizeof(CheckCallbackBufferData));
    }
}

