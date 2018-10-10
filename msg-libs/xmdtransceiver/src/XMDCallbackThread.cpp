#include "XMDCallbackThread.h"
#include "LoggerWrapper.h"
#include <unistd.h>
#include <sstream> 

XMDCallbackThread::XMDCallbackThread(PacketDispatcher* dispatcher, XMDCommonData* commonData) {
    dispatcher_ = dispatcher;
    commonData_ = commonData;
    stopFlag_ = false;
}

XMDCallbackThread::~XMDCallbackThread() {}

void* XMDCallbackThread::process() {
    while(!stopFlag_) {
        CallbackQueueData* data = commonData_->callbackQueuePop();
        if (NULL == data) {
            usleep(1000);
            continue;
        }

        if (data->type == FEC_STREAM) {
            dispatcher_->handleStreamData(data->connId, data->streamId, data->groupId, (char*)data->data, data->len);
            delete data;
            continue;
        } else {
            std::stringstream ss;
            ss << data->connId << data->streamId;
            std::string tmpKey = ss.str();
            uint32_t lastCallbackGroupId = commonData_->getLastCallbackGroupId(tmpKey);
            if (lastCallbackGroupId != data->groupId) {
                commonData_->insertCallbackDataMap(tmpKey, data->groupId, data);
                continue;
            } else {
                dispatcher_->handleStreamData(data->connId, data->streamId, data->groupId, (char*)data->data, data->len);
                delete data;
                commonData_->updateLastCallbackGroupId(tmpKey, data->groupId + 1);

                bool flag = true;
                uint32_t lastGroupId = data->groupId;
                while (flag) {
                    CallbackQueueData* callbackData = commonData_->getCallbackData(tmpKey, lastGroupId);
                    if (callbackData == NULL) {
                        break;
                    }
                    dispatcher_->handleStreamData(callbackData->connId, callbackData->streamId, 
                                                  callbackData->groupId, (char*)callbackData->data, callbackData->len);
                    delete callbackData;
                    commonData_->updateLastCallbackGroupId(tmpKey, callbackData->groupId + 1);
                }
            }
        }
    
        
    }

    return NULL;
}

void XMDCallbackThread::stop() {
    stopFlag_ = true;
}

