#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // _WIN32
#include <errno.h>
#include <sstream>
#include <memory>
#include <cstring>

#include "XMDSendThread.h"
#include "common.h"
#include "PacketBuilder.h"
#include "XMDLoggerWrapper.h"
#include "sleep_define.h"
#include "XMDTransceiver.h"

XMDSendThread::XMDSendThread(XMDCommonData* commonData, PacketDispatcher* dispactcher, XMDTransceiver* transceiver) {
    stopFlag_ = false;
    listenfd_ = 0;
    commonData_ = commonData;
    testPacketLoss_ = 0;
    dispatcher_ = dispactcher;
    transceiver_ = transceiver;
    is_reset_socket_ = true;
    send_fail_count_ = 0;
    last_check_time_ms_ = current_ms();
    speed_per_ms_ = DEFAULT_FLOW_CONTROL_SPEED;
}

XMDSendThread::XMDSendThread() {
    commonData_ = NULL;
}

XMDSendThread::~XMDSendThread() {
    
}


void XMDSendThread::SetListenFd(int fd) {
    listenfd_ = fd;
}


void* XMDSendThread::process() {
    char tname[16];
    memset(tname, 0, 16);
    strcpy(tname, "xmd send");

#ifdef _WIN32
#else
#ifdef _IOS_MIMC_USE_ 
    pthread_setname_np(tname);
#else           
	prctl(PR_SET_NAME, tname);
#endif
#endif // _WIN32

    while (!stopFlag_) {
        SendQueueData* sendData = commonData_->socketSendQueuePop();
        if (sendData == NULL) {
            usleep(XMD_SEND_PROCESS_SLEEP_MS);
            continue;
        }

        send(sendData->ip, sendData->port, (char*)sendData->data, sendData->len);
        
        if (sendData->isResend) {
            WorkerThreadData* workerThreadData = new WorkerThreadData(WORKER_RESEND_TIMEOUT, sendData, sendData->len);
            TimerThreadData* timerThreadData = new TimerThreadData();
            timerThreadData->connId = sendData->connId;
            timerThreadData->time = current_ms() + sendData->reSendInterval;
            timerThreadData->data = (void*)workerThreadData;
            timerThreadData->len = sizeof(WorkerThreadData) + sizeof(SendQueueData) + sendData->len;
            commonData_->timerQueuePush(timerThreadData, sendData->connId);
        } else {
            delete sendData;
        }

    }
    

    XMDLoggerWrapper::instance()->debug("xmd send thread stop end");
    return NULL;
}

void XMDSendThread::send(uint32_t ip, int port, char* data , int len) {
    if (rand32() % 100 < testPacketLoss_) {
        XMDLoggerWrapper::instance()->debug("test drop this packet");
        return;
    }

    flowControl();
    
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    //memset(&addr, 0, addrLen);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);

    bool is_fail = false;
    int ret = sendto(listenfd_, data, len, 0, (struct sockaddr*)&addr, addrLen);
    if (ret < 0) {
        if (errno == EAGAIN) {
            XMDLoggerWrapper::instance()->info("XMDSendThread first send fail try to resend.");
            usleep(100);
            int resend_ret = sendto(listenfd_, data, len, 0, (struct sockaddr*)&addr, addrLen);
            if (resend_ret < 0) {
                XMDLoggerWrapper::instance()->warn("XMDSendThread resend data fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
                is_fail = true;
            }
        } else {
            XMDLoggerWrapper::instance()->warn("XMDSendThread send data fail, ip:%u,port:%u,errmsg:%s,len:%d.", ip, port, strerror(errno), len);
            is_fail = true;
        }
    }
    else {
        is_reset_socket_ = true;
        send_fail_count_ = 0;
        dispatcher_->setIsCallBackSocketErr(true);
    }

    if (is_fail && is_reset_socket_) {
        send_fail_count_++;
        if (send_fail_count_ >= 2) {
            is_reset_socket_ = false;
            dispatcher_->handleSocketError(errno, "socket send err");
        }
        if (!stopFlag_ && transceiver_->resetSocket() < 0) {
            is_reset_socket_ = false;
        }        
    }

    XMDLoggerWrapper::instance()->debug("XMDSendThread send data, len:%d, ip:%u,port:%u.time:%ld", len, ip, port, current_ms());
}

void XMDSendThread::flowControl() {
    uint64_t current_time_ms = current_ms();
    if (current_time_ms <= last_check_time_ms_) {
        if (speed_per_ms_ > 0) {
            speed_per_ms_--;
        } else {
            usleep(1000);
            //XMDLoggerWrapper::instance()->warn("flow control");
        }
    } else {
        last_check_time_ms_ = current_time_ms;
        speed_per_ms_ = DEFAULT_FLOW_CONTROL_SPEED;
    }
}


