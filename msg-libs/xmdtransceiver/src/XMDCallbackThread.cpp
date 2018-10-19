#include "XMDCallbackThread.h"
#include "XMDLoggerWrapper.h"
#include <unistd.h>
#include <sstream> 
#include "common.h"

XMDCallbackThread::XMDCallbackThread(PacketDispatcher* dispatcher, XMDCommonData* commonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    stopFlag_ = false;
}

XMDCallbackThread::~XMDCallbackThread() {}

void* XMDCallbackThread::process() {
    bool isSleep = false;
    uint64_t lastCheckTime = 0;
    while(!stopFlag_) {
        CallbackQueueData* data = commonData_->callbackQueuePop();
        if (NULL == data) {
            isSleep = true;
        } else {
            if (data->type == FEC_STREAM) {
                dispatcher_->handleStreamData(data->connId, data->streamId, data->groupId, (char*)data->data, data->len);
                delete data;
            } else {
                std::stringstream ss;
                ss << data->connId << data->streamId;
                std::string tmpKey = ss.str();
                uint32_t lastCallbackGroupId = 0;
                commonData_->getLastCallbackGroupId(tmpKey, lastCallbackGroupId);
                if ((data->groupId != 0) && (lastCallbackGroupId + 1 != data->groupId)) {
                    if(data->groupId <= lastCallbackGroupId) {
                        XMDLoggerWrapper::instance()->debug("data group id less than last callback group id.conn(%ld),stream(%d),group(%d)",
                                                             data->connId, data->streamId, data->groupId);
                    } else {
                        StreamInfo sInfo;
                        if(commonData_->getStreamInfo(data->connId, data->streamId, sInfo)) {
                            commonData_->insertCallbackDataMap(tmpKey, data->groupId, sInfo.callbackWaitTimeout, data);
                        }
                    }
                }else {
                    dispatcher_->handleStreamData(data->connId, data->streamId, data->groupId, (char*)data->data, data->len);
                    commonData_->updateLastCallbackGroupId(tmpKey, data->groupId);
                    uint32_t lastGroupId = data->groupId;
                    delete data;
                    
                    bool flag = true;
                    while (flag) {
                        CallbackQueueData* callbackData = NULL;
                        int result = commonData_->getCallbackData(tmpKey, lastGroupId, callbackData);
                        if (result < 0 || callbackData == NULL) {
                            break;
                        }
                        dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, 
                                                      callbackData->groupId, (char*)callbackData->data, callbackData->len);
                        commonData_->updateLastCallbackGroupId(tmpKey, callbackData->groupId);
                        delete callbackData;
                    }
                }
            }
        }

        uint64_t currentTime = current_ms();
        if (currentTime - lastCheckTime > 1) {
            checkCallbackBuffer();
            lastCheckTime = currentTime;
        }

        if (isSleep) {
            usleep(1000);
        }
    }

    return NULL;
}

void XMDCallbackThread::checkCallbackBuffer() {
    std::vector<uint64_t> connVec = commonData_->getConnVec();
    for (size_t i = 0; i < connVec.size(); i++) {
        uint64_t connId = connVec[i];
        ConnInfo connInfo;
        if(!commonData_->getConnInfo(connId, connInfo)){
            commonData_->deleteFromConnVec(connId);
            continue;
        }

        std::unordered_map<uint16_t,StreamInfo>::iterator it = connInfo.streamMap.begin();
        for (; it != connInfo.streamMap.end(); it++) {
            uint64_t streamId = it->first;
            std::stringstream ss;
            ss << connId << streamId;
            std::string tmpKey = ss.str();

            CallbackQueueData* callbackData = NULL;
            uint32_t lastCallbackGroupId = 0;
            commonData_->getLastCallbackGroupId(tmpKey, lastCallbackGroupId);
            while (commonData_->getCallbackData(tmpKey, lastCallbackGroupId, callbackData) >= 0) {
                if (callbackData == NULL) {
                    break;
                }

                if (callbackData->groupId == lastCallbackGroupId) {
                    delete callbackData;
                    callbackData = NULL;
                    continue;
                }

                dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, 
                                                  callbackData->groupId, (char*)callbackData->data, callbackData->len);
                commonData_->updateLastCallbackGroupId(tmpKey, callbackData->groupId);
                lastCallbackGroupId = callbackData->groupId;
                
                delete callbackData;
                callbackData = NULL;
            }
        }
    }
}

void XMDCallbackThread::stop() {
    stopFlag_ = true;
}

