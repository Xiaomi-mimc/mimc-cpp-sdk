#include "XMDTimerThread.h"
#include "XMDLoggerWrapper.h"
#ifdef _WIN32
#include <stdlib.h>
#else
#include <unistd.h>
#endif // _WIN32

TimerThread::TimerThread(int id, XMDCommonData* commonData) {
    thread_id_ = id;
    commonData_ = commonData;
    stopFlag_ = false;
}

TimerThread::~TimerThread() {
}

void* TimerThread::process() {
    char tname[16];
    memset(tname, 0, 16);
    strcpy(tname, "xmd Timer");
#ifdef _WIN32
#else
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np(tname);
#else           
    prctl(PR_SET_NAME, tname);
#endif    
#endif // _WIN32
    while (!stopFlag_) {
        TimerThreadData* timerThreadData = commonData_->timerQueuePriorityPop(thread_id_);
        if (timerThreadData == NULL) {
            usleep(5000);
            continue;
        }

        WorkerThreadData* workerData = (WorkerThreadData*)timerThreadData->data;
        if (workerData->dataType == WORKER_DATAGREAM_TIMEOUT) {
            SendQueueData* queueData = (SendQueueData*)workerData->data;
            if (commonData_->isSocketSendQueueFull(queueData->len)) {
                XMDLoggerWrapper::instance()->warn("socket queue full %d drop datagram packet", commonData_->getSocketSendQueueUsedSize());
                delete queueData;
            } else {
                commonData_->socketSendQueuePush(queueData);
            }
            delete workerData;
        } else {
            commonData_->workerQueuePush(workerData, timerThreadData->connId);
        }
        delete timerThreadData;
    }

    return NULL;
}

void TimerThread::stop() {
    stopFlag_ = true;
}


